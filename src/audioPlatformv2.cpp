/*
 * audioPlatformv2.cpp
 * 
 * Programmer: Michael Sikora <m.sikora@uky.edu>
 * Date: 2018.05.23
 * Title: audioPlatform
 * Research for: Dr. Kevin D. Donohue at University of Kentucky
 * 
 * This version was made to develop the simultaneous servo and audio threads
 * 
 */

#include "audioPlatformv2.h"

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter = 0; // iterator to test threads
	
// Servo pulse widths for major orientations. 
// Program Calibrate was used to find these values
float servo0[3] = {0.65, 1.5, 2.5}; 
float servo1[2] = {1.35, 2.35};
float servo2[3] = {0.55, 1.4, 2.4};
float servo3[2] = {1.75, 2.9};
float servo4[3] = {0.15, 1.55, 2.55}; 
float servo5[2] = {1.1, 2.0};

/////////////////////////////////////////////////////////////////////////////////
int input( void * /*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus /*status*/, void *userData ) {
  
  Data* localData = (Data *) userData;

  // Simply copy the data to our allocated buffer.
  unsigned int frames = nBufferFrames;
  if ( (localData->iframeCounter + nBufferFrames) > localData->itotalFrames ) { // The next buffer will overflow
    frames = localData->itotalFrames - localData->iframeCounter; // Get number of frames to avoid overflow
    localData->bufferBytes = frames * localData->ichannels * sizeof( MY_TYPE ); // Set non-overflow buffer size
  }

  unsigned long offset = localData->iframeCounter * localData->ichannels;
  std::memcpy( localData->ibuffer+offset, inputBuffer, localData->bufferBytes );
  localData->iframeCounter += frames;

  if ( localData->iframeCounter >= localData->itotalFrames ) return 2;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////
int inout( void *outputBuffer, void *inputBuffer, unsigned int /*nBufferFrames*/,
           double /*streamTime*/, RtAudioStreamStatus status, void *userData ) {
	
  Data* localData = (Data *) userData;
	  
  // Since the number of input and output channels is equal, we can do
  // a simple buffer copy operation here.
  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  unsigned int *bytes = (unsigned int *) localData->bufferBytes;
  std::memcpy( outputBuffer, inputBuffer, *bytes );
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////
int outFromWav( void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus status, void *userData ) {
	
  printf("running\n");
  Data* localData = (Data *) userData; // get user data
  // float *obuf = (float*) outputBuffer; // get output buffer for iterating
  // float *ibuf = (float*) inputBuffer; // get input buffer for iterating
  unsigned int frames = nBufferFrames;
  
  printf("running\n");
  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  if ( localData->oframeCounter + nBufferFrames > localData->ototalFrames) { // The next buffer will overflow
    frames = localData->ototalFrames - localData->oframeCounter; // Get number of frames to avoid overflow
    localData->bufferBytes = frames * localData->ochannels * sizeof( MY_TYPE ); // Set non-overflow buffer size
  }
  
  // copy buffer from wavtable to output
  unsigned long offset = localData->oframeCounter * localData->ochannels;
  std::memcpy( outputBuffer, localData->wavfile+offset, localData->bufferBytes );
  localData->oframeCounter += frames;

  if ( localData->oframeCounter >= localData->ototalFrames ) return 2; // Overflow
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////
void leave(RtAudio &adac, Data &userData) {
		if ( adac.isStreamOpen() ) adac.closeStream(); // close the audio stream
		if ( userData.ibuffer ) free( userData.ibuffer ); // free memory
		if ( userData.wavfile ) free( userData.wavfile ); // free memory
}

/////////////////////////////////////////////////////////////////////////////////
void *task_AUDIOINOUT(void* arg) {
	////// Testing Thread with counter mutex
	int threadNum = *((int*)arg);
	printf("hello world from AUDIO thread %d\n", threadNum);
	
	pthread_mutex_lock( &mutex1 );
	counter += 1;
	printf("Counter value %d\n", counter);
	pthread_mutex_unlock( &mutex1 );
	
	////// Audio Settings
	Data userData; // data struct for sending data to audio callback
	userData.ichannels = 3; // Integer
	userData.fs = 16000; // Hertz
	userData.bufferFrames = 512; // number of frames in buffer
	userData.device = 0; 
	userData.offset = 0; 
	userData.itotalTime = 5.0;
	FILE *fp; // File for output
	std::vector <std::string> filenames; // Store filenames
	int l = 0; // location index
	int o = 0; // orientation index
	int N_l = 1; // Number of locations
	int N_o = 1; // Number of orientations
	
	/////////////// GENERATE FILENAMES WITH DATE
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
	l = 0; o = 0; // reset l and o to 0 for future iteration
	/////////////// 
	
	RtAudio adc;
	if ( adc.getDeviceCount() < 1 ) {
		std::cout << "\nNo audio devices found!\n";
		exit( 1 );
	}
	
	adc.showWarnings( true ); // Let RtAudio print messages to stderr.

	// Set our stream parameters for input only.
	RtAudio::StreamParameters iParams;
	iParams.deviceId = userData.device;
	iParams.nChannels = userData.ichannels;
	iParams.firstChannel = userData.offset;
	if(userData.device == 0) iParams.deviceId = adc.getDefaultInputDevice();
	////////////////
	
	// OUTLINE : Record Audio for set number of seconds
	RtAudio::StreamOptions options; // For setting RtAudio built in stream options
	userData.ibuffer = 0; userData.iframeCounter = 0;
	userData.bufferBytes = userData.bufferFrames * userData.ichannels * sizeof( MY_TYPE );
	userData.itotalFrames = (unsigned long) (userData.fs * userData.itotalTime);
	userData.itotalBytes = userData.itotalFrames * userData.ichannels * sizeof( MY_TYPE );
		
	if(adc.isStreamOpen()) adc.closeStream(); // if an audio stream is already open close it
		
	try {
		adc.openStream( NULL, &iParams, FORMAT, userData.fs, &userData.bufferFrames, &input, (void *)&userData );
	} catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
		leave(adc, userData);
		return NULL;
	}

	// Allocate the entire data buffer before starting stream.
	userData.ibuffer = (MY_TYPE*) malloc( userData.itotalBytes );
	if ( userData.ibuffer == 0 ) {
		std::cout << "Memory allocation error ... quitting!\n";
		leave(adc, userData);
		return NULL;
	}

	// RUN STREAM
	try {
		printf("Starting Stream... \n");
		adc.startStream();

		std::cout << "\nStream latency = " << adc.getStreamLatency() << " frames" << std::endl;
		std::cout << "\nRecording for " << userData.itotalTime << " seconds ..." << std::endl;
		std::cout << "writing file" + filenames[0] + "(buffer frames = " << userData.bufferFrames << ")." << std::endl;
		while ( adc.isStreamRunning() ) {
			SLEEP( 100 ); // wake every 100 ms to check if we're done
		}

		// Stop the stream.
		adc.stopStream();
		leave(adc, userData);
	} catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
		leave(adc, userData);
		return NULL;
	}

	// Now write the entire data to the file.
	fp = fopen( filenames[0].c_str(), "wb" );
	fwrite( userData.ibuffer, sizeof( MY_TYPE ), userData.itotalFrames * userData.ichannels, fp );
	fclose( fp );
	printf("Recording Complete\n");
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////
void *task_AUDIOIN(void* arg) {
	////// Testing Thread with counter mutex
	int threadNum = *((int*)arg);
	printf("hello world from AUDIO thread %d\n", threadNum);
	
	pthread_mutex_lock( &mutex1 );
	counter += 1;
	printf("Counter value %d\n", counter);
	pthread_mutex_unlock( &mutex1 );
	
	////// Audio Settings
	Data userData; // data struct for sending data to audio callback
	userData.ichannels = 3; // Integer
	userData.fs = 16000; // Hertz
	userData.bufferFrames = 1024; // number of frames in buffer
	userData.device = 0; 
	userData.offset = 0; 
	userData.itotalTime = 5.0;
	FILE *fp; // File for output
	std::vector <std::string> filenames; // Store filenames
	int l = 0; // location index
	int o = 0; // orientation index
	int N_l = 1; // Number of locations
	int N_o = 1; // Number of orientations
	
	/////////////// GENERATE FILENAMES WITH DATE
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
	l = 0; o = 0; // reset l and o to 0 for future iteration
	/////////////// 
	
	RtAudio adc;
	if ( adc.getDeviceCount() < 1 ) {
		std::cout << "\nNo audio devices found!\n";
		exit( 1 );
	}
	
	adc.showWarnings( true ); // Let RtAudio print messages to stderr.

	// Set our stream parameters for input only.
	RtAudio::StreamParameters iParams;
	iParams.deviceId = userData.device;
	iParams.nChannels = userData.ichannels;
	iParams.firstChannel = userData.offset;
	if(userData.device == 0) iParams.deviceId = adc.getDefaultInputDevice();
	////////////////
	
	// OUTLINE : Record Audio for set number of seconds
	RtAudio::StreamOptions options; // For setting RtAudio built in stream options
	userData.ibuffer = 0; 
	userData.iframeCounter = 0;
	userData.bufferBytes = userData.bufferFrames * userData.ichannels * sizeof( MY_TYPE );
	userData.itotalFrames = (unsigned long) (userData.fs * userData.itotalTime);
	userData.itotalBytes = userData.itotalFrames * userData.ichannels * sizeof( MY_TYPE );
  
	printf("\n %d, %d, %d \n", userData.bufferBytes, userData.itotalFrames,userData.itotalBytes);
	
	if(adc.isStreamOpen()) adc.closeStream(); // if an audio stream is already open close it
		
	try {
		adc.openStream( NULL, &iParams, FORMAT, userData.fs, &userData.bufferFrames, &input, (void *)&userData );
	} catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
		leave(adc, userData);
		return NULL;
	}

	// Allocate the entire data buffer before starting stream.
	userData.ibuffer = (MY_TYPE*) malloc( userData.itotalBytes  );
	if ( userData.ibuffer == 0 ) {
		std::cout << "Memory allocation error ... quitting!\n";
		leave(adc, userData);
		return NULL;
	}

	// RUN STREAM
	try {
		printf("Starting Stream... \n");
		adc.startStream();
	} catch ( RtAudioError& e ) {
		std::cout << "\n !!! " << e.getMessage() << '\n' << std::endl;
		leave(adc, userData);
		return NULL;
	}
	
	std::cout << "\nStream latency = " << adc.getStreamLatency() << " frames" << std::endl;
	std::cout << "\nRecording for " << userData.itotalTime << " seconds ..." << std::endl;
	std::cout << "writing file" + filenames[0] + " \n(buffer frames = " << userData.bufferFrames << ")." << std::endl;
	while ( adc.isStreamRunning() ) {
		SLEEP( 100 ); // wake every 100 ms to check if we're done
	}
		
	printf("DONE");
	// Stop the stream.
	leave(adc, userData);

	// Now write the entire data to the file.
	fp = fopen( filenames[0].c_str(), "wb" );
	fwrite( userData.ibuffer, sizeof( MY_TYPE ), userData.itotalFrames * userData.ichannels, fp );
	fclose( fp );
	printf("Recording Complete\n");
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////
void *task_AUDIOOUT(void* arg) {
	////// Testing Thread with counter mutex
	int threadNum = *((int*)arg);
	printf("hello world from AUDIO thread %d\n", threadNum);
	
	pthread_mutex_lock( &mutex1 );
	counter += 1;
	printf("Counter value %d\n", counter);
	pthread_mutex_unlock( &mutex1 );
	
	////// Audio Settings	
	RtAudio dac;
	Data userData; // data struct for sending data to audio callback
	userData.fs = 16000; // Hertz
	userData.bufferFrames = 512; // number of frames in buffer
	userData.device = 0; 
	userData.offset = 0; 
	userData.ototalTime = 3.0;
	userData.ototalFrames = (unsigned long) (userData.fs * userData.ototalTime);
	
	////// Read in Wav
	const char* fname = "../input/filteredWN.wav";
	//~ const char* fname = "../input/filteredSine220.wav";
	SndfileHandle file;
	file = SndfileHandle(fname);
	
	printf("Opened file '%s' \n", fname);
	printf(" Sample rate : %d \n", file.samplerate());
	printf(" Channels    : %d \n", file.channels());
	printf(" Frames    : %d \n", (int)file.frames());
	printf(" Format    : %d \n", file.format());
	printf(" Writing %d Frames to wavfile \n", userData.ototalFrames);
	printf(" sizeof float: %d, and sizeof MY TYPE %d \n", sizeof(float), sizeof(MY_TYPE));
	
	userData.ochannels = file.channels();
	userData.wavfile = 0;
	// Allocate the entire data buffer before starting stream.
	userData.wavfile = (float*) calloc( userData.ototalFrames, sizeof(float) );
	// Assumes wav file is in proper format with settings pre-defined to match program
	file.read(userData.wavfile, userData.ototalFrames);
	close(file);
	printf("File loaded\n");
	//////////////

	if ( dac.getDeviceCount() < 1 ) {
		std::cout << "\nNo audio devices found!\n";
		exit( 1 );
	}
	
	dac.showWarnings( true ); // Let RtAudio print messages to stderr.
	
	printf("Point A.. \n");
	
	// Set our stream parameters for input only.
	RtAudio::StreamParameters oParams, iParams;
	oParams.deviceId = userData.device;
	oParams.nChannels = userData.ochannels;
	oParams.firstChannel = userData.offset;
	iParams.deviceId = userData.device;
	iParams.nChannels = userData.ochannels;
	iParams.firstChannel = userData.offset;
	if(userData.device == 0) oParams.deviceId = dac.getDefaultInputDevice();
	////////////////
	
	// OUTLINE : Record Audio for set number of seconds
	RtAudio::StreamOptions options; // For setting RtAudio built in stream options
	userData.oframeCounter = 0;
	userData.bufferBytes = userData.bufferFrames * userData.ochannels * sizeof( MY_TYPE );
	userData.ototalBytes = userData.ototalFrames * userData.ochannels * sizeof( MY_TYPE );
		
	//~ if(dac.isStreamOpen()) dac.closeStream(); // if an audio stream is already open close it
		
	printf("Point B.. \n");
	try {
		dac.openStream( &oParams, &iParams, FORMAT, userData.fs, &userData.bufferFrames, &outFromWav, (void *)&userData );
	} catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
	printf("Point C.. \n");
		leave(dac, userData);
		return NULL;
	}

	// RUN STREAM
	try {
		printf("Starting Stream... \n");
		dac.startStream();

		std::cout << "\nStream latency = " << dac.getStreamLatency() << " frames" << std::endl;
		std::cout << "\nRecording for " << userData.ototalTime << " seconds ..." << std::endl;
		while ( dac.isStreamRunning() ) {
			SLEEP( 100 ); // wake every 100 ms to check if we're done
		}
		
		// Stop the stream.
		dac.closeStream();
		leave(dac, userData);
	} catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
		leave(dac, userData);
		return NULL;
	}

	printf("Recording Complete\n");
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////
void *task_WAVREAD(void* arg) {
	int threadNum = *((int*)arg);
	printf("hello world from WAVREAD thread %d\n", threadNum);
	
	//~ Data* userData; // data struct for sending data to audio callback
	//~ userData->ototalTime = 3.0;
	
	//~ const char* fname = "../input/filteredWN.wav";
	//~ read_wav_file( fname, userData );
	
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////
void *task_PANTILTDEMO(void* arg) {
	int threadNum = *((int*)arg);
	printf("hello world from PANTILT thread %d\n", threadNum);

	pthread_mutex_lock( &mutex1 );
	counter++;
	printf("Counter value %d\n", counter);
	pthread_mutex_unlock( &mutex1 );
	
	// OUTLINE : 2.Setup PCA9685
	int fd = pca9685Setup(PIN_BASE, 0x40, HERTZ);
	if (fd < 0)
	{
		printf("Error in setup\n");
		printf(" Error: %s \n", strerror(fd));
		return NULL;
	}

	pca9685PWMReset(fd);
	printf("Frequency is set to %d hertz\n", HERTZ);
	printf("Returned fd value from setup is %d\n", fd);
	
	
////
	srand(time(0));
	int pin; // selects which servo to send PWM to
	int N = 10; // Number of iterations for demo
	int N_servo = 2; // Number of servos to run
	float num[N_servo];
	float upper[N_servo];
	float lower[N_servo];
    float servoarray[N_servo][N];
    float servospan[N_servo];
    float servoinc[N_servo];
    
	lower[0] = servo4[0];
    upper[0] = servo4[2];
	lower[1] = servo5[0];
    upper[1] = servo5[1];
    for(int ss = 0; ss < N_servo; ++ss) {
		servospan[ss] = upper[ss]-lower[ss];
		servoinc[ss] = servospan[ss]/(N-1);
		for(int jj = 0; jj < N; ++jj) {
			servoarray[ss][jj] = upper[ss]-servoinc[ss]*jj;
		}
	}
////
	
// Random Orientations	
	for(int jj = 0; jj < N; ++jj) { 
		tic = current_timestamp();
	    for(int ss = 0; ss < N_servo; ++ss) {	
			num[ss] = ((float)rand()/(float)(RAND_MAX/1)*(upper[ss]-lower[ss]))+lower[ss];
			printf(": %1.3f : ",num[ss]);
			pwmWrite(PIN_BASE + ss, calcTicks(num[ss], HERTZ)); 
		}
		toc = current_timestamp();
		optime = (toc-tic)/1000000;
		printf(" Delay: %lld ms \n", optime);
		delay(500);    		 
	}
	pca9685PWMReset(fd);
	delay(2000);
	
// Iterative Orientations
	for(int jj = 0; jj < N; ++jj) { 
	    for(int ss = 0; ss < N_servo; ++ss) {	
			pwmWrite(PIN_BASE + ss, calcTicks(servoarray[ss][jj], HERTZ));			
		}
		delay(500);    		 
	}
	pca9685PWMReset(fd);

	return NULL;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{	
	// MULTITHREADING
	int err;
	int N_threads = 2;
	pthread_t thread[N_threads];
	func_ptr tasks[N_threads] = {task_PANTILTDEMO,task_AUDIOIN}; // task_PANTILT
	
	// OUTLINE : Wait for input to start
	printf("This program will demonstrate the simultaneously use of the PWM driver and the RtAudio library\n");
	WaitEnter();
	
	// Create Threads
	for(int tt = 0; tt < N_threads; ++tt) {
		if ((err = pthread_create(&thread[tt], NULL, tasks[tt], (void *)&thread[tt])) != 0) // Error occurs
			printf("Thread creation failed: &s \n", strerror(err));	
	}
	
	// Wait for threads to complete tasks
	for(int tt = 0; tt < N_threads; ++tt) {
		pthread_join(thread[tt],NULL);
	}
	
	// Done, Exit Program
	printf("Program Complete, Exiting\n");
	exit(EXIT_SUCCESS);
	return 0;
}
