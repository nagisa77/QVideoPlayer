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
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}
#include <SDL.h>
#include <spdlog/spdlog.h>

#include <QPainter>

static SwsContext* sws_ctx = nullptr;
static SwrContext* swr_ctx = nullptr;

static void AudioCallbackBridge(void* userdata, Uint8* stream, int len) {
  auto* instance = static_cast<VideoPlayerView*>(userdata);
  instance->AudioCallback(userdata, stream, len);
}

void VideoPlayerView::AudioCallback(void* userdata, Uint8* stream, int len) {
  auto opt_frame = audio_frames_.popOrEmpty();
  if (!*opt_frame) {
    memset(stream, 0, len);
    return;
  }

  AVFrame* frame = *opt_frame;

  // 目标音频规格
  int64_t out_channel_layout = AV_CH_LAYOUT_STEREO;

  // 使用 SDL 音频设备的配置来设置输出格式
  int out_sample_rate = obtained_.freq;
  AVSampleFormat out_sample_fmt;
  switch (obtained_.format) {
    case AUDIO_U8:
      out_sample_fmt = AV_SAMPLE_FMT_U8;
      break;
    case AUDIO_S16SYS:
      out_sample_fmt = AV_SAMPLE_FMT_S16;
      break;
    case AUDIO_S32SYS:
      out_sample_fmt = AV_SAMPLE_FMT_S32;
      break;
    case AUDIO_F32SYS:
      out_sample_fmt = AV_SAMPLE_FMT_FLT;
      break;
    default:
      out_sample_fmt = AV_SAMPLE_FMT_S16;
      break;
  }

  if (frame->sample_rate <= 0) {
    memset(stream, 0, len);
    return;
  }

  // 创建并初始化 SwrContext
  if (!swr_ctx) {
    swr_ctx = swr_alloc();
    av_opt_set_int(swr_ctx, "in_channel_layout", frame->channel_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", frame->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",
                          (AVSampleFormat)frame->format, 0);
    av_opt_set_int(swr_ctx, "out_channel_layout", out_channel_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", out_sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", out_sample_fmt, 0);
    swr_init(swr_ctx);
  }

  // 准备输出缓冲区
  uint8_t* outBuffer[2] = {nullptr, nullptr};
  int out_samples = av_rescale_rnd(
      swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
      out_sample_rate, frame->sample_rate, AV_ROUND_UP);
  int out_buffer_size = av_samples_alloc(
      outBuffer, nullptr, av_get_channel_layout_nb_channels(out_channel_layout),
      out_samples, out_sample_fmt, 0);

  if (out_buffer_size < 0) {
    memset(stream, 0, len);
    av_freep(&outBuffer[0]);
    return;
  }

  // 转换音频数据
  swr_convert(swr_ctx, outBuffer, out_samples, (const uint8_t**)frame->data,
              frame->nb_samples);

  // 计算复制到 SDL 缓冲区的字节数
  int bytes_to_copy = std::min(len, out_buffer_size);

  // 复制音频数据到 SDL 缓冲区
  SDL_memcpy(stream, outBuffer[0], bytes_to_copy);

  // 释放资源
  av_freep(&outBuffer[0]);
  
  av_frame_free(&frame); 
}

// 初始化 SDL 音频
bool VideoPlayerView::InitSdlAudio(AVFrame* frame) {
  SDL_AudioSpec wanted_spec;

  // 设置音频格式
  wanted_spec.freq = frame->sample_rate;
  wanted_spec.channels = frame->channels;
  wanted_spec.silence = 0;
  wanted_spec.samples = frame->nb_samples;
  wanted_spec.callback = AudioCallbackBridge;
  wanted_spec.userdata = this;

  switch (frame->format) {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
      wanted_spec.format = AUDIO_U8;
      break;
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
      wanted_spec.format = AUDIO_S16SYS;
      break;
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
      wanted_spec.format = AUDIO_S32SYS;
      break;
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
      wanted_spec.format = AUDIO_F32SYS;
      break;
    // 添加其他需要支持的格式
    default:
      spdlog::info("Unsupported audio format");
      return false;
  }

  // 打开音频设备
  if (SDL_OpenAudio(&wanted_spec, &obtained_) < 0) {
    spdlog::info("Could not open audio: {}", SDL_GetError());
    return false;
  }

  // 开始播放
  SDL_PauseAudio(0);

  return true;
}

static QImage convertToQImage(AVFrame* frame) {
  // 检查 frame 的格式
  if (frame->format != AV_PIX_FMT_YUV420P &&
      frame->format != AV_PIX_FMT_YUVJ420P) {
    // 如果不是期望的 YUV 格式，则返回空 QImage
    // 或者你可以添加其他格式的支持
    return QImage();
  }

  // 如果 sws_ctx 未创建或帧格式/大小发生变化，则重新创建 sws_ctx
  if (!sws_ctx /* || 检查帧格式或大小是否改变 */) {
    if (sws_ctx) {
      sws_freeContext(sws_ctx);  // 释放旧的 context
    }
    sws_ctx = sws_getContext(frame->width, frame->height,
                             static_cast<AVPixelFormat>(frame->format),
                             frame->width, frame->height, AV_PIX_FMT_RGB32,
                             SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws_ctx) {
      return QImage();
    }
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
  //  sws_freeContext(sws_ctx);
  av_freep(&dest[0]);

  return img;
}

VideoPlayerView::VideoPlayerView() : QWidget(nullptr), audio_frames_(1000) {
  spdlog::info("VideoPlayerView");

  connect(this, &VideoPlayerView::frameReady, this,
          &VideoPlayerView::renderFrame);

  connect(this, &VideoPlayerView::audioFrameReady, this,
          &VideoPlayerView::prepareAudioFrame);

  VideoCodec::getInstance().Register(this);
  VideoCodec::getInstance().StartCodec(
      "/Users/chenjiating/Downloads/output.mp4");
}

VideoPlayerView::~VideoPlayerView() {
  spdlog::info("~VideoPlayerView");

  VideoCodec::getInstance().StopCodec();
  VideoCodec::getInstance().UnRegister(this);
}

void VideoPlayerView::OnVideoFrame(AVFrame* frame) {
  QImage image = convertToQImage(frame);
  emit frameReady(image);
}

void VideoPlayerView::OnAudioFrame(AVFrame* frame) {
  emit audioFrameReady(frame);
}

void VideoPlayerView::prepareAudioFrame(AVFrame* frame) {
  if (first_audio_frame_ && InitSdlAudio(frame)) {
    first_audio_frame_ = false;
  }
  audio_frames_.push(frame);
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
