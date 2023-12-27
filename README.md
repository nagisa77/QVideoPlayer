# QVideoPlayer

## 简介

`QVideoPlayer` 是一个基于 Qt 和 SDL 的视频播放器，利用 FFmpeg 进行媒体处理。它结合了 Qt 的强大界面能力和 SDL 的音频处理效率，提供了流畅的视频播放体验。

## 核心功能

- **视频播放**：使用 FFmpeg 库解码视频流，支持多种视频格式。
- **音频播放**：结合 SDL 库播放音频，提供同步的音视频体验。
- **用户界面**：基于 Qt 框架，提供图形界面交互。
- **媒体控制**：包括播放、暂停、停止等基本媒体播放控制。

## 技术细节

### FFmpeg 原理

FFmpeg 是一个多媒体框架，用于处理音频和视频数据。在 `QVideoPlayer` 中，FFmpeg 被用来解码视频文件，提取音视频数据流。

- **视频解码**：通过 `avcodec_send_packet` 和 `avcodec_receive_frame` 函数，FFmpeg 将视频数据包解码成帧。
- **音频解码**：类似地，音频流也通过 FFmpeg 解码，将音频包转换为可播放的帧。

### SDL 音频处理

SDL (Simple DirectMedia Layer) 是一个跨平台开发库，用于处理音频、键盘、鼠标等设备。在本项目中，SDL 被用来处理音频输出。

- **音频回调**：通过 `SDL_OpenAudio` 函数初始化音频设备，并设置回调函数，当音频设备需要更多数据时触发回调。

### Qt 框架

Qt 是一个跨平台的 C++ 应用程序框架，用于开发具有图形用户界面的应用程序。`QVideoPlayer` 使用 Qt 来构建和管理用户界面。

- **事件处理**：Qt 的事件系统被用来处理用户输入，如按键事件。

### 多线程与同步

项目采用多线程技术来分离视频解码和播放逻辑，提高性能和响应速度。

- **视频和音频队列**：使用 `BlockingQueue` 来存储解码后的音视频帧，确保线程安全的数据访问。

## 编译和运行

确保你的系统已安装 Qt 6、SDL 2、FFmpeg 和 Boost C++ Libraries。

### 编译步骤

1. 克隆仓库：
   ```
   git clone https://github.com/nagisa77/QVideoPlayer
   ```
2. 进入项目目录：
   ```
   cd QVideoPlayer
   ```
3. 创建并进入构建目录：
   ```
   mkdir build && cd build
   ```
4. 使用 CMake 生成 Makefile：
   ```
   cmake ..
   ```
5. 编译项目：
   ```
   make
   ```

### 运行

在构建目录下，运行编译好的可执行文件，传递视频文件路径作为参数：
```
./QVideoPlayer path_to_video_file
```

## 开发者指南

### 项目结构

- `main.c`：程序入口，设置 Qt 应用和播放器视图。
- `video_codec.hpp/cpp`：处理视频和音频的编解码逻辑。
- `video_player_view.hpp/cpp`：Qt 视图类，负责渲染视频帧和处理用户输入。
- `blocking_queue.h`：线程安全的队列，用于存储解码后的帧。

###