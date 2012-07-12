#include "driver.h"
#include "dirty.h"
#include "minimal.h"

/* from video.c */
extern char *dirty_old;
extern char *dirty_new;
extern int gfx_xoffset;	/* Dingoo lines to skip - dest origin */
extern int gfx_yoffset;	/* Dingoo cols to skip - dest origin */
extern int gfx_display_lines;	/* Lines to copy from src -> dest */
extern int gfx_display_columns;	/* Columns to copy from src -> dest */
extern int gfx_width;	/* MAME game width drawn to dest (usually 320) */
extern int gfx_height;	/* MAME game lines rendered to dest */
extern int skiplines;	/* MAME game columns skip */
extern int skipcolumns;	/* MAME game lines skip */
extern int video_scale;	/* Scaling method 0=none, 1=horiz, 2=half, 3=best */

extern "C" {
	void clut8to16(unsigned short *dst, unsigned char *src, unsigned short *pal, int count);
}


UINT32 *palette_16bit_lookup;

#define SCREEN8 dingoo_screen15
#define SCREEN16 dingoo_screen15
#define FLUSH_CACHE  

#define BLIT_8_TO_16_half(DEST,SRC,LEN)  \
{ \
	register int n = LEN/8; \
	register unsigned int *u32dest = (unsigned int *)DEST; \
	register unsigned char *src = SRC; \
	while (n) \
	{ \
		*u32dest++ = (dingoo_palette[*(src+3)] << 16) | dingoo_palette[*src]; \
		src+=4; \
		*u32dest++ = (dingoo_palette[*(src+3)] << 16) | dingoo_palette[*src]; \
		src+=4; \
		*u32dest++ = (dingoo_palette[*(src+3)] << 16) | dingoo_palette[*src]; \
		src+=4; \
		*u32dest++ = (dingoo_palette[*(src+3)] << 16) | dingoo_palette[*src]; \
		src+=4; \
		n--; \
	} \
	if ((n=(LEN&7))) \
	{ \
		register unsigned short *u16dest = ((unsigned short *)DEST) + (LEN & (~7)); \
		do { \
			*u16dest++=dingoo_palette[*src]; \
			src+=2; \
		} while (--n); \
	} \
}

#define BLIT_8_TO_16_32bit(DEST,SRC,LEN)  \
{ \
	register int n = LEN/8; \
	register unsigned int *u32dest = (unsigned int *)DEST; \
	register unsigned char *src = SRC; \
	while (n) \
	{ \
		*u32dest++ = (dingoo_palette[*(src++ + 1)] << 16) | dingoo_palette[*src++]; \
		*u32dest++ = (dingoo_palette[*(src++ + 1)] << 16) | dingoo_palette[*src++]; \
		*u32dest++ = (dingoo_palette[*(src++ + 1)] << 16) | dingoo_palette[*src++]; \
		*u32dest++ = (dingoo_palette[*(src++ + 1)] << 16) | dingoo_palette[*src++]; \
		n--; \
	} \
	if ((n=(LEN&7))) \
	{ \
		register unsigned short *u16dest = ((unsigned short *)DEST) + (LEN & (~7)); \
		do { \
			*u16dest++=dingoo_palette[*src++]; \
		} while (--n); \
	} \
}


INLINE unsigned int mix_color16 (unsigned int color1, unsigned int color2)
{
    return dingoo_video_color15((dingoo_video_getr15(color1)+dingoo_video_getr15(color2))>>1,(dingoo_video_getg15(color1)+dingoo_video_getg15(color2))>>1,(dingoo_video_getb15(color1)+dingoo_video_getb15(color2))>>1);
}


void blitscreen_dirty1_color8(struct osd_bitmap *bitmap)
{
	int x, y;
	int width=(bitmap->line[1] - bitmap->line[0]);
	unsigned char *lb=bitmap->line[skiplines] + skipcolumns;
	unsigned short *address=SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);

	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; )
		{
			int w = 16;
			if (ISDIRTY(x,y))
			{
				int h;
				unsigned char *lb0 = lb + x;
				unsigned short *address0 = address + x;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    			w += 16;
				if (x + w > gfx_display_columns)
                    			w = gfx_display_columns - x;
				for (h = 0; ((h < 16) && ((y + h) < gfx_display_lines)); h++)
				{
					//memcpy(address0,lb0,w);
					//BLIT_8_TO_16_32bit(address0,lb0,w)
					clut8to16(address0, lb0, dingoo_palette, w);
					lb0 += width;
					address0 += gfx_width;
				}
			}
			x += w;
        	}
		lb += 16 * width;
		address += 16 * gfx_width;
	}
	FLUSH_CACHE
}

INLINE void blitscreen_dirty0_color8_noscale(struct osd_bitmap *bitmap)
{
	int y=gfx_display_lines;
	int width=(bitmap->line[1] - bitmap->line[0]);
	int columns=gfx_display_columns;
	unsigned char *lb = bitmap->line[skiplines] + skipcolumns;
	unsigned short *address = SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);

	do
	{
		//memcpy(address,lb,columns);
		//BLIT_8_TO_16_32bit(address,lb,columns)
		clut8to16(address, lb, dingoo_palette, columns);
		lb+=width;
		address+=gfx_width;
		y--;
	}
	while (y);
}

INLINE void blitscreen_dirty0_color8_halfscale(struct osd_bitmap *bitmap)
{
	int y=(gfx_display_lines>>1);
	int width=(bitmap->line[1] - bitmap->line[0]);
	int columns=gfx_display_columns>>1;
	unsigned char *lb = bitmap->line[skiplines] + skipcolumns;
	unsigned short *address = SCREEN8 + (gfx_xoffset/2) + ((gfx_yoffset/2) * gfx_width);

	do
	{
		//memcpy(address,lb,columns);
		BLIT_8_TO_16_half(address,lb,columns)
		lb+=(width*2);
		address+=gfx_width;
		y--;
	}
	while (y);
}

INLINE void blitscreen_dirty0_color8_horzscale(struct osd_bitmap *bitmap)
{
	unsigned short *buffer_scr = (unsigned short *)(SCREEN8+gfx_xoffset+(DINGOO_SCREEN_WIDTH*gfx_yoffset));
	unsigned char *buffer_mem = (unsigned char *)(bitmap->line[skiplines]+skipcolumns);
	int buffer_mem_offset = (bitmap->line[1] - bitmap->line[0])-gfx_width;
	int step,i;
	int x,y=gfx_display_lines;
	
	if (gfx_width>DINGOO_SCREEN_WIDTH)
	{
		/* Scale Down */
		step=DINGOO_SCREEN_WIDTH/(gfx_width-DINGOO_SCREEN_WIDTH);
		do {
			x=DINGOO_SCREEN_WIDTH; i=step;
			do {
				*buffer_scr++=(dingoo_palette[*buffer_mem++]);
				x--; i--;
				if (!i) { buffer_mem++; i=step; }
			} while (x);
			buffer_mem+=buffer_mem_offset;		
			y--;
		} while (y);
	}
	else
	{
		/* Scale Up */
		step=DINGOO_SCREEN_WIDTH/(DINGOO_SCREEN_WIDTH-gfx_width);
		do {
			x=DINGOO_SCREEN_WIDTH; i=1;
			do {
				i--;
				if (i) { *buffer_scr++=(dingoo_palette[*buffer_mem++]); }
				else { *buffer_scr++=(dingoo_palette[*buffer_mem]); i=step; }
				x--;
			} while (x);						
			buffer_mem+=buffer_mem_offset;		
			y--;
		} while (y);
	}
	
	FLUSH_CACHE
}


/* video_scale = 3/4 Best fit blitters */

extern unsigned int iAddX;
extern unsigned int iModuloX;
extern unsigned int iAddY;
extern unsigned int iModuloY;

INLINE void blitscreen_dirty0_color8_fitscale_merge1(struct osd_bitmap *bitmap)
{
  unsigned int lines=gfx_display_lines;
  unsigned int columns=gfx_display_columns;
  unsigned int src_width=bitmap->line[1] - bitmap->line[0];
  unsigned int dst_width=gfx_width;
  unsigned char *src=bitmap->line[skiplines] + skipcolumns;
  unsigned short *dst=SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);

  unsigned int _iAddY =iAddY;
  unsigned int _iModuloY = iModuloY;
  unsigned int _iAddX = iAddX;
  unsigned int _iModuloX = iModuloX;
  
  unsigned int merge = 0;
  int accumulatorY = 0;

  for (unsigned int h = 0; h < lines; h++)
  {
    register unsigned int pixel = 0;
    register unsigned short *dstX=dst;
    register unsigned char *srcX=src;
    register int accumulatorX = 0;

    for (register int w = columns - 1; w >= 0; w--)
    {
      register unsigned int srcPixel = dingoo_palette[*srcX++];
      if (accumulatorX >= _iAddX)
      {
        srcPixel = mix_color16(pixel, srcPixel);
      }
      pixel = srcPixel;
  
      accumulatorX += _iAddX;
      if (accumulatorX >= _iModuloX || w == 0)
      {
        if (merge)
        {
           pixel = mix_color16(*dstX, pixel);
        }
        *dstX = pixel;
        dstX++;
        accumulatorX -= _iModuloX;
      }
    }

    accumulatorY += _iAddY;
    if (accumulatorY >= _iModuloY || h == lines - 1)
    {
      dst += dst_width;
      merge = 0;
      accumulatorY -= _iModuloY;
    }
    else
    {
      merge = 1;
    }
    src += src_width; // Next src line
  }

}


INLINE void blitscreen_dirty0_color8_fitscale_merge0(struct osd_bitmap *bitmap)
{
  unsigned int lines=gfx_display_lines;
  unsigned int columns=gfx_display_columns;
  unsigned int src_width=bitmap->line[1] - bitmap->line[0];
  unsigned int dst_width=gfx_width;
  unsigned char *src=bitmap->line[skiplines] + skipcolumns;
  unsigned short *dst=SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);

  unsigned int _iAddY =iAddY;
  unsigned int _iModuloY = iModuloY;
  unsigned int _iAddX = iAddX;
  unsigned int _iModuloX = iModuloX;
  
  int accumulatorY = 0;

  for (unsigned int h = 0; h < lines; h++)
  {
    register unsigned int pixel = 0;
    register unsigned short *dstX=dst;
    register unsigned char *srcX=src;
    register int accumulatorX = 0;

    accumulatorY += _iAddY;
    if (accumulatorY >= _iModuloY || h == lines - 1)
    {
      accumulatorY -= _iModuloY;

      for (register int w = columns - 1; w >= 0; w--)
      {
        register unsigned int srcPixel = dingoo_palette[*srcX++];
        if (accumulatorX >= _iAddX)
        {
          srcPixel = mix_color16(pixel, srcPixel);
        }
        pixel = srcPixel;
    
        accumulatorX += _iAddX;
        if (accumulatorX >= _iModuloX || w == 0)
        {
          *dstX++ = pixel;
          accumulatorX -= _iModuloX;
        }
      }
      dst += dst_width;
    }

    src += src_width;
  }
}

void blitscreen_dirty0_color8(struct osd_bitmap *bitmap)
{
    if (video_scale == 1) /* Horizontal Only */
    {
        blitscreen_dirty0_color8_horzscale(bitmap);
    }
    else if (video_scale == 2) /* Half Scale */
    {
        blitscreen_dirty0_color8_halfscale(bitmap);
    }
    else if (video_scale == 3) /* Best Fit Scale */
    {
        blitscreen_dirty0_color8_fitscale_merge1(bitmap);
    }
    else if (video_scale == 4) /* Fast Fit Scale */
    {
        blitscreen_dirty0_color8_fitscale_merge0(bitmap);
    }
    else /* Default is normal blit with no scaling */
    {
        blitscreen_dirty0_color8_noscale(bitmap);
    }
}

void blitscreen_dirty1_palettized16(struct osd_bitmap *bitmap)
{
	int x, y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned short *lb=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	unsigned short *address=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; )
		{
			int w = 16;
			if (ISDIRTY(x,y))
			{
				int h;
				unsigned short *lb0 = lb + x;
				unsigned short *address0 = address + x;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    			w += 16;
				if (x + w > gfx_display_columns)
                    			w = gfx_display_columns - x;
				for (h = 0; ((h < 16) && ((y + h) < gfx_display_lines)); h++)
				{
					int wx;
					for (wx=0;wx<w;wx++)
					{
						address0[wx] = palette_16bit_lookup[lb0[wx]];
					}
					lb0 += width;
					address0 += gfx_width;
				}
			}
			x += w;
        	}
		lb += 16 * width;
		address += 16 * gfx_width;
	}
	FLUSH_CACHE
}

INLINE void blitscreen_dirty0_palettized16_noscale(struct osd_bitmap *bitmap)
{
	int x,y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	unsigned short *address = SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	for (y = 0; y < gfx_display_lines; y++)
	{
		for (x = 0; x < columns; x++)
		{
			address[x] = palette_16bit_lookup[lb[x]];
		}
		lb+=width;
		address+=gfx_width;
	}
	
	FLUSH_CACHE
}

INLINE void blitscreen_dirty0_palettized16_horzscale(struct osd_bitmap *bitmap)
{
	unsigned short *buffer_scr = SCREEN16 + gfx_xoffset + (DINGOO_SCREEN_WIDTH*gfx_yoffset);
	unsigned short *buffer_mem = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	int buffer_mem_offset = ((bitmap->line[1] - bitmap->line[0])>>1)-gfx_width;
	int step,i;
	int x,y=gfx_display_lines;
	
	if (gfx_width>DINGOO_SCREEN_WIDTH)
	{
		/* Scale Up */
		step=DINGOO_SCREEN_WIDTH/(gfx_width-DINGOO_SCREEN_WIDTH);
		do {
			x=DINGOO_SCREEN_WIDTH; i=step;
			do {
				*buffer_scr++=palette_16bit_lookup[*buffer_mem++];
				x--; i--;
				if (!i) { if (x) { *buffer_scr++=mix_color16(palette_16bit_lookup[*buffer_mem++],palette_16bit_lookup[*buffer_mem++]); x--; i=step-1; } else { buffer_mem++; i=step; } }
			} while (x);
			buffer_mem+=buffer_mem_offset;		
			y--;
		} while (y);
	}
	else
	{
		/* Scale Down */
		step=DINGOO_SCREEN_WIDTH/(DINGOO_SCREEN_WIDTH-gfx_width);
		do {
			x=DINGOO_SCREEN_WIDTH; i=1;
			do {
				i--;
				if (i) { *buffer_scr++=palette_16bit_lookup[*buffer_mem++]; }
				else { *buffer_scr++=mix_color16(palette_16bit_lookup[buffer_mem[0]],palette_16bit_lookup[buffer_mem[-1]]); i=step; }
				x--;
			} while (x);						
			buffer_mem+=buffer_mem_offset;		
			y--;
		} while (y);
	}
	
	FLUSH_CACHE
}

INLINE void blitscreen_dirty0_palettized16_halfscale(struct osd_bitmap *bitmap)
{
	int x,y;
	unsigned int lines=gfx_display_lines>>1;
	int width=bitmap->line[1] - bitmap->line[0];
	int columns=(gfx_display_columns>>1);
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	unsigned short *address = SCREEN16 + (gfx_xoffset/2) + ((gfx_yoffset/2) * gfx_width);

	for (y = 0; y < lines; y++)
	{
		for (x = 0; x < columns; x++)
		{
			address[x] = mix_color16(palette_16bit_lookup[lb[(x<<1)]], palette_16bit_lookup[lb[(x<<1)+1]]);
		}
		lb+=width;
		address+=gfx_width;
	}

	FLUSH_CACHE
}

INLINE void blitscreen_dirty0_palettized16_fitscale_merge0(struct osd_bitmap *bitmap)
{
  unsigned int lines=gfx_display_lines;
  unsigned int columns=gfx_display_columns;
  unsigned int src_width=(bitmap->line[1] - bitmap->line[0])>>1;
  unsigned int dst_width=gfx_width;
  unsigned short *src=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
  unsigned short *dst=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

  unsigned int _iAddY =iAddY;
  unsigned int _iModuloY = iModuloY;
  unsigned int _iAddX = iAddX;
  unsigned int _iModuloX = iModuloX;
  
  int accumulatorY = 0;

  for (unsigned int h = 0; h < lines; h++)
  {

    accumulatorY += _iAddY;
    if (accumulatorY >= _iModuloY || h == lines - 1)
    {
      register unsigned int pixel = 0;
      register unsigned short *dstX=dst;
      register unsigned short *srcX=src;
      register int accumulatorX = 0;

      accumulatorY -= _iModuloY;

      for (register int w = columns - 1; w >= 0; w--)
      {
        register unsigned int srcPixel = palette_16bit_lookup[*srcX++];
        if (accumulatorX >= _iAddX)
        {
          srcPixel = mix_color16(pixel, srcPixel);
        }
        pixel = srcPixel;
    
        accumulatorX += _iAddX;
        if (accumulatorX >= _iModuloX || w == 0)
        {
          *dstX++ = pixel;
          accumulatorX -= _iModuloX;
        }
      }
      dst += dst_width;
    }

    src += src_width;
  }
}


INLINE void blitscreen_dirty0_palettized16_fitscale_merge1(struct osd_bitmap *bitmap)
{
  unsigned int lines=gfx_display_lines;
  unsigned int columns=gfx_display_columns;
  unsigned int src_width=(bitmap->line[1] - bitmap->line[0])>>1;
  unsigned int dst_width=gfx_width;
  unsigned short *src=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
  unsigned short *dst=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

  unsigned int _iAddY =iAddY;
  unsigned int _iModuloY = iModuloY;
  unsigned int _iAddX = iAddX;
  unsigned int _iModuloX = iModuloX;
  
  unsigned int merge = 0;
  int accumulatorY = 0;

  for (unsigned int h = 0; h < lines; h++)
  {
    register unsigned int pixel = 0;
    register unsigned short *dstX=dst;
    register unsigned short *srcX=src;
    register int accumulatorX = 0;

    for (register int w = columns - 1; w >= 0; w--)
    {
      register unsigned int srcPixel = palette_16bit_lookup[*srcX++];
      if (accumulatorX >= _iAddX)
      {
        srcPixel = mix_color16(pixel, srcPixel);
      }
      pixel = srcPixel;
  
      accumulatorX += _iAddX;
      if (accumulatorX >= _iModuloX || w == 0)
      {
        if (merge)
        {
           pixel = mix_color16(*dstX, pixel);
        }
        *dstX = pixel;
        dstX++;
        accumulatorX -= _iModuloX;
      }
    }

    accumulatorY += _iAddY;
    if (accumulatorY >= _iModuloY || h == lines - 1)
    {
      dst += dst_width;
      merge = 0;
      accumulatorY -= _iModuloY;
    }
    else
    {
      merge = 1;
    }
    src += src_width; // Next src line
  }

}


void blitscreen_dirty0_palettized16(struct osd_bitmap *bitmap)
{
    if (video_scale == 1) /* Horizontal Only */
    {
        blitscreen_dirty0_palettized16_horzscale(bitmap);
    }
    else if (video_scale == 2) /* Half Scale */
    {
        blitscreen_dirty0_palettized16_halfscale(bitmap);
    }
    else if (video_scale == 3) /* Best Fit Scale */
    {
        blitscreen_dirty0_palettized16_fitscale_merge1(bitmap);
    }
    else if (video_scale == 4) /* Fast Fit Scale */
    {
        blitscreen_dirty0_palettized16_fitscale_merge0(bitmap);
    }
    else
    {
        blitscreen_dirty0_palettized16_noscale(bitmap);
    }
}

void blitscreen_dirty1_color16(struct osd_bitmap *bitmap)
{
	int x, y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned short *lb=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	unsigned short *address=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; )
		{
			int w = 16;
			if (ISDIRTY(x,y))
			{
				int h;
				unsigned short *lb0 = lb + x;
				unsigned short *address0 = address + x;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    			w += 16;
				if (x + w > gfx_display_columns)
                    			w = gfx_display_columns - x;
				for (h = 0; ((h < 16) && ((y + h) < gfx_display_lines)); h++)
				{
					memcpy(address0,lb0,w<<1);
					lb0 += width;
					address0 += gfx_width;
				}
			}
			x += w;
        	}
		lb += 16 * width;
		address += 16 * gfx_width;
	}
}

INLINE void blitscreen_dirty0_color16_noscale(struct osd_bitmap *bitmap)
{
	int y=gfx_display_lines;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns<<1;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	unsigned short *address = SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	do
	{
	    	memcpy(address,lb,columns);
		lb+=width;
		address+=gfx_width;
		y--;
	}
	while (y);
	
	FLUSH_CACHE
}

INLINE void blitscreen_dirty0_color16_horzscale(struct osd_bitmap *bitmap)
{
	unsigned short *buffer_scr = SCREEN16 + gfx_xoffset + (DINGOO_SCREEN_WIDTH*gfx_yoffset);
	unsigned short *buffer_mem = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	int buffer_mem_offset = ((bitmap->line[1] - bitmap->line[0])>>1)-gfx_width;
	int step,i;
	int x,y=gfx_display_lines;
	
	if (gfx_width>DINGOO_SCREEN_WIDTH)
	{
		/* Scale Up */
		step=DINGOO_SCREEN_WIDTH/(gfx_width-DINGOO_SCREEN_WIDTH);
		do {
			x=DINGOO_SCREEN_WIDTH; i=step;
			do {
				*buffer_scr++=*buffer_mem++;
				x--; i--;
				if (!i) { if (x) { *buffer_scr++=mix_color16(*buffer_mem++,*buffer_mem++); x--; i=step-1; } else { buffer_mem++; i=step; } }
			} while (x);
			buffer_mem+=buffer_mem_offset;		
			y--;
		} while (y);
	}
	else
	{
		/* Scale Down */
		step=DINGOO_SCREEN_WIDTH/(DINGOO_SCREEN_WIDTH-gfx_width);
		do {
			x=DINGOO_SCREEN_WIDTH; i=1;
			do {
				i--;
				if (i) { *buffer_scr++=*buffer_mem++; }
				else { *buffer_scr++=mix_color16(buffer_mem[0],buffer_mem[-1]); i=step; }
				x--;
			} while (x);						
			buffer_mem+=buffer_mem_offset;		
			y--;
		} while (y);
	}
	
	FLUSH_CACHE
}

INLINE void blitscreen_dirty0_color16_halfscale(struct osd_bitmap *bitmap)
{
        int x,y;
        unsigned int lines=gfx_display_lines>>1;
        int width=bitmap->line[1] - bitmap->line[0];
        int columns=(gfx_display_columns>>1);
        unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
        unsigned short *address = SCREEN16 + (gfx_xoffset/2) + ((gfx_yoffset/2) * gfx_width);

        for (y = 0; y < lines; y++)
        {
                for (x = 0; x < columns; x++)
                {
                        address[x] = mix_color16(lb[(x<<1)], lb[(x<<1)+1]);
                }
                lb+=width;
                address+=gfx_width;
        }

        FLUSH_CACHE
}

INLINE void blitscreen_dirty0_color16_fitscale_merge0(struct osd_bitmap *bitmap)
{
  unsigned int lines=gfx_display_lines;
  unsigned int columns=gfx_display_columns;
  unsigned int src_width=(bitmap->line[1] - bitmap->line[0])>>1;
  unsigned int dst_width=gfx_width;
  unsigned short *src=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
  unsigned short *dst=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

  unsigned int _iAddY =iAddY;
  unsigned int _iModuloY = iModuloY;
  unsigned int _iAddX = iAddX;
  unsigned int _iModuloX = iModuloX;
  
  int accumulatorY = 0;

  for (unsigned int h = 0; h < lines; h++)
  {
    accumulatorY += _iAddY;
    if (accumulatorY >= _iModuloY || h == lines - 1)
    {
      register unsigned int pixel = 0;
      register unsigned short *dstX=dst;
      register unsigned short *srcX=src;
      register int accumulatorX = 0;

      accumulatorY -= _iModuloY;

      for (register int w = columns - 1; w >= 0; w--)
      {
        register unsigned int srcPixel = *srcX++;
        if (accumulatorX >= _iAddX)
        {
          srcPixel = mix_color16(pixel, srcPixel);
        }
        pixel = srcPixel;
    
        accumulatorX += _iAddX;
        if (accumulatorX >= _iModuloX || w == 0)
        {
          *dstX++ = pixel;
          accumulatorX -= _iModuloX;
        }
      }
      dst += dst_width;
    }

    src += src_width;
  }
}


INLINE void blitscreen_dirty0_color16_fitscale_merge1(struct osd_bitmap *bitmap)
{
  unsigned int lines=gfx_display_lines;
  unsigned int columns=gfx_display_columns;
  unsigned int src_width=(bitmap->line[1] - bitmap->line[0])>>1;
  unsigned int dst_width=gfx_width;
  unsigned short *src=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
  unsigned short *dst=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

  unsigned int _iAddY =iAddY;
  unsigned int _iModuloY = iModuloY;
  unsigned int _iAddX = iAddX;
  unsigned int _iModuloX = iModuloX;
  
  unsigned int merge = 0;
  int accumulatorY = 0;

  for (unsigned int h = 0; h < lines; h++)
  {
    register unsigned int pixel = 0;
    register unsigned short *dstX=dst;
    register unsigned short *srcX=src;
    register int accumulatorX = 0;

    for (register int w = columns - 1; w >= 0; w--)
    {
      register unsigned int srcPixel = *srcX++;
      if (accumulatorX >= _iAddX)
      {
        srcPixel = mix_color16(pixel, srcPixel);
      }
      pixel = srcPixel;
  
      accumulatorX += _iAddX;
      if (accumulatorX >= _iModuloX || w == 0)
      {
        if (merge)
        {
           pixel = mix_color16(*dstX, pixel);
        }
        *dstX = pixel;
        dstX++;
        accumulatorX -= _iModuloX;
      }
    }

    accumulatorY += _iAddY;
    if (accumulatorY >= _iModuloY || h == lines - 1)
    {
      dst += dst_width;
      merge = 0;
      accumulatorY -= _iModuloY;
    }
    else
    {
      merge = 1;
    }
    src += src_width; // Next src line
  }

}


void blitscreen_dirty0_color16(struct osd_bitmap *bitmap)
{
    if (video_scale == 1) /* Horizontal Only */
    {
        blitscreen_dirty0_color16_horzscale(bitmap);
    }
    else if (video_scale == 2) /* Half Scale */
    {
        blitscreen_dirty0_color16_halfscale(bitmap);
    }
    else if (video_scale == 3) /* Best Fit Scale */
    {
        blitscreen_dirty0_color16_fitscale_merge1(bitmap);
    }
    else if (video_scale == 4) /* Fast Fit Scale */
    {
        blitscreen_dirty0_color16_fitscale_merge0(bitmap);
    }
    else
    {
        blitscreen_dirty0_color16_noscale(bitmap);
    }
}



