#ifndef PACKET_ADVENTURE_RENDER_TARGET_H
#define PACKET_ADVENTURE_RENDER_TARGET_H

#include <stddef.h>
#include <stdint.h>

#define PA_SCREEN_WIDTH 480
#define PA_SCREEN_HEIGHT 272

typedef enum {
    PA_PIXEL_FORMAT_565 = 0,
    PA_PIXEL_FORMAT_8888 = 1
} PaPixelFormat;

typedef struct {
    void *pixels;
    int width;
    int height;
    int stride;
    PaPixelFormat format;
} PaSurface;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} PaRect;

uint32_t pa_rgb(unsigned int r, unsigned int g, unsigned int b);

#endif
