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
/// \brief Handles the graphical game display for the SDL implementation.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL/SDL.h>

#include "dingoo.h"
#include "dingoo-video.h"

#include "../common/vidblit.h"
#include "../../fceu.h"
#include "../../version.h"

#include "dface.h"

#include "../common/configSys.h"

// GLOBALS
SDL_Surface *screen;
SDL_Surface *nes_screen; // 256x224

uint32 vm = 0; // 0 - 320x240, 1 - 400x240, 2 - 480x272
uint32 screen_x, screen_y;

#define NUMOFVIDEOMODES 3
struct {
	uint32 x;
	uint32 y;
} VModes[NUMOFVIDEOMODES] = {
	{320, 240},
	{400, 240},
	{480, 272}
};


extern Config *g_config;

// STATIC GLOBALS
static int s_curbpp;
static int s_srendline, s_erendline;
static int s_tlines;
static int s_inited;

static int s_eefx;
static int s_clipSides;
int s_fullscreen;
static int noframe;

static int FDSTimer = 0;
int FDSSwitchRequested = 0;

#define NWIDTH	(256 - (s_clipSides ? 16 : 0))
#define NOFFSET	(s_clipSides ? 8 : 0)

/* Blur effect taken from vidblit.cpp */
static uint32 palettetranslate[65536 * 4];
static uint32 CBM[3] = { 63488, 2016, 31 };
static uint16 s_psdl[256];

struct Color {
	uint8 r;
	uint8 g;
	uint8 b;
	uint8 unused;
};

static struct Color s_cpsdl[256];

#define BLUR_RED	30
#define BLUR_GREEN	30
#define BLUR_BLUE	20

/**
 * Attempts to destroy the graphical video display.  Returns 0 on
 * success, -1 on failure.
 */

//draw input aids if we are fullscreen
bool FCEUD_ShouldDrawInputAids() {
	return s_fullscreen != 0;
}

int KillVideo() {
	// return failure if the video system was not initialized
	if (s_inited == 0)
		return -1;

	SDL_FreeSurface(screen);
	SDL_FreeSurface(nes_screen);
	s_inited = 0;
	return 0;
}

/**
 * These functions determine an appropriate scale factor for fullscreen/
 */
inline double GetXScale(int xres) {
	return ((double) xres) / NWIDTH;
}
inline double GetYScale(int yres) {
	return ((double) yres) / s_tlines;
}
void FCEUD_VideoChanged() {
	int buf;
	g_config->getOption("SDL.PAL", &buf);
	if (buf)
		PAL = 1;
	else
		PAL = 0;
}
/**
 * Attempts to initialize the graphical video display.  Returns 0 on
 * success, -1 on failure.
 */
int InitVideo(FCEUGI *gi) {
	FCEUI_printf("Initializing video...\n");

	// load the relevant configuration variables
	g_config->getOption("SDL.Fullscreen", &s_fullscreen);
	g_config->getOption("SDL.ClipSides", &s_clipSides);

	// check the starting, ending, and total scan lines
	FCEUI_GetCurrentVidSystem(&s_srendline, &s_erendline);
	s_tlines = s_erendline - s_srendline + 1;

	int brightness;
	g_config->getOption("SDL.Brightness", &brightness);

	s_inited = 1;
	FDSSwitchRequested = 0;

	int desbpp;
	g_config->getOption("SDL.SpecialFilter", &s_eefx);

	SetPaletteBlitToHigh((uint8 *) s_cpsdl);

	//Init video subsystem
	if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO))
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1) {
			fprintf(stderr,"%s",SDL_GetError());
		}

	// initialize dingoo video mode
	//screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	//screen = SDL_SetVideoMode(320, 240, 16, SDL_SWSURFACE);

	for(vm = NUMOFVIDEOMODES-1; vm >= 0; vm--)
	{
		if(SDL_VideoModeOK(VModes[vm].x, VModes[vm].y, 16, SDL_HWSURFACE | SDL_DOUBLEBUF) != 0)
		{
			screen = SDL_SetVideoMode(VModes[vm].x, VModes[vm].y, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			screen_x = VModes[vm].x;
			screen_y = VModes[vm].y;
			break;
		}
	}

	// a hack to bind inner buffer to nes_screen surface
	extern uint8 *XBuf;

	nes_screen = SDL_CreateRGBSurfaceFrom(XBuf, 256, 224, 8, 256, 0, 0, 0, 0);
	if(!nes_screen)
		printf("Error in SDL_CreateRGBSurfaceFrom\n");
	SDL_SetPalette(nes_screen, SDL_LOGPAL, (SDL_Color *)s_cpsdl, 0, 256);

	SDL_ShowCursor(0);

	/* clear screen */
	dingoo_clear_video();

	return 0;
}

/**
 * Toggles the full-screen display.
 */
void ToggleFS() {
}

/* Taken from /src/drivers/common/vidblit.cpp */
static void CalculateShift(uint32 *CBM, int *cshiftr, int *cshiftl)
{
	int a, x, z, y;
	cshiftl[0] = cshiftl[1] = cshiftl[2] = -1;
	for (a = 0; a < 3; a++) {
		for (x = 0, y = -1, z = 0; x < 32; x++) {
			if (CBM[a] & (1 << x)) {
				if (cshiftl[a] == -1)
					cshiftl[a] = x;
				z++;
			}
		}
		cshiftr[a] = (8 - z);
	}
}

void SetPaletteBlitToHigh(uint8 *src)
{
	int cshiftr[3];
	int cshiftl[3];
	int x, y;

	CalculateShift(CBM, cshiftr, cshiftl);

	if (s_eefx)
		for (x = 0; x < 256; x++) {
			uint32 r, g, b;
			for (y = 0; y < 256; y++) {
				r = src[x << 2] * (100 - BLUR_RED);
				g = src[(x << 2) + 1] * (100 - BLUR_GREEN);
				b = src[(x << 2) + 2] * (100 - BLUR_BLUE);

				r += src[y << 2] * BLUR_RED;
				g += src[(y << 2) + 1] * BLUR_GREEN;
				b += src[(y << 2) + 2] * BLUR_BLUE;
				r /= 100;
				g /= 100;
				b /= 100;

				if (r > 255)
					r = 255;
				if (g > 255)
					g = 255;
				if (b > 255)
					b = 255;
				palettetranslate[x | (y << 8)] = ((r >> cshiftr[0])
						<< cshiftl[0]) | ((g >> cshiftr[1]) << cshiftl[1])
						| ((b >> cshiftr[2]) << cshiftl[2]);
			}
		}
	else
		for (x = 0; x < 65536; x++) {
			uint16 lower, upper;

			lower = (src[((x & 255) << 2)] >> cshiftr[0]) << cshiftl[0];
			lower |= (src[((x & 255) << 2) + 1] >> cshiftr[1]) << cshiftl[1];
			lower |= (src[((x & 255) << 2) + 2] >> cshiftr[2]) << cshiftl[2];
			upper = (src[((x >> 8) << 2)] >> cshiftr[0]) << cshiftl[0];
			upper |= (src[((x >> 8) << 2) + 1] >> cshiftr[1]) << cshiftl[1];
			upper |= (src[((x >> 8) << 2) + 2] >> cshiftr[2]) << cshiftl[2];

			palettetranslate[x] = lower | (upper << 16);
		}
}

/**
 * Sets the color for a particular index in the palette.
 */
void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
	s_cpsdl[index].r = r;
	s_cpsdl[index].g = g;
	s_cpsdl[index].b = b;

	//uint32 col = (r << 16) | (g << 8) | b;
	//s_psdl[index] = (uint16)COL32_TO_16(col);
	s_psdl[index] = dingoo_video_color15(r, g, b);

	if (index == 255)
		SetPaletteBlitToHigh((uint8 *) s_cpsdl);
}

/**
 * Gets the color for a particular index in the palette.
 */
void FCEUD_GetPalette(uint8 index, uint8 *r, uint8 *g, uint8 *b)
{
	*r = s_cpsdl[index].r;
	*g = s_cpsdl[index].g;
	*b = s_cpsdl[index].b;
}

uint16 * FCEUD_GetPaletteArray16()
{
	return s_psdl;
}

/** 
 * Pushes the palette structure into the underlying video subsystem.
 */
static void RedoPalette() {
}

// XXX soules - console lock/unlock unimplemented?

///Currently unimplemented.
void LockConsole() {
}

///Currently unimplemented.
void UnlockConsole() {
}

#define READU16(x) 	(uint16) ((uint16)(x)[0] | (uint16)(x)[1] << 8) 

/**
 * Pushes the given buffer of bits to the screen.
 */
void BlitScreen(uint8 *XBuf) {
	int x, x2, y, y2;
	extern uint32 screen_x, screen_y;

	// Taken from fceugc
	// FDS switch disk requested - need to eject, select, and insert
	// but not all at once!
	if (FDSSwitchRequested) {
		switch (FDSSwitchRequested) {
		case 1:
			FDSSwitchRequested++;
			FCEUI_FDSInsert(); // eject disk
			FDSTimer = 0;
			break;
		case 2:
			if (FDSTimer > 60) {
				FDSSwitchRequested++;
				FDSTimer = 0;
				FCEUI_FDSSelect(); // select other side
				FCEUI_FDSInsert(); // insert disk
			}
			break;
		case 3:
			if (FDSTimer > 200) {
				FDSSwitchRequested = 0;
				FDSTimer = 0;
			}
			break;
		}
		FDSTimer++;
	}

	// TODO - Move these to its own file?
	if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);

	register uint8 *pBuf = XBuf;

/*
	upscale 256*224 to 384*272 (easier than to 480*272)
	horizontal interpolation
	384 = 256 * 1.5
	for each line: 4 pixels => 6 pixels (*1.5) (64 blocks)
	a b c d => a (ab) b c (cd) d
	vertical interpolation:
	272 = 48 * 5 + 8 * 4 = 8*(6*5 + 1*4) (56 blocks)
	6 blocks of 5 scanlines (interpolated from 4 scanlines), 1 block of 4 scanlines (no interpolation)
	line 0
	line 1
	interpolated line 1 + 2 (skipped each 7th block, (blocknum % 6) == 0 )
	line 2
	line 3
*/
	if(s_fullscreen == 2 && vm == 2 && !s_eefx) { // if 480x272, fullscreen on, blur off

	#define AVERAGEHI(AB) ((((AB) & 0xF7DE0000) >> 1) + (((AB) & 0xF7DE) << 15))
	#define AVERAGELO(CD) ((((CD) & 0xF7DE) >> 1) + (((CD) & 0xF7DE0000) >> 17))
	#define AVERAGE(z, x) ((((z) & 0xF7DEF7DE) >> 1) + (((x) & 0xF7DEF7DE) >> 1))

	#define RENDERCHUNK(INSCANLINE, OUTSCANLINE) \
	{ \
		register uint32 ab, cd, a_ab, b_c, cd_d; \
		ab = palettetranslate[*(uint16 *)(pBuf + 0 + (INSCANLINE) * 256)]; \
		cd = palettetranslate[*(uint16 *)(pBuf + 2 + (INSCANLINE) * 256)]; \
		a_ab = (ab & 0xFFFF) + AVERAGEHI(ab); \
		b_c = (ab >> 16) + ((cd & 0xFFFF) << 16); \
		cd_d = (cd & 0xFFFF0000) + AVERAGELO(cd); \
		*(dest + 0 + (OUTSCANLINE) * 480/2) = a_ab; \
		*(dest + 1 + (OUTSCANLINE) * 480/2) = b_c; \
		*(dest + 2 + (OUTSCANLINE) * 480/2) = cd_d; \
	}

	#define RENDERICHUNK(OUTSCANLINE) \
	{ \
		*(dest + 0 + (OUTSCANLINE) * 480/2) = AVERAGE(*(dest + 0 + (OUTSCANLINE-1) * 480/2),*(dest + 0 + (OUTSCANLINE+1) * 480/2)); \
		*(dest + 1 + (OUTSCANLINE) * 480/2) = AVERAGE(*(dest + 1 + (OUTSCANLINE-1) * 480/2),*(dest + 1 + (OUTSCANLINE+1) * 480/2)); \
		*(dest + 2 + (OUTSCANLINE) * 480/2) = AVERAGE(*(dest + 2 + (OUTSCANLINE-1) * 480/2),*(dest + 2 + (OUTSCANLINE+1) * 480/2)); \
	}

		register uint32 *dest = (uint32 *) screen->pixels;
		pBuf += 8 * 256; // start from 8th line
		dest += (480 - 384) / 4;

		for(y = 224/4; y; y--) { // 56 blocks of 4-line chunks
			uint32 mf = y % 6; // mf == 0 - render 4 lines, mf != 0 - render 4->5 lines

			for(x = 256; x; x -= 4) {
				__builtin_prefetch(dest + 4, 1);
				RENDERCHUNK(0, 0); // 1st line
				RENDERCHUNK(1, 1); // 2nd line
				RENDERCHUNK(2, (mf == 0 ? 2 : 3)); // 3rd line
				if(mf != 0) RENDERICHUNK(2); // 2nd+3rd interpolated line
				RENDERCHUNK(3, (mf == 0 ? 3 : 4)); // 4th line
				dest += 3;
				pBuf += 4;
			}

			dest += (480 - 384) / 2 + 480/2 * (mf == 0 ? 3 : 4); 
			pBuf += 256 * 3; // cause we already rolled thru 256 pixel
		}

	} else
	if (s_fullscreen == 1) { // Semi fullscreen (280x240)
		pBuf += (s_srendline * 256) + 8;
		register uint16 *dest = (uint16 *) screen->pixels;
		//dest += (320 * s_srendline) + 20;
		dest += (screen_x * s_srendline) + (screen_x - 280) / 2 + ((screen_y - 240) / 2) * screen_x;

		if (s_eefx) { // Blur effect
			for (y = s_tlines; y; y--) {
				for (x = 240; x; x -= 6) {
					__builtin_prefetch(dest + 2, 1);
					*dest++ = palettetranslate[*pBuf | (*(pBuf - 1) << 8)];
					*dest++ = palettetranslate[*(pBuf + 1) | (*(pBuf) << 8)];
					*dest++	= palettetranslate[*(pBuf + 2) | (*(pBuf + 1) << 8)];
					*dest++	= palettetranslate[*(pBuf + 3) | (*(pBuf + 2) << 8)];
					*dest++	= palettetranslate[*(pBuf + 3) | (*(pBuf + 2) << 8)];
					*dest++	= palettetranslate[*(pBuf + 4) | (*(pBuf + 3) << 8)];
					*dest++	= palettetranslate[*(pBuf + 5) | (*(pBuf + 4) << 8)];
					pBuf += 6;
				}
				pBuf += 16;
				//dest += 40;
				dest += screen_x - 280;
			}
		} else { // semi fullscreen no blur
			for (y = s_tlines; y; y--) {
				for (x = 240; x; x -= 6) {
					__builtin_prefetch(dest + 2, 1);
					*dest++ = s_psdl[*pBuf];
					*dest++ = s_psdl[*(pBuf + 1)];
					*dest++ = s_psdl[*(pBuf + 2)];
					*dest++ = s_psdl[*(pBuf + 3)];
					*dest++ = s_psdl[*(pBuf + 3)];
					*dest++ = s_psdl[*(pBuf + 4)];
					*dest++ = s_psdl[*(pBuf + 5)];
					pBuf += 6;
				}
				pBuf += 16;
				//dest += 40;
				dest += screen_x - 280;
			}
		}
	} else if (s_fullscreen == 2) {
		pBuf += (s_srendline * 256) + 8;

		if (s_eefx) { // Blur effect
			register uint16 *dest = (uint16 *) screen->pixels;
			//dest += (320 * s_srendline);
			dest += (screen_x * s_srendline) + (screen_x - 320) / 2 + ((screen_y - 240) / 2) * screen_x;

			for (y = s_tlines; y; y--) {
				for (x = 240; x; x -= 6) {
					__builtin_prefetch(dest + 2, 1);
					*dest++ = palettetranslate[*pBuf | (*(pBuf - 1) << 8)];
					*dest++ = palettetranslate[*pBuf | (*(pBuf - 1) << 8)];
					*dest++ = palettetranslate[*(pBuf + 1) | (*(pBuf) << 8)];
					*dest++	= palettetranslate[*(pBuf + 2) | (*(pBuf + 1) << 8)];
					*dest++	= palettetranslate[*(pBuf + 3) | (*(pBuf + 2) << 8)];
					*dest++	= palettetranslate[*(pBuf + 3) | (*(pBuf + 2) << 8)];
					*dest++	= palettetranslate[*(pBuf + 4) | (*(pBuf + 3) << 8)];
					*dest++	= palettetranslate[*(pBuf + 5) | (*(pBuf + 4) << 8)];
					pBuf += 6;
				}
				pBuf += 16;
				//
				dest += screen_x - 320;
			}
		} else { // Fullscreen (320x240) no blur
			register uint32 *dest = (uint32 *) screen->pixels;
			//dest += (160 * s_srendline);
			dest += (screen_x / 2 * s_srendline) + (screen_x - 320) / 4 + ((screen_y - 240) / 4) * screen_x; //(160 * s_srendline);
			for (y = s_tlines; y; y--) {
				for (x = 240; x; x -= 6) {
					__builtin_prefetch(dest + 4, 1);
					*dest++ = palettetranslate[*(uint16 *) pBuf];
					*dest++ = palettetranslate[READU16(pBuf+1)];
					*dest++ = palettetranslate[READU16(pBuf+3)];
					*dest++ = palettetranslate[*(uint16 *) (pBuf + 4)];
					pBuf += 6;
				}
				pBuf += 16;
				//
				dest += (screen_x - 320) / 2;
			}
		}
	} else { // No fullscreen
		//int pinc = (320 - NWIDTH) >> 1;
		int32 pinc = (screen_x - NWIDTH) >> 1;

		if (s_eefx) { // DaBlur!!
			register uint16 *dest = (uint16 *) screen->pixels;
			static uint8 last = 0x00;

			// XXX soules - not entirely sure why this is being done yet
			pBuf += (s_srendline * 256) + NOFFSET;
			//dest += (s_srendline * 320) + pinc;
			dest += (screen_x * s_srendline) + pinc + ((screen_y - 240) / 2) * screen_x;

			for (y = s_tlines; y; y--, pBuf += 256 - NWIDTH) {
				for (x = NWIDTH >> 2; x; x--) {
					__builtin_prefetch(dest + 2, 1);
					*dest++ = palettetranslate[*pBuf | (last << 8)];
					*dest++ = palettetranslate[*(pBuf + 1) | (*pBuf << 8)];
					*dest++	= palettetranslate[*(pBuf + 2) | (*(pBuf + 1) << 8)];
					*dest++	= palettetranslate[*(pBuf + 3) | (*(pBuf + 2) << 8)];
					last = *(pBuf + 3);
					pBuf += 4;
				}
				dest += pinc << 1;
			}
		} else {
			//SDL_Rect dstrect;
			
			// center windows
			//dstrect.x = (VModes[vm].x - 256) / 2;
			//dstrect.y = (VModes[vm].y - 224) / 2;

			// doesn't work in rzx-50 dingux
			//SDL_BlitSurface(nes_screen, 0, screen, &dstrect);

			register uint32 *dest = (uint32 *) screen->pixels;

			// XXX soules - not entirely sure why this is being done yet
			pBuf += (s_srendline * 256) + NOFFSET;
			//dest += (s_srendline * 320) + pinc >> 1;
			dest += (screen_x/2 * s_srendline) + pinc / 2 + ((screen_y - 240) / 4) * screen_x;

			for (y = s_tlines; y; y--, pBuf += 256 - NWIDTH) {
				for (x = NWIDTH >> 3; x; x--) {
					__builtin_prefetch(dest + 4, 1);
					*dest++ = palettetranslate[*(uint16 *) pBuf];
					*dest++ = palettetranslate[*(uint16 *) (pBuf + 2)];
					*dest++ = palettetranslate[*(uint16 *) (pBuf + 4)];
					*dest++ = palettetranslate[*(uint16 *) (pBuf + 6)];
					pBuf += 8;
				}
				dest += pinc;
			}
		}
	}

	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}

/**
 *  Converts an x-y coordinate in the window manager into an x-y
 *  coordinate on FCEU's screen.
 */
uint32 PtoV(uint16 x, uint16 y) {
	y = (uint16) ((double) y);
	x = (uint16) ((double) x);
	if (s_clipSides) {
		x += 8;
	}
	y += s_srendline;
	return (x | (y << 16));
}

bool disableMovieMessages = false;
bool FCEUI_AviDisableMovieMessages() {
	if (disableMovieMessages)
		return true;

	return false;
}

void FCEUI_SetAviDisableMovieMessages(bool disable) {
	disableMovieMessages = disable;
}

//clear both screens (for double buffer)
void dingoo_clear_video(void) {
	SDL_FillRect(screen,NULL,SDL_MapRGBA(screen->format, 0, 0, 0, 255));
	SDL_Flip(screen);
	SDL_FillRect(screen,NULL,SDL_MapRGBA(screen->format, 0, 0, 0, 255));
	SDL_Flip(screen);
}
