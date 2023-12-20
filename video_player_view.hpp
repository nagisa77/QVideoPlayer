//
//  video_player_view.hpp
//  QVideoPlayer
//
//  Created by jt on 2023/12/20.
//

#ifndef video_player_view_hpp
#define video_player_view_hpp

#include <stdio.h>

#include <QtWidgets/QWidget>

#include "video_codec.hpp"

class VideoPlayerView : public QWidget, public VideoCodecListener {
  using super_class = QWidget;
  using this_class = VideoPlayerView;

  Q_OBJECT

 public:
  VideoPlayerView();
  ~VideoPlayerView();

  void renderFrame(QImage frame);
  void paintEvent(QPaintEvent* event) override;

 signals:
  void frameReady(QImage frame);

 private:
  void OnVideoFrame(AVFrame* frame) override;

 private:
  QImage current_frame_;
};

#endif /* video_player_view_hpp */
