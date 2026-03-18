#ifndef PACKET_RENDER_CLI_H
#define PACKET_RENDER_CLI_H

#include <stdio.h>

#include "packet/core.h"

typedef enum {
    PA_CLI_MODE_GLYPH = 0,
    PA_CLI_MODE_DENSITY = 1,
    PA_CLI_MODE_COMPOSITE = 2
} PaCliRenderMode;

void pa_cli_render_frame(FILE *out, const PaApp *app, PaCliRenderMode mode, int auto_run);

#endif
