#ifndef PACKET_ADVENTURE_UI_RENDER_H
#define PACKET_ADVENTURE_UI_RENDER_H

#include "packet_adventure/app.h"

void pa_ui_render_copy_cache(PaSurface *surface, const PaApp *app);
void pa_ui_render_status(PaSurface *surface, const PaApp *app);
void pa_ui_render_log(PaSurface *surface, const PaApp *app);
void pa_ui_render_debug_map(PaSurface *surface, const PaApp *app);

#endif
