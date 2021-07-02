cmake_minimum_required(VERSION 3.10)

add_library(vidio 
	vidio.hpp
	vidio.cpp
)

add_executable(test_copy test_copy.cpp)
target_link_libraries(test_copy vidio)

add_executable(test_playback test_playback.cpp)
target_link_libraries(test_playback vidio X11 pthread)
