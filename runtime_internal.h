#ifndef PACKET_ADVENTURE_RUNTIME_INTERNAL_H
#define PACKET_ADVENTURE_RUNTIME_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include "packet/core.h"

#define PA_MOVE_COOLDOWN 5u
#define PA_PARAM_COOLDOWN 8u
#define PA_SWEEP_PERIOD 240u
#define PA_PW_PERIOD (PA_SWEEP_PERIOD)
#define PA_MINIMAP_LOGICAL_W 24
#define PA_MINIMAP_LOGICAL_H 24
#define PA_MINIMAP_SCALE 4
#define PA_MINIMAP_SPAN_X 48
#define PA_MINIMAP_SPAN_Y 48
#define PA_MANIFEST_SPAN_X 56
#define PA_MANIFEST_SPAN_Y 40
#define PA_TURBULENCE_THRESHOLD 120u

typedef struct {
    PaOperatorType op;
    int x;
    int y;
} PaZoneCenter;

typedef struct {
    PaOperatorType op;
    int anchor_x;
    int anchor_y;
    int spread_x;
    int spread_y;
    int copies;
} PaRegionPlan;

uint32_t pa_lcg_next(uint32_t *state);
int pa_rand_range(uint32_t *state, int min_value, int max_value);
int pa_clamp(int value, int min_value, int max_value);
int pa_wrap_coord(int value, int size);
int pa_torus_delta(int from, int to, int size);
int pa_torus_lerp(int from, int to, int step, int steps, int size);
uint32_t pa_chaos_seed_from_calm(const PaApp *app);
const PaOperatorProfile *pa_operator_profile(PaOperatorType op);
bool pa_operator_world_enabled(PaOperatorType op);
bool pa_operator_process_enabled(PaOperatorType op);
bool pa_operator_allowed(PaOperatorType a, PaOperatorType b);
PaZoneType pa_operator_zone(PaOperatorType op);
PaOperatorType pa_find_bridge_operator(PaOperatorType a, PaOperatorType b);
int pa_pf_metric(int dx, int dy);
void pa_select_path(PaOperatorType *ops, uint32_t *seed);
unsigned int pa_cell_noise(int x, int y, int salt);
void pa_add_region_seeds(PaZoneCenter *centers, int *count, const PaRegionPlan *plan, uint32_t *seed);

int pa_manifest_mf_strength_impl(const PaApp *app, int x, int y);
bool pa_manifest_cell_is_turbulence_impl(const PaApp *app, int x, int y);
void pa_manifest_refresh_density_impl(PaApp *app);
void pa_manifest_refresh_world_fields_impl(PaApp *app);
void pa_manifest_refresh_density_local_impl(PaApp *app);
void pa_manifest_refresh_world_fields_local_impl(PaApp *app);

void pa_packet_adjust_field_impl(PaApp *app, int delta);
bool pa_packet_pf_contains_cell_impl(const PaApp *app, int cell_x, int cell_y);
void pa_packet_try_move_impl(PaApp *app, int dx, int dy);
void pa_packet_update_field_impl(PaApp *app);

void pa_world_generate_impl(PaApp *app);
void pa_world_regenerate_chaos_impl(PaApp *app);
void pa_world_advance_layer_impl(PaApp *app);
void pa_world_regen_begin_cycle_impl(PaApp *app);
void pa_world_regen_column_impl(PaApp *app, int col_x);
void pa_world_regen_end_cycle_impl(PaApp *app);
void pa_world_manifest_cell_impl(PaApp *app, int x, int y);

#endif
