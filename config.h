#ifndef ZOOC_CONFIG_H
#define ZOOC_CONFIG_H

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    float min_scale;
    float max_scale;
    float drag_friction;
    float scale_friction;
    float scroll_speed;
    float key_move_speed;
    bool windowed;
} Config;

Config load_config(const char*);
void write_default_config(FILE*);

#endif
