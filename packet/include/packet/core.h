#ifndef PACKET_CORE_H
#define PACKET_CORE_H

#include <stdbool.h>
#include <stdint.h>

#define PA_VIEW_COLS 15
#define PA_VIEW_ROWS 15
#define PA_WORLD_COLS 256
#define PA_WORLD_ROWS 192

enum {
    PA_INPUT_UP = 1u << 0,
    PA_INPUT_DOWN = 1u << 1,
    PA_INPUT_LEFT = 1u << 2,
    PA_INPUT_RIGHT = 1u << 3,
    PA_INPUT_PARAM_DEC = 1u << 4,
    PA_INPUT_PARAM_INC = 1u << 5,
    PA_INPUT_MAP_TOGGLE = 1u << 6,
    PA_INPUT_REGEN = 1u << 7,
    PA_INPUT_SW_TOGGLE = 1u << 8,
    PA_INPUT_SW_STEP = 1u << 9,
    PA_INPUT_SW_SLOWER = 1u << 10,
    PA_INPUT_SW_FASTER = 1u << 11,
};

typedef enum {
    PA_TILE_VOID = 0,
    PA_TILE_WALL = 1,
    PA_TILE_FLOOR = 2,
} PaTileType;

typedef enum {
    PA_ZONE_CALM = 0,
    PA_ZONE_BOUNDARY = 1,
    PA_ZONE_CHAOS = 2,
} PaZoneType;

typedef enum {
    PA_OP_FLOW = 0,
    PA_OP_CONNECT = 1,
    PA_OP_DISSOLVE = 2,
    PA_OP_ENCODE = 3,
    PA_OP_CHOOSE = 4,
    PA_OP_OBSERVE = 5,
    PA_OP_CYCLE = 6,
    PA_OP_LOGIC = 7,
    PA_OP_RUNTIME = 8,
    PA_OP_MANIFEST = 9,
    PA_OP_COUNT = 10
} PaOperatorType;

typedef struct {
    uint8_t world_strength;
    uint8_t process_strength;
    int8_t stability_bias;
    int8_t pressure_bias;
} PaOperatorProfile;

typedef struct {
    uint32_t frame;
    uint32_t sweep_frame;
    uint32_t input_mask;
    uint32_t prev_input_mask;
    uint32_t move_cooldown;
    uint32_t param_cooldown;
    uint32_t entropy;
    uint32_t topology_revision;
    uint32_t world_seed;
    uint32_t regen_cycle_seed;
    uint32_t sw_cycle_count;
    uint32_t layer_index;
    uint32_t debug_regen_begin_count;
    uint32_t debug_regen_end_count;
    uint32_t debug_regen_columns;
    uint32_t debug_regen_changed_cells;
    uint32_t debug_regen_last_changed_cells;
    uint32_t debug_world_hash;
    uint8_t sweep_speed;
    uint8_t debug_regen_last_column;
    int packet_x;
    int packet_y;
    int transition_x;
    int transition_y;
    int packet_field_radius;
    bool map_open;
    bool sweep_paused;
    bool transition_reached;
    bool quit_requested;
    uint8_t tiles[PA_WORLD_ROWS][PA_WORLD_COLS];
    uint8_t zones[PA_WORLD_ROWS][PA_WORLD_COLS];
    uint8_t operators[PA_WORLD_ROWS][PA_WORLD_COLS];
    uint16_t base_operator_area[PA_OP_COUNT];
    uint8_t local_entropy_rate[PA_WORLD_ROWS][PA_WORLD_COLS];
    uint8_t local_density[PA_WORLD_ROWS][PA_WORLD_COLS];
    bool manifested[PA_WORLD_ROWS][PA_WORLD_COLS];
    bool calm[PA_WORLD_ROWS][PA_WORLD_COLS];
} PaApp;

void pa_app_init(PaApp *app);
void pa_app_set_input(PaApp *app, uint32_t input_mask);
void pa_app_update(PaApp *app);
const char *pa_app_build_id(void);

#endif
