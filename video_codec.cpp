//
//  video_codec.cpp
//  QVideoPlayer
//
//  Created by jt on 2023/12/18.
//

#include "video_codec.hpp"

#include <spdlog/spdlog.h>
#include <sys/time.h>

#include <functional>

#include "blocking_queue.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
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
  
  std::queue<AVFrame*> empty_q;
  empty_q.push(nullptr);
  fq_.replace(empty_q);
  
  if (getting_frame_thread_.joinable()) {
    getting_frame_thread_.join();
  }
  spdlog::info("StopCodec success");
}

void VideoCodec::OnFrame(AVFrame* frame) {
  fq_.push(frame);  // 将 frame 放入队列
}

void VideoCodec::Codec(const std::string& file_path) {
  spdlog::info("start Codec");

  const char* video_path = file_path.c_str();
  AVFormatContext* pFormatCtx = NULL;
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

  const AVCodec* codec = avcodec_find_decoder(
      pFormatCtx->streams[video_stream_index]->codecpar->codec_id);
  AVCodecContext* pCodecCtx = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(
      pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);

  if (avcodec_open2(pCodecCtx, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    return -1;
  }

  AVPacket pkt;
  AVFrame* frame = av_frame_alloc();
  uint64_t idx = 0;

  struct timeval start, end;
  gettimeofday(&start, NULL);

  while (av_read_frame(pFormatCtx, &pkt) >= 0 && !stop_requested_) {
    if (pkt.stream_index == video_stream_index) {
      if (avcodec_send_packet(pCodecCtx, &pkt) == 0) {
        int ret = avcodec_receive_frame(pCodecCtx, frame);

        if (ret == 0) {
          int width = frame->width;
          int height = frame->height;
          if (width <= 0 || height <= 0) {
            continue;
          }

          AVFrame* frame_to_cb = av_frame_alloc();

          // 复制帧属性
          if (av_frame_copy_props(frame_to_cb, frame) < 0) {
            av_frame_free(&frame_to_cb);
            continue;
          }

          frame_to_cb->format = AV_PIX_FMT_YUV420P;
          frame_to_cb->width = frame->width;
          frame_to_cb->height = frame->height;

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
        }
      }
    }
    av_packet_unref(&pkt);
  }

  gettimeofday(&end, NULL);
  double elapsedTime =
      (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
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
  
  while ((frame = fq_.pop())) {
    if (frame->pts != AV_NOPTS_VALUE) {
      double time = av_q2d(stream_time_base_) * frame->pts;
      if (frame->pts == 0) {
        listener_->OnVideoFrame(frame);
        av_frame_free(&frame);  // 释放帧
        
        continue;
      }
//      spdlog::info("Frame time: {}", time);
//      spdlog::info("Original PTS: {}", frame->pts);
//      spdlog::info("Time base: num = {}, den = {}", stream_time_base_.num,
//                   stream_time_base_.den);
      
      WaitForFrame(time);
      
      listener_->OnVideoFrame(frame);
    }
    av_frame_free(&frame);  // 释放帧
    frame = nullptr; // 设置为 nullptr 避免重复释放
  }
  
  spdlog::info("decode ended");
}

void VideoCodec::WaitForFrame(double frame_time) {
    // 将帧时间转换为微秒
    int64_t frame_time_us = static_cast<int64_t>(frame_time * 1000000.0);

    // 检查是否是第一帧
    if (is_first_frame_) {
        first_frame_time_us_ = av_gettime();
        is_first_frame_ = false;
    }

    // 获取当前时间
    int64_t now_us = av_gettime();
    frame_time_us += first_frame_time_us_;

    // 如果当前时间早于预期播放时间，则等待
    if (now_us < frame_time_us) {
        int64_t wait_time_us = frame_time_us - now_us;
//        spdlog::info("now: {}, frame_time_us: {}, wait for {} us",
//                     wait_time_us,
//                     frame_time_us,
//                     wait_time_us);
        std::this_thread::sleep_for(std::chrono::microseconds(wait_time_us));
    }
}
