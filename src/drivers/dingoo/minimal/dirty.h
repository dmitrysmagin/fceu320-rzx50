#define MAX_GFX_WIDTH 1600
#define MAX_GFX_HEIGHT 1600
#define DIRTY_H 256 			/* 160 would be enough, but * 256 is faster */
#define DIRTY_V 100


typedef char dirtygrid[DIRTY_V * DIRTY_H];
#define ISDIRTY(x,y) (dirty_new[((y)>>4) * DIRTY_H + ((x)>>4)] || dirty_old[((y)>>4) * DIRTY_H + ((x)>>4)])
#define MARKDIRTY(x,y) dirty_new[((y)>>4) * DIRTY_H + ((x)>>4)] = 1
