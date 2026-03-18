#pragma once
#include <pebble.h>
#include "../layout.h"
#include "../settings.h"

// Uncomment to test sun-clock: full revolution driven by seconds (one per minute)
#define TEST_SUN_CLOCK_SECONDS

typedef enum {
    COMPLICATION_GRAPH = 0,
    COMPLICATION_MINIVIEW,
    COMPLICATION_BOTTOM_LEFT,
    COMPLICATION_BOTTOM_RIGHT,
    COMPLICATION_COUNT,
} ComplicationSlot;

void providers_init(Layer *window_layer, Layout *layout, GFont font_20, GFont font_28);
void providers_apply_settings(void);
void providers_on_minute_tick(struct tm *tick_time);
void providers_on_weather_data(DictionaryIterator *iter);
void providers_on_solar_data(DictionaryIterator *iter);
void providers_on_lunar_data(DictionaryIterator *iter);
void providers_deinit(void);

Layer *providers_get_window_layer(void);
Layout *providers_get_layout(void);
GFont providers_get_font_20(void);
GFont providers_get_font_28(void);
Layer *providers_get_layer(ComplicationSlot slot);
void providers_set_layer(ComplicationSlot slot, Layer *layer);
