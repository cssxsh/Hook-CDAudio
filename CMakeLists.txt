cmake_minimum_required(VERSION 3.29)
project(Hook-CDAudio)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(MSVC60_PATH $ENV{MSVC60_PATH})
set(CMAKE_INSTALL_PREFIX $ENV{GAME_PATH})

include_directories("${MSVC60_PATH}/VC98/Include")
include_directories("${MSVC60_PATH}/VC98/MFC/Include")
include_directories("${MSVC60_PATH}/VC98/ATL/Include")
link_directories("${MSVC60_PATH}/VC98/Lib")
link_directories("${MSVC60_PATH}/VC98/MFC/Lib")
link_directories("${MSVC60_PATH}/VC98/ATL/Lib")

add_compile_options(/GS-)
add_compile_options(/Zc:sizedDealloc-)
add_library(winmm SHARED library.cpp)

install(TARGETS winmm RUNTIME)
