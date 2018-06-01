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

typedef float MY_TYPE;
#define FORMAT RTAUDIO_FLOAT32

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
	MY_TYPE* 	  ibuffer;		 // input buffer
	unsigned long itotalBytes;   // Total Bytes for recording
	unsigned long itotalFrames;  // Total frames for recording
	unsigned long iframeCounter; // current index of recording in frames
	double		  itotalTime;
	// OUTPUT DATA
    MY_TYPE* 		  wavfile;	 // Wav File (interleaved)
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
};

// Timestamp variables
uint64_t optime;
uint64_t tic;
uint64_t toc;

// Current Timestamp
uint64_t current_timestamp() {
    struct timespec te; 
    clock_gettime(CLOCK_REALTIME, &te);
    printf("nanoseconds: %lld\n", te.tv_nsec/1000000);
    return te.tv_nsec;
}

// Calculates the length of the pulse in samples from pulse width
int calcTicks(float impulseMs, int hertz) {
	float cycleMs = 1000.0f / hertz;
	return (int)(MAX_PWM * impulseMs / cycleMs + 0.5f);
}

// MultiChannel Recording to a raw file for a set number of seconds
// with interleaved buffers
int input( void * /*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus /*status*/, void *userData );
		   
int inout( void *outputBuffer, void *inputBuffer, unsigned int /*nBufferFrames*/,
           double /*streamTime*/, RtAudioStreamStatus status, void *userData );
  
int outFromWav( void *outputBuffer, void* inputBuffer, unsigned int /*nBufferFrames*/,
           double /*streamTime*/, RtAudioStreamStatus status, void *userData );
		   
// Cleanup to be called when exiting		   
void leave( RtAudio &adc, Data &data );
  
// Function to pause program and wait for user input
inline void WaitEnter() { std::cout << "Press Enter to continue..."; while (std::cin.get()!='\n'); }

// Reads in a wav file and gets information from the file header
static void read_wav_file ( const char* fname, Data* userData ) { 
	//~ SndfileHandle file;
	//~ file = SndfileHandle(fname);
	
	//~ Data *localdata = (Data*) userData;
	
	//~ printf("Opened file '%s' \n", fname);
	//~ printf(" Sample rate : %d \n", file.samplerate());
	//~ printf(" Channels    : %d \n", file.channels());
	//~ printf(" Frames    : %d \n", (int)file.frames());
	//~ printf(" Format    : %d \n", file.format());
	//~ printf(" Writing %d Frames to wavfile \n", (*userData).ototalFrames);
	
	//~ userData->ochannels = file.channels();
	//~ printf(" Point A ");
	
	//~ // Allocate the entire data buffer before starting stream.
	//~ userData->wavfile = (float*) malloc( (*userData).ototalFrames );
	//~ // Assumes wav file is in proper format with settings pre-defined to match program
	//~ file.read(userData->wavfile , (*userData).ototalFrames);
	//~ puts("");
	//~ printf("File loaded\n");
}

// MAIN TASKS TO RUN
typedef void* (*func_ptr)(void*); // Callback array for tasks
void* task_AUDIOINOUT(void* arg);
void* task_AUDIOIN(void* arg);
void* task_AUDIOOUT(void* arg);
void* task_PANTILTDEMO(void* arg);
void* task_WAVREAD(void* arg);

int main(int argc, char **argv);
