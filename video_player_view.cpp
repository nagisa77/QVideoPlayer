//
//  video_player_view.cpp
//  QVideoPlayer
//
//  Created by jt on 2023/12/20.
//

#include "video_player_view.hpp"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <spdlog/spdlog.h>

#include <QPainter>

static QImage convertToQImage(AVFrame* frame) {
  // 检查 frame 的格式
  if (frame->format != AV_PIX_FMT_YUV420P &&
      frame->format != AV_PIX_FMT_YUVJ420P) {
    // 如果不是期望的 YUV 格式，则返回空 QImage
    // 或者你可以添加其他格式的支持
    return QImage();
  }

  // 创建一个 SwsContext 用于图像转换
  SwsContext* sws_ctx = sws_getContext(
      frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
      frame->width, frame->height, AV_PIX_FMT_RGB32, SWS_BILINEAR, nullptr,
      nullptr, nullptr);
  if (!sws_ctx) {
    return QImage();
  }

  // 设置输出图像参数
  uint8_t* dest[4] = {nullptr};
  int dest_linesize[4] = {0};
  av_image_alloc(dest, dest_linesize, frame->width, frame->height,
                 AV_PIX_FMT_RGB32, 1);

  // 转换图像格式
  sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, dest,
            dest_linesize);

  // 将转换后的数据复制到 QImage 中
  QImage img(dest[0], frame->width, frame->height, dest_linesize[0],
             QImage::Format_RGB32);

  // 释放资源
  sws_freeContext(sws_ctx);
  av_freep(&dest[0]);

  return img;
}

VideoPlayerView::VideoPlayerView() : QWidget(nullptr) {
  spdlog::info("VideoPlayerView");
  connect(this, &VideoPlayerView::frameReady, this,
          &VideoPlayerView::renderFrame);

  VideoCodec::getInstance().Register(this);
  VideoCodec::getInstance().StartCodec(
      "/Users/chenjiating/Downloads/file_example_MP4_1920_18MG-2.mp4");
}

VideoPlayerView::~VideoPlayerView() {
  spdlog::info("~VideoPlayerView");

  VideoCodec::getInstance().StopCodec();
  VideoCodec::getInstance().UnRegister(this);
}

void VideoPlayerView::OnVideoFrame(AVFrame* frame) {
  QImage image = convertToQImage(frame);  // 需要实现这个函数
  emit frameReady(image);
}

void VideoPlayerView::renderFrame(QImage frame) {
  // 保存或处理图像，然后更新 widget
  current_frame_ = frame;
  update();  // 触发重绘
}

void VideoPlayerView::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  if (!current_frame_.isNull()) {
    QSize windowSize = this->size();
    QImage scaledFrame = current_frame_.scaled(windowSize, Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);
    int startX = (windowSize.width() - scaledFrame.width()) / 2;
    int startY = (windowSize.height() - scaledFrame.height()) / 2;
    painter.drawImage(startX, startY, scaledFrame);
  }
}
