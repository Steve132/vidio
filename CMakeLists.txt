cmake_minimum_required(VERSION 3.10)

project(vidio)
add_library(vidio 
	vidio.hpp
	vidio.cpp
)
target_include_directories(vidio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
#add_executable(test_copy test_copy.cpp)
#target_link_libraries(test_copy vidio)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)

add_executable(test_playback test_playback.cpp CImg.h)
target_link_libraries(test_playback vidio X11 pthread)
add_executable(test_subprocess_oop test_subprocess_oop.cpp subprocess.hpp)

endif()
