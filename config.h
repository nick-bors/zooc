#include <stdbool.h>
#include <stdio.h>

typedef struct {
    float min_scale;
    float max_scale;
    float drag_friction;
    float scale_friction;
    float scroll_speed;

    bool windowed;
} Config;

void writeDefaultConf(FILE*);
Config loadConf(const char*);
