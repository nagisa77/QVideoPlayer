//
//  main.c
//  QVideoPlayer
//
//  Created by jt on 2023/12/13.
//
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <cstdio>
#include <iostream>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include "video_player_view.hpp"

int main(int argc, const char* argv[]) {
  spdlog::info("hello");
  
  int q_argc = argc;
  char** q_argv = (char**)argv;
  QApplication app(q_argc, q_argv);

  VideoPlayerView video_player;
  video_player.show();

  return app.exec();
}
