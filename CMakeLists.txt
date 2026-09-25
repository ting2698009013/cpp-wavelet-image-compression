cmake_minimum_required(VERSION 3.16)
project(wavelet_image_compression LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(image_compression
    src/main.cpp
    src/Huffman.cpp
    src/ImageIO.cpp
)

if(MSVC)
    target_compile_options(image_compression PRIVATE /W4 /utf-8)
else()
    target_compile_options(image_compression PRIVATE -Wall -Wextra -Wpedantic)
endif()
