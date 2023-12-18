//
//  main.c
//  QVideoPlayer
//
//  Created by jt on 2023/12/13.
//
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <png.h>
#include <sys/time.h>

int save_frame_to_png_origin(const char *filename, AVFrame *frame) {
  // 打开PNG文件
  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    return -1;
  }

  // 初始化PNG结构体
  png_structp png_ptr =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fclose(fp);
    return -2;
  }

  // 初始化PNG信息结构体
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, NULL);
    fclose(fp);
    return -3;
  }

  // 设置错误处理函数
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return -4;
  }

  // 设置PNG文件头信息
  png_set_IHDR(png_ptr, info_ptr, frame->width, frame->height, 8,
               PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  // 写PNG文件头信息
  png_init_io(png_ptr, fp);
  //    png_set_rows(png_ptr, info_ptr, frame->data);
  png_bytep *row_pointers =
      (png_bytep *)malloc(sizeof(png_bytep) * frame->height);
  for (int y = 0; y < frame->height; y++) {
    row_pointers[y] = frame->data[0] + y * frame->linesize[0];
  }

  png_set_rows(png_ptr, info_ptr, row_pointers);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
  
  // 释放 row_pointers
  free(row_pointers);

  // 清理PNG结构体
  png_destroy_write_struct(&png_ptr, &info_ptr);

  // 关闭文件
  fclose(fp);

  return 0;
}

int save_frame_to_png(const char *filename, AVFrame *frame) {
  // Convert the frame to RGB format
  struct SwsContext *sws_ctx = sws_getContext(
      frame->width, frame->height, frame->format, frame->width, frame->height,
      AV_PIX_FMT_RGB24, SWS_SINC, NULL, NULL, NULL);
  if (!sws_ctx) {
    fprintf(stderr, "Could not initialize the conversion context\n");
    return -1;
  }

  AVFrame *rgb_frame = av_frame_alloc();
  rgb_frame->format = AV_PIX_FMT_RGB24;
  rgb_frame->width = frame->width;
  rgb_frame->height = frame->height;
  av_frame_get_buffer(rgb_frame, 0);

  sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0,
            frame->height, rgb_frame->data, rgb_frame->linesize);

  // Save the RGB frame as a PNG image
  // ... (use the same code as in your original save_frame_to_png function)
  save_frame_to_png_origin(filename, rgb_frame);

  // Free resources
  av_frame_free(&rgb_frame);
  sws_freeContext(sws_ctx);

  return 0;
}

int main(int argc, const char *argv[]) {
  if (argc <= 1) {
    return -1;
  }

  const char *video_path = argv[1];

  AVFormatContext *pFormatCtx = NULL;
  if (avformat_open_input(&pFormatCtx, video_path, NULL, NULL) != 0) {
    printf("avformat_open_input error\n");
    return -1;
  }

  if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
    printf("avformat_find_stream_info error\n");
    return -1;
  }

  int video_stream_index = -1;
  //  int audio_stream_index = -1;
  for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_index = i;
    }
    //
    //    if (pFormatCtx->streams[i]->codecpar->codec_type ==
    //    AVMEDIA_TYPE_AUDIO) {
    //      audio_stream_index = i;
    //    }
  }

  AVCodec *codec = avcodec_find_decoder(
      pFormatCtx->streams[video_stream_index]->codecpar->codec_id);
  AVCodecContext *pCodecCtx = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(
      pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);

  if (avcodec_open2(pCodecCtx, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    return -1;
  }

  AVPacket pkt;
  AVFrame *frame = av_frame_alloc();
  uint64_t idx = 0;

  struct timeval start, end;
  gettimeofday(&start, NULL);
  uint64_t frameCount = 0;

  while (av_read_frame(pFormatCtx, &pkt) >= 0) {
    if (pkt.stream_index == video_stream_index) {
      if (avcodec_send_packet(pCodecCtx, &pkt)) {
        int ret = avcodec_receive_frame(pCodecCtx, frame);
        if (ret == 0) {
          int width = frame->width;
          int height = frame->height;
          if (width <= 0 || height <= 0) {
            // printf("无效的帧大小：width=%d, height=%d\n", width, height);
            continue;
          }
          const char *save_dir = "/Users/chenjiating/Downloads/frames";
          char filename[256];
          memset(filename, 0, sizeof(filename));
          sprintf(filename, "%s/frame%d.png", save_dir, idx++);

          printf("width=%d, height=%d, format=%d\n", frame->width,
                 frame->height, frame->format);
          save_frame_to_png(filename, frame);

          frameCount++;
        }
      }
    }
    av_packet_unref(&pkt);
  }

  gettimeofday(&end, NULL);
  double elapsedTime =
      (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
  printf("Decoded frames: %llu\n", frameCount);
  printf("Elapsed time: %.2f seconds\n", elapsedTime);

  // 释放AVFormatContext
  avformat_close_input(&pFormatCtx);
}
