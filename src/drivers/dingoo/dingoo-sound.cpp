/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/// \file
/// \brief Handles sound emulation using the SDL.

#include <stdio.h>
#include <string.h> 
#include <stdlib.h>

#include "dingoo.h"

#include "../common/configSys.h"
extern Config *g_config;

static int16 *s_Buffer = 0;
static unsigned int s_BufferSize;
static unsigned int s_BufferRead;
static unsigned int s_BufferWrite;
static volatile unsigned int s_BufferIn;

static int s_mute = 0;

/**
 * Callback from Slaanesh's minimal library to get and play audio data.
 */
static void fillaudio(void *stream, int len) {
	int16 *tmps = (int16*) stream;
	len >>= 1;
	// debug code
	//printf("s_BufferIn: %i s_BufferWrite = %i s_BufferRead = %i s_BufferSize = %i\n",
	// s_BufferIn, s_BufferWrite, s_BufferRead, s_BufferSize);

	// ensure that we're not writing garbage data to the soundcard
	if (s_BufferWrite > s_BufferRead)
		s_BufferWrite = s_BufferRead;

	while (len) {
		int16 sample = 0;
		if (s_BufferIn) {
			sample = s_Buffer[s_BufferRead];
			s_BufferRead = (s_BufferRead + 1) % s_BufferSize;
			s_BufferIn--;
		} else {
			sample = 0;
		}

		*tmps = sample;
		tmps++;
		len--;
	}
}

/**
 * Initialize the audio subsystem.
 */
int InitSound() {
	int sound, soundrate, soundbufsize, soundvolume, soundtrianglevolume,
			soundsquare1volume, soundsquare2volume, soundnoisevolume,
			soundpcmvolume, soundq, soundp, lowpass;

	FCEUI_printf("Initializing audio...\n");

	g_config->getOption("SDL.Sound", &sound);
	if (!sound) {
		return 0;
	}

	// load configuration variables
	g_config->getOption("SDL.Sound.Rate", &soundrate);
	g_config->getOption("SDL.Sound.BufSize", &soundbufsize);
	g_config->getOption("SDL.Sound.Volume", &soundvolume);
	g_config->getOption("SDL.Sound.Quality", &soundq);
	g_config->getOption("SDL.Sound.TriangleVolume", &soundtrianglevolume);
	g_config->getOption("SDL.Sound.Square1Volume", &soundsquare1volume);
	g_config->getOption("SDL.Sound.Square2Volume", &soundsquare2volume);
	g_config->getOption("SDL.Sound.NoiseVolume", &soundnoisevolume);
	g_config->getOption("SDL.Sound.PCMVolume", &soundpcmvolume);
	g_config->getOption("SDL.Sound.Priority", &soundp);
	g_config->getOption("SDL.Sound.LowPass", &lowpass);

	// s_BufferSize = soundbufsize * soundrate / 1000;
	s_BufferSize = 1024;

	s_Buffer = (int16 *) malloc(sizeof(int16) * s_BufferSize);
	s_BufferRead = s_BufferWrite = s_BufferIn = 0;

	//printf("SDL Size: %d, Internal size: %d\n",spec.samples,s_BufferSize);

	dingoo_sound_thread_priority(soundp);
	// dingoo_sound_set_callback(fillaudio);
	dingoo_sound_thread_start();

	dingoo_sound_set_stereo(0);
	dingoo_sound_set_rate(soundrate);

	FCEUI_SetSoundVolume(soundvolume);
	FCEUI_SetSoundQuality(soundq);
	FCEUI_Sound(soundrate);
	FCEUI_SetTriangleVolume(soundtrianglevolume);
	FCEUI_SetSquare1Volume(soundsquare1volume);
	FCEUI_SetSquare2Volume(soundsquare2volume);
	FCEUI_SetNoiseVolume(soundnoisevolume);
	FCEUI_SetPCMVolume(soundpcmvolume);
	FCEUI_SetLowPass(lowpass);
	return (1);
}

/**
 * Returns the size of the audio buffer.
 */
uint32 GetMaxSound(void) {
	return (s_BufferSize);
}

/**
 * Returns the amount of free space in the audio buffer.
 */
uint32 GetWriteSound(void) {
	return (s_BufferSize - s_BufferIn);
}

/**
 * Send a sound clip to the audio subsystem.
 */
void WriteSound(int32 *buf, int Count) {
	int len = Count;
	register int32 *src = buf;
	register int16 *dest = s_Buffer;
	while (Count--) {
		*dest++ = *src++;
	}

	dingoo_sound_play(s_Buffer, len << 1);

#if 0
	extern int EmulationPaused;
	if (EmulationPaused == 0)
	while(Count) {
		while (s_BufferIn == s_BufferSize) {
			dingoo_timer_delay(1);
		}

		s_Buffer[s_BufferWrite] = *buf;
		Count--;
		s_BufferWrite = (s_BufferWrite + 1) % s_BufferSize;
		s_BufferIn++;
		buf++;
	}
#endif
}

/**
 * Pause (1) or unpause (0) the audio output.
 */
void SilenceSound(int n) {
	dingoo_sound_thread_mute();
}

/**
 * Shut down the audio subsystem.
 */
int KillSound(void) {
	FCEUI_Sound(0);
	dingoo_sound_thread_stop();
	if (s_Buffer) {
		free((void *) s_Buffer);
		s_Buffer = 0;
	}
	return (0);
}

/**
 * Adjust the volume either down (-1), up (1), or to the default (0).
 * Unmutes if mute was active before.
 */
void FCEUD_SoundVolumeAdjust(int n) {
	int soundvolume;
	g_config->getOption("SDL.SoundVolume", &soundvolume);

	switch (n) {
	case -1:
		soundvolume -= 10;
		if (soundvolume < 0) {
			soundvolume = 0;
		}
		break;
	case 0:
		soundvolume = 100;
		break;
	case 1:
		soundvolume += 10;
		if (soundvolume > 150) {
			soundvolume = 150;
		}
		break;
	}

	s_mute = 0;
	FCEUI_SetSoundVolume(soundvolume);
	g_config->setOption("SDL.SoundVolume", soundvolume);

	FCEU_DispMessage("Sound volume %d.", soundvolume);
}

/**
 * Toggles the sound on or off.
 */
void FCEUD_SoundToggle(void) {
	if (s_mute) {
		int soundvolume;
		g_config->getOption("SDL.SoundVolume", &soundvolume);

		s_mute = 0;
		FCEUI_SetSoundVolume(soundvolume);
		FCEU_DispMessage("Sound mute off.");
	} else {
		s_mute = 1;
		FCEUI_SetSoundVolume(0);
		FCEU_DispMessage("Sound mute on.");
	}
}
