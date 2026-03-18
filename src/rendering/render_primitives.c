#include "packet_adventure/render_primitives.h"

#include <string.h>

static uint16_t pa_pack_565(uint32_t color) {
    unsigned int r = color & 0xFF;
    unsigned int g = (color >> 8) & 0xFF;
    unsigned int b = (color >> 16) & 0xFF;
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

uint32_t pa_rgb(unsigned int r, unsigned int g, unsigned int b) {
    return (r & 0xFFu) | ((g & 0xFFu) << 8) | ((b & 0xFFu) << 16);
}

static void pa_put_pixel(PaSurface *surface, int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || x >= surface->width || y >= surface->height) {
        return;
    }

    if (surface->format == PA_PIXEL_FORMAT_8888) {
        uint32_t *pixels = (uint32_t *)surface->pixels;
        pixels[y * surface->stride + x] = color;
    } else {
        uint16_t *pixels = (uint16_t *)surface->pixels;
        pixels[y * surface->stride + x] = pa_pack_565(color);
    }
}

void pa_copy_surface(PaSurface *dst, const PaSurface *src) {
    size_t row_bytes;
    int y;

    if (dst->width != src->width || dst->height != src->height || dst->format != src->format) {
        return;
    }

    row_bytes = (size_t)dst->width * (dst->format == PA_PIXEL_FORMAT_8888 ? sizeof(uint32_t) : sizeof(uint16_t));
    for (y = 0; y < dst->height; ++y) {
        char *dst_row;
        const char *src_row;

        if (dst->format == PA_PIXEL_FORMAT_8888) {
            dst_row = (char *)((uint32_t *)dst->pixels + (y * dst->stride));
            src_row = (const char *)((const uint32_t *)src->pixels + (y * src->stride));
        } else {
            dst_row = (char *)((uint16_t *)dst->pixels + (y * dst->stride));
            src_row = (const char *)((const uint16_t *)src->pixels + (y * src->stride));
        }
        memcpy(dst_row, src_row, row_bytes);
    }
}

void pa_clear(PaSurface *surface, uint32_t color) {
    int x;
    int y;
    for (y = 0; y < surface->height; ++y) {
        for (x = 0; x < surface->width; ++x) {
            pa_put_pixel(surface, x, y, color);
        }
    }
}

void pa_fill_rect(PaSurface *surface, PaRect rect, uint32_t color) {
    int x;
    int y;
    for (y = rect.y; y < rect.y + rect.h; ++y) {
        for (x = rect.x; x < rect.x + rect.w; ++x) {
            pa_put_pixel(surface, x, y, color);
        }
    }
}

void pa_draw_rect(PaSurface *surface, PaRect rect, uint32_t color) {
    int x;
    int y;
    for (x = rect.x; x < rect.x + rect.w; ++x) {
        pa_put_pixel(surface, x, rect.y, color);
        pa_put_pixel(surface, x, rect.y + rect.h - 1, color);
    }
    for (y = rect.y; y < rect.y + rect.h; ++y) {
        pa_put_pixel(surface, rect.x, y, color);
        pa_put_pixel(surface, rect.x + rect.w - 1, y, color);
    }
}

void pa_draw_line(PaSurface *surface, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? -(y1 - y0) : -(y0 - y1);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        pa_put_pixel(surface, x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        if (2 * err >= dy) {
            err += dy;
            x0 += sx;
        }
        if (2 * err <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}
