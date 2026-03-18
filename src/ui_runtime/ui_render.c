#include "packet_adventure/ui_render.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "packet_adventure/font_renderer.h"
#include "packet_adventure/render_primitives.h"
#include "packet_adventure/runtime_internal.h"

#define PA_PANEL_BG 0
#define PA_PANEL_EDGE 1
#define PA_PACKET_CORE 2
#define PA_PACKET_GLOW 3
#define PA_NODE_CORE 4
#define PA_NODE_GLOW 5
#define PA_STATUS_TEXT 6
#define PA_STATUS_ACCENT 7
#define PA_LOG_TEXT 8
#define PA_SWEEP_COLOR 9
#define PA_VOID_COLOR 10

static uint32_t g_ui_cache_revision_8888 = UINT32_MAX;
static uint32_t g_ui_cache_revision_565 = UINT32_MAX;
static uint32_t g_ui_pixels_8888[PA_SCREEN_WIDTH * PA_SCREEN_HEIGHT];
static uint16_t g_ui_pixels_565[PA_SCREEN_WIDTH * PA_SCREEN_HEIGHT];
static uint32_t g_minimap_substrate[PA_MINIMAP_LOGICAL_W * PA_MINIMAP_LOGICAL_H];
static uint16_t g_minimap_world_x[PA_MINIMAP_LOGICAL_W * PA_MINIMAP_LOGICAL_H];
static uint16_t g_minimap_world_y[PA_MINIMAP_LOGICAL_W * PA_MINIMAP_LOGICAL_H];
static uint32_t g_minimap_substrate_revision = UINT32_MAX;
static int g_minimap_center_x = INT_MIN;
static int g_minimap_center_y = INT_MIN;

static const PaOperatorType k_operator_display_order[PA_OP_COUNT] = {
    PA_OP_FLOW, PA_OP_CONNECT, PA_OP_DISSOLVE, PA_OP_ENCODE, PA_OP_CHOOSE,
    PA_OP_OBSERVE, PA_OP_CYCLE, PA_OP_LOGIC, PA_OP_RUNTIME, PA_OP_MANIFEST
};

static uint32_t pa_color_theme(int id) {
    switch (id) {
        case PA_PANEL_BG: return pa_rgb(4, 8, 11);
        case PA_PANEL_EDGE: return pa_rgb(18, 42, 48);
        case PA_PACKET_CORE: return pa_rgb(255, 255, 255);
        case PA_PACKET_GLOW: return pa_rgb(118, 230, 255);
        case PA_NODE_CORE: return pa_rgb(255, 170, 58);
        case PA_NODE_GLOW: return pa_rgb(255, 238, 176);
        case PA_STATUS_TEXT: return pa_rgb(215, 228, 230);
        case PA_STATUS_ACCENT: return pa_rgb(255, 118, 62);
        case PA_LOG_TEXT: return pa_rgb(144, 228, 255);
        case PA_SWEEP_COLOR: return pa_rgb(255, 255, 255);
        case PA_VOID_COLOR: return pa_rgb(0, 0, 0);
        default: return pa_rgb(255, 0, 255);
    }
}

static uint32_t pa_operator_rgb(unsigned int r, unsigned int g, unsigned int b) {
    return pa_rgb(b, g, r);
}

static uint32_t pa_operator_color(PaOperatorType op) {
    switch (op) {
        case PA_OP_FLOW: return pa_operator_rgb(46, 206, 255);
        case PA_OP_CONNECT: return pa_operator_rgb(255, 220, 60);
        case PA_OP_DISSOLVE: return pa_operator_rgb(190, 80, 255);
        case PA_OP_ENCODE: return pa_operator_rgb(80, 160, 255);
        case PA_OP_CHOOSE: return pa_operator_rgb(255, 58, 58);
        case PA_OP_OBSERVE: return pa_operator_rgb(240, 245, 255);
        case PA_OP_CYCLE: return pa_operator_rgb(255, 160, 40);
        case PA_OP_LOGIC: return pa_operator_rgb(80, 255, 120);
        case PA_OP_RUNTIME: return pa_operator_rgb(90, 100, 180);
        case PA_OP_MANIFEST: return pa_operator_rgb(255, 240, 200);
        default: return pa_operator_rgb(255, 255, 255);
    }
}

static uint32_t pa_dim(uint32_t color, unsigned int numerator, unsigned int denominator) {
    unsigned int r = color & 0xFFu;
    unsigned int g = (color >> 8) & 0xFFu;
    unsigned int b = (color >> 16) & 0xFFu;

    if (denominator == 0u) {
        denominator = 1u;
    }

    return pa_rgb(
        (r * numerator) / denominator,
        (g * numerator) / denominator,
        (b * numerator) / denominator
    );
}

static uint32_t pa_add_rgb(uint32_t color, int dr, int dg, int db) {
    int r = (int)(color & 0xFFu) + dr;
    int g = (int)((color >> 8) & 0xFFu) + dg;
    int b = (int)((color >> 16) & 0xFFu) + db;

    if (r < 0) {
        r = 0;
    } else if (r > 255) {
        r = 255;
    }
    if (g < 0) {
        g = 0;
    } else if (g > 255) {
        g = 255;
    }
    if (b < 0) {
        b = 0;
    } else if (b > 255) {
        b = 255;
    }
    return pa_rgb((unsigned int)r, (unsigned int)g, (unsigned int)b);
}

static PaRect pa_observe_rect(void) {
    return (PaRect){88, 20, 240, 240};
}

static PaRect pa_status_rect(void) {
    return (PaRect){356, 20, 110, 240};
}

static PaRect pa_log_rect(void) {
    return (PaRect){14, 6, 452, 10};
}

static PaRect pa_map_rect(void) {
    return (PaRect){
        (PA_SCREEN_WIDTH - PA_WORLD_COLS) / 2,
        (PA_SCREEN_HEIGHT - PA_WORLD_ROWS) / 2,
        PA_WORLD_COLS,
        PA_WORLD_ROWS
    };
}

static int pa_camera_x(const PaApp *app) {
    return pa_clamp(app->packet_x - (PA_VIEW_COLS / 2), 0, PA_WORLD_COLS - PA_VIEW_COLS);
}

static int pa_camera_y(const PaApp *app) {
    return pa_clamp(app->packet_y - (PA_VIEW_ROWS / 2), 0, PA_WORLD_ROWS - PA_VIEW_ROWS);
}

static void pa_build_ui_cache(PaSurface *surface) {
    PaRect observe = pa_observe_rect();
    PaRect status = pa_status_rect();
    PaRect log = pa_log_rect();

    pa_clear(surface, pa_color_theme(PA_VOID_COLOR));
    pa_fill_rect(surface, status, pa_color_theme(PA_PANEL_BG));
    pa_draw_line(surface, status.x - 10, status.y, status.x - 10, status.y + status.h, pa_dim(pa_color_theme(PA_PANEL_EDGE), 7u, 10u));
    pa_draw_line(surface, observe.x - 14, observe.y, observe.x - 14, observe.y + observe.h, pa_dim(pa_color_theme(PA_PANEL_EDGE), 3u, 10u));
    pa_draw_line(surface, log.x, log.y + log.h + 4, log.x + log.w, log.y + log.h + 4, pa_dim(pa_color_theme(PA_PANEL_EDGE), 4u, 10u));
}

void pa_ui_render_copy_cache(PaSurface *surface, const PaApp *app) {
    PaSurface cache_surface;

    if (surface->format == PA_PIXEL_FORMAT_8888) {
        if (g_ui_cache_revision_8888 != app->topology_revision) {
            cache_surface.pixels = g_ui_pixels_8888;
            cache_surface.width = PA_SCREEN_WIDTH;
            cache_surface.height = PA_SCREEN_HEIGHT;
            cache_surface.stride = PA_SCREEN_WIDTH;
            cache_surface.format = PA_PIXEL_FORMAT_8888;
            pa_build_ui_cache(&cache_surface);
            g_ui_cache_revision_8888 = app->topology_revision;
        }
        cache_surface.pixels = g_ui_pixels_8888;
        cache_surface.width = PA_SCREEN_WIDTH;
        cache_surface.height = PA_SCREEN_HEIGHT;
        cache_surface.stride = PA_SCREEN_WIDTH;
        cache_surface.format = PA_PIXEL_FORMAT_8888;
        pa_copy_surface(surface, &cache_surface);
    } else {
        if (g_ui_cache_revision_565 != app->topology_revision) {
            cache_surface.pixels = g_ui_pixels_565;
            cache_surface.width = PA_SCREEN_WIDTH;
            cache_surface.height = PA_SCREEN_HEIGHT;
            cache_surface.stride = PA_SCREEN_WIDTH;
            cache_surface.format = PA_PIXEL_FORMAT_565;
            pa_build_ui_cache(&cache_surface);
            g_ui_cache_revision_565 = app->topology_revision;
        }
        cache_surface.pixels = g_ui_pixels_565;
        cache_surface.width = PA_SCREEN_WIDTH;
        cache_surface.height = PA_SCREEN_HEIGHT;
        cache_surface.stride = PA_SCREEN_WIDTH;
        cache_surface.format = PA_PIXEL_FORMAT_565;
        pa_copy_surface(surface, &cache_surface);
    }
}

static void pa_draw_trigram(PaSurface *surface, int x, int y, int top, int mid, int bot, uint32_t color) {
    int row_y[3] = {y, y + 4, y + 8};
    int bits[3] = {top, mid, bot};
    int i;

    for (i = 0; i < 3; ++i) {
        if (bits[i] != 0) {
            pa_fill_rect(surface, (PaRect){x, row_y[i], 10, 1}, color);
        } else {
            pa_fill_rect(surface, (PaRect){x, row_y[i], 4, 1}, color);
            pa_fill_rect(surface, (PaRect){x + 6, row_y[i], 4, 1}, color);
        }
    }
}

static void pa_draw_operator_glyph(PaSurface *surface, int x, int y, PaOperatorType op, uint32_t color) {
    switch (op) {
        case PA_OP_FLOW:
            pa_draw_line(surface, x + 0, y + 8, x + 2, y + 6, color);
            pa_draw_line(surface, x + 2, y + 6, x + 4, y + 8, color);
            pa_draw_line(surface, x + 4, y + 8, x + 6, y + 6, color);
            pa_draw_line(surface, x + 6, y + 6, x + 8, y + 8, color);
            pa_draw_line(surface, x + 8, y + 8, x + 10, y + 6, color);
            break;
        case PA_OP_CONNECT:
            pa_draw_trigram(surface, x, y + 1, 1, 1, 1, color);
            break;
        case PA_OP_DISSOLVE:
            pa_draw_trigram(surface, x, y + 1, 0, 0, 0, color);
            break;
        case PA_OP_ENCODE:
            pa_draw_trigram(surface, x, y + 1, 0, 1, 0, color);
            break;
        case PA_OP_CHOOSE:
            pa_draw_trigram(surface, x, y + 1, 1, 0, 0, color);
            break;
        case PA_OP_OBSERVE:
            pa_draw_trigram(surface, x, y + 1, 0, 1, 1, color);
            break;
        case PA_OP_CYCLE:
            pa_draw_trigram(surface, x, y + 1, 1, 0, 1, color);
            break;
        case PA_OP_LOGIC:
            pa_draw_trigram(surface, x, y + 1, 0, 0, 1, color);
            break;
        case PA_OP_RUNTIME:
            pa_draw_trigram(surface, x, y + 1, 1, 1, 0, color);
            break;
        case PA_OP_MANIFEST:
            pa_draw_line(surface, x + 5, y + 1, x + 0, y + 10, color);
            pa_draw_line(surface, x + 5, y + 1, x + 10, y + 10, color);
            pa_draw_line(surface, x + 0, y + 10, x + 10, y + 10, color);
            break;
        default:
            pa_fill_rect(surface, (PaRect){x + 4, y + 4, 2, 2}, color);
            break;
    }
}

static bool pa_pf_contains_cell_ui(const PaApp *app, int cell_x, int cell_y) {
    int dx = pa_torus_delta(app->packet_x, cell_x, PA_WORLD_COLS);
    int dy = pa_torus_delta(app->packet_y, cell_y, PA_WORLD_ROWS);

    return pa_pf_metric(dx, dy) <= (app->packet_field_radius * 2);
}

static void pa_collect_active_operators(const PaApp *app, bool active[PA_OP_COUNT]) {
    int dx;
    int dy;

    memset(active, 0, sizeof(bool) * PA_OP_COUNT);

    for (dy = -app->packet_field_radius - 1; dy <= app->packet_field_radius + 1; ++dy) {
        for (dx = -app->packet_field_radius - 1; dx <= app->packet_field_radius + 1; ++dx) {
            int x = pa_wrap_coord(app->packet_x + dx, PA_WORLD_COLS);
            int y = pa_wrap_coord(app->packet_y + dy, PA_WORLD_ROWS);
            PaOperatorType op;

            if (!pa_pf_contains_cell_ui(app, x, y)) {
                continue;
            }
            op = (PaOperatorType)app->operators[y][x];
            active[op] = true;
        }
    }
}

static void pa_build_status_minimap_substrate(const PaApp *app) {
    int mx;
    int my;

    for (my = 0; my < PA_MINIMAP_LOGICAL_H; ++my) {
        int sample_y = ((my * PA_MINIMAP_SPAN_Y) / PA_MINIMAP_LOGICAL_H) - (PA_MINIMAP_SPAN_Y / 2);
        int world_y = pa_wrap_coord(app->packet_y + sample_y, PA_WORLD_ROWS);

        for (mx = 0; mx < PA_MINIMAP_LOGICAL_W; ++mx) {
            int sample_x = ((mx * PA_MINIMAP_SPAN_X) / PA_MINIMAP_LOGICAL_W) - (PA_MINIMAP_SPAN_X / 2);
            int world_x = pa_wrap_coord(app->packet_x + sample_x, PA_WORLD_COLS);
            int strength = pa_manifest_mf_strength_impl(app, world_x, world_y);
            uint32_t color = pa_color_theme(PA_VOID_COLOR);
            int idx = my * PA_MINIMAP_LOGICAL_W + mx;

            if (app->calm[world_y][world_x] || strength > 0) {
                color = pa_operator_color((PaOperatorType)app->operators[world_y][world_x]);
            }
            g_minimap_world_x[idx] = (uint16_t)world_x;
            g_minimap_world_y[idx] = (uint16_t)world_y;
            g_minimap_substrate[idx] = (color == pa_color_theme(PA_VOID_COLOR))
                ? color
                : pa_dim(color, 3u, 10u);
        }
    }

    g_minimap_substrate_revision = app->topology_revision;
    g_minimap_center_x = app->packet_x;
    g_minimap_center_y = app->packet_y;
}

static void pa_draw_status_minimap(PaSurface *surface, const PaApp *app, int x, int y) {
    PaRect map = {x, y, PA_MINIMAP_LOGICAL_W * PA_MINIMAP_SCALE, PA_MINIMAP_LOGICAL_H * PA_MINIMAP_SCALE};
    int sweep_world_x = (int)(((app->sweep_frame % PA_SWEEP_PERIOD) * (uint32_t)(PA_WORLD_COLS - 1)) / PA_SWEEP_PERIOD);
    int mx;
    int my;

    if (g_minimap_substrate_revision != app->topology_revision ||
        g_minimap_center_x != app->packet_x ||
        g_minimap_center_y != app->packet_y) {
        pa_build_status_minimap_substrate(app);
    }

    pa_fill_rect(surface, map, pa_color_theme(PA_VOID_COLOR));
    for (my = 0; my < PA_MINIMAP_LOGICAL_H; ++my) {
        for (mx = 0; mx < PA_MINIMAP_LOGICAL_W; ++mx) {
            int idx = my * PA_MINIMAP_LOGICAL_W + mx;
            int world_x = (int)g_minimap_world_x[idx];
            int world_y = (int)g_minimap_world_y[idx];
            uint32_t color = g_minimap_substrate[idx];
            int wave_dx = pa_torus_delta(sweep_world_x, world_x, PA_WORLD_COLS);
            int px = map.x + (mx * PA_MINIMAP_SCALE);
            int py = map.y + (my * PA_MINIMAP_SCALE);
            bool calm = app->calm[world_y][world_x];
            bool manifested = app->manifested[world_y][world_x];
            int strength = pa_manifest_mf_strength_impl(app, world_x, world_y);
            unsigned int density = app->local_density[world_y][world_x];
            uint32_t signal = pa_color_theme(PA_PANEL_EDGE);

            if (strength == 0 && !calm) {
                continue;
            }

            if (wave_dx >= 0 && wave_dx < 2) {
                signal = pa_add_rgb(color, 26, 26, 26);
                pa_fill_rect(surface, (PaRect){px + 1, py + 1, 2, 2}, signal);
            } else if (wave_dx >= 0 && wave_dx < 8) {
                signal = pa_dim(color, 7u, 10u);
                pa_fill_rect(surface, (PaRect){px + 1, py + 1, 2, 2}, signal);
            } else if (wave_dx > -24 && wave_dx < 0) {
                signal = pa_dim(color, 4u, 10u);
                pa_fill_rect(surface, (PaRect){px + 1, py + 1, 1, 1}, signal);
            }

            if (calm) {
                pa_fill_rect(surface, (PaRect){px + 2, py + 2, 1, 1}, pa_dim(pa_color_theme(PA_STATUS_TEXT), 3u, 10u));
            } else if (manifested) {
                pa_fill_rect(surface, (PaRect){px + 2, py + 2, 1, 1}, pa_dim(pa_color_theme(PA_PANEL_EDGE), 5u, 10u));
            } else if (density >= 96u) {
                pa_fill_rect(surface, (PaRect){px + 2, py + 2, 1, 1}, pa_dim(color, 6u, 10u));
            }
        }
    }

    pa_draw_rect(surface, map, pa_color_theme(PA_PANEL_EDGE));

    {
        int packet_mx = map.x + (map.w / 2);
        int packet_my = map.y + (map.h / 2);
        int target_dx = pa_torus_delta(app->packet_x, app->transition_x, PA_WORLD_COLS);
        int target_dy = pa_torus_delta(app->packet_y, app->transition_y, PA_WORLD_ROWS);

        if (target_dx >= -(PA_MINIMAP_SPAN_X / 2) && target_dx < (PA_MINIMAP_SPAN_X / 2) &&
            target_dy >= -(PA_MINIMAP_SPAN_Y / 2) && target_dy < (PA_MINIMAP_SPAN_Y / 2)) {
            int target_mx = map.x + (((target_dx + (PA_MINIMAP_SPAN_X / 2)) * map.w) / PA_MINIMAP_SPAN_X);
            int target_my = map.y + (((target_dy + (PA_MINIMAP_SPAN_Y / 2)) * map.h) / PA_MINIMAP_SPAN_Y);

            pa_fill_rect(surface, (PaRect){target_mx - 1, target_my - 1, 3, 3}, pa_color_theme(PA_NODE_GLOW));
            pa_fill_rect(surface, (PaRect){target_mx, target_my, 1, 1}, pa_color_theme(PA_NODE_CORE));
        }

        pa_fill_rect(surface, (PaRect){packet_mx - 1, packet_my - 1, 3, 3}, pa_color_theme(PA_PACKET_GLOW));
        pa_fill_rect(surface, (PaRect){packet_mx, packet_my, 1, 1}, pa_color_theme(PA_PACKET_CORE));
    }
}

void pa_ui_render_status(PaSurface *surface, const PaApp *app) {
    char line[64];
    bool active[PA_OP_COUNT];
    PaRect observe = pa_observe_rect();
    PaRect status = pa_status_rect();
    int glyph_x = 48;
    int glyph_y0 = observe.y + 8;
    int glyph_step = 22;
    int x = status.x;
    int y = status.y + 8;
    int i;

    pa_collect_active_operators(app, active);

    for (i = 0; i < PA_OP_COUNT; ++i) {
        PaOperatorType op = k_operator_display_order[i];
        uint32_t color = active[op] ? pa_operator_color(op) : pa_dim(pa_operator_color(op), 2u, 10u);

        pa_draw_operator_glyph(surface, glyph_x, glyph_y0 + (i * glyph_step), op, color);
    }

    pa_draw_status_minimap(surface, app, x + 7, status.y + 24);

    y = status.y + 126;
    snprintf(line, sizeof(line), "L%02u %03d/%03d", (unsigned int)app->layer_index, app->packet_y % 1000, app->packet_x % 1000);
    pa_draw_text_small(surface, x, y, line, pa_color_theme(PA_STATUS_TEXT));
}

void pa_ui_render_log(PaSurface *surface, const PaApp *app) {
    char line[96];
    PaRect log = pa_log_rect();
    unsigned int pulse = 3u + ((app->frame % 48u) < 24u ? (app->frame % 24u) : (24u - (app->frame % 24u)));
    uint32_t color = pa_dim(pa_add_rgb(pa_color_theme(PA_LOG_TEXT), 8, 12, 18), pulse, 26u);
    int x;

    snprintf(line, sizeof(line), "PROCESS ACTIVE");
    x = log.x + ((log.w - pa_text_small_width(line)) / 2);
    pa_draw_text_small(surface, x, log.y + 1, line, color);
}

void pa_ui_render_debug_map(PaSurface *surface, const PaApp *app) {
    PaRect map = pa_map_rect();
    int cam_x = pa_camera_x(app);
    int cam_y = pa_camera_y(app);
    int x;
    int y;

    pa_clear(surface, pa_color_theme(PA_VOID_COLOR));
    pa_draw_text_small(surface, map.x, map.y - 12, "DEBUG MAP / WHOLE WORLD", pa_color_theme(PA_STATUS_TEXT));
    pa_draw_text_small(surface, map.x + 156, map.y - 12, "TAB/SELECT", pa_color_theme(PA_STATUS_ACCENT));

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            int strength = pa_manifest_mf_strength_impl(app, x, y);
            uint32_t color = pa_color_theme(PA_VOID_COLOR);
            unsigned int density = app->local_density[y][x];

            if (app->calm[y][x] || strength > 0) {
                color = pa_operator_color((PaOperatorType)app->operators[y][x]);
            }

            if (!app->calm[y][x] && strength == 0) {
                color = pa_color_theme(PA_VOID_COLOR);
            } else if (app->tiles[y][x] == PA_TILE_VOID) {
                color = pa_dim(color, 1u, 8u);
            } else if (app->calm[y][x]) {
                color = pa_add_rgb(pa_dim(color, 7u, 10u), 10, 10, 10);
            } else if (app->manifested[y][x]) {
                color = pa_dim(color, 6u, 10u);
            } else {
                color = pa_dim(color, 3u, 10u);
            }

            if (!app->calm[y][x] && density > 0u) {
                color = pa_add_rgb(color, (int)(density / 24u), (int)(density / 28u), (int)(density / 20u));
            }

            pa_fill_rect(surface, (PaRect){map.x + x, map.y + y, 1, 1}, color);
        }
    }

    pa_draw_rect(surface, (PaRect){map.x - 1, map.y - 1, map.w + 2, map.h + 2}, pa_color_theme(PA_PANEL_EDGE));
    pa_draw_rect(
        surface,
        (PaRect){map.x + cam_x, map.y + cam_y, PA_VIEW_COLS, PA_VIEW_ROWS},
        pa_color_theme(PA_STATUS_TEXT)
    );

    pa_fill_rect(surface, (PaRect){map.x + app->transition_x - 1, map.y + app->transition_y - 1, 3, 3}, pa_color_theme(PA_NODE_GLOW));
    pa_fill_rect(surface, (PaRect){map.x + app->transition_x, map.y + app->transition_y, 1, 1}, pa_color_theme(PA_NODE_CORE));

    pa_fill_rect(surface, (PaRect){map.x + app->packet_x - 1, map.y + app->packet_y - 1, 3, 3}, pa_color_theme(PA_PACKET_GLOW));
    pa_fill_rect(surface, (PaRect){map.x + app->packet_x, map.y + app->packet_y, 1, 1}, pa_color_theme(PA_PACKET_CORE));
}
