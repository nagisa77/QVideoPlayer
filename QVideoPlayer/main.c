//
//  main.c
//  QVideoPlayer
//
//  Created by jt on 2023/12/13.
//
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <png.h>

int save_frame_to_png(const char *filename, AVFrame *frame) {
    // 打开PNG文件
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        return -1;
    }

    // 初始化PNG结构体
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
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
    png_set_IHDR(png_ptr, info_ptr, frame->width, frame->height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // 写PNG文件头信息
    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, frame->data);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    // 清理PNG结构体
    png_destroy_write_struct(&png_ptr, &info_ptr);

    // 关闭文件
    fclose(fp);

    return 0;
}


int main(int argc, const char * argv[]) {
  if (argc <= 1) {
    return -1;
  }
  
  const char* video_path = argv[1];
  
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
//    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
//      audio_stream_index = i;
//    }
  }
  
  AVCodec* codec = avcodec_find_decoder(pFormatCtx->streams[video_stream_index]->codecpar->codec_id);
  AVCodecContext* pCodecCtx = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);
  
  AVPacket pkt;
  AVFrame* frame = av_frame_alloc();
  uint64_t idx = 0;
  while (av_read_frame(pFormatCtx, &pkt) >= 0) {
    if (pkt.stream_index == video_stream_index) {
      if (avcodec_send_packet(pCodecCtx, &pkt)) {
        while (avcodec_receive_frame(pCodecCtx, frame)) {
          printf("width=%d, height=%d, format=%d\n", frame->width, frame->height, frame->format);

          int width = frame->width;
          int height = frame->height;
          if (width <= 0 || height <= 0) {
            // printf("无效的帧大小：width=%d, height=%d\n", width, height);
            continue;
          }
          const char* save_dir = "/Users/chenjiating/Downloads/QVideoPlayer/frame";
          char filename[256];
          memset(filename, 0, sizeof(filename));
          sprintf(filename, "%s/frame%d.png", save_dir, idx++);
          save_frame_to_png(filename, frame);
          break;
        }
      }
    }
//    if (pkt.stream_index == audio_stream_index) {
//      //
//    }
    av_packet_unref(&pkt);
  }
  

  // 释放AVFormatContext
  avformat_close_input(&pFormatCtx);
}
