cmake_minimum_required (VERSION 3.2)

#project (AudioPlatformv2)

#include (CTest)

#enable_testing()

set(HEADER_LOC ${PROJECT_SOURCE_DIR}/include/)
include_directories(${HEADER_LOC})
message("Header files are ${HEADER_LOC}")

#~ add_executable(audioTest tests/main.c)
#~ add_executable(calibrate tests/calibrate.c)
#~ add_executable(audioprobe tests/audioprobe.cpp ${HEADER_LOC}/RtAudio.h)
#~ add_executable(duplex tests/duplex.cpp ${HEADER_LOC}/RtAudio.h)
#~ add_executable(threshold tests/threshold.cpp ${HEAER_LOC}/RtAudio.h)
#~ add_executable(sine tests/sine.cpp ${HEADER_LOC}/RtAudio.h)
#~ add_executable(callbacktest tests/callbacktest.cpp ${HEADER_LOC}/RtAudio.h)
#~ add_executable(servoDemo tests/servoDemo.cpp ${HEADER_LOC}/RtAudio.h)
#~ add_executable(audioPlatform tests/audioPlatform.cpp ${HEADER_LOC}/RtAudio.h)
add_executable(audioPlatformv2 audioPlatformv2.cpp audioPlatformv2.h)
#~ add_executable(playthrough playthrough.cpp ${HEADER_LOC}/RtAudio.h)

#add_test(example_test audioPlatformv2)

#~ Find Libraries
find_library(rtaudio_loc NAMES rtaudio)
message("rtaudio is at: ${rtaudio_loc}")

find_library(wiringPi_loc wiringPi)
message("wiringPi is at: ${wiringPi_loc}")

find_library(pthread_loc pthread)
message("pthread is at: ${pthread_loc}")

find_library(sndfile_loc sndfile)
message("libsndfile is at: ${sndfile_loc}")

#~ target_link_libraries(calibrate wiringPi ${wiringPi_loc} wiringPiPca9685)
#~ target_link_libraries(audioprobe rtaudio rtaudio ${rtaudio_loc})
#~ target_link_libraries(audioPlatform rtaudio rtaudio ${rtaudio_loc})
#~ target_link_libraries(audioPlatform wiringPi ${wiringPi_loc} wiringPiPca9685)
#~ target_link_libraries(duplex rtaudio rtaudio ${rtaudio_loc})
#~ target_link_libraries(threshold rtaudio rtaudio ${rtaudio_loc})
#~ target_link_libraries(threshold wiringPi ${wiringPi_loc} wiringPiPca9685)
#~ target_link_libraries(threshold pthread ${pthread_loc} pthread)
#~ target_link_libraries(sine rtaudio rtaudio ${rtaudio_loc})
#~ target_link_libraries(callbacktest rtaudio rtaudio ${rtaudio_loc})
#~ target_link_libraries(servoDemo rtaudio rtaudio ${rtaudio_loc})
#~ target_link_libraries(servoDemo wiringPi ${wiringPi_loc} wiringPiPca9685)
target_link_libraries(audioPlatformv2 rtaudio rtaudio ${rtaudio_loc})
target_link_libraries(audioPlatformv2 wiringPi ${wiringPi_loc} wiringPiPca9685)
target_link_libraries(audioPlatformv2 pthread ${pthread_loc} pthread)
target_link_libraries(audioPlatformv2 sndfile ${sndfile_loc} pthread)
#~ target_link_libraries(playthrough rtaudio rtaudio ${rtaudio_loc})
