//
//  main.c
//  QVideoPlayer
//
//  Created by jt on 2023/12/13.
//
#include <libavcodec/avcodec.h>
int main(int argc, const char * argv[]) {
  unsigned version = avcodec_version();
  printf("version: %d\n", version);
  return 0;
}
