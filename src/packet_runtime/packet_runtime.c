#include <stdint.h>

#include "packet_adventure/runtime_internal.h"

bool pa_packet_pf_contains_cell_impl(const PaApp *app, int cell_x, int cell_y) {
    int dx = pa_torus_delta(app->packet_x, cell_x, PA_WORLD_COLS);
    int dy = pa_torus_delta(app->packet_y, cell_y, PA_WORLD_ROWS);
    int threshold = app->packet_field_radius * 2;

    return pa_pf_metric(dx, dy) <= threshold;
}

void pa_packet_try_move_impl(PaApp *app, int dx, int dy) {
    int next_x = pa_wrap_coord(app->packet_x + dx, PA_WORLD_COLS);
    int next_y = pa_wrap_coord(app->packet_y + dy, PA_WORLD_ROWS);
    uint32_t pf_cost_scale;

    if (app->tiles[next_y][next_x] == PA_TILE_WALL) {
        return;
    }

    app->packet_x = next_x;
    app->packet_y = next_y;
    pf_cost_scale = 4u + (uint32_t)app->packet_field_radius;
    app->entropy += (app->local_entropy_rate[next_y][next_x] * pf_cost_scale) / 4u;
    app->manifested[next_y][next_x] = true;
    if (app->packet_x == app->transition_x && app->packet_y == app->transition_y) {
        app->transition_reached = true;
    }
}

void pa_packet_adjust_field_impl(PaApp *app, int delta) {
    int next = app->packet_field_radius + delta;

    if (next < 1) {
        next = 1;
    }
    if (next > 8) {
        next = 8;
    }
    app->packet_field_radius = next;
}

void pa_packet_update_field_impl(PaApp *app) {
    uint32_t pw_phase = app->frame % PA_PW_PERIOD;
    int pw_front = (int)((pw_phase * (uint32_t)(app->packet_field_radius * 2 + 1)) / PA_PW_PERIOD);
    int y;
    int x;

    for (y = -app->packet_field_radius - 1; y <= app->packet_field_radius + 1; ++y) {
        for (x = -app->packet_field_radius - 1; x <= app->packet_field_radius + 1; ++x) {
            int world_y = pa_wrap_coord(app->packet_y + y, PA_WORLD_ROWS);
            int world_x = pa_wrap_coord(app->packet_x + x, PA_WORLD_COLS);
            int metric;
            int ring;

            if (!pa_packet_pf_contains_cell_impl(app, world_x, world_y)) {
                continue;
            }

            app->manifested[world_y][world_x] = true;
            metric = pa_pf_metric(
                pa_torus_delta(app->packet_x, world_x, PA_WORLD_COLS),
                pa_torus_delta(app->packet_y, world_y, PA_WORLD_ROWS)
            );
            ring = metric / 2;
            if (ring <= pw_front) {
                app->calm[world_y][world_x] = true;
            }
        }
    }
}
