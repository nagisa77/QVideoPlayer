# QVideoPlayer

## Introduction

`QVideoPlayer` is a video player based on Qt and SDL, utilizing FFmpeg for media processing. It combines the powerful interface capabilities of Qt with the audio processing efficiency of SDL, offering a smooth video playback experience.

## Core Features

- **Video Playback**: Decodes video streams using the FFmpeg library, supporting various video formats.
- **Audio Playback**: Integrates with the SDL library for audio playback, providing synchronized audio and video experience.
- **User Interface**: Built on the Qt framework, offering graphical user interface interactions.
- **Media Control**: Includes basic media playback controls such as play, pause, and stop.

## Technical Details

### FFmpeg Principles

FFmpeg is a multimedia framework used for processing audio and video data. In `QVideoPlayer`, FFmpeg is utilized to decode video files and extract audio and video streams.

- **Video Decoding**: Through the `avcodec_send_packet` and `avcodec_receive_frame` functions, FFmpeg decodes video packets into frames.
- **Audio Decoding**: Similarly, audio streams are decoded via FFmpeg, converting audio packets into playable frames.

### SDL Audio Processing

SDL (Simple DirectMedia Layer) is a cross-platform development library used for handling audio, keyboard, mouse, and other devices. In this project, SDL is used to handle audio output.

- **Audio Callback**: Initializes the audio device using `SDL_OpenAudio` function and sets up a callback function, triggered when the audio device requires more data.

### Qt Framework

Qt is a cross-platform C++ application framework for developing applications with graphical user interfaces. `QVideoPlayer` uses Qt to build and manage the user interface.

- **Event Handling**: Qt's event system is used for handling user input, such as key press events.

### Multithreading and Synchronization

The project employs multithreading to separate video decoding and playback logic, enhancing performance and responsiveness.

- **Video and Audio Queues**: Uses `BlockingQueue` to store decoded audio and video frames, ensuring thread-safe data access.

## Compilation and Running

Ensure your system has Qt 6, SDL 2, FFmpeg, and Boost C++ Libraries installed.

### Compilation Steps

1. Clone the repository:
   ```
   git clone https://github.com/nagisa77/QVideoPlayer
   ```
2. Enter the project directory:
   ```
   cd QVideoPlayer
   ```
3. Create and enter the build directory:
   ```
   mkdir build && cd build
   ```
4. Generate Makefile using CMake:
   ```
   cmake ..
   ```
5. Compile the project:
   ```
   make
   ```

### Running

In the build directory, run the compiled executable file, passing the path to the video file as an argument:
```
./QVideoPlayer path_to_video_file
```

## Developer's Guide

### Project Structure

- `main.c`: Program entry point, setting up the Qt application and player view.
- `video_codec.hpp/cpp`: Handles the logic of video and audio codec processing.
- `video_player_view.hpp/cpp`: Qt view class, responsible for rendering video frames and handling user inputs.
- `blocking_queue.h`: A thread-safe queue for storing decoded frames.