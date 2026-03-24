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
  GRect small_text_bounds;
  GRect medium_text_bounds;
  GFont small_font;
  GFont medium_font;

  // Used by all single-icon modes
  uint32_t icon_resource_id;

  // Used by MINIVIEW_MODE_CLOCK_DOTS: icon and line endpoint offset from center
  GPoint icon_offset;

  // Used by MINIVIEW_MODE_ICON_COLUMN: top, middle, bottom icon resource IDs
  uint32_t column_icon_resource_ids[3];
} MiniviewConfig;

Layer *miniview_create(GRect bounds, MiniviewConfig config);
void miniview_set_small_text(Layer *layer, const char *text);
void miniview_set_medium_text(Layer *layer, const char *text);
void miniview_set_icon_angle(Layer *layer, int32_t angle);
void miniview_set_icon_resource_id(Layer *layer, uint32_t resource_id);
void miniview_set_column_icon(Layer *layer, int index, uint32_t resource_id);
void miniview_destroy(Layer *layer);
