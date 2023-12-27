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
}

static void StaticFrameCallback(AVFramePtr frame) {
  VideoCodec& codec = VideoCodec::getInstance();
  codec.OnFrame(std::move(frame));
}

VideoCodec::VideoCodec() : fq_(100), afq_(100) {}  // 防止屯帧

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
  getting_audio_frame_thread_ =
      std::thread(&VideoCodec::ProcessAudioFrameFromQueue, this);
}

void VideoCodec::StopCodec() {
  spdlog::info("StopCodec");
  stop_requested_ = true;
  if (codec_thread_.joinable()) {
    codec_thread_.join();
  }
  
  stream_time_base_ready_ = true; 
  
  std::queue<AVFramePtr> empty_q;
  empty_q.push(nullptr);
  fq_.replace(empty_q);
  afq_.replace(empty_q);

  if (getting_frame_thread_.joinable()) {
    getting_frame_thread_.join();
  }

  if (getting_audio_frame_thread_.joinable()) {
    getting_audio_frame_thread_.join();
  }
  spdlog::info("StopCodec success");
}

void VideoCodec::OnFrame(AVFramePtr frame) {
  fq_.push(frame);
}

void VideoCodec::OnAudioFrame(AVFramePtr frame) {
  afq_.push(frame);
}

void VideoCodec::Codec(const std::string& file_path) {
  spdlog::info("start Codec");

  const char* video_path = file_path.c_str();
  AVFormatContext* pFormatCtx = NULL;
  if (avformat_open_input(&pFormatCtx, video_path, NULL, NULL) != 0) {
    printf("avformat_open_input error\n");
    return;
  }

  if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
    printf("avformat_find_stream_info error\n");
    return;
  }

  int video_stream_index = -1;
  int audio_stream_index = -1;
  //  int audio_stream_index = -1;
  for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_index = i;
    }
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index = i;
    }
  }
  
  if (video_stream_index == -1) {
    spdlog::error("no video found");
    listener_->OnMediaError();
    return;
  }

  stream_time_base_ = pFormatCtx->streams[video_stream_index]->time_base;
  audio_stream_time_base_ = pFormatCtx->streams[audio_stream_index]->time_base;
  stream_time_base_ready_ = true;

  const AVCodec* codec = avcodec_find_decoder(
      pFormatCtx->streams[video_stream_index]->codecpar->codec_id);
  AVCodecContext* pCodecCtx = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(
      pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);

  const AVCodec* audio_codec = avcodec_find_decoder(
      pFormatCtx->streams[audio_stream_index]->codecpar->codec_id);
  AVCodecContext* pAudioCodecCtx = avcodec_alloc_context3(audio_codec);
  avcodec_parameters_to_context(
      pAudioCodecCtx, pFormatCtx->streams[audio_stream_index]->codecpar);

  if (avcodec_open2(pCodecCtx, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    return; 
  }

  if (avcodec_open2(pAudioCodecCtx, audio_codec, NULL) < 0) {
    fprintf(stderr, "Could not open audio codec\n");
    return;
  }

  AVPacket pkt;
  auto frame = createAVFramePtr();
  uint64_t idx = 0;

  struct timeval start, end;
  gettimeofday(&start, NULL);

  while (av_read_frame(pFormatCtx, &pkt) >= 0 && !stop_requested_) {
    if (pkt.stream_index == audio_stream_index) {
      if (avcodec_send_packet(pAudioCodecCtx, &pkt) == 0) {
        int ret = avcodec_receive_frame(pAudioCodecCtx, frame.get());
        if (ret == 0) {
          auto frame_to_cb = createAVFramePtr();

          if (av_frame_copy_props(frame_to_cb.get(), frame.get()) < 0) {
            continue;
          }

          frame_to_cb->format = frame->format;
          frame_to_cb->channel_layout = frame->channel_layout;
          frame_to_cb->nb_samples = frame->nb_samples;
          frame_to_cb->sample_rate = frame->sample_rate;

          if (av_frame_get_buffer(frame_to_cb.get(), 32) < 0) {
            continue;
          }

          if (av_frame_copy(frame_to_cb.get(), frame.get()) < 0) {
            continue;
          }

          if (av_frame_copy_props(frame_to_cb.get(), frame.get()) < 0) {
            continue;
          }

          OnAudioFrame(std::move(frame_to_cb));
        }
      }
    } else if (pkt.stream_index == video_stream_index) {
      if (avcodec_send_packet(pCodecCtx, &pkt) == 0) {
        int ret = avcodec_receive_frame(pCodecCtx, frame.get());

        if (ret == 0) {
          int width = frame->width;
          int height = frame->height;
          if (width <= 0 || height <= 0) {
            continue;
          }

          auto frame_to_cb = createAVFramePtr();

          if (av_frame_copy_props(frame_to_cb.get(), frame.get()) < 0) {
            continue;
          }

          frame_to_cb->format =
              pFormatCtx->streams[video_stream_index]->codecpar->format;
          frame_to_cb->width = frame->width;
          frame_to_cb->height = frame->height;

          int r = av_frame_get_buffer(frame_to_cb.get(), 32);

          if (av_frame_copy(frame_to_cb.get(), frame.get()) < 0) {
            continue;
          }

          if (av_frame_copy_props(frame_to_cb.get(), frame.get()) < 0) {
            continue;
          }

          OnFrame(std::move(frame_to_cb));  // call back!
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
}

void VideoCodec::ProcessFrameFromQueue() {
  AVFramePtr frame;
  while (!stream_time_base_ready_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  while ((frame = fq_.pop())) {
    if (frame->pts != AV_NOPTS_VALUE) {
      double time = av_q2d(stream_time_base_) * frame->pts;
      if (frame->pts == 0) {
        listener_->OnVideoFrame(std::move(frame));

        continue;
      }

      WaitForFrameAudio(time);

      listener_->OnVideoFrame(std::move(frame));
    }
    frame = nullptr;
  }

  spdlog::info("decode ended");
}

void VideoCodec::ProcessAudioFrameFromQueue() {
  AVFramePtr frame;
  while (!stream_time_base_ready_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  while ((frame = afq_.pop())) {
    if (frame->pts != AV_NOPTS_VALUE) {
      double time = av_q2d(audio_stream_time_base_) * frame->pts;
      if (frame->pts == 0) {
        listener_->OnAudioFrame(std::move(frame));
        continue;
      }

      WaitForFrame(time);

      listener_->OnAudioFrame(std::move(frame));
    }
    frame = nullptr; 
  }

  spdlog::info("audio decode ended");
}

void VideoCodec::WaitForFrame(double frame_time) {
  int64_t frame_time_us = static_cast<int64_t>(frame_time * 1000000.0);

  if (is_first_frame_) {
    first_frame_time_us_ = av_gettime();
    is_first_frame_ = false;
  }

  int64_t now_us = av_gettime();
  frame_time_us += first_frame_time_us_;

  if (now_us < frame_time_us) {
    int64_t wait_time_us = frame_time_us - now_us;
    std::this_thread::sleep_for(std::chrono::microseconds(wait_time_us));
  }
}

void VideoCodec::WaitForFrameAudio(double frame_time) {
  int64_t frame_time_us = static_cast<int64_t>(frame_time * 1000000.0);

  if (is_first_audio_frame_) {
    first_audio_frame_time_us_ = av_gettime();
    is_first_audio_frame_ = false;
  }

  int64_t now_us = av_gettime();
  frame_time_us += first_audio_frame_time_us_;

  if (now_us < frame_time_us) {
    int64_t wait_time_us = frame_time_us - now_us;
    std::this_thread::sleep_for(std::chrono::microseconds(wait_time_us));
  }
}

