//
//  main.cpp
//  QVideoPlayer
//
//  Created by jt on 2023/12/13.
//

#include <iostream>
#include <libavcodec/avcodec.h>

int main(int argc, const char * argv[]) {
  //这里简单的输出一个版本号
  std::cout << "Hello FFmpeg!" << std::endl;
  unsigned version = avcodec_version();
  std::cout << "version is:" << version;
  return 0;  return 0;
}
