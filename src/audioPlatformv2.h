



#include "pca9685.h" // PWM driver connected over I2C
#include "RtAudio.h" // Audio I/O library from McGill University
#include "sndfile.hh"

#include <wiringPi.h> // A very popular connections library, using for I2C 
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <vector>
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

#define BUFFER_LEN 48000 // 3 seconds at 16kHz

// Raw multichannel audio input data struct
struct Data {
	MY_TYPE* buffer;
	//~ MY_TYPE* outBuffer;
	unsigned long bufferBytes;
	unsigned long totalFrames;
	unsigned long frameCounter;
	unsigned int channels;
};

// Audio I/O settings for user interfacing
struct ioSettings {
	unsigned int channels;
	unsigned int fs; 
	unsigned int bufferFrames;
	unsigned int device;
	unsigned int offset;
	double recordTime;
	unsigned long totalBytes;
};

// Calculates the length of the pulse in samples from pulse width
int calcTicks(float impulseMs, int hertz); 

// MultiChannel Recording to a raw file for a set number of seconds
// with interleaved buffers
int input( void * /*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus /*status*/, void *iData );
		   
int inout( void *outputBuffer, void *inputBuffer, unsigned int /*nBufferFrames*/,
           double /*streamTime*/, RtAudioStreamStatus status, void *iData );
           
// Cleanup to be called when exiting		   
void leave( RtAudio &adc, Data &data );

// Resets the audio stream
void resetStream( RtAudio &adc, Data &iData, RtAudio::StreamParameters &oParams,
				  RtAudio::StreamParameters &iParams, ioSettings &rec );
		   
// Function to pause program and wait for user input
inline void WaitEnter() { std::cout << "Press Enter to continue..."; while (std::cin.get()!='\n'); }

static void read_wav_file ( const char* fname, float* buffer ) { 
	SndfileHandle file;
	file = SndfileHandle(fname);
	
	printf("Opened file '%s' \n", fname);
	printf(" Sample rate : %d \n", file.samplerate());
	printf(" Channels    : %d \n", file.channels());
	printf(" Frames    : %d \n", (int)file.frames());
	printf(" Format    : %d \n", file.format());
	
	int length = (int) file.frames();
	if (BUFFER_LEN < file.samplerate()*10) {
		file.read(buffer, BUFFER_LEN);
		puts("");
	} else {
		printf("File is too large");
	}
}

// MAIN TASKS TO RUN
typedef void* (*func_ptr)(void*); // Callback array for tasks
void* task_AUDIO(void* arg);
void* task_PANTILT(void* arg);
void* task_WAVREAD(void* arg);

int main(int argc, char **argv);
