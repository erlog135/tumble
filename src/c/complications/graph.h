#pragma once
#include <pebble.h>

Layer *graph_create(GRect bounds, GRect plot_bounds, GFont label_font);
void graph_destroy(Layer *layer);
