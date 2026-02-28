#include "graph.h"
#include "../layout.h"

typedef struct {
  GraphConfig config;
  GRect plot_bounds;
  GBitmap *icon_bitmap;
  int16_t values[GRAPH_MAX_VALUES];
  uint8_t value_count;
  int16_t min_val;
  int16_t max_val;
} GraphData;

static int16_t prv_value_to_y(int16_t value, int16_t min_val, int16_t max_val, GRect plot) {
  if (max_val == min_val) return plot.origin.y + plot.size.h - 1;
  return plot.origin.y + plot.size.h - 1
    - (int16_t)((int32_t)(value - min_val) * (plot.size.h - 1) / (max_val - min_val));
}

static int16_t prv_index_to_x(uint8_t idx, uint8_t count, GRect plot) {
  if (count <= 1) return plot.origin.x + plot.size.w / 2;
  return plot.origin.x + (int16_t)((int32_t)idx * (plot.size.w - 1) / (count - 1));
}

static void prv_draw_header(GContext *ctx, GraphData *data, GRect bounds) {
  GSize icon_sz = GSizeZero;
  if (data->icon_bitmap) {
    icon_sz = gbitmap_get_bounds(data->icon_bitmap).size;
  }

  const char *label = data->config.label_text ? data->config.label_text : "";
  GSize text_sz = graphics_text_layout_get_content_size(
    label, data->config.label_font,
    GRect(0, 0, bounds.size.w, TINY_FONT_HEIGHT),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);

  int16_t gap = (icon_sz.w > 0 && text_sz.w > 0) ? GRAPH_HEADER_ICON_TEXT_GAP : 0;
  int16_t group_w = icon_sz.w + gap + text_sz.w;
  int16_t center_y = TINY_FONT_HEIGHT / 2;
  int16_t cur_x = (bounds.size.w - group_w) / 2;

  if (data->icon_bitmap) {
    GRect icon_rect = GRect(cur_x, center_y - icon_sz.h / 2, icon_sz.w, icon_sz.h);
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, data->icon_bitmap, icon_rect);
    cur_x += icon_sz.w + gap;
  }

  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, label, data->config.label_font,
    GRect(cur_x, center_y - text_sz.h / 2, text_sz.w + 2, text_sz.h),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void prv_draw_axes(GContext *ctx, GraphData *data) {
  GRect plot = data->plot_bounds;
  int16_t x1 = plot.origin.x;
  int16_t x2 = plot.origin.x + plot.size.w - 1;
  int16_t y_top = plot.origin.y;
  int16_t y_bot = plot.origin.y + plot.size.h - 1;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);

  graphics_draw_line(ctx, GPoint(x1, y_top), GPoint(x1, y_bot));
  graphics_draw_line(ctx, GPoint(x2, y_top), GPoint(x2, y_bot));
  graphics_draw_line(ctx, GPoint(x1, y_bot), GPoint(x2, y_bot));

  // Horizontal tick marks inward from the bottom edge
  uint8_t hm = data->config.h_markers;
  for (uint8_t i = 1; i <= hm; i++) {
    int16_t tx = x1 + (int16_t)((int32_t)i * (x2 - x1) / (hm + 1));
    graphics_draw_line(ctx, GPoint(tx, y_bot), GPoint(tx, y_bot - GRAPH_MARKER_SIZE));
  }

  // Vertical tick marks inward from both vertical edges
  uint8_t vm = data->config.v_markers;
  for (uint8_t i = 1; i <= vm; i++) {
    int16_t ty = y_bot - (int16_t)((int32_t)i * (y_bot - y_top) / (vm + 1));
    graphics_draw_line(ctx, GPoint(x1, ty), GPoint(x1 + GRAPH_MARKER_SIZE, ty));
    graphics_draw_line(ctx, GPoint(x2, ty), GPoint(x2 - GRAPH_MARKER_SIZE, ty));
  }

  // Top lip: identical to a v_marker but at y_top (y offset = 0)
  if (data->config.top_lip) {
    graphics_draw_line(ctx, GPoint(x1, y_top), GPoint(x1 + GRAPH_MARKER_SIZE, y_top));
    graphics_draw_line(ctx, GPoint(x2, y_top), GPoint(x2 - GRAPH_MARKER_SIZE, y_top));
  }
}

static void prv_draw_data(GContext *ctx, GraphData *data) {
  if (data->value_count == 0) return;

  GRect plot = data->plot_bounds;
  int16_t y_bot = plot.origin.y + plot.size.h - 1;
  int16_t min_v = data->min_val;
  int16_t max_v = data->max_val;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);

  switch (data->config.style) {
    case GRAPH_STYLE_LINE: {
      for (uint8_t i = 1; i < data->value_count; i++) {
        GPoint a = GPoint(
          prv_index_to_x(i - 1, data->value_count, plot),
          prv_value_to_y(data->values[i - 1], min_v, max_v, plot));
        GPoint b = GPoint(
          prv_index_to_x(i, data->value_count, plot),
          prv_value_to_y(data->values[i], min_v, max_v, plot));
        graphics_draw_line(ctx, a, b);
      }
      break;
    }

    case GRAPH_STYLE_BARS: {
      for (uint8_t i = 0; i < data->value_count; i++) {
        int16_t bx = prv_index_to_x(i, data->value_count, plot);
        int16_t by = prv_value_to_y(data->values[i], min_v, max_v, plot);
        graphics_draw_line(ctx, GPoint(bx, y_bot), GPoint(bx, by));
      }
      break;
    }

    case GRAPH_STYLE_FILLED: {
      // Build a closed polygon: bottom-left corner → data points → bottom-right corner
      GPoint points[GRAPH_MAX_VALUES + 2];
      uint16_t n = 0;
      points[n++] = GPoint(prv_index_to_x(0, data->value_count, plot), y_bot);
      for (uint8_t i = 0; i < data->value_count; i++) {
        points[n++] = GPoint(
          prv_index_to_x(i, data->value_count, plot),
          prv_value_to_y(data->values[i], min_v, max_v, plot));
      }
      points[n++] = GPoint(
        prv_index_to_x(data->value_count - 1, data->value_count, plot), y_bot);
      GPathInfo path_info = { .num_points = n, .points = points };
      GPath *path = gpath_create(&path_info);
      graphics_context_set_fill_color(ctx, GColorWhite);
      gpath_draw_filled(ctx, path);
      gpath_destroy(path);
      break;
    }
  }
}

static void graph_update_proc(Layer *layer, GContext *ctx) {
  GraphData *data = layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);

  prv_draw_header(ctx, data, bounds);
  prv_draw_axes(ctx, data);
  prv_draw_data(ctx, data);
}

Layer *graph_create(GRect bounds, GRect plot_bounds, GraphConfig config) {
  Layer *layer = layer_create_with_data(bounds, sizeof(GraphData));
  GraphData *data = layer_get_data(layer);
  data->config = config;
  data->plot_bounds = plot_bounds;
  data->value_count = 0;
  data->min_val = 0;
  data->max_val = 0;

  data->icon_bitmap = (config.icon_resource_id != 0)
    ? gbitmap_create_with_resource(config.icon_resource_id)
    : NULL;

  layer_set_update_proc(layer, graph_update_proc);
  return layer;
}

void graph_set_values(Layer *layer, const int16_t *values, uint8_t count,
                      int16_t min_val, int16_t max_val) {
  GraphData *data = layer_get_data(layer);
  uint8_t n = count < GRAPH_MAX_VALUES ? count : GRAPH_MAX_VALUES;
  for (uint8_t i = 0; i < n; i++) {
    data->values[i] = values[i];
  }
  data->value_count = n;
  data->min_val = min_val;
  data->max_val = max_val;
  layer_mark_dirty(layer);
}

void graph_destroy(Layer *layer) {
  GraphData *data = layer_get_data(layer);
  if (data->icon_bitmap) {
    gbitmap_destroy(data->icon_bitmap);
  }
  layer_destroy(layer);
}
