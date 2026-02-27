#include "bottom.h"

TextLayer *bottom_complication_create(GRect bounds, GFont font) {
  TextLayer *layer = text_layer_create(bounds);
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, GTextAlignmentCenter);
  return layer;
}

void bottom_complication_destroy(TextLayer *layer) {
  text_layer_destroy(layer);
}
