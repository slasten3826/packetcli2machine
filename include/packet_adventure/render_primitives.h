#ifndef PACKET_ADVENTURE_RENDER_PRIMITIVES_H
#define PACKET_ADVENTURE_RENDER_PRIMITIVES_H

#include <stdint.h>

#include "packet_adventure/render_target.h"

void pa_copy_surface(PaSurface *dst, const PaSurface *src);
void pa_clear(PaSurface *surface, uint32_t color);
void pa_fill_rect(PaSurface *surface, PaRect rect, uint32_t color);
void pa_draw_rect(PaSurface *surface, PaRect rect, uint32_t color);
void pa_draw_line(PaSurface *surface, int x0, int y0, int x1, int y1, uint32_t color);

#endif
