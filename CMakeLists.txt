cmake_minimum_required(VERSION 3.5)
set (CMAKE_CXX_STANDARD 14)
project(play-rec)



file(GLOB SOURCES
	src/*.h
	src/*.cpp
	src/util/*.h
	src/util/*.c
	dependencies/miniaudio/*.h
	dependencies/args/args.hxx
)


include_directories(dependencies/miniaudio)
include_directories(dependencies/args)

add_executable(play-rec ${SOURCES})

