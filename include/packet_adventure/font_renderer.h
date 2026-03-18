#ifndef PACKET_ADVENTURE_FONT_RENDERER_H
#define PACKET_ADVENTURE_FONT_RENDERER_H

#include <stdint.h>

#include "packet_adventure/render_target.h"

void pa_draw_text(PaSurface *surface, int x, int y, const char *text, uint32_t color, int scale);
int pa_text_width(const char *text, int scale);
void pa_draw_text_small(PaSurface *surface, int x, int y, const char *text, uint32_t color);
int pa_text_small_width(const char *text);

#endif
