//
//  video_codec.hpp
//  QVideoPlayer
//
//  Created by jt on 2023/12/18.
//

#ifndef video_codec_hpp
#define video_codec_hpp

extern "C" {
#include <libavcodec/avcodec.h>
}
#include <stdio.h>

#include <string>
#include <thread>

#include "blocking_queue.h"

class VideoCodecListener {
 public:
  virtual void OnVideoFrame(AVFrame* frame) = 0;
};

class VideoCodec {
 public:
  // 删除复制构造函数和赋值运算符
  VideoCodec(const VideoCodec&) = delete;
  VideoCodec& operator=(const VideoCodec&) = delete;

  static VideoCodec& getInstance() {
    static VideoCodec instance;
    return instance;
  }

  void Register(VideoCodecListener* listener);
  void UnRegister(VideoCodecListener* listener);
  void StartCodec(const std::string& file_path);
  void StopCodec();
  void Codec(const std::string& file_path);
  void ProcessFrameFromQueue();
  void OnFrame(AVFrame* frame);
  void WaitForFrame(double frame_time);

 private:
  VideoCodec();
  ~VideoCodec();

 private:
  VideoCodecListener* listener_ = nullptr;
  int stop_requested_ = 0;
  std::thread codec_thread_;
  std::thread getting_frame_thread_;
  BlockingQueue<AVFrame*> fq_;
  bool is_first_frame_ = true;
  int64_t first_frame_time_us_;
  AVRational stream_time_base_;
  bool stream_time_base_ready_ = false;
};
#endif /* video_codec_hpp */
