#pragma once
#include <pebble.h>

// How far bottom-axis tick marks extend upward (pixels)
#define GRAPH_H_MARKER_SIZE 3
// How far side-axis tick marks extend inward (pixels)
#define GRAPH_V_MARKER_SIZE 6
// Size of each checker square in the filled graph mode (1 = single pixel, 2 = 2x2 blocks, etc.)
#define GRAPH_FILL_CHECKER_SIZE 2
// Gap between the header icon and label text (pixels)
#define GRAPH_HEADER_ICON_TEXT_GAP 4
// Maximum number of data values the graph can hold (matches HISTORY_24H_LEN / HISTORY_4H_LEN)
#define GRAPH_MAX_VALUES 24
// Maximum length of dynamic label text in header
#define GRAPH_LABEL_MAX 24

typedef enum {
  GRAPH_STYLE_LINE,    // continuous polyline connecting data points
  GRAPH_STYLE_BARS,    // vertical bar per data point rising from the bottom
  GRAPH_STYLE_FILLED,  // filled area under a polyline
} GraphStyle;

typedef struct {
  GraphStyle style;
  uint8_t h_markers;         // tick marks evenly spaced along the bottom axis
  uint8_t v_markers;         // tick marks evenly spaced along both vertical edges
  bool top_lip;              // tick marks at the top of both vertical edges
  bool fixed_range;          // if true, use fixed_min/fixed_max instead of auto scaling
  int8_t fixed_min;          // lower bound when fixed_range is true
  int8_t fixed_max;          // upper bound when fixed_range is true
  GFont label_font;
  uint32_t icon_resource_id; // icon shown to the left of label_text in the header
  const char *label_text;
} GraphConfig;

Layer *graph_create(GRect bounds, GRect plot_bounds, GraphConfig config);
// values[0] is the most-recent sample; values[count-1] is the oldest.
// Negative values are treated as invalid (no reading available).
// Valid values must be in [0, 127].  Min/max are computed internally.
void graph_set_values(Layer *layer, const int8_t *values, uint8_t count);
void graph_set_label_text(Layer *layer, const char *text);
void graph_set_icon(Layer *layer, uint32_t resource_id);
void graph_destroy(Layer *layer);
