#include "dingoo_sound.h"
#define PTHREAD_PRIORITY 0
//#define PTHREAD_USLEEP 1
#define MAX_SAMPLE_RATE 44100

extern int dingoo_sound_rate;
extern int dingoo_sound_stereo;

#if PTHREAD_PRIORITY
pthread_attr_t pattr;
#endif
static pthread_t dingoo_sound_thread = 0; // Thread for dingoo_sound_thread_play()
static volatile int dingoo_sound_thread_exit = 0; // Flag to end dingoo_sound_thread_play() thread
static volatile int dingoo_sound_buffer = 0; // Current sound buffer
static volatile int dingoo_sound_priority = 10;
static short dingoo_sound_buffers_total[(MAX_SAMPLE_RATE * 16) / 30];// Sound buffer
static void *dingoo_sound_buffers[16] = { // Sound buffers
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 0) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 1) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 2) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 3) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 4) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 5) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 6) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 7) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 8) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 9) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 10) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 11) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 12) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 13) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 14) / 30)),
		(void *) (dingoo_sound_buffers_total + ((MAX_SAMPLE_RATE * 15) / 30))
};
static volatile int sndlen = (MAX_SAMPLE_RATE * 2) / 60;// Current sound buffer length

static void (*audio_callback)(void *buffer, int len) = NULL;

void dingoo_sound_set_callback(void(*callback)(void *, int)) {
	audio_callback = callback;
}

void dingoo_sound_play(void *buff, int len) {
	int nbuff = (dingoo_sound_buffer + 1) & 15; // Sound buffer to write
	memcpy(dingoo_sound_buffers[nbuff], buff, len); // Write the sound buffer
	dingoo_sound_buffer = nbuff; // Update the current sound buffer
	sndlen = len; // Update the sound buffer length
}

// NOTE: Must restart thread to take effect
void dingoo_sound_thread_priority(int priority) {
	if (priority < -20)
		priority = -20;
	if (priority > 19)
		priority = 19;

	dingoo_sound_priority = priority;
}

void dingoo_sound_thread_mute(void) {
	memset(dingoo_sound_buffers_total, 0, (MAX_SAMPLE_RATE * 16 * 2) / 30);
	sndlen = (dingoo_sound_rate * 2) / 60;
}

#ifndef USE_SDL_AUDIO
static void *dingoo_sound_thread_play(void *none) {
	int nbuff = dingoo_sound_buffer; // Number of the sound buffer to play
#if PTHREAD_USLEEP
	unsigned long inifile;
	int t_sleep = 0;
	char t_sleepc[8];
	inifile = open("./sleep.cfg", O_RDONLY);
	if (inifile >= 0)
	{
		unsigned int b = read(inifile, t_sleepc, 7);
		if (b >= 1)
		t_sleepc[b-1] = '\0';
		t_sleep = atoi(t_sleepc);
		close(inifile);
	}
	printf ("Sleep = %d\n", t_sleep);
#endif

	//ioctl(dingoo_dev_dsp, SOUND_PCM_SYNC, 0);
	do {
		if (audio_callback) {
			int wbuff = (dingoo_sound_buffer + 1) & 15; // Sound buffer to write
			audio_callback(dingoo_sound_buffers[wbuff], sndlen);
			dingoo_sound_buffer = wbuff;
		}

		ao_play(dingoo_dev_sound, (char*)dingoo_sound_buffers[nbuff], sndlen);

		// ioctl SOUND_PCM_SYNC seems broken and this thread
		// takes half the CPU time when running.
		//unsigned int sleeptime = (1000000*(sndlen>>dingoo_sound_stereo))/dingoo_sound_rate;
#if PTHREAD_USLEEP
		usleep(t_sleep);
#endif
		// ioctl(dingoo_dev_dsp, SOUND_PCM_SYNC, 0);						// Synchronize Audio - wait until write is finished
		nbuff = (nbuff + (nbuff != dingoo_sound_buffer)) & 15; // Update the sound buffer to play
	} while (!dingoo_sound_thread_exit); // Until the end of the sound thread
	pthread_exit(0);
}

void dingoo_sound_thread_start(void) {
#if PTHREAD_PRIORITY
	unsigned long inifile;
	int t_priority = 0;
	char t_priorityc[8];

	sched_param param;

#endif

	dingoo_sound_thread = 0;
	dingoo_sound_thread_exit = 0;
	dingoo_sound_buffer = 0;
	dingoo_sound_set_stereo(dingoo_sound_stereo);
	dingoo_sound_set_rate(dingoo_sound_rate);
	sndlen = (dingoo_sound_rate * 2) / 60;
	dingoo_sound_thread_mute();

#if PTHREAD_PRIORITY
	/*
	 inifile = open("./threadsound.cfg", O_RDONLY);
	 if (inifile >= 0)
	 {
	 unsigned int b = read(inifile, t_priorityc, 7);
	 if (b >= 1)
	 t_priorityc[b-1] = '\0';
	 t_priority = atoi(t_priorityc);
	 close(inifile);
	 }*/
	t_priority = dingoo_sound_priority;
	printf ("Using pthread sound priority = %d\n", t_priority);

	pthread_attr_init(&pattr);
	pthread_attr_getschedparam (&pattr, &param);

	/* set the priority; others are unchanged */
	param.sched_priority = t_priority;/* -20 high to 19 low. 0 == default */

	/* setting the new scheduling param */
	pthread_attr_setschedparam (&pattr, &param);

	pthread_create( &dingoo_sound_thread, &pattr, dingoo_sound_thread_play, NULL);
#else
	pthread_create(&dingoo_sound_thread, NULL, dingoo_sound_thread_play, NULL);
#endif
}

void dingoo_sound_thread_stop(void) {
	dingoo_sound_thread_exit = 1;
	dingoo_timer_delay(500); /* wait 0.5 second */
	dingoo_sound_thread = 0;
	dingoo_sound_thread_mute();
#if PTHREAD_PRIORITY
	pthread_attr_destroy(&pattr);
#endif
}

void dingoo_sound_set_rate(int rate) {
fprintf(stderr,"set_rate");
	/*dingoo_sound_rate = rate;

	ao_close(dingoo_dev_sound);

	format.rate = rate;

	dingoo_dev_sound = ao_open_live(ao_default_driver_id(), &format, NULL);

	if (dingoo_dev_sound == NULL) {
		fprintf(stderr, "Error opening device.\n");
	}*/
	fprintf(stderr,"/set_rate");
}

void dingoo_sound_set_stereo(int stereo) {
fprintf(stderr,"set_stereo");
	/*dingoo_sound_stereo = stereo;

	ao_close(dingoo_dev_sound);

	format.channels = stereo+1;

	dingoo_dev_sound = ao_open_live(ao_default_driver_id(), &format, NULL);

	if (dingoo_dev_sound == NULL) {
		fprintf(stderr, "Error opening device.\n");
	}*/
	fprintf(stderr,"set_stereo");
}
#endif // USE_SDL_AUDIO
