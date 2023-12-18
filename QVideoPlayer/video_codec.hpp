//
//  video_codec.hpp
//  QVideoPlayer
//
//  Created by jt on 2023/12/18.
//

#ifndef video_codec_hpp
#define video_codec_hpp

#include <stdio.h>
#include <thread>
#include <string>

class VideoCodecListener {
  virtual void OnVideoFrame() = 0;
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
  
private:
  VideoCodec();
  ~VideoCodec();
  
private:
  VideoCodecListener* listener_ = nullptr;
  int stop_requested_ = 0;
  std::thread codec_thread_;
};
#endif /* video_codec_hpp */
