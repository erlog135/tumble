#include "miniview.h"
#include "../layout.h"

typedef struct {
  GRect tiny_text_bounds;
  GRect small_text_bounds;
  GFont tiny_font;
  GFont small_font;
} MiniviewData;

static void miniview_update_proc(Layer *layer, GContext *ctx) {
  MiniviewData *data = layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  uint16_t radius = bounds.size.w / 2;

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, radius);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, radius - MINIVIEW_BORDER_SIZE);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, "THU", data->tiny_font,
    data->tiny_text_bounds,
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "12", data->small_font,
    data->small_text_bounds,
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

Layer *miniview_create(GRect bounds, GRect tiny_text_bounds, GRect small_text_bounds,
                       GFont tiny_font, GFont small_font) {
  Layer *layer = layer_create_with_data(bounds, sizeof(MiniviewData));
  MiniviewData *data = layer_get_data(layer);
  data->tiny_text_bounds = tiny_text_bounds;
  data->small_text_bounds = small_text_bounds;
  data->tiny_font = tiny_font;
  data->small_font = small_font;
  layer_set_update_proc(layer, miniview_update_proc);
  return layer;
}

void miniview_destroy(Layer *layer) {
  layer_destroy(layer);
}
