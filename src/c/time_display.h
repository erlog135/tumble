#pragma once
#include <pebble.h>

Layer *time_display_create(GRect bounds, GFont seconds_font);
void time_display_destroy(Layer *layer);
/** Resize inner layers to match the container after layer_set_frame on the root. */
void time_display_sync_container_bounds(Layer *layer);
void time_display_set_time(Layer *layer, struct tm *tick_time);
void time_display_set_seconds_visible(Layer *layer, bool visible);
void time_display_set_seconds_reserved(Layer *layer, bool reserved);
void time_display_apply_settings(Layer *layer);
