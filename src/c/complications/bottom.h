#pragma once
#include <pebble.h>

typedef enum {
  BOTTOM_MODE_ICON_TEXT,  // icon to the left of text
  BOTTOM_MODE_ICON_ONLY,  // icon only
  BOTTOM_MODE_TEXT_ONLY,  // text only
} BottomMode;

typedef enum {
  BOTTOM_ALIGN_LEFT,   // content hugs left edge
  BOTTOM_ALIGN_RIGHT,  // content hugs right edge
} BottomAlign;

typedef struct {
  BottomMode mode;
  BottomAlign align;
  GFont font;
  uint32_t icon_resource_id;
} BottomConfig;

Layer *bottom_complication_create(GRect bounds, BottomConfig config);
void bottom_complication_set_text(Layer *layer, const char *text);
void bottom_complication_set_icon(Layer *layer, uint32_t resource_id);
void bottom_complication_destroy(Layer *layer);
