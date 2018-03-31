/*
 * audioPlatform.c
 * 
 * Programmer: Michael Sikora <m.sikora@uky.edu>
 * Date: 2018.05.04
 * Title: audioPlatform
 * Research for: Dr. Kevin D. Donohue at University of Kentucky
 * 
 * 
 */

#include "pca9685.h" // PWM driver connected over I2C
#include "RtAudio.h" // Audio I/O library from McGill University

#include <wiringPi.h> // A very popular connections library, using for I2C 
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <ctime> // For using time/Date in filenames

// The following Formats can be used to record the raw audio data by
// uncommenting the two lines desired.
/*
typedef char MY_TYPE;
#define FORMAT RTAUDIO_SINT8

typedef signed short MY_TYPE;
#define FORMAT RTAUDIO_SINT16

typedef S24 MY_TYPE;
#define FORMAT RTAUDIO_SINT24

typedef signed long MY_TYPE;
#define FORMAT RTAUDIO_SINT32
*/
typedef float MY_TYPE;
#define FORMAT RTAUDIO_FLOAT32
/*
typedef double MY_TYPE;
#define FORMAT RTAUDIO_FLOAT64
*/

// Platform-dependent sleep routines, used when recording audio
#if defined( __WINDOWS_ASIO__ ) || defined( __WINDOWS_DS__ ) || defined( __WINDOWS_WASAPI__ )
  #include <windows.h>
  #define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds ) 
#else // Unix variants
  #include <unistd.h>
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif

// PCA9685 values
#define PIN_BASE 300 // Based on I2C address of PCA9685
#define MAX_PWM 4096 // PWM driver resolution
#define HERTZ 60 // 60 Hz is typically the fastest for micro-servos

// Calculates the length of the pulse in samples from pulse width
int calcTicks(float impulseMs, int hertz)
{
	float cycleMs = 1000.0f / hertz;
	return (int)(MAX_PWM * impulseMs / cycleMs + 0.5f);
}

// Raw multichannel audio input data struct
struct InputData {
	MY_TYPE* buffer;
	unsigned long bufferBytes;
	unsigned long totalFrames;
	unsigned long frameCounter;
	unsigned int channels;
};

struct RecSettings {
	unsigned int channels;
	unsigned int fs; 
	unsigned int bufferFrames;
	unsigned int device;
	unsigned int offset;
	double recordTime;
	unsigned long totalBytes;
};

// MultiChannel Recording to a raw file for a set number of seconds
// Interleaved buffers
int input( void * /*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus /*status*/, void *data )
{
  InputData *iData = (InputData *) data;

  // Simply copy the data to our allocated buffer.
  unsigned int frames = nBufferFrames;
  if ( iData->frameCounter + nBufferFrames > iData->totalFrames ) {
    frames = iData->totalFrames - iData->frameCounter;
    iData->bufferBytes = frames * iData->channels * sizeof( MY_TYPE );
  }

  unsigned long offset = iData->frameCounter * iData->channels;
  std::memcpy( iData->buffer+offset, inputBuffer, iData->bufferBytes );
  iData->frameCounter += frames;

  if ( iData->frameCounter >= iData->totalFrames ) return 2;
  return 0;
}

// Formerly used as a goto with label cleanup to be called when exiting
void leave(RtAudio &adc, InputData &data) {
		if ( adc.isStreamOpen() ) adc.closeStream(); // close the audio stream
		if ( data.buffer ) free( data.buffer ); // free memory
}

void resetStream(RtAudio &adc, InputData &data, RtAudio::StreamParameters &iParams, RecSettings &in3) {
		data.buffer = 0;
		if ( adc.isStreamOpen() ) {
			adc.closeStream(); 
		} // close the audio stream
			try {
				adc.openStream( NULL, &iParams, FORMAT, in3.fs, &in3.bufferFrames, &input, (void *)&data );
			}
			catch ( RtAudioError& e ) {
				std::cout << '\n' << e.getMessage() << '\n' << std::endl;
				leave(adc, data);
				return;
			}
		data.bufferBytes = in3.bufferFrames * in3.channels * sizeof( MY_TYPE );
		data.totalFrames = (unsigned long) (in3.fs * in3.recordTime);
		data.frameCounter = 0;
		data.channels = in3.channels;
		in3.totalBytes = data.totalFrames * in3.channels * sizeof( MY_TYPE );

	// Allocate the entire data buffer before starting stream.
	data.buffer = (MY_TYPE*) malloc( in3.totalBytes );
	if ( data.buffer == 0 ) {
		std::cout << "Memory allocation error ... quitting!\n";
		leave(adc, data);
		return;
	}
		
}

// Function to pause program and wait for user input
inline void WaitEnter() { std::cout << "Press Enter to continue..."; while (std::cin.get()!='\n'); }

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
	// PROGRAM OUTLINE
	// OUTLINE : 1.Setup Audio for recording only ( Or callback dump )
	RecSettings in3;
	in3.channels = 3; in3.fs = 16000; in3.bufferFrames = 1024;
	in3.device = 0; in3.offset = 0; in3.recordTime = 5.0;
	FILE *fp;
	std::vector <std::string> filenames;
	int l = 0; // location index
	int o = 0; // orientation index
	int N_l = 1; // Number of locations
	int N_o = 2; // Number of orientations
	
	/////////////// USE DATE IN FILENAME
	time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );

    char date [20]; // Character array to store current date
    strftime (date,20,"%Y_%m_%d.",now); // Date format
    std::string datePrefix = std::string(date); // get date prefix string
    std::string dir = "../output/"; // output directory location
    std::string fname; // string to store filename format
    
    // Preload Filenames
    for(l = 0; l < N_l; ++l) {
		for(o = 0; o < N_o; ++o) {
			fname = "loc" + std::to_string(l) + "_o" + std::to_string(o) + ".raw"; // filename format
			filenames.push_back(dir + datePrefix + fname);
		}
	}
	
	l = 0; o = 0; // reset l and o to 0 for future iterationW
	/////////////// 
	
	RtAudio adc;
	if ( adc.getDeviceCount() < 1 ) {
		std::cout << "\nNo audio devices found!\n";
		exit( 1 );
	}
	
	// Let RtAudio print messages to stderr.
	adc.showWarnings( true );

	// Set our stream parameters for input only.
	RtAudio::StreamParameters iParams;
	if ( in3.device == 0 )
		iParams.deviceId = adc.getDefaultInputDevice();
	else
	iParams.deviceId = in3.device;
	iParams.nChannels = in3.channels;
	iParams.firstChannel = in3.offset;
	
	InputData data;

	// OUTLINE : 2.Setup PCA9685
	int fd = pca9685Setup(PIN_BASE, 0x40, HERTZ);
	if (fd < 0)
	{
		printf("Error in setup\n");
		return fd;
	}

	pca9685PWMReset(fd);
	printf("Frequency is set to %d hertz\n", HERTZ);
	printf("Returned fd value from setup is %d\n", fd);

	int pin; // selects which servo to send PWM to
	
	// Servo pulse widths for major orientations. 
	// Program Calibrate was used to find these values
	float servo0[3] = {0.65, 1.5, 2.5}; 
	float servo1[2] = {1.35, 2.35};
	float servo2[3] = {0.55, 1.4, 2.4};
	float servo3[2] = {1.75, 2.9};
	
	// OUTLINE : 3.Wait for input to start
	WaitEnter();
		
	// SEQUENCE A
	// OUTLINE : 4.Set both platforms to planar endfire
		
			pin = 0; // CENTERED
			pwmWrite(PIN_BASE + pin, calcTicks(servo0[1], HERTZ)); 
			pin = 1; // FLAT
			pwmWrite(PIN_BASE + pin, calcTicks(servo1[0], HERTZ)); 
			delay(500); 
			printf("Orientation 1.1\n"); 
			pin = 2; // CENTERED
			pwmWrite(PIN_BASE + pin, calcTicks(servo2[1], HERTZ));  
			pin = 3; // FLAT
			pwmWrite(PIN_BASE + pin, calcTicks(servo3[0], HERTZ));  
			delay(500);
			printf("Orientation 1.2\n"); 			
			pca9685PWMReset(fd);
				
	// OUTLINE : 5.Record Audio for set number of seconds
	
	resetStream(adc,data,iParams,in3); // ReSets the audio stream for recording
	try {
		adc.startStream();
	}
	catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
		leave(adc, data);
		return 1;
	}

	std::cout << "\nRecording for " << in3.recordTime << " seconds ..." << std::endl;
	std::cout << "writing file" + filenames[0] + "(buffer frames = " << in3.bufferFrames << ")." << std::endl;
	while ( adc.isStreamRunning() ) {
		SLEEP( 100 ); // wake every 100 ms to check if we're done
	}

	// Now write the entire data to the file.
	fp = fopen( filenames[0].c_str(), "wb" );
	fwrite( data.buffer, sizeof( MY_TYPE ), data.totalFrames * in3.channels, fp );
	fclose( fp );
		
	// OUTLINE : 6.Set both platforms to planar broadside
			
			pin = 0; // CENTERED
			pwmWrite(PIN_BASE + pin, calcTicks(servo0[1], HERTZ)); 
			pin = 1; // FLAT
			pwmWrite(PIN_BASE + pin, calcTicks(servo1[1], HERTZ)); 
			delay(500); 
			printf("Orientation 2.1\n"); 
			pin = 2; // CENTERED
			pwmWrite(PIN_BASE + pin, calcTicks(servo2[1], HERTZ));  
			pin = 3; // FLAT
			pwmWrite(PIN_BASE + pin, calcTicks(servo3[1], HERTZ));  
			delay(500);
			printf("Orientation 2.2\n"); 		
			pca9685PWMReset(fd);

	// OUTLINE : 7.Record Audio for set number of seconds
	
	resetStream(adc,data,iParams,in3); // ReSets the audio stream for recording
	try {
		adc.startStream();
	}
	catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
		leave(adc, data);
		return 1;
	}

	std::cout << "\nRecording for " << in3.recordTime << " seconds ..." << std::endl;
	std::cout << "writing file" + filenames[1] + "(buffer frames = " << in3.bufferFrames << ")." << std::endl;
	while ( adc.isStreamRunning() ) {
		SLEEP( 100 ); // wake every 100 ms to check if we're done
	}

	// Now write the entire data to the file.
	fp = fopen( filenames[1].c_str(), "wb" );
	fwrite( data.buffer, sizeof( MY_TYPE ), data.totalFrames * in3.channels, fp );
	fclose( fp );
			
	// OUTLINE : 8.Wait for input to continue ( User moves arrays to new location )
	WaitEnter();
	
	// OUTLINE : 9. Repeat steps 4 through 8 for 0.5 to 3 meters

	leave(adc, data);
	return 0;
}

