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

#include "audioPlatformv2.h"

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter = 0; // iterator to test threads
int pin; // selects which servo to send PWM to
	
// Servo pulse widths for major orientations. 
// Program Calibrate was used to find these values
float servo0[3] = {0.65, 1.5, 2.5}; 
float servo1[2] = {1.35, 2.35};
float servo2[3] = {0.55, 1.4, 2.4};
float servo3[2] = {1.75, 2.9};
float servo4[3] = {0.15, 1.55, 2.55}; 
float servo5[2] = {1.1, 2.0};

int calcTicks(float impulseMs, int hertz)
{
	float cycleMs = 1000.0f / hertz;
	return (int)(MAX_PWM * impulseMs / cycleMs + 0.5f);
}

int input( void * /*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus /*status*/, void *iData )
{
  Data *localData = (Data *) iData;

  // Simply copy the data to our allocated buffer.
  unsigned int frames = nBufferFrames;
  if ( localData->frameCounter + nBufferFrames > localData->totalFrames ) {
    frames = localData->totalFrames - localData->frameCounter;
    localData->bufferBytes = frames * localData->channels * sizeof( MY_TYPE );
  }

  unsigned long offset = localData->frameCounter * localData->channels;
  std::memcpy( localData->buffer+offset, inputBuffer, localData->bufferBytes );
  localData->frameCounter += frames;

  if ( localData->frameCounter >= localData->totalFrames ) return 2;
  return 0;
}

int inout( void *outputBuffer, void *inputBuffer, unsigned int /*nBufferFrames*/,
           double /*streamTime*/, RtAudioStreamStatus status, void *iData )
{
	  Data *localData = (Data *) iData;
	  
  // Since the number of input and output channels is equal, we can do
  // a simple buffer copy operation here.
  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  unsigned int *bytes = (unsigned int *) localData->bufferBytes;
  std::memcpy( outputBuffer, inputBuffer, *bytes );
  return 0;
}

void leave(RtAudio &adc, Data &data) {
		if ( adc.isStreamOpen() ) adc.closeStream(); // close the audio stream
		if ( data.buffer ) free( data.buffer ); // free memory
		//~ pthread_cancel(thread1);
		//~ pthread_cancel(thread2);
}


void resetStream(RtAudio &adc, Data &iData, RtAudio::StreamParameters &oParams,
				RtAudio::StreamParameters &iParams, ioSettings &rec) {
					
		iData.buffer = 0;
		RtAudio::StreamOptions options;

		iData.bufferBytes = rec.bufferFrames * rec.channels * sizeof( MY_TYPE );
		iData.totalFrames = (unsigned long) (rec.fs * rec.recordTime);
		iData.frameCounter = 0;
		iData.channels = rec.channels;
		rec.totalBytes = iData.totalFrames * rec.channels * sizeof( MY_TYPE );
		
		printf("%d\n",iData.bufferBytes);
		
		if ( adc.isStreamOpen() ) {
			adc.closeStream(); 
		} // close the audio stream
		
			try {
				adc.openStream( &oParams, &iParams, FORMAT, rec.fs, &rec.bufferFrames, &inout, (void *)&iData.bufferBytes, &options );			
				//~ adc.openStream( NULL, &iParams, FORMAT, rec.fs, &rec.bufferFrames, &input, (void *)&iData );
			}
			catch ( RtAudioError& e ) {
				std::cout << '\n' << e.getMessage() << '\n' << std::endl;
				leave(adc, iData);
				return;
			}

	// Allocate the entire data buffer before starting stream.
	iData.buffer = (MY_TYPE*) malloc( rec.totalBytes );
	if ( iData.buffer == 0 ) {
		std::cout << "Memory allocation error ... quitting!\n";
		leave(adc, iData);
		return;
	}
		
}

void *task_AUDIO(void* arg) {
	int threadNum = *((int*)arg);
	printf("hello world from AUDIO thread %d\n", threadNum);
	
	pthread_mutex_lock( &mutex1 );
	counter += 10;
	printf("Counter value %d\n", counter);
	pthread_mutex_unlock( &mutex1 );
	
	ioSettings rec; // Three inputs and one output	
	Data iData; // data struct for capturing audio
	Data oData; // data struct for capturing audio
	rec.channels = 3; rec.fs = 16000; rec.bufferFrames = 512;
	rec.device = 0; rec.offset = 0; rec.recordTime = 5.0;
	FILE *fp;
	std::vector <std::string> filenames;
	int l = 0; // location index
	int o = 0; // orientation index
	int N_l = 1; // Number of locations
	int N_o = 1; // Number of orientations
	
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
	
	l = 0; o = 0; // reset l and o to 0 for future iteration
	/////////////// 
	
	RtAudio adc;
	if ( adc.getDeviceCount() < 1 ) {
		std::cout << "\nNo audio devices found!\n";
		exit( 1 );
	}
	
	// Let RtAudio print messages to stderr.
	adc.showWarnings( true );

	//~ // Set our stream parameters for input only.
	//~ RtAudio::StreamParameters iParams;
	//~ if ( rec.device == 0 )
		//~ iParams.deviceId = adc.getDefaultInputDevice();
	//~ else
	//~ iParams.deviceId = rec.device;
	//~ iParams.nChannels = rec.channels;
	//~ iParams.firstChannel = rec.offset;
	//~ ////////////////
	
	
	// Set our stream parameters for input and output.
	RtAudio::StreamParameters iParams, oParams;
	iParams.deviceId = rec.device;
	iParams.nChannels = rec.channels;
	iParams.firstChannel = rec.offset;
	oParams.deviceId = rec.device;
	oParams.nChannels = rec.channels;
	oParams.firstChannel = rec.offset;
	////////////////
	
	// OUTLINE : Record Audio for set number of seconds
	resetStream(adc,iData,oParams,iParams,rec); // ReSets the audio stream for recording

  try {
	printf("Starting Stream... \n");
    adc.startStream();

	// Test RtAudio functionality for reporting latency.
	std::cout << "\nStream latency = " << adc.getStreamLatency() << " frames" << std::endl;
    std::cout << "\nRunning ... press <enter> to quit (buffer frames = " << rec.bufferFrames << ").\n";
	WaitEnter();

    // Stop the stream.
    adc.stopStream();
	leave(adc, iData);
  }
	catch ( RtAudioError& e ) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
		leave(adc, iData);
		return NULL;
	}

	//~ std::cout << "\nRecording for " << rec.recordTime << " seconds ..." << std::endl;
	//~ std::cout << "writing file" + filenames[0] + "(buffer frames = " << rec.bufferFrames << ")." << std::endl;
	//~ while ( adc.isStreamRunning() ) {
		//~ SLEEP( 100 ); // wake every 100 ms to check if we're done
	//~ }

	// Now write the entire data to the file.
	fp = fopen( filenames[0].c_str(), "wb" );
	fwrite( iData.buffer, sizeof( MY_TYPE ), iData.totalFrames * rec.channels, fp );
	fclose( fp );
	printf("Recording Complete\n");
	
	return NULL;
}

void *task_WAVREAD(void* arg) {
	int threadNum = *((int*)arg);
	printf("hello world from WAVREAD thread %d\n", threadNum);
	
	const char* fname = "../input/filteredWN.wav";
	read_wav_file( fname );
	
	return NULL;
}

void *task_PANTILT(void* arg) {
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
	int N = 10;
	int N_servo = 2;
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
	    for(int ss = 0; ss < N_servo; ++ss) {	
			num[ss] = ((float)rand()/(float)(RAND_MAX/1)*(upper[ss]-lower[ss]))+lower[ss];
			printf(": %1.3f : ",num[ss]);
			pwmWrite(PIN_BASE + ss, calcTicks(num[ss], HERTZ)); 
		}
		printf("\n");
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
	func_ptr tasks[N_threads] = {task_AUDIO, task_WAVREAD}; // task_PANTILT
	
	// OUTLINE : Wait for input to start
	printf("This program will demonstrate the simultaneously use of the PWM driver and the RtAudio library\n");
	WaitEnter();
	
	// Create Threads
	for(int tt = 0; tt < N_threads; ++tt) {
		if ((err = pthread_create(&thread[tt], NULL, tasks[tt], (void *)&thread[tt])) != 0) // Error occurs
			printf("Thread creation failed: &s \n", strerror(err));	
	}
	for(int tt = 0; tt < N_threads; ++tt) {
		pthread_join(thread[tt],NULL);
	}
	
	exit(EXIT_SUCCESS);
	return 0;
}
