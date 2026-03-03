#include "miniview.h"
#include "../layout.h"

typedef struct {
  MiniviewConfig config;
  GBitmap *icon_bitmap;
} MiniviewData;

#define BORDER_ARC_DEGREES 80
#define BORDER_DOT_WIDTH 3
#define BORDER_DECORATION_INNER_MARGIN 3

static void prv_draw_background(GContext *ctx, GPoint center, uint16_t radius) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, radius);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, radius - MINIVIEW_BORDER_SIZE);
}

static void prv_draw_border_decoration(GContext *ctx, GPoint center,
    uint16_t radius, uint16_t inner_radius) {
  uint16_t deco_radius = inner_radius + BORDER_DECORATION_INNER_MARGIN;
  uint16_t dot_length = MINIVIEW_BORDER_SIZE - BORDER_DECORATION_INNER_MARGIN;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);

  GRect arc_rect = GRect(center.x - deco_radius, center.y - deco_radius,
      2 * deco_radius, 2 * deco_radius);
  int32_t half_arc = DEG_TO_TRIGANGLE(BORDER_ARC_DEGREES / 2);

  // 4 arcs at N, E, S, W (0°, 90°, 180°, 270°), inset from inner edge
  int32_t arc_centers[] = {
    DEG_TO_TRIGANGLE(0),   /* North */
    DEG_TO_TRIGANGLE(90),  /* East */
    DEG_TO_TRIGANGLE(180), /* South */
    DEG_TO_TRIGANGLE(270), /* West */
  };
  for (int i = 0; i < 4; i++) {
    graphics_draw_arc(ctx, arc_rect, GOvalScaleModeFitCircle,
      arc_centers[i] - half_arc, arc_centers[i] + half_arc);
  }

  // 4 dots at exact compass positions, from inner margin to outer edge
  int16_t hw = BORDER_DOT_WIDTH / 2;
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(center.x - hw, center.y - radius,
      BORDER_DOT_WIDTH, dot_length), 0, GCornerNone); /* N */
  graphics_fill_rect(ctx, GRect(center.x + deco_radius, center.y - hw,
      dot_length, BORDER_DOT_WIDTH), 0, GCornerNone); /* E */
  graphics_fill_rect(ctx, GRect(center.x - hw, center.y + deco_radius,
      BORDER_DOT_WIDTH, dot_length), 0, GCornerNone); /* S */
  graphics_fill_rect(ctx, GRect(center.x - radius, center.y - hw,
      dot_length, BORDER_DOT_WIDTH), 0, GCornerNone); /* W */
}

static void prv_draw_icon_centered_at(GContext *ctx, GBitmap *bitmap, GPoint center) {
  GSize sz = gbitmap_get_bounds(bitmap).size;
  GRect rect = GRect(center.x - sz.w / 2, center.y - sz.h / 2, sz.w, sz.h);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, bitmap, rect);
}

static void miniview_update_proc(Layer *layer, GContext *ctx) {
  MiniviewData *data = layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  uint16_t radius = bounds.size.w / 2;
  uint16_t inner_radius = radius - MINIVIEW_BORDER_SIZE;

  prv_draw_background(ctx, center, radius);
  prv_draw_border_decoration(ctx, center, radius, inner_radius);

  switch (data->config.mode) {
    case MINIVIEW_MODE_TEXT_STACK:
      graphics_context_set_text_color(ctx, GColorBlack);
      graphics_draw_text(ctx, "THU", data->config.tiny_font,
        data->config.tiny_text_bounds,
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
      graphics_draw_text(ctx, "12", data->config.small_font,
        data->config.small_text_bounds,
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
      break;

    case MINIVIEW_MODE_ICON_TEXT: {
      if (data->icon_bitmap) {
        GSize sz = gbitmap_get_bounds(data->icon_bitmap).size;
        // Place icon in upper half, vertically centered between border and midpoint
        int16_t icon_y = MINIVIEW_BORDER_SIZE + (center.y - MINIVIEW_BORDER_SIZE - sz.h) / 2;
        GPoint icon_center = GPoint(center.x, icon_y + sz.h / 2);
        prv_draw_icon_centered_at(ctx, data->icon_bitmap, icon_center);
      }
      graphics_context_set_text_color(ctx, GColorBlack);
      graphics_draw_text(ctx, "12", data->config.small_font,
        data->config.small_text_bounds,
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
      break;
    }

    case MINIVIEW_MODE_ICON_CENTER:
      if (data->icon_bitmap) {
        prv_draw_icon_centered_at(ctx, data->icon_bitmap, center);
      }
      break;

    case MINIVIEW_MODE_CLOCK_DOTS: {
      int16_t dot_ring_radius = inner_radius - 4;

      // Fill bottom half with black. In Pebble's system 0°=top, clockwise,
      // so the bottom half (y > center) runs from 90° to 270°.
      GRect inner_rect = GRect(
        MINIVIEW_BORDER_SIZE, MINIVIEW_BORDER_SIZE,
        bounds.size.w - 2 * MINIVIEW_BORDER_SIZE,
        bounds.size.h - 2 * MINIVIEW_BORDER_SIZE
      );
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_fill_radial(ctx, inner_rect, GOvalScaleModeFitCircle,
        inner_radius, DEG_TO_TRIGANGLE(90), DEG_TO_TRIGANGLE(270));

      // Horizontal divider spanning the full inner circle diameter
      graphics_context_set_stroke_color(ctx, GColorBlack);
      graphics_context_set_stroke_width(ctx, 2);
      graphics_draw_line(ctx,
        GPoint(center.x - inner_radius, center.y),
        GPoint(center.x + inner_radius, center.y));
        
        // 14 evenly spaced dots, colored by actual position: black on white top,
        // white on black bottom. With dots starting at 12 o'clock the split is
        // i=0..3 and i=11..13 (top) vs i=4..10 (bottom), so check y directly.
        for (int i = 0; i < 14; i++) {
          int32_t angle = TRIG_MAX_ANGLE * i / 14;
          GPoint dot = GPoint(
            center.x + (int16_t)(sin_lookup(angle) * dot_ring_radius / TRIG_MAX_RATIO),
            center.y - (int16_t)(cos_lookup(angle) * dot_ring_radius / TRIG_MAX_RATIO)
          );
        
        graphics_context_set_fill_color(ctx, dot.y > center.y ? GColorWhite : GColorBlack);
        graphics_fill_rect(ctx, GRect(dot.x - 1, dot.y - 1, 2, 2), 0, GCornerNone);
      }

      // Icon drawn on the dot ring at the given offset from center
      if (data->icon_bitmap) {
        GPoint icon_pos = GPoint(
          center.x + data->config.icon_offset.x,
          center.y + data->config.icon_offset.y
        );
        prv_draw_icon_centered_at(ctx, data->icon_bitmap, icon_pos);
      }
      break;
    }

    case MINIVIEW_MODE_ICON_COLUMN: {
      if (data->icon_bitmap) {
        int16_t spread = inner_radius / 2;
        int16_t offsets_y[3] = { -spread, 0, spread };
        for (int i = 0; i < 3; i++) {
          GPoint icon_center = GPoint(center.x, center.y + offsets_y[i]);
          prv_draw_icon_centered_at(ctx, data->icon_bitmap, icon_center);
        }
      }
      break;
    }
  }
}

Layer *miniview_create(GRect bounds, MiniviewConfig config) {
  Layer *layer = layer_create_with_data(bounds, sizeof(MiniviewData));
  MiniviewData *data = layer_get_data(layer);
  data->config = config;

  bool needs_icon = config.mode == MINIVIEW_MODE_ICON_TEXT
                 || config.mode == MINIVIEW_MODE_ICON_CENTER
                 || config.mode == MINIVIEW_MODE_CLOCK_DOTS
                 || config.mode == MINIVIEW_MODE_ICON_COLUMN;

  data->icon_bitmap = (needs_icon && config.icon_resource_id != 0)
    ? gbitmap_create_with_resource(config.icon_resource_id)
    : NULL;

  layer_set_update_proc(layer, miniview_update_proc);
  return layer;
}

void miniview_destroy(Layer *layer) {
  MiniviewData *data = layer_get_data(layer);
  if (data->icon_bitmap) {
    gbitmap_destroy(data->icon_bitmap);
  }
  layer_destroy(layer);
}
