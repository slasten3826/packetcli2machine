#include <stdint.h>

#include "packet_adventure/runtime_internal.h"

static const uint8_t k_operator_rates[PA_OP_COUNT] = {
    2, 1, 4, 3, 3, 0, 2, 1, 1, 5
};

static void pa_manifest_local_bounds(const PaApp *app, int *min_x, int *max_x, int *min_y, int *max_y) {
    int span = app->packet_field_radius + 16;

    *min_x = pa_clamp(app->packet_x - span, 0, PA_WORLD_COLS - 1);
    *max_x = pa_clamp(app->packet_x + span, 0, PA_WORLD_COLS - 1);
    *min_y = pa_clamp(app->packet_y - span, 0, PA_WORLD_ROWS - 1);
    *max_y = pa_clamp(app->packet_y + span, 0, PA_WORLD_ROWS - 1);
}

int pa_manifest_mf_strength_impl(const PaApp *app, int x, int y) {
    int dx = pa_torus_delta(app->packet_x, x, PA_WORLD_COLS);
    int dy = pa_torus_delta(app->packet_y, y, PA_WORLD_ROWS);
    int metric = pa_pf_metric(dx, dy);
    int core_threshold = (app->packet_field_radius * 2) + 6;
    int full_threshold = core_threshold + 8;
    int fringe_threshold = full_threshold + 10;

    if (metric <= core_threshold) {
        return 3;
    }
    if (metric <= full_threshold) {
        return 2;
    }
    if (metric <= fringe_threshold) {
        return 1;
    }
    return 0;
}

bool pa_manifest_cell_is_turbulence_impl(const PaApp *app, int x, int y) {
    return !app->calm[y][x] &&
           app->manifested[y][x] &&
           app->local_density[y][x] >= PA_TURBULENCE_THRESHOLD;
}

void pa_manifest_refresh_density_impl(PaApp *app) {
    int x;
    int y;

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            unsigned int counts[PA_OP_COUNT] = {0};
            PaOperatorType op;
            unsigned int same_count = 0;
            unsigned int distinct = 0;
            unsigned int calm_touch = 0;
            unsigned int incompatible = 0;
            unsigned int density = 0;
            int manifest_strength = pa_manifest_mf_strength_impl(app, x, y);
            int nx;
            int ny;

            if (app->calm[y][x] || manifest_strength == 0) {
                app->local_density[y][x] = 0u;
                continue;
            }
            if (!app->manifested[y][x]) {
                pa_world_manifest_cell_impl(app, x, y);
            }
            op = (PaOperatorType)app->operators[y][x];

            for (ny = -2; ny <= 2; ++ny) {
                for (nx = -2; nx <= 2; ++nx) {
                    int sample_y = pa_wrap_coord(y + ny, PA_WORLD_ROWS);
                    int sample_x = pa_wrap_coord(x + nx, PA_WORLD_COLS);
                    PaOperatorType near = (PaOperatorType)app->operators[sample_y][sample_x];

                    counts[near] += 1u;
                    if (near == op) {
                        same_count += 1u;
                    }
                    if (!(nx == 0 && ny == 0) && app->calm[sample_y][sample_x]) {
                        calm_touch += 1u;
                    }
                    if (!pa_operator_allowed(op, near) && near != op) {
                        incompatible += 1u;
                    }
                }
            }

            for (nx = 0; nx < PA_OP_COUNT; ++nx) {
                if (counts[nx] > 0u) {
                    distinct += 1u;
                }
            }

            if (distinct > 1u) {
                density += (distinct - 1u) * 20u;
            }
            if (same_count < 16u) {
                density += (16u - same_count) * 4u;
            }
            density += incompatible * 6u;
            density += calm_touch * 3u;
            density = (density * (unsigned int)manifest_strength) / 3u;

            if (density > 255u) {
                density = 255u;
            }
            app->local_density[y][x] = (uint8_t)density;
        }
    }
}

void pa_manifest_refresh_density_local_impl(PaApp *app) {
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int x;
    int y;

    pa_manifest_local_bounds(app, &min_x, &max_x, &min_y, &max_y);

    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            unsigned int counts[PA_OP_COUNT] = {0};
            PaOperatorType op;
            unsigned int same_count = 0;
            unsigned int distinct = 0;
            unsigned int calm_touch = 0;
            unsigned int incompatible = 0;
            unsigned int density = 0;
            int manifest_strength = pa_manifest_mf_strength_impl(app, x, y);
            int nx;
            int ny;

            if (app->calm[y][x] || manifest_strength == 0) {
                app->local_density[y][x] = 0u;
                continue;
            }
            if (!app->manifested[y][x]) {
                pa_world_manifest_cell_impl(app, x, y);
            }
            op = (PaOperatorType)app->operators[y][x];

            for (ny = -2; ny <= 2; ++ny) {
                for (nx = -2; nx <= 2; ++nx) {
                    int sample_y = pa_wrap_coord(y + ny, PA_WORLD_ROWS);
                    int sample_x = pa_wrap_coord(x + nx, PA_WORLD_COLS);
                    PaOperatorType near = (PaOperatorType)app->operators[sample_y][sample_x];

                    counts[near] += 1u;
                    if (near == op) {
                        same_count += 1u;
                    }
                    if (!(nx == 0 && ny == 0) && app->calm[sample_y][sample_x]) {
                        calm_touch += 1u;
                    }
                    if (!pa_operator_allowed(op, near) && near != op) {
                        incompatible += 1u;
                    }
                }
            }

            for (nx = 0; nx < PA_OP_COUNT; ++nx) {
                if (counts[nx] > 0u) {
                    distinct += 1u;
                }
            }

            if (distinct > 1u) {
                density += (distinct - 1u) * 20u;
            }
            if (same_count < 16u) {
                density += (16u - same_count) * 4u;
            }
            density += incompatible * 6u;
            density += calm_touch * 3u;
            density = (density * (unsigned int)manifest_strength) / 3u;

            if (density > 255u) {
                density = 255u;
            }
            app->local_density[y][x] = (uint8_t)density;
        }
    }
}

void pa_manifest_refresh_world_fields_impl(PaApp *app) {
    int x;
    int y;

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            PaOperatorType op;
            int manifest_strength = pa_manifest_mf_strength_impl(app, x, y);

            if (!app->calm[y][x] &&
                manifest_strength == 0 &&
                !(x == app->packet_x && y == app->packet_y) &&
                !(x == app->transition_x && y == app->transition_y)) {
                continue;
            }
            if (!app->calm[y][x] && manifest_strength > 0 && !app->manifested[y][x]) {
                pa_world_manifest_cell_impl(app, x, y);
            }

            op = (PaOperatorType)app->operators[y][x];
            app->zones[y][x] = (uint8_t)pa_operator_zone(op);
            app->local_entropy_rate[y][x] = k_operator_rates[op];
        }
    }

    pa_manifest_refresh_density_impl(app);
}

void pa_manifest_refresh_world_fields_local_impl(PaApp *app) {
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int x;
    int y;

    pa_manifest_local_bounds(app, &min_x, &max_x, &min_y, &max_y);

    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            PaOperatorType op;
            int manifest_strength = pa_manifest_mf_strength_impl(app, x, y);

            if (!app->calm[y][x] &&
                manifest_strength == 0 &&
                !(x == app->packet_x && y == app->packet_y) &&
                !(x == app->transition_x && y == app->transition_y)) {
                continue;
            }
            if (!app->calm[y][x] && manifest_strength > 0 && !app->manifested[y][x]) {
                pa_world_manifest_cell_impl(app, x, y);
            }

            op = (PaOperatorType)app->operators[y][x];
            app->zones[y][x] = (uint8_t)pa_operator_zone(op);
            app->local_entropy_rate[y][x] = k_operator_rates[op];
        }
    }

    pa_manifest_refresh_density_local_impl(app);
}
