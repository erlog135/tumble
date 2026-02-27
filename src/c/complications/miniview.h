#pragma once
#include <pebble.h>

Layer *miniview_create(GRect bounds, GRect tiny_text_bounds, GRect small_text_bounds,
                       GFont tiny_font, GFont small_font);
void miniview_destroy(Layer *layer);
