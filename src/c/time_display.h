#pragma once
#include <pebble.h>

Layer *time_display_create(GRect bounds, GFont seconds_font);
void time_display_destroy(Layer *layer);
void time_display_set_time(Layer *layer, struct tm *tick_time);
void time_display_set_seconds_visible(Layer *layer, bool visible);
void time_display_set_seconds_reserved(Layer *layer, bool reserved);
void time_display_apply_settings(Layer *layer);
