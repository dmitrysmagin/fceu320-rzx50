#ifndef BITMAP_H
#define BITMAP_H

struct bitmap16 {
    int width, height;
    unsigned short *data;
};

struct bitmap16 *load_bmp_16bpp(unsigned char *in, int width, int height);
void free_bitmap_16bpp(struct bitmap16 *bmp);
void draw_bitmap_16bpp(unsigned short *dest, struct bitmap16 *bmp);

#endif

