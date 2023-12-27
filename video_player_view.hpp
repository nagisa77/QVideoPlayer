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
  VideoPlayerView();
  ~VideoPlayerView();

  void renderFrame(QImage frame);
  void prepareAudioFrame(AVFrame* frame);
  void paintEvent(QPaintEvent* event) override;
  void AudioCallback(void* userdata, Uint8* stream, int len);
  bool InitSdlAudio(AVFrame* frame);

 signals:
  void frameReady(QImage frame);
  void audioFrameReady(AVFrame* frame);

 private:
  void OnVideoFrame(AVFrame* frame) override;
  void OnAudioFrame(AVFrame* frame) override;

 private:
  QImage current_frame_;
  BlockingQueue<AVFrame*> audio_frames_;
  bool first_audio_frame_ = true;
  SDL_AudioSpec obtained_;
};

#endif /* video_player_view_hpp */
