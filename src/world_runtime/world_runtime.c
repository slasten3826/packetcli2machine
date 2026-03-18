#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "packet_adventure/runtime_internal.h"

static PaZoneCenter g_regen_centers[96];
static int g_regen_center_count = 0;
static bool g_regen_calm_snapshot[PA_WORLD_ROWS][PA_WORLD_COLS];
static bool g_regen_column_done[PA_WORLD_COLS];
static bool g_regen_active = false;
static uint8_t g_zone_scratch[PA_WORLD_ROWS][PA_WORLD_COLS];
enum {
    PA_MIN_ZONE_THICKNESS = 20
};
static const uint8_t k_operator_rates[PA_OP_COUNT] = {
    2, 1, 4, 3, 3, 0, 2, 1, 1, 5
};

static void pa_reset_regen_state(void) {
    g_regen_center_count = 0;
    memset(g_regen_calm_snapshot, 0, sizeof(g_regen_calm_snapshot));
    memset(g_regen_column_done, 0, sizeof(g_regen_column_done));
    g_regen_active = false;
}

static uint32_t pa_debug_world_hash(const PaApp *app) {
    uint32_t hash = 2166136261u;
    int x;
    int y;

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            hash ^= (uint32_t)app->operators[y][x];
            hash *= 16777619u;
        }
    }

    return hash;
}

static void pa_capture_base_operator_area(PaApp *app) {
    int x;
    int y;

    memset(app->base_operator_area, 0, sizeof(app->base_operator_area));
    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            PaOperatorType op = (PaOperatorType)app->operators[y][x];

            app->base_operator_area[op] += 1u;
        }
    }
}

static PaOperatorType pa_resolve_operator_thickness(const PaApp *app, int x, int y, PaOperatorType candidate) {
    unsigned int counts[PA_OP_COUNT] = {0};
    PaOperatorType dominant = candidate;
    unsigned int candidate_count;
    unsigned int dominant_count = 0u;
    int horizontal_run = 1;
    int vertical_run = 1;
    int step;
    int nx;
    int ny;

    for (step = 1; step < PA_MIN_ZONE_THICKNESS; ++step) {
        if ((PaOperatorType)app->operators[y][pa_wrap_coord(x - step, PA_WORLD_COLS)] == candidate) {
            horizontal_run += 1;
        } else {
            break;
        }
    }
    for (step = 1; step < PA_MIN_ZONE_THICKNESS; ++step) {
        if ((PaOperatorType)app->operators[y][pa_wrap_coord(x + step, PA_WORLD_COLS)] == candidate) {
            horizontal_run += 1;
        } else {
            break;
        }
    }
    for (step = 1; step < PA_MIN_ZONE_THICKNESS; ++step) {
        if ((PaOperatorType)app->operators[pa_wrap_coord(y - step, PA_WORLD_ROWS)][x] == candidate) {
            vertical_run += 1;
        } else {
            break;
        }
    }
    for (step = 1; step < PA_MIN_ZONE_THICKNESS; ++step) {
        if ((PaOperatorType)app->operators[pa_wrap_coord(y + step, PA_WORLD_ROWS)][x] == candidate) {
            vertical_run += 1;
        } else {
            break;
        }
    }

    if (horizontal_run >= PA_MIN_ZONE_THICKNESS || vertical_run >= PA_MIN_ZONE_THICKNESS) {
        return candidate;
    }

    for (ny = -(PA_MIN_ZONE_THICKNESS); ny <= PA_MIN_ZONE_THICKNESS; ++ny) {
        for (nx = -(PA_MIN_ZONE_THICKNESS); nx <= PA_MIN_ZONE_THICKNESS; ++nx) {
            int sample_y = pa_wrap_coord(y + ny, PA_WORLD_ROWS);
            int sample_x = pa_wrap_coord(x + nx, PA_WORLD_COLS);
            PaOperatorType near = (sample_x == x && sample_y == y)
                ? candidate
                : (PaOperatorType)app->operators[sample_y][sample_x];

            counts[near] += 1u;
        }
    }

    candidate_count = counts[candidate];
    for (nx = 0; nx < PA_OP_COUNT; ++nx) {
        if ((PaOperatorType)nx == candidate) {
            continue;
        }
        if (counts[nx] > dominant_count) {
            dominant_count = counts[nx];
            dominant = (PaOperatorType)nx;
        }
    }

    if (dominant != candidate &&
        dominant_count >= candidate_count * 2u &&
        pa_operator_allowed(candidate, dominant) &&
        pa_operator_allowed(dominant, candidate)) {
        return dominant;
    }

    return candidate;
}

static void pa_consolidate_operator_zones(PaApp *app) {
    int x;
    int y;

    memcpy(g_zone_scratch, app->operators, sizeof(g_zone_scratch));

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            if (app->calm[y][x]) {
                continue;
            }
            g_zone_scratch[y][x] = (uint8_t)pa_resolve_operator_thickness(
                app, x, y, (PaOperatorType)app->operators[y][x]);
        }
    }

    memcpy(app->operators, g_zone_scratch, sizeof(g_zone_scratch));
}

static void pa_bridge_fix_chaos(PaApp *app) {
    int x;
    int y;

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            int right_x = pa_wrap_coord(x + 1, PA_WORLD_COLS);
            int down_y = pa_wrap_coord(y + 1, PA_WORLD_ROWS);
            PaOperatorType op = (PaOperatorType)app->operators[y][x];
            PaOperatorType right = (PaOperatorType)app->operators[y][right_x];
            PaOperatorType down = (PaOperatorType)app->operators[down_y][x];

            if (app->calm[y][x]) {
                continue;
            }

            if (!pa_operator_allowed(op, right)) {
                if (!app->calm[y][right_x]) {
                    app->operators[y][right_x] = (uint8_t)pa_find_bridge_operator(op, right);
                } else if (!app->calm[y][x]) {
                    app->operators[y][x] = (uint8_t)pa_find_bridge_operator(right, op);
                }
            }
            if (!pa_operator_allowed(op, down)) {
                if (!app->calm[down_y][x]) {
                    app->operators[down_y][x] = (uint8_t)pa_find_bridge_operator(op, down);
                } else if (!app->calm[y][x]) {
                    app->operators[y][x] = (uint8_t)pa_find_bridge_operator(down, op);
                }
            }
        }
    }
}

static void pa_balance_operator_area(PaApp *app) {
    uint16_t current_area[PA_OP_COUNT] = {0};
    int over_op = -1;
    int under_op = -1;
    int over_delta = 0;
    int under_delta = 0;
    unsigned int local_total = 0u;
    int x;
    int y;

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            if (app->calm[y][x]) {
                continue;
            }
            current_area[app->operators[y][x]] += 1u;
            local_total += 1u;
        }
    }

    if (local_total == 0u) {
        return;
    }

    for (x = 0; x < PA_OP_COUNT; ++x) {
        int baseline = (int)(((uint32_t)app->base_operator_area[x] * local_total) /
                             (uint32_t)(PA_WORLD_ROWS * PA_WORLD_COLS));
        int tolerance = baseline / 5;
        int delta = (int)current_area[x] - baseline;

        if (tolerance < 4) {
            tolerance = 4;
        }

        if (delta > tolerance && delta > over_delta) {
            over_delta = delta;
            over_op = x;
        }
        if (delta < -tolerance && delta < under_delta) {
            under_delta = delta;
            under_op = x;
        }
    }

    if (over_op < 0 || under_op < 0) {
        return;
    }

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            PaOperatorType current;
            int nx;
            int ny;

            if (app->calm[y][x] || pa_manifest_mf_strength_impl(app, x, y) < 2) {
                continue;
            }
            current = (PaOperatorType)app->operators[y][x];
            if (current != (PaOperatorType)over_op) {
                continue;
            }

            for (ny = -1; ny <= 1; ++ny) {
                for (nx = -1; nx <= 1; ++nx) {
                    int sample_y = pa_wrap_coord(y + ny, PA_WORLD_ROWS);
                    int sample_x = pa_wrap_coord(x + nx, PA_WORLD_COLS);
                    PaOperatorType near = (PaOperatorType)app->operators[sample_y][sample_x];

                    if (near == (PaOperatorType)under_op &&
                        pa_operator_allowed((PaOperatorType)under_op, current) &&
                        pa_operator_allowed(current, (PaOperatorType)under_op)) {
                        app->operators[y][x] = (uint8_t)under_op;
                        over_delta -= 1;
                        under_delta += 1;
                        goto next_cell;
                    }
                }
            }

            if (pa_operator_allowed(current, (PaOperatorType)under_op) &&
                pa_operator_allowed((PaOperatorType)under_op, current)) {
                app->operators[y][x] = (uint8_t)under_op;
                over_delta -= 1;
                under_delta += 1;
            }

next_cell:
            if (over_delta <= 0 || under_delta >= 0) {
                return;
            }
        }
    }
}

static void pa_prepare_regen_centers(PaApp *app, uint32_t seed) {
    PaOperatorType path_ops[7];
    PaOperatorType late_a;
    PaOperatorType late_b;
    PaRegionPlan plans[9];
    int plan_count = 0;
    int route_x[6];
    int route_y[6];
    int branch_x;
    int branch_y;
    int x;
    int y;
    int i;

    pa_select_path(path_ops, &seed);
    late_a = path_ops[4];
    late_b = (late_a == PA_OP_CYCLE) ? PA_OP_LOGIC : PA_OP_CYCLE;
    if (late_a == PA_OP_LOGIC) {
        late_b = PA_OP_RUNTIME;
    } else if (late_a == PA_OP_RUNTIME) {
        late_b = PA_OP_LOGIC;
    }

    for (i = 0; i < 6; ++i) {
        route_x[i] = pa_torus_lerp(app->packet_x, app->transition_x, i, 5, PA_WORLD_COLS);
        route_y[i] = pa_torus_lerp(app->packet_y, app->transition_y, i, 5, PA_WORLD_ROWS);
        route_x[i] = pa_wrap_coord(route_x[i] + pa_rand_range(&seed, -8, 8), PA_WORLD_COLS);
        route_y[i] = pa_wrap_coord(route_y[i] + pa_rand_range(&seed, -8, 8), PA_WORLD_ROWS);
    }
    route_x[0] = app->packet_x;
    route_y[0] = app->packet_y;
    route_x[5] = app->transition_x;
    route_y[5] = app->transition_y;

    plans[plan_count++] = (PaRegionPlan){PA_OP_FLOW, route_x[0], route_y[0], 8, 10, 4};
    plans[plan_count++] = (PaRegionPlan){PA_OP_CONNECT, route_x[1], route_y[1], 10, 12, 5};
    plans[plan_count++] = (PaRegionPlan){PA_OP_ENCODE, route_x[2], route_y[2], 10, 12, 5};
    plans[plan_count++] = (PaRegionPlan){PA_OP_OBSERVE, route_x[3], route_y[3], 10, 12, 5};
    plans[plan_count++] = (PaRegionPlan){late_a, route_x[4], route_y[4], 10, 12, 5};
    plans[plan_count++] = (PaRegionPlan){PA_OP_MANIFEST, route_x[5], route_y[5], 8, 10, 4};

    branch_x = pa_wrap_coord(route_x[1] + pa_rand_range(&seed, -24, 24), PA_WORLD_COLS);
    branch_y = pa_wrap_coord(route_y[1] + pa_rand_range(&seed, -20, 20), PA_WORLD_ROWS);
    plans[plan_count++] = (PaRegionPlan){PA_OP_DISSOLVE, branch_x, branch_y, 12, 8, 3};

    branch_x = pa_wrap_coord(route_x[3] + pa_rand_range(&seed, -24, 24), PA_WORLD_COLS);
    branch_y = pa_wrap_coord(route_y[3] + pa_rand_range(&seed, -20, 20), PA_WORLD_ROWS);
    plans[plan_count++] = (PaRegionPlan){PA_OP_CHOOSE, branch_x, branch_y, 12, 8, 3};

    branch_x = pa_wrap_coord(route_x[4] + pa_rand_range(&seed, -20, 20), PA_WORLD_COLS);
    branch_y = pa_wrap_coord(route_y[4] + pa_rand_range(&seed, -20, 20), PA_WORLD_ROWS);
    plans[plan_count++] = (PaRegionPlan){late_b, branch_x, branch_y, 10, 8, 3};

    for (i = 0; i < plan_count; ++i) {
        pa_add_region_seeds(g_regen_centers, &g_regen_center_count, &plans[i], &seed);
    }

    for (y = 0; y < PA_WORLD_ROWS && g_regen_center_count < (int)(sizeof(g_regen_centers) / sizeof(g_regen_centers[0])); y += 12) {
        for (x = 0; x < PA_WORLD_COLS && g_regen_center_count < (int)(sizeof(g_regen_centers) / sizeof(g_regen_centers[0])); x += 12) {
            if (!g_regen_calm_snapshot[y][x]) {
                continue;
            }
            g_regen_centers[g_regen_center_count].op = (PaOperatorType)app->operators[y][x];
            g_regen_centers[g_regen_center_count].x = x;
            g_regen_centers[g_regen_center_count].y = y;
            g_regen_center_count += 1;
        }
    }
}

static PaOperatorType pa_regen_best_operator(const PaApp *app, uint32_t seed, int x, int y) {
    int best_score = INT_MAX;
    PaOperatorType best_op = (PaOperatorType)app->operators[y][x];
    int i;

    for (i = 0; i < g_regen_center_count; ++i) {
        int dx = pa_torus_delta(g_regen_centers[i].x, x, PA_WORLD_COLS);
        int dy = pa_torus_delta(g_regen_centers[i].y, y, PA_WORLD_ROWS);
        int score = (dx * dx * 2) + (dy * dy * 3) + (int)pa_cell_noise(x, y, i + (int)(seed & 255u));

        if (g_regen_centers[i].op == PA_OP_DISSOLVE) {
            score += (y > PA_WORLD_ROWS / 2 ? 18 : 0);
        }
        if (g_regen_centers[i].op == PA_OP_CHOOSE) {
            score += (y < PA_WORLD_ROWS / 2 ? 16 : 0);
        }
        if (g_regen_calm_snapshot[g_regen_centers[i].y][g_regen_centers[i].x]) {
            score -= 64;
        }
        if (score < best_score) {
            best_score = score;
            best_op = g_regen_centers[i].op;
        }
    }

    return best_op;
}

void pa_world_manifest_cell_impl(PaApp *app, int x, int y) {
    PaOperatorType best_op;
    PaOperatorType near;
    int sample_x;
    int sample_y;
    int i;
    static const int k_neighbor_offsets[4][2] = {
        {-1, 0},
        {1, 0},
        {0, -1},
        {0, 1}
    };

    x = pa_wrap_coord(x, PA_WORLD_COLS);
    y = pa_wrap_coord(y, PA_WORLD_ROWS);
    if (app->calm[y][x] || pa_manifest_mf_strength_impl(app, x, y) == 0) {
        return;
    }

    if (!g_regen_active) {
        pa_world_regen_begin_cycle_impl(app);
    }

    best_op = pa_regen_best_operator(app, app->regen_cycle_seed, x, y);
    for (i = 0; i < 4; ++i) {
        sample_x = pa_wrap_coord(x + k_neighbor_offsets[i][0], PA_WORLD_COLS);
        sample_y = pa_wrap_coord(y + k_neighbor_offsets[i][1], PA_WORLD_ROWS);
        if (app->calm[sample_y][sample_x]) {
            continue;
        }
        near = (PaOperatorType)app->operators[sample_y][sample_x];
        if (!pa_operator_allowed(best_op, near)) {
            best_op = pa_find_bridge_operator(near, best_op);
        }
    }

    best_op = pa_resolve_operator_thickness(app, x, y, best_op);
    app->operators[y][x] = (uint8_t)best_op;
    app->zones[y][x] = (uint8_t)pa_operator_zone(best_op);
    app->local_entropy_rate[y][x] = k_operator_rates[best_op];
    app->manifested[y][x] = true;
}

void pa_world_regen_begin_cycle_impl(PaApp *app) {
    uint32_t calm_seed = pa_chaos_seed_from_calm(app);
    uint32_t cycle_seed = app->world_seed ^ (app->sw_cycle_count * 2654435761u) ^ calm_seed;

    memcpy(g_regen_calm_snapshot, app->calm, sizeof(g_regen_calm_snapshot));
    memset(g_regen_column_done, 0, sizeof(g_regen_column_done));
    g_regen_center_count = 0;
    app->regen_cycle_seed = cycle_seed;
    pa_prepare_regen_centers(app, cycle_seed);
    g_regen_active = true;
    app->debug_regen_begin_count += 1u;
    app->debug_regen_columns = 0u;
    app->debug_regen_changed_cells = 0u;
    app->debug_regen_last_column = 0u;
}

void pa_world_regen_column_impl(PaApp *app, int col_x) {
    int y;

    if (!g_regen_active) {
        return;
    }

    col_x = pa_wrap_coord(col_x, PA_WORLD_COLS);
    if (g_regen_column_done[col_x]) {
        return;
    }

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        PaOperatorType best_op;
        PaOperatorType left;
        PaOperatorType up;
        PaOperatorType old_op;
        int strength;
        int left_x;
        int up_y;

        if (app->calm[y][col_x]) {
            continue;
        }

        strength = pa_manifest_mf_strength_impl(app, col_x, y);
        if (strength == 0) {
            continue;
        }

        old_op = (PaOperatorType)app->operators[y][col_x];
        best_op = pa_regen_best_operator(app, app->regen_cycle_seed, col_x, y);

        if (best_op != old_op) {
            app->debug_regen_changed_cells += 1u;
        }

        app->operators[y][col_x] = (uint8_t)best_op;
        if (strength == 1) {
            app->zones[y][col_x] = (uint8_t)pa_operator_zone(best_op);
            app->local_entropy_rate[y][col_x] = k_operator_rates[best_op];
            continue;
        }

        left_x = pa_wrap_coord(col_x - 1, PA_WORLD_COLS);
        up_y = pa_wrap_coord(y - 1, PA_WORLD_ROWS);
        left = (PaOperatorType)app->operators[y][left_x];
        up = (PaOperatorType)app->operators[up_y][col_x];

        if (!pa_operator_allowed(best_op, left) && !app->calm[y][left_x]) {
            best_op = pa_find_bridge_operator(left, best_op);
        }
        if (!pa_operator_allowed(best_op, up) && !app->calm[up_y][col_x]) {
            best_op = pa_find_bridge_operator(up, best_op);
        }

        best_op = pa_resolve_operator_thickness(app, col_x, y, best_op);
        app->operators[y][col_x] = (uint8_t)best_op;
        app->zones[y][col_x] = (uint8_t)pa_operator_zone(best_op);
        app->local_entropy_rate[y][col_x] = k_operator_rates[best_op];
    }

    g_regen_column_done[col_x] = true;
    app->debug_regen_columns += 1u;
    app->debug_regen_last_column = (uint8_t)col_x;
}

void pa_world_regen_end_cycle_impl(PaApp *app) {
    int x;

    if (!g_regen_active) {
        return;
    }

    for (x = 0; x < PA_WORLD_COLS; ++x) {
        if (!g_regen_column_done[x]) {
            pa_world_regen_column_impl(app, x);
        }
    }

    app->operators[app->packet_y][app->packet_x] = PA_OP_FLOW;
    app->operators[app->transition_y][app->transition_x] = PA_OP_MANIFEST;
    app->zones[app->packet_y][app->packet_x] = (uint8_t)pa_operator_zone(PA_OP_FLOW);
    app->zones[app->transition_y][app->transition_x] = (uint8_t)pa_operator_zone(PA_OP_MANIFEST);
    app->local_entropy_rate[app->packet_y][app->packet_x] = k_operator_rates[PA_OP_FLOW];
    app->local_entropy_rate[app->transition_y][app->transition_x] = k_operator_rates[PA_OP_MANIFEST];
    app->manifested[app->packet_y][app->packet_x] = true;
    app->manifested[app->transition_y][app->transition_x] = true;
    app->topology_revision += 1u;
    app->debug_regen_end_count += 1u;
    app->debug_regen_last_changed_cells = app->debug_regen_changed_cells;
    app->debug_world_hash = pa_debug_world_hash(app);
    g_regen_active = false;
}

void pa_world_regenerate_chaos_impl(PaApp *app) {
    pa_world_regen_begin_cycle_impl(app);
    pa_world_regen_end_cycle_impl(app);
}

void pa_world_generate_impl(PaApp *app) {
    uint32_t seed = (app->world_seed != 0u) ? app->world_seed : 0x2A6D365Bu;
    PaOperatorType path_ops[7];
    PaOperatorType late_a;
    PaOperatorType late_b;
    PaRegionPlan plans[9];
    PaZoneCenter centers[48];
    int center_count = 0;
    int plan_count = 0;
    int i;
    int x;
    int y;
    int route_x[6];
    int route_y[6];
    int start_x;
    int start_y;
    int target_x;
    int target_y;
    int branch_x;
    int branch_y;

    memset(app->tiles, 0, sizeof(app->tiles));
    memset(app->zones, 0, sizeof(app->zones));
    memset(app->operators, 0, sizeof(app->operators));
    memset(app->local_entropy_rate, 0, sizeof(app->local_entropy_rate));
    memset(app->local_density, 0, sizeof(app->local_density));
    memset(app->manifested, 0, sizeof(app->manifested));
    memset(app->calm, 0, sizeof(app->calm));

    pa_select_path(path_ops, &seed);
    late_a = path_ops[4];
    late_b = (late_a == PA_OP_CYCLE) ? PA_OP_LOGIC : PA_OP_CYCLE;
    if (late_a == PA_OP_LOGIC) {
        late_b = PA_OP_RUNTIME;
    } else if (late_a == PA_OP_RUNTIME) {
        late_b = PA_OP_LOGIC;
    }

    start_x = pa_rand_range(&seed, 0, PA_WORLD_COLS - 1);
    start_y = pa_rand_range(&seed, 0, PA_WORLD_ROWS - 1);
    do {
        target_x = pa_rand_range(&seed, 0, PA_WORLD_COLS - 1);
        target_y = pa_rand_range(&seed, 0, PA_WORLD_ROWS - 1);
    } while ((((pa_torus_delta(start_x, target_x, PA_WORLD_COLS) < 0 ? -pa_torus_delta(start_x, target_x, PA_WORLD_COLS) : pa_torus_delta(start_x, target_x, PA_WORLD_COLS))) +
              ((pa_torus_delta(start_y, target_y, PA_WORLD_ROWS) < 0 ? -pa_torus_delta(start_y, target_y, PA_WORLD_ROWS) : pa_torus_delta(start_y, target_y, PA_WORLD_ROWS)))) <
             ((PA_WORLD_COLS + PA_WORLD_ROWS) / 3));

    for (i = 0; i < 6; ++i) {
        route_x[i] = pa_torus_lerp(start_x, target_x, i, 5, PA_WORLD_COLS);
        route_y[i] = pa_torus_lerp(start_y, target_y, i, 5, PA_WORLD_ROWS);
        route_x[i] = pa_wrap_coord(route_x[i] + pa_rand_range(&seed, -10, 10), PA_WORLD_COLS);
        route_y[i] = pa_wrap_coord(route_y[i] + pa_rand_range(&seed, -10, 10), PA_WORLD_ROWS);
    }
    route_x[0] = start_x;
    route_y[0] = start_y;
    route_x[5] = target_x;
    route_y[5] = target_y;

    plans[plan_count++] = (PaRegionPlan){PA_OP_FLOW, route_x[0], route_y[0], 10, 14, 5};
    plans[plan_count++] = (PaRegionPlan){PA_OP_CONNECT, route_x[1], route_y[1], 12, 16, 6};
    plans[plan_count++] = (PaRegionPlan){PA_OP_ENCODE, route_x[2], route_y[2], 12, 16, 6};
    plans[plan_count++] = (PaRegionPlan){PA_OP_OBSERVE, route_x[3], route_y[3], 12, 16, 6};
    plans[plan_count++] = (PaRegionPlan){late_a, route_x[4], route_y[4], 12, 16, 6};
    plans[plan_count++] = (PaRegionPlan){PA_OP_MANIFEST, route_x[5], route_y[5], 10, 12, 4};

    branch_x = pa_wrap_coord(route_x[1] + pa_rand_range(&seed, -28, 28), PA_WORLD_COLS);
    branch_y = pa_wrap_coord(route_y[1] + pa_rand_range(&seed, -24, 24), PA_WORLD_ROWS);
    plans[plan_count++] = (PaRegionPlan){PA_OP_DISSOLVE, branch_x, branch_y, 16, 10, 4};

    branch_x = pa_wrap_coord(route_x[3] + pa_rand_range(&seed, -28, 28), PA_WORLD_COLS);
    branch_y = pa_wrap_coord(route_y[3] + pa_rand_range(&seed, -24, 24), PA_WORLD_ROWS);
    plans[plan_count++] = (PaRegionPlan){PA_OP_CHOOSE, branch_x, branch_y, 14, 10, 4};

    branch_x = pa_wrap_coord(route_x[4] + pa_rand_range(&seed, -24, 24), PA_WORLD_COLS);
    branch_y = pa_wrap_coord(route_y[4] + pa_rand_range(&seed, -24, 24), PA_WORLD_ROWS);
    plans[plan_count++] = (PaRegionPlan){late_b, branch_x, branch_y, 12, 10, 4};

    for (i = 0; i < plan_count; ++i) {
        pa_add_region_seeds(centers, &center_count, &plans[i], &seed);
    }

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            int best_score = 1 << 30;
            PaOperatorType best_op = PA_OP_FLOW;

            app->tiles[y][x] = PA_TILE_FLOOR;
            for (i = 0; i < center_count; ++i) {
                int dx = pa_torus_delta(centers[i].x, x, PA_WORLD_COLS);
                int dy = pa_torus_delta(centers[i].y, y, PA_WORLD_ROWS);
                int score = (dx * dx * 2) + (dy * dy * 3) + (int)pa_cell_noise(x, y, i);

                if (centers[i].op == PA_OP_DISSOLVE) {
                    score += (y > PA_WORLD_ROWS / 2 ? 24 : 0);
                }
                if (centers[i].op == PA_OP_CHOOSE) {
                    score += (y < PA_WORLD_ROWS / 2 ? 20 : 0);
                }
                if (score < best_score) {
                    best_score = score;
                    best_op = centers[i].op;
                }
            }

            app->operators[y][x] = (uint8_t)best_op;
            app->zones[y][x] = (uint8_t)pa_operator_zone(best_op);
            app->local_entropy_rate[y][x] = k_operator_rates[best_op];
        }
    }

    for (i = 0; i < 6; ++i) {
        int cx = route_x[i];
        int cy = route_y[i];
        int rx;
        int ry;

        for (ry = -3; ry <= 3; ++ry) {
            for (rx = -4; rx <= 4; ++rx) {
                int wx = pa_wrap_coord(cx + rx, PA_WORLD_COLS);
                int wy = pa_wrap_coord(cy + ry, PA_WORLD_ROWS);

                if ((rx * rx) + (ry * ry) <= 16) {
                    app->operators[wy][wx] = (uint8_t)plans[i].op;
                }
            }
        }
    }

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            PaOperatorType op = (PaOperatorType)app->operators[y][x];
            int right_x = pa_wrap_coord(x + 1, PA_WORLD_COLS);
            int down_y = pa_wrap_coord(y + 1, PA_WORLD_ROWS);
            PaOperatorType right = (PaOperatorType)app->operators[y][right_x];
            PaOperatorType down = (PaOperatorType)app->operators[down_y][x];

            if (!pa_operator_allowed(op, right)) {
                app->operators[y][right_x] = (uint8_t)pa_find_bridge_operator(op, right);
            }
            if (!pa_operator_allowed(op, down)) {
                app->operators[down_y][x] = (uint8_t)pa_find_bridge_operator(op, down);
            }
        }
    }

    for (i = 0; i < 2; ++i) {
        for (y = 0; y < PA_WORLD_ROWS; ++y) {
            for (x = 0; x < PA_WORLD_COLS; ++x) {
                PaOperatorType op = (PaOperatorType)app->operators[y][x];
                unsigned int counts[PA_OP_COUNT] = {0};
                PaOperatorType best_op = op;
                unsigned int best_count = 0;
                int nx;
                int ny;

                for (ny = -1; ny <= 1; ++ny) {
                    for (nx = -1; nx <= 1; ++nx) {
                        int sample_y = pa_wrap_coord(y + ny, PA_WORLD_ROWS);
                        int sample_x = pa_wrap_coord(x + nx, PA_WORLD_COLS);
                        PaOperatorType near = (PaOperatorType)app->operators[sample_y][sample_x];

                        counts[near] += 1u;
                    }
                }
                for (nx = 0; nx < PA_OP_COUNT; ++nx) {
                    if (counts[nx] > best_count && pa_operator_allowed(op, (PaOperatorType)nx)) {
                        best_count = counts[nx];
                        best_op = (PaOperatorType)nx;
                    }
                }
                if (best_count >= 5u) {
                    app->operators[y][x] = (uint8_t)best_op;
                }
            }
        }
    }

    for (y = 0; y < PA_WORLD_ROWS; ++y) {
        for (x = 0; x < PA_WORLD_COLS; ++x) {
            PaOperatorType op = (PaOperatorType)app->operators[y][x];
            app->zones[y][x] = (uint8_t)pa_operator_zone(op);
            app->local_entropy_rate[y][x] = k_operator_rates[op];
        }
    }

    app->packet_x = route_x[0];
    app->packet_y = route_y[0];
    app->transition_x = route_x[5];
    app->transition_y = route_y[5];
    app->operators[app->packet_y][app->packet_x] = PA_OP_FLOW;
    app->operators[app->transition_y][app->transition_x] = PA_OP_MANIFEST;
    app->zones[app->packet_y][app->packet_x] = (uint8_t)pa_operator_zone(PA_OP_FLOW);
    app->zones[app->transition_y][app->transition_x] = (uint8_t)pa_operator_zone(PA_OP_MANIFEST);
    app->local_entropy_rate[app->packet_y][app->packet_x] = k_operator_rates[PA_OP_FLOW];
    app->local_entropy_rate[app->transition_y][app->transition_x] = k_operator_rates[PA_OP_MANIFEST];
    app->manifested[app->packet_y][app->packet_x] = true;
    app->calm[app->packet_y][app->packet_x] = true;
    app->manifested[app->transition_y][app->transition_x] = true;
    app->entropy = 0u;
    app->move_cooldown = 0u;
    app->param_cooldown = 0u;
    app->sweep_frame = 0u;
    app->sw_cycle_count = 0u;
    app->sweep_speed = 1u;
    app->sweep_paused = false;
    app->transition_reached = false;
    pa_reset_regen_state();
    pa_consolidate_operator_zones(app);
    pa_capture_base_operator_area(app);
    app->debug_world_hash = pa_debug_world_hash(app);
    app->topology_revision += 1u;
}

void pa_world_advance_layer_impl(PaApp *app) {
    bool prev_calm[PA_WORLD_ROWS][PA_WORLD_COLS];
    bool prev_manifested[PA_WORLD_ROWS][PA_WORLD_COLS];

    memcpy(prev_calm, app->calm, sizeof(prev_calm));
    memcpy(prev_manifested, app->manifested, sizeof(prev_manifested));
    app->layer_index += 1u;
    app->world_seed = (app->world_seed * 1664525u) + 1013904223u + (app->layer_index * 97u);
    pa_world_generate_impl(app);

    {
        int x;
        int y;

        for (y = 0; y < PA_WORLD_ROWS; ++y) {
            for (x = 0; x < PA_WORLD_COLS; ++x) {
                if (prev_calm[y][x]) {
                    app->calm[y][x] = true;
                    app->manifested[y][x] = true;
                } else if (prev_manifested[y][x]) {
                    app->manifested[y][x] = true;
                }
            }
        }

        pa_reset_regen_state();
    }
}
