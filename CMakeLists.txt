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

# Video test:
add_executable(test_playback test_playback.cpp CImg.h)
if(WIN32)
target_link_libraries(test_playback vidio)
else()
target_link_libraries(test_playback vidio X11 pthread)
endif()

# Audio test:
add_executable(test_audio_playback test_audio_playback.cpp)
if(WIN32)
target_link_libraries(test_audio_playback vidio)
else()
target_link_libraries(test_audio_playback vidio X11 pthread)
endif()

# Subprocess test:
add_executable(test_subprocess_oop test_subprocess_oop.cpp subprocess.hpp)

# Video demo test:
add_executable(vidio_demo_play_file demo_play_file.cpp CImg.h)
if(WIN32)
target_link_libraries(vidio_demo_play_file vidio)
else()
target_link_libraries(vidio_demo_play_file vidio X11 pthread)
endif()

# Named pipe test:
add_library(named_pipe
	named_pipe.hpp
	named_pipe.cpp
)
target_include_directories(named_pipe INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(test_named_pipe test_named_pipe.cpp)
target_link_libraries(test_named_pipe named_pipe)

endif()
