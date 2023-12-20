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
extern "C" {
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
  codec_thread_ = std::thread(&VideoCodec::Codec, this, file_path);
}

void VideoCodec::StopCodec() {
  spdlog::info("StopCodec");
  stop_requested_ = true;
  if (codec_thread_.joinable()) {
    codec_thread_.join();
  }
  spdlog::info("StopCodec success");
}

void VideoCodec::OnFrame(AVFrame* frame) { listener_->OnVideoFrame(frame); }

void VideoCodec::Codec(const std::string& file_path) {
  spdlog::info("start Codec");
  codec(file_path.c_str(), &stop_requested_, StaticFrameCallback);
}
