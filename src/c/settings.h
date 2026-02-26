#pragma once
#include <pebble.h>

typedef struct {
    bool black_bg;
    bool invert_miniview;
    uint8_t weather_option;
    uint8_t weather_refresh_interval;
    uint8_t miniview_option;
    uint8_t graph_option;
    uint8_t seconds_option;
    uint8_t bottom_left_option;
    uint8_t bottom_right_option;
} ClaySettings;

void settings_init(void);

void settings_load(void);

void settings_save(void);

ClaySettings* settings_get(void);