#pragma once
#include <pebble.h>

TextLayer *bottom_complication_create(GRect bounds, GFont font);
void bottom_complication_destroy(TextLayer *layer);
