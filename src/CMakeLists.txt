cmake_minimum_required (VERSION 3.2)

set(HEADER_LOC ${PROJECT_SOURCE_DIR}/include/)
include_directories(${HEADER_LOC})
message("Header files are ${HEADER_LOC}")

add_executable(audioTest main.c)
add_executable(calibrate calibrate.c)
add_executable(audioprobe audioprobe.cpp ${HEADER_LOC}/RtAudio.h)

find_library(rtaudio_loc NAMES rtaudio)
message("rtaudio is at: ${rtaudio_loc}")

find_library(wiringPi_loc wiringPi)
message("wiringPi is at: ${wiringPi_loc}")

target_link_libraries(calibrate wiringPi ${wiringPi_loc} wiringPiPca9685)

target_link_libraries(audioprobe rtaudio rtaudio ${rtaudio_loc})
