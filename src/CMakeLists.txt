cmake_minimum_required (VERSION 3.2)

set(HEADER_LOC ${PROJECT_SOURCE_DIR}/include/)
include_directories(${HEADER_LOC})
message("Header files are ${HEADER_LOC}")

#~ add_executable(audioTest main.c)
#~ add_executable(calibrate calibrate.c)
#~ add_executable(audioprobe audioprobe.cpp ${HEADER_LOC}/RtAudio.h)
add_executable(audioPlatform audioPlatform.cpp ${HEADER_LOC}/RtAudio.h)
#~ add_executable(duplex duplex.cpp ${HEADER_LOC}/RtAudio.h)
#~ add_executable(threshold threshold.cpp ${HEAER_LOC}/RtAudio.h)
#~ add_executable(sine sine.cpp ${HEADER_LOC}/RtAudio.h)
#~ add_executable(callbacktest callbacktest.cpp ${HEADER_LOC}/RtAudio.h)
add_executable(servoDemo servoDemo.cpp ${HEADER_LOC}/RtAudio.h)

#~ Find Libraries
find_library(rtaudio_loc NAMES rtaudio)
message("rtaudio is at: ${rtaudio_loc}")

find_library(wiringPi_loc wiringPi)
message("wiringPi is at: ${wiringPi_loc}")

#~ find_library(pthread_loc pthread)
#~ message("pthread is at: ${pthread_loc}")

#~ target_link_libraries(calibrate wiringPi ${wiringPi_loc} wiringPiPca9685)
#~ target_link_libraries(audioprobe rtaudio rtaudio ${rtaudio_loc})
target_link_libraries(audioPlatform rtaudio rtaudio ${rtaudio_loc})
target_link_libraries(audioPlatform wiringPi ${wiringPi_loc} wiringPiPca9685)
#~ target_link_libraries(duplex rtaudio rtaudio ${rtaudio_loc})
#~ target_link_libraries(threshold rtaudio rtaudio ${rtaudio_loc})
#~ target_link_libraries(threshold wiringPi ${wiringPi_loc} wiringPiPca9685)
#~ target_link_libraries(threshold pthread ${pthread_loc} pthread)
#~ target_link_libraries(sine rtaudio rtaudio ${rtaudio_loc})
#~ target_link_libraries(callbacktest rtaudio rtaudio ${rtaudio_loc})
#~ target_link_libraries(servoDemo rtaudio rtaudio ${rtaudio_loc})
target_link_libraries(servoDemo wiringPi ${wiringPi_loc} wiringPiPca9685)
