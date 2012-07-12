#ifndef __DINGOO_SOUND_H__
#define __DINGOO_SOUND_H__

void dingoo_sound_volume(int left, int right);
void dingoo_sound_thread_mute(void);
void dingoo_sound_thread_start(void);
void dingoo_sound_thread_stop(void);
void dingoo_sound_set_rate(int rate);
void dingoo_sound_set_stereo(int stereo);

#endif
