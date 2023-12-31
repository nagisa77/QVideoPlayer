cmake_minimum_required(VERSION 3.10)
project(VideoPlayer)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)  # 开启 AUTOMOC

# 获取 Homebrew 安装的 Qt 路径
execute_process(COMMAND brew --prefix qt OUTPUT_VARIABLE QT_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
list(APPEND CMAKE_PREFIX_PATH "${QT_PATH}")

message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# 找到需要的 Qt6 模块
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

# 找到 spdlog 库
find_package(spdlog REQUIRED)

# 找到 ffmpeg 库
find_path(FFMPEG_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVCODEC_LIBRARY avcodec)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWSCALE_LIBRARY swscale)
find_library(SWRESAMPLE_LIBRARY swresample)

# 找到 SDL2 库
find_package(SDL2 REQUIRED)

# 找到 libpng 库
find_package(PNG REQUIRED)

# 找到 boost 库
# 在 find_package 中添加 thread 组件
find_package(Boost REQUIRED COMPONENTS thread)

add_executable(VideoPlayer
    main.cc
    video_codec.cpp
    video_codec.hpp  
    video_player_view.cpp
    video_player_view.hpp  
    blocking_queue.h
)

target_include_directories(VideoPlayer PRIVATE ${FFMPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIRS})
target_link_libraries(VideoPlayer
    Qt6::Core
    Qt6::Widgets
    spdlog::spdlog
    SDL2::SDL2
    ${Boost_LIBRARIES}
    ${SWRESAMPLE_LIBRARY}
    ${AVFORMAT_LIBRARY}
    ${AVCODEC_LIBRARY}
    ${AVUTIL_LIBRARY}
    ${SWSCALE_LIBRARY}
    ${PNG_LIBRARIES}
)
