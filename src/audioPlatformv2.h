
typedef float MY_TYPE;
#define FORMAT RTAUDIO_FLOAT32

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

// Raw multichannel audio input data struct
struct InputData {
	MY_TYPE* buffer;
	unsigned long bufferBytes;
	unsigned long totalFrames;
	unsigned long frameCounter;
	unsigned int channels;
};

// Audio I/O settings for user interfacing
struct RecSettings {
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
           double /*streamTime*/, RtAudioStreamStatus /*status*/, void *data );
		   
// Formerly used as a goto with label cleanup to be called when exiting		   
void leave(RtAudio &adc, InputData &data);

// resets the audio stream
void resetStream(RtAudio &adc, InputData &data, RtAudio::StreamParameters &iParams, RecSettings &in3);
		   
// Function to pause program and wait for user input
inline void WaitEnter() { std::cout << "Press Enter to continue..."; while (std::cin.get()!='\n'); }


