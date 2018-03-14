// RtAudio exsample - Sine Wave (440Hz)

#include <math.h>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <termio.h>
#include <pthread.h>

#include <RtAudio.h>

#include "pca9685.h"
#include <wiringPi.h>
#define NUM_THREADS 2

#if defined(__cplusplus)
extern "C" {
#endif

volatile bool isInterrupted = false; //Interrupt flag

typedef struct {
    unsigned int	nRate;		/* Sampling Rate (sample/sec) */
    unsigned int	nChannel;	/* Channel Number */
    unsigned int	nFrame;		/* Frame Number of Wave Table */
    float		*wftable;	/* Wave Form Table(interleaved) */
    unsigned int	cur;		/* current index of WaveFormTable(in Frame) */
} CallbackData;


#define PIN_BASE 300
#define MAX_PWM 4096
#define HERTZ 60

#define NTHREADS 5

/**
 * Calculate the number of ticks the signal should be high for the required amount of time
 */
int calcTicks(float impulseMs, int hertz)
{
	float cycleMs = 1000.0f / hertz;
	return (int)(MAX_PWM * impulseMs / cycleMs + 0.5f);
}


bool kbhit(void)
{
    struct termios original;
    tcgetattr(STDIN_FILENO, &original);

    struct termios term;
    memcpy(&term, &original, sizeof(term));

    term.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);

    int characters_buffered = 0;
    ioctl(STDIN_FILENO, FIONREAD, &characters_buffered);

    tcsetattr(STDIN_FILENO, TCSANOW, &original);

    bool pressed = (characters_buffered != 0);

    return pressed;
}

static int
rtaudio_sine(
	void			*outbuf,
	void			*inbuf,
	unsigned int		nFrames,
	double			streamtime,
	RtAudioStreamStatus	status,
	void			*userdata)
{
	(void)inbuf;
	float	*buf = (float*)outbuf;
	unsigned int remainFrames;
	CallbackData	*data = (CallbackData*)userdata;

	remainFrames = nFrames;
	while (remainFrames > 0) {
	    unsigned int sz = data->nFrame - data->cur;
	    if (sz > remainFrames)
		sz = remainFrames;
	    memcpy(buf, data->wftable+(data->cur*data->nChannel),
		   sz * data->nChannel * sizeof(float));
	    data->cur = (data->cur + sz) % data->nFrame;
	    buf += sz * data->nChannel;
	    remainFrames -= sz;
	}
	return 0;
}

static int
rtaudio_callback(
	void			*outbuf,
	void			*inbuf,
	unsigned int		nFrames,
	double			streamtime,
	RtAudioStreamStatus	status,
	void			*userdata)
{
	float 	*input = (float*)inbuf;
	float	*output = (float*)outbuf;
	unsigned int remainFrames;
	CallbackData	*data = (CallbackData*)userdata;

	for(int i = 1; i < nFrames; i++) {
		for (int j = 0; j < data->nChannel; j++){
			*output++ = *input++;
		}
	}
	
	return 0;
}


static int
rtaudio_callback_LP(
	void			*outbuf,
	void			*inbuf,
	unsigned int		nFrames,
	double			streamtime,
	RtAudioStreamStatus	status,
	void			*userdata)
{
	float 	*input = (float*)inbuf;
	float	*output = (float*)outbuf;
	unsigned int remainFrames;
	CallbackData	*data = (CallbackData*)userdata;

	unsigned int order = 2;
	// pass first values of each channel
	for (int pre = 0; pre < order*data->nChannel; pre++){
		*(output+pre) = *(input+pre);
	}
	// loop through buffer and perform difference equation 
	for(int i = 4; i < nFrames; i++) {
		for (int j = 0; j < data->nChannel; j++){
			output[i] =  input[i] + 0.25*input[i-2] + 0.1*input[i-4];
		}
	}
	
	return 0;
}

int setupServos(){
	int fd = pca9685Setup(PIN_BASE, 0x40, HERTZ);
	if (fd < 0)
	{
		printf("Error in setup\n");
		return fd;
	}

	pca9685PWMReset(fd);
	return 0;
}

void *audioTask(void* value) {
	RtAudio *audio;
    unsigned int bufsize = 4096;
    CallbackData data;
    
    try {
	audio = new RtAudio(RtAudio::MACOSX_CORE);
    }catch  (RtAudioError e){
	fprintf(stderr, "fail to create RtAudio: %s¥n", e.what());
    }
    if (!audio){
	fprintf(stderr, "fail to allocate RtAudio¥n");
    }
    
    /* probe audio devices */
    unsigned int devId = audio->getDefaultOutputDevice();

    /* Setup stream parameters */
    RtAudio::StreamParameters *outParam = new RtAudio::StreamParameters();
    RtAudio::StreamParameters *inParam = new RtAudio::StreamParameters();

    outParam->deviceId = devId;
    outParam->nChannels = 2;
    inParam->deviceId = devId;
    inParam->nChannels = 2;
    
    data.nRate = 44100;
    data.nFrame = 44100;
    data.nChannel = outParam->nChannels;
    
    audio->openStream(outParam, inParam, RTAUDIO_FLOAT32, 44100,
     		      &bufsize, rtaudio_callback_LP, &data);     		      
 
    audio->startStream();
    
        std::cout << "Press Return to quit" << std::endl;
    while(!isInterrupted) {
		if(kbhit()) {
			char c = getchar();
			if(c == '\n') {
				isInterrupted = true;
			}
		}
	}
	
    sleep(0.1);
	
	std::cout << "cleaning up" << std::endl;
    audio->stopStream();
    audio->closeStream();
    delete audio;
	std::cout << "done cleaning up" << std::endl;
	
}

int
main(int argc, char *argv[])
{  
	pthread_t tid[NUM_THREADS];
	int rc;
	int t;
	
	rc = pthread_create(tid,NULL,audioTask,(void *) t);
	pthread_exit(NULL);

	setupServos();
    int pin = 1;
	float millis = 2.9;
	float minis = 1.78;
	int maxres = 100;
	float del = (millis-minis)/maxres;

	pwmWrite(PIN_BASE + pin, calcTicks(millis, HERTZ));
	//delay(1000);
	
	for (int ii = 0; ii < maxres; ii++) {
		pwmWrite(PIN_BASE + pin, calcTicks(millis-del*ii, HERTZ));
		delay(20);
	}
    
	pwmWrite(PIN_BASE + pin, calcTicks(minis, HERTZ));	
	
    return 0;
}

#if defined(__cplusplus)
}
#endif
