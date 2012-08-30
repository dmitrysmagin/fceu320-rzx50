
#include "scaler.h"

#define AVERAGE(z, x) ((((z) & 0xF7DEF7DE) >> 1) + (((x) & 0xF7DEF7DE) >> 1))
#define AVERAGEHI(AB) ((((AB) & 0xF7DE0000) >> 1) + (((AB) & 0xF7DE) << 15))
#define AVERAGELO(CD) ((((CD) & 0xF7DE) >> 1) + (((CD) & 0xF7DE0000) >> 17))

extern uint32 palettetranslate[65536 * 4];

/*
	Upscale 256x224 -> 320x240

	Horizontal upscale:
		320/256=1.25  --  do some horizontal interpolation
		8p -> 10p
		4dw -> 5dw

		coarse interpolation:
		[ab][cd][ef][gh] -> [a(ab)][(bc)(cd)][de][(ef)(fg)][(gh)h]

		fine interpolation
		[ab][cd][ef][gh] -> [a(0.25a+0.75b)][(0.5b+0.5c)(0.75c+0.25d)][de][(0.25e+0.75f)(0.5f+0.5g)][(0.75g+0.25h)h]


	Vertical upscale:
		Bresenham algo with simple interpolation

	Parameters:
	uint32 *dst - pointer to 320x240x16bpp buffer
	uint8 *src - pointer to 256x224x8bpp buffer
	palette is taken from palettetranslate[]
	no pitch corrections are made!
*/

void upscale_320x240(uint32 *dst, uint8 *src)
{
	int midh = 240 * 3 / 4;
	int Eh = 0;
	int source = 0;
	int dh = 0;
	int y, x;

	for (y = 0; y < 240; y++)
	{
		source = dh * 256;

		for (x = 0; x < 320/10; x++)
		{
			register uint32 ab, cd, ef, gh;

			__builtin_prefetch(dst + 4, 1);
			__builtin_prefetch(src + source + 4, 0);

			ab = palettetranslate[*(uint16 *)(src + source)] & 0xF7DEF7DE;
			cd = palettetranslate[*(uint16 *)(src + source + 2)] & 0xF7DEF7DE;
			ef = palettetranslate[*(uint16 *)(src + source + 4)] & 0xF7DEF7DE;
			gh = palettetranslate[*(uint16 *)(src + source + 6)] & 0xF7DEF7DE;

			if(Eh >= midh) { // average + 256
				ab = AVERAGE(ab, palettetranslate[*(uint16 *)(src + source + 256)]) & 0xF7DEF7DE; // to prevent overflow
				cd = AVERAGE(cd, palettetranslate[*(uint16 *)(src + source + 256 + 2)]) & 0xF7DEF7DE; // to prevent overflow
				ef = AVERAGE(ef, palettetranslate[*(uint16 *)(src + source + 256 + 4)]) & 0xF7DEF7DE; // to prevent overflow
				gh = AVERAGE(gh, palettetranslate[*(uint16 *)(src + source + 256 + 6)]) & 0xF7DEF7DE; // to prevent overflow
			}

			*dst++ = (ab & 0xFFFF) + AVERAGEHI(ab);
			*dst++  = ((ab >> 17) + ((cd & 0xFFFF) >> 1)) + AVERAGEHI(cd);
			*dst++  = (cd >> 16) + (ef << 16);
			*dst++  = AVERAGELO(ef) + (((ef & 0xFFFF0000) >> 1) + ((gh & 0xFFFF) << 15));
			*dst++  = AVERAGELO(gh) + (gh & 0xFFFF0000);

			source += 8;

		}
		Eh += 224; if(Eh >= 240) { Eh -= 240; dh++; }
	}
}

/*
	Upscale 256x224 -> 384x240 (for 400x240)

	Horizontal interpolation
		384/256=1.5
		4p -> 6p
		2dw -> 3dw

		for each line: 4 pixels => 6 pixels (*1.5) (64 blocks)
		[ab][cd] => [a(ab)][bc][(cd)d]

	Vertical upscale:
		Bresenham algo with simple interpolation

	Parameters:
	uint32 *dst - pointer to 400x240x16bpp buffer
	uint8 *src - pointer to 256x224x8bpp buffer
	palette is taken from palettetranslate[]
	pitch correction
*/

void upscale_384x240(uint32 *dst, uint8 *src)
{
	int midh = 240 * 3 / 4;
	int Eh = 0;
	int source = 0;
	int dh = 0;
	int y, x;

	dst += (400 - 384) / 4;

	for (y = 0; y < 240; y++)
	{
		source = dh * 256;

		for (x = 0; x < 384/6; x++)
		{
			register uint32 ab, cd;

			__builtin_prefetch(dst + 4, 1);
			__builtin_prefetch(src + source + 4, 0);

			ab = palettetranslate[*(uint16 *)(src + source)] & 0xF7DEF7DE;
			cd = palettetranslate[*(uint16 *)(src + source + 2)] & 0xF7DEF7DE;

			if(Eh >= midh) { // average + 256
				ab = AVERAGE(ab, palettetranslate[*(uint16 *)(src + source + 256)]) & 0xF7DEF7DE; // to prevent overflow
				cd = AVERAGE(cd, palettetranslate[*(uint16 *)(src + source + 256 + 2)]) & 0xF7DEF7DE; // to prevent overflow
			}

			*dst++ = (ab & 0xFFFF) + AVERAGEHI(ab);
			*dst++ = (ab >> 16) + ((cd & 0xFFFF) << 16);
			*dst++ = (cd & 0xFFFF0000) + AVERAGELO(cd);

			source += 4;

		}
		dst += (400 - 384) / 2; 
		Eh += 224; if(Eh >= 240) { Eh -= 240; dh++; }
	}
}

/*
	Upscale 256x224 -> 384x272 (for 480x240)

	Horizontal interpolation
		384/256=1.5
		4p -> 6p
		2dw -> 3dw

		for each line: 4 pixels => 6 pixels (*1.5) (64 blocks)
		[ab][cd] => [a(ab)][bc][(cd)d]

	Vertical upscale:
		Bresenham algo with simple interpolation

	Parameters:
	uint32 *dst - pointer to 480x272x16bpp buffer
	uint8 *src - pointer to 256x224x8bpp buffer
	palette is taken from palettetranslate[]
	pitch correction
*/

void upscale_384x272(uint32 *dst, uint8 *src)
{
	int midh = 272 * 3 / 4;
	int Eh = 0;
	int source = 0;
	int dh = 0;
	int y, x;

	dst += (480 - 384) / 4;

	for (y = 0; y < 272; y++)
	{
		source = dh * 256;

		for (x = 0; x < 384/6; x++)
		{
			register uint32 ab, cd;

			__builtin_prefetch(dst + 4, 1);
			__builtin_prefetch(src + source + 4, 0);

			ab = palettetranslate[*(uint16 *)(src + source)] & 0xF7DEF7DE;
			cd = palettetranslate[*(uint16 *)(src + source + 2)] & 0xF7DEF7DE;

			if(Eh >= midh) { // average + 256
				ab = AVERAGE(ab, palettetranslate[*(uint16 *)(src + source + 256)]) & 0xF7DEF7DE; // to prevent overflow
				cd = AVERAGE(cd, palettetranslate[*(uint16 *)(src + source + 256 + 2)]) & 0xF7DEF7DE; // to prevent overflow
			}

			*dst++ = (ab & 0xFFFF) + AVERAGEHI(ab);
			*dst++ = (ab >> 16) + ((cd & 0xFFFF) << 16);
			*dst++ = (cd & 0xFFFF0000) + AVERAGELO(cd);

			source += 4;

		}
		dst += (480 - 384) / 2; 
		Eh += 224; if(Eh >= 272) { Eh -= 272; dh++; }
	}
}
#if 0
/*
	Upscale 256x224 -> 384*272 (for 480x272)

	Horizontal interpolation
		384/256=1.5
		4p -> 6p
		2dw -> 3dw

		for each line: 4 pixels => 6 pixels (*1.5) (64 blocks)
		[ab][cd] => [a(ab)][bc][(cd)d]


	Vertical upscale
		272 = 48 * 5 + 8 * 4 = 8*(6*5 + 1*4) (56 blocks)
		6 blocks of 5 scanlines (interpolated from 4 scanlines), 1 block of 4 scanlines (no interpolation)
		line 0
		line 1
		interpolated line 1 + 2 (skipped each 7th block, (blocknum % 6) == 0 )
		line 2
		line 3
*/
void upscale_384x272(uint32_t *dst, uint8_t *src)
{
	#define RENDERCHUNK(INSCANLINE, OUTSCANLINE) \
	{ \
		register uint32 ab, cd, a_ab, b_c, cd_d; \
		ab = palettetranslate[*(uint16 *)(src + 0 + (INSCANLINE) * 256)] & 0xF7DEF7DE; \
		cd = palettetranslate[*(uint16 *)(src + 2 + (INSCANLINE) * 256)] & 0xF7DEF7DE; \
		a_ab = (ab & 0xFFFF) + AVERAGEHI(ab); \
		b_c = (ab >> 16) + ((cd & 0xFFFF) << 16); \
		cd_d = (cd & 0xFFFF0000) + AVERAGELO(cd); \
		*(dst + 0 + (OUTSCANLINE) * 480/2) = a_ab; \
		*(dst + 1 + (OUTSCANLINE) * 480/2) = b_c; \
		*(dst + 2 + (OUTSCANLINE) * 480/2) = cd_d; \
	}

	#define RENDERICHUNK(OUTSCANLINE) \
	{ \
		*(dst + 0 + (OUTSCANLINE) * 480/2) = AVERAGE(*(dst + 0 + (OUTSCANLINE-1) * 480/2),*(dst + 0 + (OUTSCANLINE+1) * 480/2)); \
		*(dst + 1 + (OUTSCANLINE) * 480/2) = AVERAGE(*(dst + 1 + (OUTSCANLINE-1) * 480/2),*(dst + 1 + (OUTSCANLINE+1) * 480/2)); \
		*(dst + 2 + (OUTSCANLINE) * 480/2) = AVERAGE(*(dst + 2 + (OUTSCANLINE-1) * 480/2),*(dst + 2 + (OUTSCANLINE+1) * 480/2)); \
	}
	uint32 x, y;

	dst += (480 - 384) / 4;

	for(y = 224/4; y; y--) { // 56 blocks of 4-line chunks
		uint32 mf = y % 6; // mf == 0 - render 4 lines, mf != 0 - render 4->5 lines

		for(x = 256; x; x -= 4) {
			__builtin_prefetch(dst + 4, 1);
			RENDERCHUNK(0, 0); // 1st line
			RENDERCHUNK(1, 1); // 2nd line
			RENDERCHUNK(2, (mf == 0 ? 2 : 3)); // 3rd line
			if(mf != 0) RENDERICHUNK(2); // 2nd+3rd interpolated line
			RENDERCHUNK(3, (mf == 0 ? 3 : 4)); // 4th line
			dst += 3;
			src += 4;
		}

		dst += (480 - 384) / 2 + 480/2 * (mf == 0 ? 3 : 4); 
		src += 256 * 3; // cause we already rolled thru 256 pixel
	}
}
#endif