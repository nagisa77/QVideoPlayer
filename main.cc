//
//  main.c
//  QVideoPlayer
//
//  Created by jt on 2023/12/13.
//
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <QApplication>
#include <QLabel>
#include <cstdio>
#include <iostream>

#include "video_player_view.hpp"

int main(int argc, const char* argv[]) {
  spdlog::info("hello");
  
  if (argc == 1) {
    spdlog::error("use ./VideoPlayer path_to_video_file");
    return -1; 
  }
  
  int q_argc = argc;
  char** q_argv = (char**)argv;
  QApplication app(q_argc, q_argv);

  VideoPlayerView video_player(argv[1]);
  video_player.show();

  return app.exec();
}
