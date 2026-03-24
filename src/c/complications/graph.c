#include "graph.h"
#include "../layout.h"
#include <string.h>

typedef struct {
  GraphConfig config;
  GRect plot_bounds;
  GBitmap *icon_bitmap;
  int8_t values[GRAPH_MAX_VALUES]; // values[0] = most recent, values[count-1] = oldest
  uint8_t value_count;
  char label_buffer[GRAPH_LABEL_MAX];
} GraphData;

// display_idx 0 = leftmost (oldest), display_idx count-1 = rightmost (newest)
static int16_t prv_index_to_x(uint8_t display_idx, uint8_t count, GRect plot) {
  if (count <= 1) return plot.origin.x + plot.size.w / 2;
  return plot.origin.x + (int16_t)((int32_t)display_idx * (plot.size.w - 1) / (count - 1));
}

// Convert array index (0=newest) to display index (0=leftmost/oldest)
static inline uint8_t prv_arr_to_disp(uint8_t arr_idx, uint8_t count) {
  return count - 1 - arr_idx;
}

static int16_t prv_value_to_y(int8_t value, int16_t mn, int16_t mx, GRect plot) {
  if (mx == mn) return plot.origin.y + plot.size.h - 1;
  return plot.origin.y + plot.size.h - 1
    - (int16_t)((int32_t)(value - mn) * (plot.size.h - 1) / (mx - mn));
}

static void prv_draw_header(GContext *ctx, GraphData *data, GRect bounds) {
  GSize icon_sz = GSizeZero;
  if (data->icon_bitmap) {
    icon_sz = gbitmap_get_bounds(data->icon_bitmap).size;
  }

  const char *label = (data->label_buffer[0] != '\0')
    ? data->label_buffer
    : (data->config.label_text ? data->config.label_text : "");
  GSize text_sz = graphics_text_layout_get_content_size(
    label, data->config.label_font,
    GRect(0, 0, bounds.size.w, TINY_FONT_HEIGHT),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);

  int16_t gap = (icon_sz.w > 0 && text_sz.w > 0) ? GRAPH_HEADER_ICON_TEXT_GAP : 0;
  int16_t group_w = icon_sz.w + gap + text_sz.w;
  int16_t center_y = TINY_FONT_HEIGHT / 2;
#ifdef PBL_ROUND
  int16_t cur_x = bounds.size.w - group_w;
#else
  int16_t cur_x = (bounds.size.w - group_w) / 2;
#endif

  if (data->icon_bitmap) {
    GRect icon_rect = GRect(cur_x, center_y - icon_sz.h / 2, icon_sz.w, icon_sz.h);
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    graphics_draw_bitmap_in_rect(ctx, data->icon_bitmap, icon_rect);
    cur_x += icon_sz.w + gap;
  }

  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, label, data->config.label_font,
    GRect(cur_x, center_y - text_sz.h / 2 - SMALL_FONT_BOTTOM_MARGIN,
      text_sz.w + 2, text_sz.h),
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

  uint8_t hm = data->config.h_markers;
  for (uint8_t i = 1; i <= hm; i++) {
    int16_t tx = x1 + (int16_t)((int32_t)i * (x2 - x1) / (hm + 1));
    graphics_draw_line(ctx, GPoint(tx, y_bot), GPoint(tx, y_bot - GRAPH_H_MARKER_SIZE));
  }

  uint8_t vm = data->config.v_markers;
  for (uint8_t i = 1; i <= vm; i++) {
    int16_t ty = y_bot - (int16_t)((int32_t)i * (y_bot - y_top) / (vm + 1));
    graphics_draw_line(ctx, GPoint(x1, ty), GPoint(x1 + GRAPH_V_MARKER_SIZE, ty));
    graphics_draw_line(ctx, GPoint(x2, ty), GPoint(x2 - GRAPH_V_MARKER_SIZE, ty));
  }

  if (data->config.top_lip) {
    graphics_draw_line(ctx, GPoint(x1, y_top), GPoint(x1 + GRAPH_V_MARKER_SIZE, y_top));
    graphics_draw_line(ctx, GPoint(x2, y_top), GPoint(x2 - GRAPH_V_MARKER_SIZE, y_top));
  }
}

static void prv_draw_data(GContext *ctx, GraphData *data) {
  uint8_t count = data->value_count;
  if (count == 0) return;

  GRect plot = data->plot_bounds;
  int16_t y_bot = plot.origin.y + plot.size.h - 1;

  // Compute min/max — either fixed by config or derived from the data
  int16_t mn, mx;
  if (data->config.fixed_range) {
    mn = data->config.fixed_min;
    mx = data->config.fixed_max;
  } else {
    mn = 127; mx = 0;
    for (uint8_t i = 0; i < count; i++) {
      if (data->values[i] >= 0) {
        if (data->values[i] < mn) mn = data->values[i];
        if (data->values[i] > mx) mx = data->values[i];
      }
    }
    if (mn > mx) { mn = 0; mx = 127; } // all invalid — nothing meaningful to draw
  }

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);

  switch (data->config.style) {

    case GRAPH_STYLE_LINE: {
      // Draw a segment only when both adjacent samples are valid.
      // Invalid samples break the line — no interpolation across gaps.
      for (uint8_t i = 1; i < count; i++) {
        if (data->values[i - 1] < 0 || data->values[i] < 0) continue;
        GPoint a = GPoint(
          prv_index_to_x(prv_arr_to_disp(i - 1, count), count, plot),
          prv_value_to_y(data->values[i - 1], mn, mx, plot));
        GPoint b = GPoint(
          prv_index_to_x(prv_arr_to_disp(i, count), count, plot),
          prv_value_to_y(data->values[i], mn, mx, plot));
        graphics_draw_line(ctx, a, b);
      }
      break;
    }

    case GRAPH_STYLE_BARS: {
      // Bars are anchored to the right edge (newest data) and spaced at fixed
      // intervals leftward. Capturing all data is lowest priority — bars that
      // fall outside the left edge are simply skipped.
      int16_t step = (int16_t)(GRAPH_BAR_WIDTH + GRAPH_BAR_SPACING);
      int16_t x_right = plot.origin.x + plot.size.w - 1;

      graphics_context_set_stroke_width(ctx, GRAPH_BAR_WIDTH);
      for (uint8_t i = 0; i < count; i++) {
        // i == 0 is newest; place it at the right, older bars extend left
        int16_t bx = x_right - (int16_t)i * step;
        if (bx < plot.origin.x) break; // off the left edge — stop
        if (data->values[i] < 0) continue;
        int16_t by = prv_value_to_y(data->values[i], mn, mx, plot);
        graphics_draw_line(ctx, GPoint(bx, y_bot), GPoint(bx, by));
      }
      graphics_context_set_stroke_width(ctx, 1);
      break;
    }

    case GRAPH_STYLE_FILLED: {
      // For each valid segment, walk every pixel column from y_bot up to the
      // interpolated line height and stamp a checkerboard (every other pixel white).
      for (uint8_t i = 1; i < count; i++) {
        if (data->values[i - 1] < 0 || data->values[i] < 0) continue;

        // ax/ay = older sample (left endpoint), bx/by = newer sample (right endpoint)
        int16_t ax = prv_index_to_x(prv_arr_to_disp(i,     count), count, plot);
        int16_t ay = prv_value_to_y(data->values[i],     mn, mx, plot);
        int16_t bx = prv_index_to_x(prv_arr_to_disp(i - 1, count), count, plot);
        int16_t by = prv_value_to_y(data->values[i - 1], mn, mx, plot);

        for (int16_t x = ax; x <= bx; x++) {
          int16_t ceil_y = (bx > ax)
            ? ay + (int16_t)((int32_t)(by - ay) * (x - ax) / (bx - ax))
            : (ay < by ? ay : by);
          for (int16_t y = y_bot; y >= ceil_y; y--) {
            if (((x / GRAPH_FILL_CHECKER_SIZE) + (y / GRAPH_FILL_CHECKER_SIZE)) % 2 == 0) {
              graphics_draw_pixel(ctx, GPoint(x, y));
            }
          }
        }
      }
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

  data->icon_bitmap = (config.icon_resource_id != 0)
    ? gbitmap_create_with_resource(config.icon_resource_id)
    : NULL;

  data->label_buffer[0] = '\0';

  layer_set_update_proc(layer, graph_update_proc);
  return layer;
}

void graph_set_values(Layer *layer, const int8_t *values, uint8_t count) {
  GraphData *data = layer_get_data(layer);
  uint8_t n = count < GRAPH_MAX_VALUES ? count : GRAPH_MAX_VALUES;
  memcpy(data->values, values, n * sizeof(int8_t));
  data->value_count = n;
  layer_mark_dirty(layer);
}

void graph_set_icon(Layer *layer, uint32_t resource_id) {
  GraphData *data = layer_get_data(layer);
  if (data->icon_bitmap) {
    gbitmap_destroy(data->icon_bitmap);
  }
  data->icon_bitmap = (resource_id != 0)
    ? gbitmap_create_with_resource(resource_id)
    : NULL;
  layer_mark_dirty(layer);
}

void graph_set_label_text(Layer *layer, const char *text) {
  GraphData *data = layer_get_data(layer);
  if (text) {
    strncpy(data->label_buffer, text, GRAPH_LABEL_MAX - 1);
    data->label_buffer[GRAPH_LABEL_MAX - 1] = '\0';
  } else {
    data->label_buffer[0] = '\0';
  }
  layer_mark_dirty(layer);
}

void graph_destroy(Layer *layer) {
  GraphData *data = layer_get_data(layer);
  if (data->icon_bitmap) {
    gbitmap_destroy(data->icon_bitmap);
  }
  layer_destroy(layer);
}
