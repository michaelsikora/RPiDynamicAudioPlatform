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
  unsigned int frames = nBufferFrames;

  // LAST BUFFER BEFORE OVERFLOW  
  if ( (localData->iframeCounter + nBufferFrames) > localData->itotalFrames ) { // The next buffer will overflow
    frames = localData->itotalFrames - localData->iframeCounter; // Get number of frames to avoid overflow
    localData->bufferBytes = frames * localData->ichannels * sizeof( MY_TYPE ); // Set non-overflow buffer size
  }
  
  // copy buffer from wavtable to output
  unsigned long offset = localData->iframeCounter * localData->ichannels;
  std::memcpy( localData->ibuffer+offset, inputBuffer, localData->bufferBytes );
  localData->iframeCounter += frames;

  // EXIT IF OVERFLOW
  if ( localData->iframeCounter >= localData->itotalFrames ) return 2;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////
int inoutdirect( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus status, void *userData ) {
	
  Data* localData = (Data *) userData;
  
  // STATUS ERROR
  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  // copy buffer from wavtable to output
  // unsigned int *bytes = (unsigned int *) localData->bufferBytes;
  std::memcpy( outputBuffer, inputBuffer, nBufferFrames );
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////
int inout( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *userData ) {
	
  Data* localData = (Data *) userData; // get user data
  // float *obuf = (float*) outputBuffer; // get output buffer for iterating
  // float *ibuf = (float*) inputBuffer; // get input buffer for iterating
  unsigned int frames = nBufferFrames;
  
  // STATUS ERROR
  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  // LAST BUFFER BEFORE OVERFLOW
  if ( localData->oframeCounter + nBufferFrames > localData->ototalFrames) { // The next buffer will overflow
    frames = localData->ototalFrames - localData->oframeCounter; // Get number of frames to avoid overflow
    localData->bufferBytes = frames * localData->ochannels * sizeof( MY_TYPE ); // Set non-overflow buffer size
  }
  
  //~ std::cout << " streamTime: " << streamTime << std::endl;
  
  // copy buffer from wavtable to output and input to input buffer
  unsigned long offset = localData->oframeCounter * localData->ochannels;
  std::memcpy( outputBuffer, localData->wavfile+offset, localData->bufferBytes );
  std::memcpy( localData->ibuffer+offset, inputBuffer, localData->bufferBytes );
  localData->oframeCounter += frames;

  // EXIT IF OVERFLOW
  if ( localData->oframeCounter >= localData->ototalFrames ) return 2; // Overflow
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////
int outFromWav( void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus status, void *userData ) {
			   
  Data* localData = (Data *) userData; // get user data
  unsigned int frames = nBufferFrames;
  
  // STATUS ERROR
  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  // LAST BUFFER BEFORE OVERFLOW
  if ( localData->oframeCounter + nBufferFrames > localData->ototalFrames) { // The next buffer will overflow
    frames = localData->ototalFrames - localData->oframeCounter; // Get number of frames to avoid overflow
    localData->bufferBytes = frames * localData->ochannels * sizeof( MY_TYPE ); // Set non-overflow buffer size
  }
  
  // copy buffer from wavtable to output
  unsigned long offset = localData->oframeCounter * localData->ochannels;
  std::memcpy( outputBuffer, localData->wavfile+offset, localData->bufferBytes );
  localData->oframeCounter += frames;

  // EXIT IF OVERFLOW
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
	userData.ichannels = 1; // Integer
	userData.ochannels = 1; // Integer
	userData.fs = 16000; // Hertz
	userData.bufferFrames = 1024; // number of frames in buffer
	userData.device = 0; 
	userData.offset = 0; 
	userData.itotalTime = 3.0;
	userData.ototalTime = 3.0;
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

	// Set our stream parameters
	RtAudio::StreamParameters iParams;
	iParams.deviceId = userData.device;
	iParams.nChannels = userData.ichannels;
	iParams.firstChannel = userData.offset;
	if(userData.device == 0) iParams.deviceId = adc.getDefaultInputDevice();
	RtAudio::StreamParameters oParams;
	oParams.deviceId = userData.device;
	oParams.nChannels = userData.ichannels;
	oParams.firstChannel = userData.offset;
	if(userData.device == 0) oParams.deviceId = adc.getDefaultInputDevice();
	////////////////
	
	RtAudio::StreamOptions options; // For setting RtAudio built in stream options
	userData.ibuffer = 0; userData.iframeCounter = 0;
	userData.bufferBytes = userData.bufferFrames * userData.ichannels * sizeof( MY_TYPE );
	userData.itotalFrames = (unsigned long) (userData.fs * userData.itotalTime);
	userData.itotalBytes = userData.itotalFrames * userData.ichannels * sizeof( MY_TYPE );
	userData.ototalFrames = (unsigned long) (userData.fs * userData.ototalTime);
	
	////// Read in Wav
	//~ const char* fname = "../input/filteredSine220.wav";
	const char* filename = "../input/filteredWN.wav";
	
	userData.wavfile = 0;
	//~ userData.wavfile = (MY_TYPE*) calloc( userData.ototalFrames, sizeof(MY_TYPE) );
	
	SndfileHandle file;
	file = SndfileHandle(filename);

	printf("Opened file '%s' \n", filename);
	printf(" Sample rate : %d \n", file.samplerate());
	printf(" Channels    : %d \n", file.channels());
	printf(" Frames    : %d \n", (int)file.frames());
	printf(" Format    : %d \n", file.format());
	printf(" Writing %d Frames to wavfile \n", userData.ototalFrames);
	userData.ochannels = file.channels();
	
	// Allocate the entire data buffer before starting stream.
	userData.wavfile = (float*) malloc( userData.ototalFrames * sizeof(float));
	// Assumes wav file is in proper format with settings pre-defined to match program
	file.read(userData.wavfile, userData.ototalFrames);
	printf("File loaded\n");
	//////////////	
	
	// Allocate the entire data buffer before starting stream.
	userData.ibuffer = (MY_TYPE*) malloc( userData.itotalBytes  );
	if ( userData.ibuffer == 0 ) {
		std::cout << "Memory allocation error ... quitting!\n";
		leave(adc, userData);
		return NULL;
	}
	
	if(adc.isStreamOpen()) adc.closeStream(); // if an audio stream is already open close it
		
	try {
		adc.openStream( &oParams, &iParams, FORMAT, userData.fs, &userData.bufferFrames, &inout, (void *)&userData );
	} catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
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
	
	// Now write the entire data to the file.
	fp = fopen( filenames[0].c_str(), "wb" );
	fwrite( userData.ibuffer, sizeof( MY_TYPE ), userData.itotalFrames * userData.ichannels, fp );
	fclose( fp );
	printf("Recording Complete\n");
	
	// Stop the stream.
	leave(adc, userData);
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
	
	// Now write the entire data to the file.
	fp = fopen( filenames[0].c_str(), "wb" );
	fwrite( userData.ibuffer, sizeof( MY_TYPE ), userData.itotalFrames * userData.ichannels, fp );
	fclose( fp );
	printf("Recording Complete\n");
	
	// Stop the stream.
	leave(adc, userData);
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
	userData.bufferFrames = 1024; // number of frames in buffer
	userData.device = 0; 
	userData.offset = 0; 
	userData.ototalTime = 3.0;
	userData.ototalFrames = (unsigned long) (userData.fs * userData.ototalTime);
	
	////// Read in Wav
	//~ const char* fname = "../input/filteredSine220.wav";
	const char* fname = "../input/filteredWN.wav";
	
	userData.wavfile = 0;
	//~ userData.wavfile = (MY_TYPE*) calloc( userData.ototalFrames, sizeof(MY_TYPE) );
	
	SndfileHandle file;
	file = SndfileHandle(fname);
	
	printf("Opened file '%s' \n", fname);
	printf(" Sample rate : %d \n", file.samplerate());
	printf(" Channels    : %d \n", file.channels());
	printf(" Frames    : %d \n", (int)file.frames());
	printf(" Format    : %d \n", file.format());
	printf(" Writing %d Frames to wavfile \n", userData.ototalFrames);
	userData.ochannels = file.channels();
	
	// Allocate the entire data buffer before starting stream.
	userData.wavfile = (float*) malloc( userData.ototalFrames * sizeof(float));
	// Assumes wav file is in proper format with settings pre-defined to match program
	file.read(userData.wavfile, userData.ototalFrames);
	printf("File loaded\n");
	//////////////

	if ( dac.getDeviceCount() < 1 ) {
		std::cout << "\nNo audio devices found!\n";
		exit( 1 );
	}
	
	dac.showWarnings( true ); // Let RtAudio print messages to stderr.
		
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
		
	if(dac.isStreamOpen()) dac.closeStream(); // if an audio stream is already open close it	
	try {
		dac.openStream( &oParams, &iParams, FORMAT, userData.fs, &userData.bufferFrames, &outFromWav, (void *)&userData );
	} catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
		leave(dac, userData);
		return NULL;
	}
	
	// RUN STREAM
	try {
		printf("Starting Stream... \n");
		dac.startStream();
	} catch ( RtAudioError& e ) {
		std::cout << "\n !!! " << e.getMessage() << '\n' << std::endl;
		leave(dac, userData);
		return NULL;
	}

	std::cout << "\n Stream latency = " << dac.getStreamLatency() << " frames" << std::endl;
	std::cout << " Running Playback for " << userData.ototalTime << " seconds ..." << std::endl;
	while ( dac.isStreamRunning() ) {
		SLEEP( 100 ); // wake every 100 ms to check if we're done
		//~ std::cout << "..." << std::endl;
	}
		
	// Stop the stream.
	leave(dac, userData);

	printf("Playback Complete\n");
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
    int ORIENTATION_DELAY = 400;
    
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
			float normrand = (float)rand()/(float)(RAND_MAX/1);
			num[ss] = (normrand*(upper[ss]-lower[ss]))+lower[ss];
			printf(" : %1.4f : ",normrand);
			pwmWrite(PIN_BASE + ss, calcTicks(num[ss], HERTZ)); 
		}
		delay(ORIENTATION_DELAY);
		toc = current_timestamp(tic);
	}
	pca9685PWMReset(fd);
	delay(2000);
	
// Iterative Orientations
	for(int jj = 0; jj < N; ++jj) { 
		tic = current_timestamp();
	    for(int ss = 0; ss < N_servo; ++ss) {	
			printf(" : %1.4f : ",(servoarray[ss][jj]-lower[ss])/(upper[ss]-lower[ss]));
			pwmWrite(PIN_BASE + ss, calcTicks(servoarray[ss][jj], HERTZ));			
		}
		delay(ORIENTATION_DELAY);   
		toc = current_timestamp(tic); 		 
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
	func_ptr tasks[N_threads] = {task_PANTILTDEMO,task_AUDIOINOUT}; // task_PANTILT
	//~ func_ptr tasks[N_threads] = {task_AUDIOOUT}; // task_PANTILT

	
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
