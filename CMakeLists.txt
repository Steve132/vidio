cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=gnu++11")

add_library(vidio platform_posix.cpp vidio.cpp)

add_executable(test_copy test_copy.cpp)
target_link_libraries(test_copy vidio)

add_executable(test_playback test_playback.cpp)
target_link_libraries(test_playback vidio X11 pthread)
