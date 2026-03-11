#pragma once
#include <pebble.h>

typedef enum {
  MINIVIEW_MODE_TEXT_STACK,   // small text on top of big text
  MINIVIEW_MODE_ICON_TEXT,    // icon on top of small text
  MINIVIEW_MODE_ICON_CENTER,  // centered icon
  MINIVIEW_MODE_CLOCK_DOTS,   // icon at arbitrary offset over 14-dot circle with line
  MINIVIEW_MODE_ICON_COLUMN,  // three icons stacked vertically, horizontally centered
} MiniviewMode;

typedef struct {
  MiniviewMode mode;

  // Used by MINIVIEW_MODE_TEXT_STACK and MINIVIEW_MODE_ICON_TEXT
  GRect tiny_text_bounds;
  GRect small_text_bounds;
  GFont tiny_font;
  GFont small_font;

  // Used by all icon modes
  uint32_t icon_resource_id;

  // Used by MINIVIEW_MODE_CLOCK_DOTS: icon and line endpoint offset from center
  GPoint icon_offset;
} MiniviewConfig;

Layer *miniview_create(GRect bounds, MiniviewConfig config);
void miniview_set_tiny_text(Layer *layer, const char *text);
void miniview_set_small_text(Layer *layer, const char *text);
void miniview_set_icon_angle(Layer *layer, int32_t angle);
void miniview_destroy(Layer *layer);
