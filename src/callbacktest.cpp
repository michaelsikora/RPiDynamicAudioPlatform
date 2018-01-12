// RtAudio exsample - Sine Wave (440Hz)

#include <math.h>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <termio.h>
#include <RtAudio.h>

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

int
main(void)
{
    RtAudio *audio;
    unsigned int bufsize = 4096;
    CallbackData data;

    try {
	audio = new RtAudio(RtAudio::MACOSX_CORE);
    }catch  (RtAudioError e){
	fprintf(stderr, "fail to create RtAudio: %s¥n", e.what());
	return 1;
    }
    if (!audio){
	fprintf(stderr, "fail to allocate RtAudio¥n");
	return 1;
    }
    /* probe audio devices */
    unsigned int devId = audio->getDefaultOutputDevice();

    /* Setup output stream parameters */
    RtAudio::StreamParameters *outParam = new RtAudio::StreamParameters();
    /* Setup input stream parameters */
    RtAudio::StreamParameters *inParam = new RtAudio::StreamParameters();

    outParam->deviceId = devId;
    outParam->nChannels = 2;
    inParam->deviceId = devId;
    inParam->nChannels = 2;

//    audio->openStream(outParam, inParam, RTAUDIO_FLOAT32, 44100,
//     		      &bufsize, rtaudio_sine, &data);
    audio->openStream(outParam, inParam, RTAUDIO_FLOAT32, 44100,
     		      &bufsize, rtaudio_callback_LP, &data);     		      

    /* Create Wave Form Table */
    data.nRate = 44100;
    /* Frame Number is based on Freq(440Hz) and Sampling Rate(44100) */
    /* hmm... nFrame = 44100 is enough approximation, maybe... */
    data.nFrame = 44100;
    data.nChannel = outParam->nChannels;
    data.cur = 0;
    data.wftable = (float *)calloc(data.nChannel * data.nFrame, sizeof(float));
    if (!data.wftable)
    {
	delete audio;
	fprintf(stderr, "fail to allocate memory¥n");
	return 1;
    }
    for(unsigned int i = 0; i < data.nFrame; i++) {
      float v = sin(i * M_PI * 2 * 261.56 / data.nRate);
      for(unsigned int j = 0; j < data.nChannel; j++) {
	  data.wftable[i*data.nChannel+j] = v;
      }
    }

    audio->startStream();
    //sleep(10);
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
	
    return 0;
}

#if defined(__cplusplus)
}
#endif
