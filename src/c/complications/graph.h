#pragma once
#include <pebble.h>

// How far axis tick marks extend inward (pixels)
#define GRAPH_MARKER_SIZE 3
// Gap between the header icon and label text (pixels)
#define GRAPH_HEADER_ICON_TEXT_GAP 4
// Maximum number of data values the graph can hold
#define GRAPH_MAX_VALUES 48

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
  GFont label_font;
  uint32_t icon_resource_id; // icon shown to the left of label_text in the header
  const char *label_text;
} GraphConfig;

Layer *graph_create(GRect bounds, GRect plot_bounds, GraphConfig config);
void graph_set_values(Layer *layer, const int16_t *values, uint8_t count,
                      int16_t min_val, int16_t max_val);
void graph_destroy(Layer *layer);
