//
//  video_codec.cpp
//  QVideoPlayer
//
//  Created by jt on 2023/12/18.
//

#include "video_codec.hpp"

#include <spdlog/spdlog.h>

#include <functional>

#include "concurrentqueue.h"
#include <sys/time.h>

extern "C" {
#include <libavformat/avformat.h>
#include "video_codec_impl.h"
}

static void StaticFrameCallback(AVFrame* frame) {
  VideoCodec& codec = VideoCodec::getInstance();
  codec.OnFrame(frame);
}

VideoCodec::VideoCodec() {}

VideoCodec::~VideoCodec() {}

void VideoCodec::Register(VideoCodecListener* listener) {
  listener_ = listener;
}

void VideoCodec::UnRegister(VideoCodecListener* listener) {
  if (listener != listener_) {
    spdlog::error("listener error");
  }
  listener_ = nullptr;
}

void VideoCodec::StartCodec(const std::string& file_path) {
  stop_requested_ = false;
  stream_time_base_ready_ = false;
  codec_thread_ = std::thread(&VideoCodec::Codec, this, file_path);
  getting_frame_thread_ = std::thread(&VideoCodec::ProcessFrameFromQueue, this);
}

void VideoCodec::StopCodec() {
  spdlog::info("StopCodec");
  stop_requested_ = true;
  if (codec_thread_.joinable()) {
    codec_thread_.join();
  }
  if (getting_frame_thread_.joinable()) {
    getting_frame_thread_.join();
  }
  spdlog::info("StopCodec success");
}

void VideoCodec::OnFrame(AVFrame* frame) {
  fq_.enqueue(frame);  // 将 frame 放入队列
}

void VideoCodec::Codec(const std::string& file_path) {
  spdlog::info("start Codec");
  
  const char* video_path = file_path.c_str();
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
  
  stream_time_base_ = pFormatCtx->streams[video_stream_index]->time_base;
  stream_time_base_ready_ = true;
  
  const AVCodec *codec = avcodec_find_decoder(
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

  while (av_read_frame(pFormatCtx, &pkt) >= 0 && !stop_requested_) {
    if (pkt.stream_index == video_stream_index) {
      if (avcodec_send_packet(pCodecCtx, &pkt)) {
        av_frame_get_buffer(frame, 32);

        int ret = avcodec_receive_frame(pCodecCtx, frame);

        if (ret == 0) {
          int width = frame->width;
          int height = frame->height;
          if (width <= 0 || height <= 0) {
            //            printf("无效的帧大小：width=%d, height=%d\n", width,
            //            height);
            continue;
          }
          //          const char *save_dir =
          //          "/Users/chenjiating/Downloads/frames"; char filename[256];
          //          memset(filename, 0, sizeof(filename));
          //          sprintf(filename, "%s/frame%d.png", save_dir, idx++);

//          printf("width=%d, height=%d, format=%d\n", frame->width,
//                 frame->height, frame->format);
          //          save_frame_to_png(filename, frame);

          AVFrame *frame_to_cb = av_frame_alloc();

          // 复制帧属性
          if (av_frame_copy_props(frame_to_cb, frame) < 0) {
            av_frame_free(&frame_to_cb);
            continue;
          }

          frame_to_cb->format = AV_PIX_FMT_YUV420P;  // 设置为适当的像素格式
          frame_to_cb->width = frame->width;         // 设置宽度
          frame_to_cb->height = frame->height;  // 设置高度

          int r = av_frame_get_buffer(frame_to_cb, 32);

          // 复制帧数据
          if (av_frame_copy(frame_to_cb, frame) < 0) {
            av_frame_free(&frame_to_cb);
            continue;
          }

          // 复制帧属性
          if (av_frame_copy_props(frame_to_cb, frame) < 0) {
            av_frame_free(&frame_to_cb);
            continue;
          }

          OnFrame(frame_to_cb);  // call back!

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

  // free
  avcodec_free_context(&pCodecCtx);
  avformat_close_input(&pFormatCtx);
  av_frame_free(&frame);

  return 0;
}

void VideoCodec::ProcessFrameFromQueue() {
  AVFrame* frame;
  while (!stream_time_base_ready_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  
  while (true) {
    fq_.try_dequeue(frame);
    if (frame && frame->pts != AV_NOPTS_VALUE) {
      // 使用视频流的时间基转换 pts 到实际时间
      double time = av_q2d(stream_time_base_) * frame->pts;
      if (frame->pts == 0) {
        listener_->OnVideoFrame(frame);
        continue;
      }
      spdlog::info("Frame time: {}", time);
      spdlog::info("Original PTS: {}", frame->pts);
      spdlog::info("Time base: num = {}, den = {}", stream_time_base_.num, stream_time_base_.den);

      
      // 等待直到达到这个时间点
      // 这里需要根据你的应用场景实现具体的同步逻辑
      WaitForFrame(time);
      
      listener_->OnVideoFrame(frame); 
    }
//    av_frame_free(&frame);  // 释放帧
  }
}

void VideoCodec::WaitForFrame(double frame_time) {
  using namespace std::chrono;

  // 获取当前时间
  auto now = steady_clock::now();

  // 检查是否是第一帧
  if (is_first_frame_) {
      last_frame_time_ = now;
      is_first_frame_ = false;
  }

  // 计算下一帧的预期播放时间
  auto next_frame_time = last_frame_time_ + duration_cast<steady_clock::duration>(duration<double>(frame_time));

  // 如果当前时间早于预期播放时间，则等待
  if (now < next_frame_time) {
      std::this_thread::sleep_for((next_frame_time - now) / 1000);
  }

  // 更新上一帧的播放时间
  last_frame_time_ = now + duration_cast<steady_clock::duration>(duration<double>(frame_time));
}

