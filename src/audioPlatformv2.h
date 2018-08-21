/*
	Michael Sikora <m.sikora@uky.edu>
	2018.05.31

*/

#include "pca9685.h" // PWM driver connected over I2C
#include "RtAudio.h" // Audio I/O library from McGill University
#include "sndfile.hh"

#include <wiringPi.h> // A very popular connections library, using for I2C 
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <sys/time.h> // For UNIX Timestamps
#include <ctime> // For using time/Date in filenames
#include <pthread.h> // POSIX multi-threading

// Audio data format
typedef float MY_TYPE;
#define FORMAT RTAUDIO_SINT16

// Sleep routines, used when recording audio
#include <unistd.h>
#define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )

// PCA9685 values
#define PIN_BASE 300 // Based on I2C address of PCA9685
#define MAX_PWM 4096 // PWM driver resolution
#define HERTZ 60 // 60 Hz is typically the fastest for micro-servos

// Raw multichannel audio input data struct
struct Data {
	// BUFFER DATA
	unsigned int bufferBytes;   // Number of Bytes in buffer
	unsigned int bufferFrames;  // Number of Frames in buffer
	// INPUT DATA
	unsigned long itotalBytes;   // Total Bytes for recording
	unsigned long itotalFrames;  // Total frames for recording
	unsigned long iframeCounter; // current index of recording in frames
	double		  itotalTime;
	// OUTPUT DATA
	unsigned long ototalBytes;   // Total Bytes in wavfile
	unsigned long ototalFrames;   // Total frames in wavfile	
    unsigned long  oframeCounter; // current index of wavfile in frames
	double		  ototalTime;
	// AUDIO SETTINGS
	unsigned int  ichannels;  	 // number of channels to input
	unsigned int  ochannels;  	 // number of channels to output
	unsigned int  fs; 			 // sampling frequency
	unsigned int  device;		 // device id
	unsigned int  offset;		 // channel offset
	// BUFFERS
	float* 	  ibuffer;		 // input buffer
    float* 	  wavfile;	 // Wav File (interleaved)
};

// Timestamp variables
struct timespec tic; 
struct timespec toc;
long optime_sec;
long optime_nsec;

// Current Timestamp
struct timespec current_timestamp() { // TIC
    struct timespec te; 
    clock_gettime(CLOCK_REALTIME, &te);
    return te;
}

struct timespec current_timestamp(struct timespec tic) { // TOC
    struct timespec toc; 
    clock_gettime(CLOCK_REALTIME, &toc);
	optime_sec = toc.tv_sec - tic.tv_sec;
	optime_nsec = toc.tv_nsec - tic.tv_nsec;
	long optime = optime_sec*1000000 + optime_nsec/1000;
	printf("        Delay: 0.%.4d s \n", optime);
	return toc;
}

// Calculates the length of the pulse in samples from pulse width
int calcTicks(float impulseMs, int hertz) {
	float cycleMs = 1000.0f / hertz;
	return (int)(MAX_PWM * impulseMs / cycleMs + 0.5f);
}

#ifdef __cplusplus
extern "C" {
#endif

// MultiChannel Recording to a raw file for a set number of seconds
// with interleaved buffers
int input( void * /*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus /*status*/, void *userData );
		   
int inoutdirect( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus status, void *userData );

int inout( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *userData );
   
int outFromWav( void *outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus status, void *userData );
		   
// Cleanup to be called when exiting		   
void leave( RtAudio &adc, Data &data );
  
#ifdef __cplusplus
}
#endif

// Function to pause program and wait for user input
inline void WaitEnter() { std::cout << "Press Enter to continue..."; while (std::cin.get()!='\n'); }
void printHelp();

// MAIN TASKS TO RUN
typedef void* (*func_ptr)(void*); // Callback array for tasks
void* task_AUDIOINOUT(void* arg);
void* task_AUDIOIN(void* arg);
void* task_AUDIOOUT(void* arg);
void* task_WAVREAD(void* arg);

void* task_PANTILTRAND(void* arg);
void* task_PANTILTITERATE(void* arg);

void* task_NULL(void* arg){ return NULL; };

// Main program, will run tasks in parallel
int main(int argc, char **argv);
