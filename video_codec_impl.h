//
//  video_codec_impl.h
//  QVideoPlayer
//
//  Created by jt on 2023/12/18.
//

#ifndef video_codec_impl_h
#define video_codec_impl_h
#include <libavcodec/avcodec.h>
#include <stdio.h>

typedef void (*FrameCallback)(AVFrame*);

int codec(const char* video_path, int* stop_flag, FrameCallback cb);

#endif /* video_codec_impl_h */
