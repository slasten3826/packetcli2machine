#include "packet/render_cli.h"

#include <stdio.h>

#include "packet_adventure/runtime_internal.h"

#define PA_CLI_FRAME_END "---PACKET-CLI-FRAME-END---"

static char pa_operator_glyph(PaOperatorType op, bool calm) {
    char glyph = '?';

    switch (op) {
        case PA_OP_FLOW: glyph = 'f'; break;
        case PA_OP_CONNECT: glyph = 'c'; break;
        case PA_OP_DISSOLVE: glyph = 'd'; break;
        case PA_OP_ENCODE: glyph = 'e'; break;
        case PA_OP_CHOOSE: glyph = 'h'; break;
        case PA_OP_OBSERVE: glyph = 'o'; break;
        case PA_OP_CYCLE: glyph = 'y'; break;
        case PA_OP_LOGIC: glyph = 'l'; break;
        case PA_OP_RUNTIME: glyph = 'r'; break;
        case PA_OP_MANIFEST: glyph = 'm'; break;
        default: glyph = '?'; break;
    }

    if (calm && glyph >= 'a' && glyph <= 'z') {
        glyph = (char)(glyph - ('a' - 'A'));
    }
    return glyph;
}

static char pa_density_glyph(unsigned int density, bool turbulence, bool calm) {
    static const char k_ramp[] = " .:-=+*#%@";
    unsigned int index = (density * 9u) / 255u;

    if (calm) {
        return 'C';
    }
    if (turbulence) {
        return '!';
    }
    return k_ramp[index];
}

static const char *pa_cli_mode_name(PaCliRenderMode mode) {
    switch (mode) {
        case PA_CLI_MODE_GLYPH: return "glyph";
        case PA_CLI_MODE_DENSITY: return "density";
        case PA_CLI_MODE_COMPOSITE: return "composite";
        default: return "unknown";
    }
}

void pa_cli_render_frame(FILE *out, const PaApp *app, PaCliRenderMode mode, int auto_run) {
    int min_x = app->packet_x - (PA_VIEW_COLS / 2);
    int min_y = app->packet_y - (PA_VIEW_ROWS / 2);
    unsigned int visible_manifest = 0u;
    unsigned int visible_crystal = 0u;
    unsigned int visible_turbulence = 0u;
    unsigned int visible_walls = 0u;
    unsigned int visible_latent = 0u;
    int vy;

    fprintf(out, "\x1b[H\x1b[2J");
    fprintf(out, "PACKET DEV CLI  frame=%u sweep=%u sw=%s field=%d mode=%s run=%s turbulence>=%u\n",
        app->frame,
        app->sweep_frame,
        app->sweep_paused ? "paused" : "live",
        app->packet_field_radius,
        pa_cli_mode_name(mode),
        auto_run ? "auto" : "step",
        PA_TURBULENCE_THRESHOLD);
    fprintf(out, "packet=(%d,%d) transition=(%d,%d) layer=%u hash=%08X\n",
        app->packet_x, app->packet_y,
        app->transition_x, app->transition_y,
        app->layer_index,
        app->debug_world_hash);

    for (vy = 0; vy < PA_VIEW_ROWS; ++vy) {
        int vx;

        for (vx = 0; vx < PA_VIEW_COLS; ++vx) {
            int wx = pa_wrap_coord(min_x + vx, PA_WORLD_COLS);
            int wy = pa_wrap_coord(min_y + vy, PA_WORLD_ROWS);
            int mf_strength = pa_manifest_mf_strength_impl(app, wx, wy);
            bool calm = app->calm[wy][wx];
            bool manifested = mf_strength > 0;
            bool turbulence = pa_manifest_cell_is_turbulence_impl(app, wx, wy);
            unsigned int density = app->local_density[wy][wx];
            char glyph = ' ';
            char right = ' ';

            if (wx == app->packet_x && wy == app->packet_y) {
                glyph = '@';
            } else if (wx == app->transition_x && wy == app->transition_y) {
                glyph = '*';
            } else if (app->tiles[wy][wx] == PA_TILE_WALL) {
                glyph = '#';
            } else if (calm) {
                glyph = pa_operator_glyph((PaOperatorType)app->operators[wy][wx], true);
            } else if (manifested) {
                glyph = pa_operator_glyph((PaOperatorType)app->operators[wy][wx], false);
            }

            if (app->tiles[wy][wx] == PA_TILE_WALL) {
                visible_walls += 1u;
            } else if (calm) {
                visible_crystal += 1u;
            } else if (manifested) {
                visible_manifest += 1u;
                if (turbulence) {
                    visible_turbulence += 1u;
                }
            } else {
                visible_latent += 1u;
            }

            switch (mode) {
                case PA_CLI_MODE_GLYPH:
                    right = pa_density_glyph(density, turbulence, calm);
                    break;
                case PA_CLI_MODE_DENSITY:
                    glyph = pa_density_glyph(density, turbulence, calm);
                    right = (char)('0' + mf_strength);
                    break;
                case PA_CLI_MODE_COMPOSITE:
                default:
                    if (turbulence && glyph != '@' && glyph != '*' && glyph != '#') {
                        glyph = '!';
                    }
                    right = (char)('0' + mf_strength);
                    break;
            }

            fputc(glyph, out);
            if (vx == PA_VIEW_COLS - 1) {
                fprintf(out, " | ");
            }
            fputc(right, out);
        }
        fputc('\n', out);
    }

    fprintf(out, "view: latent=%u manifest=%u crystal=%u turbulence=%u walls=%u\n",
        visible_latent,
        visible_manifest,
        visible_crystal,
        visible_turbulence,
        visible_walls);
    fprintf(out, "legend: @ packet, * node, # wall, uppercase=CRYSTAL, lowercase=MANIFEST, !=TURBULENCE, right=density/mf\n");
    fprintf(out, "%s\n", PA_CLI_FRAME_END);
    fflush(out);
}
