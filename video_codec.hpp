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

struct AVFrameDeleter {
  void operator()(AVFrame* frame) const {
    if (frame) {
      av_frame_free(&frame);
    }
  }
};

using AVFramePtr = std::shared_ptr<AVFrame>;

inline AVFramePtr createAVFramePtr() {
    return AVFramePtr(av_frame_alloc(), AVFrameDeleter());
}

class VideoCodecListener {
 public:
  virtual void OnVideoFrame(AVFramePtr frame) = 0;
  virtual void OnAudioFrame(AVFramePtr frame) = 0;
  virtual void OnMediaError() = 0;
};

class VideoCodec {
 public:
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
  void ProcessAudioFrameFromQueue();
  void OnFrame(AVFramePtr frame);
  void OnAudioFrame(AVFramePtr frame);
  void WaitForFrame(double frame_time);
  void WaitForFrameAudio(double frame_time);

 private:
  VideoCodec();
  ~VideoCodec();

 private:
  VideoCodecListener* listener_ = nullptr;
  int stop_requested_ = 0;
  std::thread codec_thread_;
  std::thread getting_frame_thread_;
  std::thread getting_audio_frame_thread_;
  BlockingQueue<AVFramePtr> fq_;
  BlockingQueue<AVFramePtr> afq_;
  bool is_first_frame_ = true;
  int64_t first_frame_time_us_;
  bool is_first_audio_frame_ = true;
  int64_t first_audio_frame_time_us_;

  AVRational stream_time_base_;
  AVRational audio_stream_time_base_;
  bool stream_time_base_ready_ = false;
};
#endif /* video_codec_hpp */
