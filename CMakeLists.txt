cmake_minimum_required(VERSION 3.5)
set (CMAKE_CXX_STANDARD 14)
project(play-rec)



file(GLOB SOURCES
    src/*.h
    src/*.cpp
	dependencies/miniaudio/*.h
)


add_subdirectory(dependencies/libsamplerate)
add_subdirectory(dependencies/args)
include_directories(dependencies/miniaudio)
add_executable(play-rec ${SOURCES})

