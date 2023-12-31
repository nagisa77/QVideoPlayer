//
//  video_player_view.hpp
//  QVideoPlayer
//
//  Created by jt on 2023/12/20.
//

#ifndef video_player_view_hpp
#define video_player_view_hpp

#include <SDL.h>
#include <stdio.h>

#include <QWidget>

#include "blocking_queue.h"
#include "video_codec.hpp"

class VideoPlayerView : public QWidget, public VideoCodecListener {
  using super_class = QWidget;
  using this_class = VideoPlayerView;

  Q_OBJECT

 public:
  VideoPlayerView(const char* path);
  ~VideoPlayerView();

  void renderFrame(QImage frame);
  void paintEvent(QPaintEvent* event) override;
  void AudioCallback(void* userdata, Uint8* stream, int len);
  bool InitSdlAudio(AVFramePtr frame);

 signals:
  void frameReady(QImage frame);
  void audioFrameReady(AVFramePtr frame);

 private:
  void OnVideoFrame(AVFramePtr frame) override;
  void OnAudioFrame(AVFramePtr frame) override;
  void OnMediaError() override;
  void keyPressEvent(QKeyEvent *event) override;

 private:
  QImage current_frame_;
  BlockingQueue<AVFramePtr> audio_frames_;
  bool first_audio_frame_ = true;
  SDL_AudioSpec obtained_;
  bool pause_ = false; 
};

#endif /* video_player_view_hpp */
