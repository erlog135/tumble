#include "bottom.h"
#include "../layout.h"

#define BOTTOM_TEXT_MAX_LEN 32
#define BOTTOM_ICON_TEXT_GAP 4
#define BOTTOM_EDGE_MARGIN 2

typedef struct {
  BottomConfig config;
  GBitmap *icon_bitmap;
  char text[BOTTOM_TEXT_MAX_LEN];
} BottomData;

static void bottom_update_proc(Layer *layer, GContext *ctx) {
  BottomData *data = layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);
  int16_t center_y = bounds.size.h / 2;

  GSize icon_size = GSizeZero;
  if (data->icon_bitmap && data->config.mode != BOTTOM_MODE_TEXT_ONLY) {
    icon_size = gbitmap_get_bounds(data->icon_bitmap).size;
  }

  GSize text_size = GSizeZero;
  if (data->config.mode != BOTTOM_MODE_ICON_ONLY && data->text[0] != '\0') {
    text_size = graphics_text_layout_get_content_size(
      data->text, data->config.font,
      GRect(0, 0, bounds.size.w, bounds.size.h),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  }

  int16_t icon_and_gap = (data->config.mode == BOTTOM_MODE_ICON_TEXT && icon_size.w > 0)
    ? icon_size.w + BOTTOM_ICON_TEXT_GAP : 0;
  int16_t total_width;
  switch (data->config.mode) {
    case BOTTOM_MODE_ICON_TEXT: total_width = icon_and_gap + text_size.w; break;
    case BOTTOM_MODE_ICON_ONLY: total_width = icon_size.w;                break;
    default:                    total_width = text_size.w;                break;
  }

  int16_t start_x = (data->config.align == BOTTOM_ALIGN_LEFT)
    ? BOTTOM_EDGE_MARGIN
    : bounds.size.w - total_width - BOTTOM_EDGE_MARGIN;

  int16_t cur_x = start_x;

  if (data->icon_bitmap && data->config.mode != BOTTOM_MODE_TEXT_ONLY) {
    GRect icon_rect = GRect(cur_x, center_y - icon_size.h / 2, icon_size.w, icon_size.h);
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    graphics_draw_bitmap_in_rect(ctx, data->icon_bitmap, icon_rect);
    cur_x += icon_and_gap;
  }

  if (data->config.mode != BOTTOM_MODE_ICON_ONLY && data->text[0] != '\0') {
    GRect text_rect = GRect(cur_x,
      center_y - text_size.h / 2 - SMALL_FONT_BOTTOM_MARGIN,
      text_size.w + 2, text_size.h);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, data->text, data->config.font, text_rect,
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
}

Layer *bottom_complication_create(GRect bounds, BottomConfig config) {
  Layer *layer = layer_create_with_data(bounds, sizeof(BottomData));
  BottomData *data = layer_get_data(layer);
  data->config = config;
  data->text[0] = '\0';

  data->icon_bitmap = (config.mode != BOTTOM_MODE_TEXT_ONLY && config.icon_resource_id != 0)
    ? gbitmap_create_with_resource(config.icon_resource_id)
    : NULL;

  layer_set_update_proc(layer, bottom_update_proc);
  return layer;
}

void bottom_complication_set_icon(Layer *layer, uint32_t resource_id) {
  BottomData *data = layer_get_data(layer);
  if (data->icon_bitmap) {
    gbitmap_destroy(data->icon_bitmap);
  }
  data->icon_bitmap = (resource_id != 0)
    ? gbitmap_create_with_resource(resource_id)
    : NULL;
  layer_mark_dirty(layer);
}

void bottom_complication_set_text(Layer *layer, const char *text) {
  BottomData *data = layer_get_data(layer);
  strncpy(data->text, text, BOTTOM_TEXT_MAX_LEN - 1);
  data->text[BOTTOM_TEXT_MAX_LEN - 1] = '\0';
  layer_mark_dirty(layer);
}

void bottom_complication_destroy(Layer *layer) {
  BottomData *data = layer_get_data(layer);
  if (data->icon_bitmap) {
    gbitmap_destroy(data->icon_bitmap);
  }
  layer_destroy(layer);
}
