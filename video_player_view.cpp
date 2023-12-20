//
//  video_player_view.cpp
//  QVideoPlayer
//
//  Created by jt on 2023/12/20.
//

#include "video_player_view.hpp"
#include <spdlog/spdlog.h>

VideoPlayerView::VideoPlayerView() : QWidget(nullptr) {
  spdlog::info("VideoPlayerView");
  
  VideoCodec::getInstance().Register(this);
  VideoCodec::getInstance().StartCodec("/Users/chenjiating/Downloads/file_example_MP4_1920_18MG-2.mp4");
}

VideoPlayerView::~VideoPlayerView() {
  spdlog::info("~VideoPlayerView");
  
  VideoCodec::getInstance().StopCodec();
  VideoCodec::getInstance().UnRegister(this);
}

void VideoPlayerView::OnVideoFrame() {
  spdlog::info("OnVideoFrame");
}
