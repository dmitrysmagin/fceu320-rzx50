
#include <stdlib.h>

#include "../dingoo.h"
#include "bitmap.h"

struct bitmap16 *load_bmp_16bpp(unsigned char *in, int width, int height) 
{
    if( in == NULL ) return NULL;

    struct bitmap16 *bmp = (struct bitmap16 *)malloc(sizeof(struct bitmap16));
    bmp->data = (unsigned short *)malloc(width*height*sizeof(unsigned short));

    bmp->width = width;
    bmp->height = height;
    
	memcpy(bmp->data, in, width*height*sizeof(unsigned short));

    return bmp;
}

void free_bitmap_16bpp(struct bitmap16 *bmp)
{
    if( bmp && bmp->data) {
        free(bmp->data);
        free(bmp);
    }
}

void draw_bitmap_16bpp(unsigned short *dest, struct bitmap16 *bmp)
{
}


