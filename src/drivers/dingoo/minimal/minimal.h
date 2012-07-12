/*
  Dingoo minimal library by Slaanesh
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/cachectl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

#ifndef __MINIMAL_H__
#define __MINIMAL_H__

/* System defines */
#define DINGOO_DEFAULT_MHZ	336
#define DINGOO_DEFAULT_BRIGHT	70
#define DINGOO_SCREEN_WIDTH	320
#define DINGOO_SCREEN_HEIGHT	240
#define DINGOO_BLACK		0x0000	/* Black 16-bit pixel */
#define DINGOO_WHITE		0xFFFF	/* White 16-bit pixel */
/* These are for Dingoo controller inputs */
#define MASK_GPIO2 	0x00020000 /*00000000000000100000000000000000*/
#define MASK_GPIO3	0x280EC067 /*00101000000011101100000001100111*/
/* This mask is used to select buttons which when combined with the SELECT
 * key create a "new" key */
#define MASK_SEL_SHIFT  0x0009C007 /*00000000000010011100000000000111*/

/* Handy Macros */
#define dingoo_video_color8(C,R,G,B) (dingoo_palette[C] = ((((R)&0xF8)<<8)|(((G)&0xFC)<<3)|(((B)&0xF8)>>3)))
#define dingoo_video_color15(R,G,B) ((((R)&0xF8)<<8)|(((G)&0xFC)<<3)|(((B)&0xF8)>>3))
#define dingoo_video_getr15(C) (((C)>>8)&0xF8)
#define dingoo_video_getg15(C) (((C)>>3)&0xFC)
#define dingoo_video_getb15(C) (((C)<<3)&0xF8)

enum  {
    DINGOO_UP    =	1<<6,
	DINGOO_DOWN  =	1<<27,
	DINGOO_LEFT  =	1<<5,
	DINGOO_RIGHT =	1<<18,
	DINGOO_B     =	1<<1,
	DINGOO_X     =	1<<19,
	DINGOO_Y     =	1<<2,
    DINGOO_A     =	1<<0,
	DINGOO_L     = 	1<<14,
	DINGOO_R     =	1<<15,
	DINGOO_START =	1<<16,	/* actually pin 17, merged gpio2 and gpio3 */
	DINGOO_SELECT =	1<<17,
	DINGOO_POWER =  1<<29,
				/* Virtual keys
				 * SELECT + 'button'
				 * Same as base values except shifted << 7 */
	DINGOO_SEL_B =	(DINGOO_B)<<7,	
	DINGOO_SEL_X =	(DINGOO_X)<<7,
	DINGOO_SEL_Y =	(DINGOO_Y)<<7,
    DINGOO_SEL_A =	(DINGOO_A)<<7,
	DINGOO_SEL_L = 	(DINGOO_L)<<7,
	DINGOO_SEL_R =	(DINGOO_R)<<7,
	DINGOO_SEL_START =(DINGOO_START)<<7
};
                                            
extern unsigned short		dingoo_palette[256];
extern int			dingoo_sound_rate;
extern int			dingoo_sound_stereo;

extern void dingoo_init(int rate, int bits, int stereo, unsigned int mhz, unsigned int brightness);
extern void dingoo_deinit(void);
extern void dingoo_timer_delay(clock_t milliseconds);
extern void dingoo_sound_play(void *buff, int len);
extern void dingoo_video_flip(void);
extern void dingoo_video_flip_single(void);
extern void dingoo_video_setpalette(void);
extern unsigned long dingoo_joystick_read(unsigned int use_virtual);
extern void dingoo_joystick_press (void);
extern void dingoo_sound_volume(int left, int right);
extern void dingoo_sound_set_callback(void (*callback)(void *, int));
extern void dingoo_sound_thread_priority(int priority);
extern void dingoo_sound_thread_mute(void);
extern void dingoo_sound_thread_start(void);
extern void dingoo_sound_thread_stop(void);
extern void dingoo_sound_set_rate(int rate);
extern void dingoo_sound_set_stereo(int stereo);
extern clock_t dingoo_timer_read(void);
extern void dingoo_timer_profile(void);
extern void dingoo_set_video_mode(int bpp,int width,int height);
extern void dingoo_set_clock(unsigned int mhz);
extern void dingoo_video_wait_vsync(void);
extern void dingoo_clear_video(void);
extern void dingoo_flush_video(void);
extern int dingoo_get_capacity();
extern void dingoo_set_brightness(int value);
extern void dingoo_release_power_slider(); //TODO remove, this shouldn't exist

extern void dingoo_printf(char* fmt, ...);
extern void dingoo_printf_init(void);
extern void dingoo_gamelist_text_out(int x, int y, char *eltexto);
extern void dingoo_gamelist_text_out_fmt(int x, int y, char* fmt, ...);

#endif
