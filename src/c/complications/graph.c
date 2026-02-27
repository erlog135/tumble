#include "graph.h"
#include "../layout.h"

typedef struct {
  GRect plot_bounds;
  GFont label_font;
} GraphData;

static void graph_update_proc(Layer *layer, GContext *ctx) {
  GraphData *data = layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);
  GRect plot_bounds = data->plot_bounds;

  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, "GRAPH", data->label_font,
    GRect(0, 0, bounds.size.w, TINY_FONT_HEIGHT),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  int16_t y_top = plot_bounds.origin.y;
  int16_t y_bot = plot_bounds.origin.y + plot_bounds.size.h - 1;
  int16_t x1 = plot_bounds.origin.x;
  int16_t x2 = plot_bounds.origin.x + plot_bounds.size.w - 1;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, GPoint(x1, y_top), GPoint(x1, y_bot));
  graphics_draw_line(ctx, GPoint(x2, y_top), GPoint(x2, y_bot));
  graphics_draw_line(ctx, GPoint(x1, y_bot), GPoint(x2, y_bot));
}

Layer *graph_create(GRect bounds, GRect plot_bounds, GFont label_font) {
  Layer *layer = layer_create_with_data(bounds, sizeof(GraphData));
  GraphData *data = layer_get_data(layer);
  data->plot_bounds = plot_bounds;
  data->label_font = label_font;
  layer_set_update_proc(layer, graph_update_proc);
  return layer;
}

void graph_destroy(Layer *layer) {
  layer_destroy(layer);
}
