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

  int q_argc = argc;
  char** q_argv = (char**)argv;
  QApplication app(q_argc, q_argv);

  VideoPlayerView video_player;
  video_player.show();

  return app.exec();
}
