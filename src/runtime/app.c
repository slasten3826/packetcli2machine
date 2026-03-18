#include "packet_adventure/app.h"

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "packet_adventure/font_renderer.h"
#include "packet_adventure/render_primitives.h"
#include "packet_adventure/runtime_internal.h"
#include "packet_adventure/ui_render.h"

#ifndef PA_BUILD_ID
#define PA_BUILD_ID "dev"
#endif

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

static int pa_mf_strength(const PaApp *app, int x, int y);
static void pa_generate_world(PaApp *app);
static void pa_advance_layer(PaApp *app);
static void pa_try_move(PaApp *app, int dx, int dy);
static void pa_adjust_packet_field(PaApp *app, int delta);
static bool pa_pf_contains_cell(const PaApp *app, int cell_x, int cell_y);
static void pa_update_packet_field(PaApp *app);

static const PaOperatorProfile k_operator_profiles[PA_OP_COUNT] = {
    {1u, 1u, -1,  1}, /* FLOW */
    {1u, 1u,  1,  0}, /* CONNECT */
    {1u, 1u, -2,  2}, /* DISSOLVE */
    {1u, 1u,  2, -1}, /* ENCODE */
    {1u, 1u,  0,  2}, /* CHOOSE */
    {1u, 1u,  0, -1}, /* OBSERVE */
    {1u, 1u,  1,  1}, /* CYCLE */
    {1u, 1u,  2,  0}, /* LOGIC */
    {1u, 1u,  2,  1}, /* RUNTIME */
    {1u, 1u,  1,  0}, /* MANIFEST */
};

static int pa_sweep_last_column(uint32_t phase) {
    return (int)(((((uint32_t)phase + 1u) * (uint32_t)PA_WORLD_COLS) - 1u) / PA_SWEEP_PERIOD);
}

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

/* The current renderer/theme stack was tuned under a red/blue-swapped display
   interpretation. Operator colors are canon-facing and must appear on-screen
   exactly as specified in operator_color_canon.md, so encode them through this
   helper instead of changing the whole project's palette model at once. */
static uint32_t pa_operator_rgb(unsigned int r, unsigned int g, unsigned int b) {
    return pa_rgb(b, g, r);
}

static uint32_t pa_operator_color(PaOperatorType op) {
    switch (op) {
        /* Canonical colors from claude/operator_color_canon.md */
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

uint32_t pa_lcg_next(uint32_t *state) {
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

int pa_rand_range(uint32_t *state, int min_value, int max_value) {
    uint32_t span = (uint32_t)(max_value - min_value + 1);
    return min_value + (int)(pa_lcg_next(state) % span);
}

int pa_clamp(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

int pa_wrap_coord(int value, int size) {
    if (size <= 0) {
        return 0;
    }
    value %= size;
    if (value < 0) {
        value += size;
    }
    return value;
}

int pa_torus_delta(int from, int to, int size) {
    int delta = to - from;
    int alt;

    if (delta > (size / 2)) {
        delta -= size;
    } else if (delta < -(size / 2)) {
        delta += size;
    }

    alt = delta > 0 ? delta - size : delta + size;
    if ((alt < 0 ? -alt : alt) < (delta < 0 ? -delta : delta)) {
        delta = alt;
    }

    return delta;
}

int pa_torus_lerp(int from, int to, int step, int steps, int size) {
    int delta = pa_torus_delta(from, to, size);
    return pa_wrap_coord(from + ((delta * step) / steps), size);
}

uint32_t pa_chaos_seed_from_calm(const PaApp *app) {
    uint32_t signature = 2166136261u;
    int x;
    int y;

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            if (!app->calm[y][x]) {
                continue;
            }
            signature ^= (uint32_t)(x * 3 + y * 5 + ((int)app->operators[y][x] * 17));
            signature *= 16777619u;
        }
    }

    return signature;
}

const PaOperatorProfile *pa_operator_profile(PaOperatorType op) {
    static const PaOperatorProfile k_fallback = {0u, 0u, 0, 0};

    if (op < 0 || op >= PA_OP_COUNT) {
        return &k_fallback;
    }
    return &k_operator_profiles[op];
}

bool pa_operator_world_enabled(PaOperatorType op) {
    return pa_operator_profile(op)->world_strength > 0u;
}

bool pa_operator_process_enabled(PaOperatorType op) {
    return pa_operator_profile(op)->process_strength > 0u;
}

bool pa_operator_allowed(PaOperatorType a, PaOperatorType b) {
    static const uint16_t k_mask[PA_OP_COUNT] = {
        (1u << PA_OP_CONNECT) | (1u << PA_OP_DISSOLVE),
        (1u << PA_OP_FLOW) | (1u << PA_OP_ENCODE) | (1u << PA_OP_CHOOSE),
        (1u << PA_OP_FLOW) | (1u << PA_OP_ENCODE),
        (1u << PA_OP_CONNECT) | (1u << PA_OP_DISSOLVE) | (1u << PA_OP_CHOOSE) | (1u << PA_OP_OBSERVE) | (1u << PA_OP_CYCLE),
        (1u << PA_OP_CONNECT) | (1u << PA_OP_ENCODE) | (1u << PA_OP_OBSERVE) | (1u << PA_OP_LOGIC),
        (1u << PA_OP_ENCODE) | (1u << PA_OP_CHOOSE) | (1u << PA_OP_CYCLE) | (1u << PA_OP_LOGIC) | (1u << PA_OP_RUNTIME),
        (1u << PA_OP_ENCODE) | (1u << PA_OP_OBSERVE) | (1u << PA_OP_LOGIC) | (1u << PA_OP_RUNTIME),
        (1u << PA_OP_CHOOSE) | (1u << PA_OP_OBSERVE) | (1u << PA_OP_CYCLE) | (1u << PA_OP_RUNTIME) | (1u << PA_OP_MANIFEST),
        (1u << PA_OP_OBSERVE) | (1u << PA_OP_CYCLE) | (1u << PA_OP_LOGIC) | (1u << PA_OP_MANIFEST),
        (1u << PA_OP_LOGIC) | (1u << PA_OP_RUNTIME),
    };

    if (a < 0 || a >= PA_OP_COUNT || b < 0 || b >= PA_OP_COUNT) {
        return false;
    }
    return (k_mask[a] & (1u << b)) != 0u;
}

PaZoneType pa_operator_zone(PaOperatorType op) {
    switch (op) {
        case PA_OP_CONNECT:
        case PA_OP_OBSERVE:
        case PA_OP_RUNTIME:
            return PA_ZONE_CALM;
        case PA_OP_ENCODE:
        case PA_OP_CHOOSE:
        case PA_OP_CYCLE:
        case PA_OP_LOGIC:
            return PA_ZONE_BOUNDARY;
        case PA_OP_FLOW:
        case PA_OP_DISSOLVE:
        case PA_OP_MANIFEST:
        default:
            return PA_ZONE_CHAOS;
    }
}

void pa_select_path(PaOperatorType *ops, uint32_t *seed) {
    PaOperatorType late_ops[3] = {PA_OP_CYCLE, PA_OP_LOGIC, PA_OP_RUNTIME};
    PaOperatorType branch_ops[3] = {PA_OP_DISSOLVE, PA_OP_CHOOSE, PA_OP_RUNTIME};
    PaOperatorType fallback = PA_OP_FLOW;
    int i;

    for (i = 0; i < PA_OP_COUNT; ++i) {
        if (pa_operator_world_enabled((PaOperatorType)i)) {
            fallback = (PaOperatorType)i;
            break;
        }
    }

    ops[0] = pa_operator_world_enabled(PA_OP_FLOW) ? PA_OP_FLOW : fallback;
    ops[1] = pa_operator_world_enabled(PA_OP_CONNECT) ? PA_OP_CONNECT : ops[0];
    ops[2] = pa_operator_world_enabled(PA_OP_ENCODE) ? PA_OP_ENCODE : ops[1];
    ops[3] = pa_operator_world_enabled(PA_OP_OBSERVE) ? PA_OP_OBSERVE : ops[2];
    ops[4] = fallback;
    ops[5] = pa_operator_world_enabled(PA_OP_MANIFEST) ? PA_OP_MANIFEST : ops[3];
    ops[6] = fallback;

    for (i = 0; i < 8; ++i) {
        PaOperatorType candidate = late_ops[pa_rand_range(seed, 0, 2)];

        if (pa_operator_world_enabled(candidate)) {
            ops[4] = candidate;
            break;
        }
    }

    for (i = 0; i < 8; ++i) {
        PaOperatorType candidate = branch_ops[pa_rand_range(seed, 0, 2)];

        if (pa_operator_world_enabled(candidate)) {
            ops[6] = candidate;
            break;
        }
    }

    if (ops[4] == PA_OP_RUNTIME && ops[6] == PA_OP_RUNTIME) {
        ops[6] = pa_operator_world_enabled(PA_OP_CHOOSE) ? PA_OP_CHOOSE : ops[6];
    }
}

unsigned int pa_cell_noise(int x, int y, int salt) {
    uint32_t v = (uint32_t)(x * 1103515245u) ^ (uint32_t)(y * 12345u) ^ (uint32_t)(salt * 2654435761u);
    v ^= v >> 13;
    v *= 1274126177u;
    v ^= v >> 16;
    return v & 255u;
}

PaOperatorType pa_find_bridge_operator(PaOperatorType a, PaOperatorType b) {
    static const PaOperatorType k_preferred[] = {
        /* Keep OBSERVE away from the front: it is too globally compatible
         * and otherwise becomes the default repair operator everywhere. */
        PA_OP_ENCODE, PA_OP_CYCLE, PA_OP_CONNECT, PA_OP_RUNTIME, PA_OP_LOGIC,
        PA_OP_OBSERVE, PA_OP_CHOOSE, PA_OP_DISSOLVE, PA_OP_FLOW, PA_OP_MANIFEST
    };
    unsigned int i;

    if (pa_operator_allowed(a, b)) {
        return b;
    }

    for (i = 0; i < sizeof(k_preferred) / sizeof(k_preferred[0]); ++i) {
        PaOperatorType c = k_preferred[i];

        if (pa_operator_allowed(a, c) && pa_operator_allowed(c, b)) {
            return c;
        }
    }

    return a;
}

void pa_add_region_seeds(PaZoneCenter *centers, int *count, const PaRegionPlan *plan, uint32_t *seed) {
    int i;
    const PaOperatorProfile *profile = pa_operator_profile(plan->op);

    if (profile->world_strength == 0u) {
        return;
    }

    for (i = 0; i < plan->copies; ++i) {
        int dx = 0;
        int dy = 0;

        if (plan->spread_x > 0) {
            dx = pa_rand_range(seed, -plan->spread_x, plan->spread_x);
        }
        if (plan->spread_y > 0) {
            dy = pa_rand_range(seed, -plan->spread_y, plan->spread_y);
        }

        centers[*count].op = plan->op;
        centers[*count].x = pa_clamp(plan->anchor_x + dx, 1, PA_WORLD_COLS - 2);
        centers[*count].y = pa_clamp(plan->anchor_y + dy, 1, PA_WORLD_ROWS - 2);
        *count += 1;
    }
}

static PaRect pa_observe_rect(void) {
    return (PaRect){88, 20, 240, 240};
}

static int pa_cell_w(void) {
    return 16;
}

static int pa_cell_h(void) {
    return 16;
}

static void pa_view_to_screen(int view_x, int view_y, int *screen_x, int *screen_y) {
    PaRect observe = pa_observe_rect();
    int inset_x = pa_cell_w() / 2;
    int inset_y = pa_cell_h() / 2;

    *screen_x = observe.x + inset_x + (view_x * pa_cell_w()) + (pa_cell_w() / 2);
    *screen_y = observe.y + inset_y + (view_y * pa_cell_h()) + (pa_cell_h() / 2);
}

static int pa_camera_x(const PaApp *app) {
    return pa_wrap_coord(app->packet_x - (PA_VIEW_COLS / 2), PA_WORLD_COLS);
}

static int pa_camera_y(const PaApp *app) {
    return pa_wrap_coord(app->packet_y - (PA_VIEW_ROWS / 2), PA_WORLD_ROWS);
}

static bool pa_world_to_view(int cam, int world, int size, int span, int *view_out) {
    int delta = pa_torus_delta(cam, world, size);

    if (delta < 0 || delta >= span) {
        return false;
    }
    *view_out = delta;
    return true;
}

static int pa_mf_strength(const PaApp *app, int x, int y) {
    return pa_manifest_mf_strength_impl(app, x, y);
}


static void pa_generate_world(PaApp *app) {
    pa_world_generate_impl(app);
}

static void pa_advance_layer(PaApp *app) {
    pa_world_advance_layer_impl(app);
}

static void pa_try_move(PaApp *app, int dx, int dy) {
    pa_packet_try_move_impl(app, dx, dy);
}

static void pa_adjust_packet_field(PaApp *app, int delta) {
    pa_packet_adjust_field_impl(app, delta);
}

int pa_pf_metric(int dx, int dy) {
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    int major = adx > ady ? adx : ady;
    int minor = adx > ady ? ady : adx;
    return (major * 2) + minor;
}

static bool pa_pf_contains_cell(const PaApp *app, int cell_x, int cell_y) {
    return pa_packet_pf_contains_cell_impl(app, cell_x, cell_y);
}

static void pa_update_packet_field(PaApp *app) {
    pa_packet_update_field_impl(app);
}

static void pa_draw_visible_world(PaSurface *surface, const PaApp *app) {
    int cam_x = pa_camera_x(app);
    int cam_y = pa_camera_y(app);
    uint32_t sweep = app->sweep_frame % PA_SWEEP_PERIOD;
    int sweep_world_x = (int)((sweep * (uint32_t)(PA_WORLD_COLS - 1)) / PA_SWEEP_PERIOD);
    uint32_t pw_phase = app->frame % PA_PW_PERIOD;
    int pw_front = (int)((pw_phase * (uint32_t)(app->packet_field_radius * 2 + 1)) / PA_PW_PERIOD);
    int pw_pulse = (int)(pw_phase < (PA_PW_PERIOD / 2u) ? pw_phase : (PA_PW_PERIOD - pw_phase));
    int vy;
    int vx;

    for (vy = 0; vy < PA_VIEW_ROWS; ++vy) {
        for (vx = 0; vx < PA_VIEW_COLS; ++vx) {
            int wx = pa_wrap_coord(cam_x + vx, PA_WORLD_COLS);
            int wy = pa_wrap_coord(cam_y + vy, PA_WORLD_ROWS);
            int sx;
            int sy;
            PaOperatorType op;
            uint32_t base;
            uint32_t center_base;
            uint32_t density_signal;
            uint32_t density_glow;
            int wave_dx;
            int pf_ring;
            int density;
            int mf_strength;
            bool in_pf;
            bool sw_front;
            bool sw_tail;
            bool pw_live;
            bool pw_front_cell;

            pa_view_to_screen(vx, vy, &sx, &sy);

            if (app->tiles[wy][wx] == PA_TILE_WALL) {
                pa_draw_line(surface, sx - 7, sy - 7, sx + 7, sy - 7, pa_color_theme(PA_PANEL_EDGE));
                pa_draw_line(surface, sx + 7, sy - 7, sx + 7, sy + 7, pa_color_theme(PA_PANEL_EDGE));
                pa_draw_line(surface, sx + 7, sy + 7, sx - 7, sy + 7, pa_color_theme(PA_PANEL_EDGE));
                pa_draw_line(surface, sx - 7, sy + 7, sx - 7, sy - 7, pa_color_theme(PA_PANEL_EDGE));
                continue;
            }

            op = (PaOperatorType)app->operators[wy][wx];
            base = pa_operator_color(op);
            center_base = pa_dim(base, 2u, 10u);
            density = (int)app->local_density[wy][wx];
            density_signal = pa_dim(pa_add_rgb(base, 6, 8, 10), (unsigned int)(2 + (density / 32)), 18u);
            density_glow = pa_dim(pa_add_rgb(base, 18, 22, 26), (unsigned int)(2 + (density / 20)), 20u);
            wave_dx = wx - sweep_world_x;
            in_pf = pa_pf_contains_cell(app, wx, wy);
            pf_ring = pa_pf_metric(wx - app->packet_x, wy - app->packet_y) / 2;
            mf_strength = pa_mf_strength(app, wx, wy);
            sw_front = wave_dx <= 0 && wave_dx > -2;
            sw_tail = wave_dx <= 0 && wave_dx > -8;
            pw_live = in_pf && app->calm[wy][wx];
            pw_front_cell = pw_live && pf_ring == pw_front;

            if (!app->calm[wy][wx]) {
                if (mf_strength == 0) {
                    pa_fill_rect(surface, (PaRect){sx, sy, 1, 1}, pa_color_theme(PA_VOID_COLOR));
                    continue;
                } else if (mf_strength == 1) {
                    center_base = pa_dim(center_base, 2u, 3u);
                    density_signal = pa_dim(density_signal, 1u, 2u);
                    density_glow = pa_dim(density_glow, 1u, 2u);
                }
            }

            pa_fill_rect(surface, (PaRect){sx, sy, 1, 1}, pa_color_theme(PA_VOID_COLOR));

            if (app->calm[wy][wx]) {
                uint32_t calm_signal = pa_dim(pa_add_rgb(base, 12, 12, 10), 5u, 10u);

                if (sw_front) {
                    pa_fill_rect(surface, (PaRect){sx - 1, sy - 1, 3, 3}, pa_dim(calm_signal, 8u, 10u));
                } else if (sw_tail) {
                    pa_fill_rect(surface, (PaRect){sx, sy, 2, 2}, pa_dim(calm_signal, 6u, 10u));
                } else {
                    pa_fill_rect(surface, (PaRect){sx, sy, 2, 2}, pa_dim(calm_signal, 4u, 10u));
                }

                if (pw_live) {
                    unsigned int pulse_strength = 2u + (unsigned int)(pw_pulse / 24);
                    uint32_t pulse_color = pa_dim(pa_add_rgb(pa_color_theme(PA_PACKET_GLOW), 10, 6, 0), pulse_strength, 12u);

                    if (pw_front_cell) {
                        pa_fill_rect(surface, (PaRect){sx - 1, sy - 1, 3, 3}, pa_dim(pa_add_rgb(pa_color_theme(PA_PACKET_GLOW), 26, 14, 0), 9u, 12u));
                    } else if (pf_ring < pw_front) {
                        pa_fill_rect(surface, (PaRect){sx, sy, 1, 1}, pulse_color);
                    }
                }
            } else if (app->manifested[wy][wx] && mf_strength >= 1) {
                if (mf_strength >= 3) {
                    pa_fill_rect(surface, (PaRect){sx, sy, 2, 2}, pa_dim(pa_add_rgb(center_base, -2, 4, 10), 7u, 10u));
                    if (density >= 96) {
                        pa_fill_rect(surface, (PaRect){sx + 1, sy + 1, 1, 1}, density_signal);
                    }
                } else if (mf_strength == 2) {
                    pa_fill_rect(surface, (PaRect){sx, sy, 1, 1}, pa_dim(pa_add_rgb(center_base, -4, 2, 8), 6u, 10u));
                    if (density >= 112) {
                        pa_fill_rect(surface, (PaRect){sx + 1, sy + 1, 1, 1}, density_signal);
                    }
                } else {
                    pa_fill_rect(surface, (PaRect){sx, sy, 1, 1}, pa_dim(center_base, 3u, 4u));
                }
            } else if (sw_front && mf_strength >= 1) {
                pa_fill_rect(surface, (PaRect){sx - 1, sy - 1, 3, 3}, pa_dim(pa_add_rgb(base, 10, 12, 12), 6u, 10u));
                if (density >= 56) {
                    pa_fill_rect(surface, (PaRect){sx, sy, 1, 1}, density_glow);
                }
            } else if (sw_tail && mf_strength >= 1) {
                pa_fill_rect(surface, (PaRect){sx, sy, 2, 2}, pa_dim(pa_add_rgb(base, 4, 6, 10), 4u, 10u));
                if (density >= 96) {
                    pa_fill_rect(surface, (PaRect){sx + 1, sy + 1, 1, 1}, density_signal);
                }
            } else if (mf_strength >= 2 && density >= 120) {
                pa_fill_rect(surface, (PaRect){sx, sy, 1, 1}, density_signal);
            } else {
                pa_fill_rect(surface, (PaRect){sx, sy, 1, 1}, center_base);
            }
        }
    }
}

static void pa_draw_entities(PaSurface *surface, const PaApp *app) {
    int cam_x = pa_camera_x(app);
    int cam_y = pa_camera_y(app);
    int pulse = (int)(app->frame % 24u);
    int glow = pulse < 12 ? pulse : (24 - pulse);
    {
        int view_x;
        int view_y;

        if (pa_world_to_view(cam_x, app->transition_x, PA_WORLD_COLS, PA_VIEW_COLS, &view_x) &&
            pa_world_to_view(cam_y, app->transition_y, PA_WORLD_ROWS, PA_VIEW_ROWS, &view_y)) {
            int sx;
            int sy;

            pa_view_to_screen(view_x, view_y, &sx, &sy);
            pa_fill_rect(surface, (PaRect){sx - 3, sy - 3, 7, 7}, pa_dim(pa_color_theme(PA_NODE_GLOW), (unsigned int)(4 + glow), 16u));
            pa_fill_rect(surface, (PaRect){sx - 1, sy - 1, 3, 3}, pa_color_theme(PA_NODE_CORE));
        }
    }

    {
        int view_x;
        int view_y;
        int sx;
        int sy;

        if (!pa_world_to_view(cam_x, app->packet_x, PA_WORLD_COLS, PA_VIEW_COLS, &view_x) ||
            !pa_world_to_view(cam_y, app->packet_y, PA_WORLD_ROWS, PA_VIEW_ROWS, &view_y)) {
            return;
        }

        pa_view_to_screen(view_x, view_y, &sx, &sy);
        pa_fill_rect(surface, (PaRect){sx - 3, sy - 3, 7, 7}, pa_dim(pa_color_theme(PA_NODE_GLOW), (unsigned int)(4 + glow), 16u));
        pa_fill_rect(surface, (PaRect){sx - 3, sy - 3, 7, 7}, pa_dim(pa_color_theme(PA_PACKET_GLOW), (unsigned int)(5 + glow), 16u));
        pa_fill_rect(surface, (PaRect){sx - 1, sy - 1, 3, 3}, pa_color_theme(PA_PACKET_CORE));
    }
}

void pa_app_init(PaApp *app) {
    memset(app, 0, sizeof(*app));
    app->packet_field_radius = 2;
    app->world_seed = 0x2A6D365Bu;
    pa_generate_world(app);
    pa_world_regen_begin_cycle_impl(app);
}

void pa_app_set_input(PaApp *app, uint32_t input_mask) {
    app->input_mask = input_mask;
}

void pa_app_update(PaApp *app) {
    uint32_t pressed = app->input_mask & ~app->prev_input_mask;
    uint32_t prev_sweep_frame = app->sweep_frame;

    if ((pressed & PA_INPUT_MAP_TOGGLE) != 0u) {
        app->map_open = !app->map_open;
    }
    if ((pressed & PA_INPUT_REGEN) != 0u) {
        app->world_seed = (app->world_seed * 1664525u) + 1013904223u + (app->frame | 1u);
        pa_generate_world(app);
    }
    if ((pressed & PA_INPUT_SW_TOGGLE) != 0u) {
        app->sweep_paused = !app->sweep_paused;
    }
    if ((pressed & PA_INPUT_SW_SLOWER) != 0u) {
        if (app->sweep_speed > 1u) {
            app->sweep_speed -= 1u;
        }
    }
    if ((pressed & PA_INPUT_SW_FASTER) != 0u) {
        if (app->sweep_speed < 8u) {
            app->sweep_speed += 1u;
        }
    }
    app->prev_input_mask = app->input_mask;

    app->frame += 1u;

    {
        if (!app->sweep_paused) {
            app->sweep_frame += app->sweep_speed;
        } else if ((pressed & PA_INPUT_SW_STEP) != 0u) {
            app->sweep_frame += 1u;
        }
    }

    {
        uint32_t prev_cycle = prev_sweep_frame / PA_SWEEP_PERIOD;
        uint32_t sweep_cycle = app->sweep_frame / PA_SWEEP_PERIOD;
        uint32_t prev_phase = prev_sweep_frame % PA_SWEEP_PERIOD;
        uint32_t phase = app->sweep_frame % PA_SWEEP_PERIOD;
        int start_col;
        int end_col;
        int col;

        if (prev_cycle != app->sw_cycle_count) {
            pa_world_regen_begin_cycle_impl(app);
            app->sw_cycle_count = prev_cycle;
        }

        if (sweep_cycle != prev_cycle) {
            start_col = pa_sweep_last_column(prev_phase) + 1;
            for (col = start_col; col < PA_WORLD_COLS; ++col) {
                pa_world_regen_column_impl(app, col);
            }
            pa_world_regen_end_cycle_impl(app);
            app->sw_cycle_count = sweep_cycle;
            pa_world_regen_begin_cycle_impl(app);
            prev_phase = 0u;
        }

        start_col = pa_sweep_last_column(prev_phase) + 1;
        end_col = pa_sweep_last_column(phase);
        if (prev_sweep_frame == 0u && app->sweep_frame == 0u) {
            start_col = 0;
            end_col = 0;
        }
        for (col = start_col; col <= end_col; ++col) {
            pa_world_regen_column_impl(app, col);
        }
    }

    if (app->map_open) {
        return;
    }

    if (app->move_cooldown > 0u) {
        app->move_cooldown -= 1u;
    }
    if (app->param_cooldown > 0u) {
        app->param_cooldown -= 1u;
    }

    if (app->param_cooldown == 0u) {
        if ((app->input_mask & PA_INPUT_PARAM_DEC) != 0u) {
            pa_adjust_packet_field(app, -1);
            app->param_cooldown = PA_PARAM_COOLDOWN;
        } else if ((app->input_mask & PA_INPUT_PARAM_INC) != 0u) {
            pa_adjust_packet_field(app, 1);
            app->param_cooldown = PA_PARAM_COOLDOWN;
        }
    }

    if (app->transition_reached) {
        pa_advance_layer(app);
        return;
    }

    pa_update_packet_field(app);

    if (app->move_cooldown == 0u) {
        if ((app->input_mask & PA_INPUT_UP) != 0u) {
            pa_try_move(app, 0, -1);
            app->move_cooldown = PA_MOVE_COOLDOWN;
        } else if ((app->input_mask & PA_INPUT_DOWN) != 0u) {
            pa_try_move(app, 0, 1);
            app->move_cooldown = PA_MOVE_COOLDOWN;
        } else if ((app->input_mask & PA_INPUT_LEFT) != 0u) {
            pa_try_move(app, -1, 0);
            app->move_cooldown = PA_MOVE_COOLDOWN;
        } else if ((app->input_mask & PA_INPUT_RIGHT) != 0u) {
            pa_try_move(app, 1, 0);
            app->move_cooldown = PA_MOVE_COOLDOWN;
        }
    }

    pa_update_packet_field(app);
}

void pa_app_render(const PaApp *app, PaSurface *surface) {
    if (app->map_open) {
        pa_ui_render_debug_map(surface, app);
        return;
    }

    pa_ui_render_copy_cache(surface, app);
    pa_draw_visible_world(surface, app);
    pa_draw_entities(surface, app);
    pa_ui_render_status(surface, app);
    pa_ui_render_log(surface, app);
}

const char *pa_app_build_id(void) {
    return PA_BUILD_ID;
}
