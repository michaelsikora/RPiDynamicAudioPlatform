/*
 * audioPlatformv2.c 
 * 
 * Programmer: Michael Sikora <m.sikora@uky.edu>
 * Date: 2018.05.23
 * Title: audioPlatform
 * Research for: Dr. Kevin D. Donohue at University of Kentucky
 * 
 * This version was made to develop the simultaneous servo and audio threads
 * 
 */

#include "pca9685.h" // PWM driver connected over I2C
#include "RtAudio.h" // Audio I/O library from McGill University
#include "audioPlatformv2.h"

#include <wiringPi.h> // A very popular connections library, using for I2C 
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <ctime> // For using time/Date in filenames
#include <pthread_t>

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
	
	int err;
	pthread_t thread1, thread2;
	
	if ((err = pthread_create(&thread1, NULL, &audio_task, NULL)) != 0) // Error occurs
		printf("Thread creation failed: &s \n", strerror(err));
		
	
	
	
	pthread_cancel(thread1);
	pthread_join(thread1,NULL);
	
	
	
	
	
/* 	// PROGRAM OUTLINE
	// OUTLINE : 1.Setup Audio for recording only ( Or callback dump )
	
	RecSettings rec; // Three inputs and one output	
	InputData data; // data struct for capturing audio
	rec.channels = 3; rec.fs = 16000; rec.bufferFrames = 1024;
	rec.device = 0; rec.offset = 0; rec.recordTime = 5.0;
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
	if ( rec.device == 0 )
		iParams.deviceId = adc.getDefaultInputDevice();
	else
	iParams.deviceId = rec.device;
	iParams.nChannels = rec.channels;
	iParams.firstChannel = rec.offset;
	
	
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
	
	resetStream(adc,data,iParams,rec); // ReSets the audio stream for recording
	try {
		adc.startStream();
	}
	catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
		leave(adc, data);
		return 1;
	}

	std::cout << "\nRecording for " << rec.recordTime << " seconds ..." << std::endl;
	std::cout << "writing file" + filenames[0] + "(buffer frames = " << rec.bufferFrames << ")." << std::endl;
	while ( adc.isStreamRunning() ) {
		SLEEP( 100 ); // wake every 100 ms to check if we're done
	}

	// Now write the entire data to the file.
	fp = fopen( filenames[0].c_str(), "wb" );
	fwrite( data.buffer, sizeof( MY_TYPE ), data.totalFrames * rec.channels, fp );
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
	
	resetStream(adc,data,iParams,rec); // ReSets the audio stream for recording
	try {
		adc.startStream();
	}
	catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
		leave(adc, data);
		return 1;
	}

	std::cout << "\nRecording for " << rec.recordTime << " seconds ..." << std::endl;
	std::cout << "writing file" + filenames[1] + "(buffer frames = " << rec.bufferFrames << ")." << std::endl;
	while ( adc.isStreamRunning() ) {
		SLEEP( 100 ); // wake every 100 ms to check if we're done
	}

	// Now write the entire data to the file.
	fp = fopen( filenames[1].c_str(), "wb" );
	fwrite( data.buffer, sizeof( MY_TYPE ), data.totalFrames * rec.channels, fp );
	fclose( fp );
			
	// OUTLINE : 8.Wait for input to continue ( User moves arrays to new location )
	WaitEnter();
	
	// OUTLINE : 9. Repeat steps 4 through 8 for 0.5 to 3 meters

	leave(adc, data); */
	return 0;
}



// Calculates the length of the pulse in samples from pulse width
int calcTicks(float impulseMs, int hertz)
{
	float cycleMs = 1000.0f / hertz;
	return (int)(MAX_PWM * impulseMs / cycleMs + 0.5f);
}


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

void leave(RtAudio &adc, InputData &data) {
		if ( adc.isStreamOpen() ) adc.closeStream(); // close the audio stream
		if ( data.buffer ) free( data.buffer ); // free memory
}

void resetStream(RtAudio &adc, InputData &data, RtAudio::StreamParameters &iParams, RecSettings &rec) {
		data.buffer = 0;
		if ( adc.isStreamOpen() ) {
			adc.closeStream(); 
		} // close the audio stream
			try {
				adc.openStream( NULL, &iParams, FORMAT, rec.fs, &rec.bufferFrames, &input, (void *)&data );
			}
			catch ( RtAudioError& e ) {
				std::cout << '\n' << e.getMessage() << '\n' << std::endl;
				leave(adc, data);
				return;
			}
		data.bufferBytes = rec.bufferFrames * rec.channels * sizeof( MY_TYPE );
		data.totalFrames = (unsigned long) (rec.fs * rec.recordTime);
		data.frameCounter = 0;
		data.channels = rec.channels;
		rec.totalBytes = data.totalFrames * rec.channels * sizeof( MY_TYPE );

	// Allocate the entire data buffer before starting stream.
	data.buffer = (MY_TYPE*) malloc( rec.totalBytes );
	if ( data.buffer == 0 ) {
		std::cout << "Memory allocation error ... quitting!\n";
		leave(adc, data);
		return;
	}
		
}

void* ThreadStart(void* arg) {
	int threadNum = *((int*)arg);
	printf("hello world from thread %d\n", threadNum);
	
	return NULL;
}
