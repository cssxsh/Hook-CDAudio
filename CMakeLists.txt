cmake_minimum_required(VERSION 3.29)
project(Hook-CDAudio)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(CMAKE_INSTALL_PREFIX $ENV{GAME_PATH})

add_compile_options(/utf-8)
add_compile_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_library(winmm SHARED wave.cpp hook.cpp)

install(TARGETS winmm DESTINATION ".")
