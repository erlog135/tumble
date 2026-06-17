#include "miniview.h"
#include "../layout.h"
#include "../providers/providers.h"
#include "../settings.h"
#include <string.h>

typedef struct {
  MiniviewConfig config;
  GRect small_text_bounds;
  GRect medium_text_bounds;
  GFont small_font;
  GFont medium_font;
  GBitmap *icon_bitmap;
  GBitmap *column_bitmaps[3];
  char small_text[12];
  char medium_text[12];
} MiniviewData;

#define BORDER_ARC_DEGREES 80
#define BORDER_DOT_WIDTH 3
#define BORDER_DECORATION_INNER_MARGIN 3
/** Inset compass ticks only, so they do not touch the outer black ring edge. */
#define BORDER_DECORATION_DOT_OUTER_MARGIN 1

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
#define DOT_RING_INSET 10  // inset from inner_radius to the centre of the dot/icon ring
#else
#define DOT_RING_INSET 7
#endif




/** @param white_face true: white inner fill; false: black inner fill. */
static void prv_draw_background_fill(GContext *ctx, GPoint center, uint16_t radius, bool white_face) {
  graphics_context_set_fill_color(ctx, white_face ? GColorWhite : GColorBlack);
  graphics_fill_circle(ctx, center, radius - MINIVIEW_BORDER_SIZE);
}

static void prv_draw_border_outline(GContext *ctx, GPoint center, uint16_t radius) {
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, MINIVIEW_BORDER_SIZE);
  graphics_draw_circle(ctx, center, radius - MINIVIEW_BORDER_SIZE / 2);
}

static void prv_draw_border_decoration(GContext *ctx, GPoint center,
    uint16_t radius, uint16_t inner_radius) {
  uint16_t deco_radius = inner_radius + BORDER_DECORATION_INNER_MARGIN;
  uint16_t dot_length = MINIVIEW_BORDER_SIZE - BORDER_DECORATION_INNER_MARGIN;
  uint16_t dot_m = BORDER_DECORATION_DOT_OUTER_MARGIN;
  if (dot_length <= dot_m) {
    dot_m = 0;
  }
  uint16_t dot_draw_len = dot_length - dot_m;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, MINIVIEW_DECORATION_WIDTH);

  GRect arc_rect = GRect(center.x - deco_radius, center.y - deco_radius,
      2 * deco_radius + 1, 2 * deco_radius + 1);
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

  // 4 dots at compass positions; shortened on the outer side so they clear the ring edge
  int16_t hw = BORDER_DOT_WIDTH / 2;
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(center.x - hw, center.y - radius + dot_m,
      BORDER_DOT_WIDTH, dot_draw_len), 0, GCornerNone); /* N */
  graphics_fill_rect(ctx, GRect(center.x + deco_radius + 1, center.y - hw,
      dot_draw_len, BORDER_DOT_WIDTH), 0, GCornerNone); /* E */
  graphics_fill_rect(ctx, GRect(center.x - hw, center.y + deco_radius + 1,
      BORDER_DOT_WIDTH, dot_draw_len), 0, GCornerNone); /* S */
  graphics_fill_rect(ctx, GRect(center.x - radius + dot_m, center.y - hw,
      dot_draw_len, BORDER_DOT_WIDTH), 0, GCornerNone); /* W */
}

static void prv_draw_icon_centered_at(GContext *ctx, GBitmap *bitmap, GPoint center,
                                      GCompOp comp_op) {
  GSize sz = gbitmap_get_bounds(bitmap).size;
  GRect rect = GRect(center.x - sz.w / 2, center.y - sz.h / 2, sz.w, sz.h);
  graphics_context_set_compositing_mode(ctx, comp_op);
  graphics_draw_bitmap_in_rect(ctx, bitmap, rect);
}

/** Count UTF-8 code points (e.g. U+00B0 degree is one character, not two bytes). */
static size_t prv_utf8_char_count(const char *s) {
  size_t n = 0;
  for (; *s; s++) {
    if ((*s & 0xC0) != 0x80) n++;
  }
  return n;
}

/** Medium line uses small font when text is longer than 3 characters (fits miniview). */
static GFont prv_font_for_medium_line(const MiniviewData *data, const char *text) {
  if (prv_utf8_char_count(text) > 3) {
    return data->small_font;
  }
  return data->medium_font;
}

static GColor prv_miniview_text_color(void) {
  return settings_get()->black_miniview_bg ? GColorWhite : GColorBlack;
}

static GCompOp prv_miniview_icon_op(void) {
  return settings_get()->black_miniview_bg ? GCompOpAssign : GCompOpAssignInverted;
}

static void miniview_update_proc(Layer *layer, GContext *ctx) {
  MiniviewData *data = layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  uint16_t radius = bounds.size.w / 2;
  uint16_t inner_radius = radius - MINIVIEW_BORDER_SIZE;

  graphics_context_set_antialiased(ctx, false);
  bool sun_clock = (data->config.mode == MINIVIEW_MODE_CLOCK_DOTS);
  bool white_face = sun_clock || !settings_get()->black_miniview_bg;
  if (data->config.moon_phase) {
    white_face = false;
  }
  prv_draw_background_fill(ctx, center, radius, white_face);

  switch (data->config.mode) {
    case MINIVIEW_MODE_TEXT_STACK: {
      int16_t c1 = (data->small_text[0] != '\0') ? SMALL_FONT_CAP_HEIGHT : 0;
      GFont text_font = prv_font_for_medium_line(data, data->medium_text);
      int16_t c2 = (data->medium_text[0] != '\0') ? ((text_font == data->medium_font) ? MEDIUM_FONT_CAP_HEIGHT : SMALL_FONT_CAP_HEIGHT) : 0;

      int16_t h1 = SMALL_FONT_HEIGHT;
      int16_t h2 = (text_font == data->medium_font) ? MEDIUM_FONT_HEIGHT : SMALL_FONT_HEIGHT;

      int16_t box_y1 = 0;
      int16_t box_y2 = 0;

      if (c1 > 0 && c2 > 0) {
        int16_t total_h = c1 + c2 + MINIVIEW_ELEMENT_PADDING;
        int16_t top_y = center.y - total_h / 2;
        box_y1 = top_y + c1 - h1;
        box_y2 = top_y + c1 + MINIVIEW_ELEMENT_PADDING + c2 - h2;
      } else if (c1 > 0) {
        box_y1 = center.y - h1 + c1 / 2;
      } else if (c2 > 0) {
        box_y2 = center.y - h2 + c2 / 2;
      }

      if (c1 > 0) {
        graphics_context_set_text_color(ctx, prv_miniview_text_color());
        graphics_draw_text(ctx, data->small_text, data->small_font,
          GRect(0, box_y1, bounds.size.w, h1),
          GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
      }
      if (c2 > 0) {
        graphics_context_set_text_color(ctx, prv_miniview_text_color());
        graphics_draw_text(ctx, data->medium_text, text_font,
          GRect(0, box_y2, bounds.size.w, h2),
          GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
      }
      break;
    }

    case MINIVIEW_MODE_ICON_TEXT: {
      int16_t h1 = data->icon_bitmap ? gbitmap_get_bounds(data->icon_bitmap).size.h : 0;
      GFont text_font = prv_font_for_medium_line(data, data->medium_text);
      int16_t c2 = (data->medium_text[0] != '\0') ? ((text_font == data->medium_font) ? MEDIUM_FONT_CAP_HEIGHT : SMALL_FONT_CAP_HEIGHT) : 0;
      int16_t h2 = (data->medium_text[0] != '\0') ? ((text_font == data->medium_font) ? MEDIUM_FONT_HEIGHT : SMALL_FONT_HEIGHT) : 0;

      int16_t cy_icon = 0;
      int16_t box_y2 = 0;

      if (h1 > 0 && c2 > 0) {
        int16_t total_h = h1 + c2 + MINIVIEW_ELEMENT_PADDING;
        int16_t top_y = center.y - total_h / 2;
        cy_icon = top_y + h1 / 2;
        box_y2 = top_y + h1 + MINIVIEW_ELEMENT_PADDING + c2 - h2;
      } else if (h1 > 0) {
        cy_icon = center.y;
      } else if (c2 > 0) {
        box_y2 = center.y - h2 + c2 / 2;
      }

      if (h1 > 0 && data->icon_bitmap) {
        prv_draw_icon_centered_at(ctx, data->icon_bitmap,
          GPoint(center.x, cy_icon), prv_miniview_icon_op());
      }
      if (c2 > 0) {
        graphics_context_set_text_color(ctx, prv_miniview_text_color());
        graphics_draw_text(ctx, data->medium_text, text_font,
          GRect(0, box_y2, bounds.size.w, h2),
          GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
      }
      break;
    }

    case MINIVIEW_MODE_ICON_CENTER:
      if (data->icon_bitmap) {
        GCompOp icon_op = data->config.moon_phase ? GCompOpAssign : prv_miniview_icon_op();
        prv_draw_icon_centered_at(ctx, data->icon_bitmap, center, icon_op);
      }
      break;

    case MINIVIEW_MODE_CLOCK_DOTS: {
      int16_t dot_ring_radius = inner_radius - DOT_RING_INSET;

      // Fill bottom half with black. In Pebble's system 0°=top, clockwise,
      // so the bottom half (y > center) runs from 90° to 270°.
      GRect inner_rect = GRect(
        MINIVIEW_BORDER_SIZE/2,
        MINIVIEW_BORDER_SIZE/2,
        bounds.size.w - MINIVIEW_BORDER_SIZE/2,
        bounds.size.h - MINIVIEW_BORDER_SIZE/2
      );
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_fill_radial(ctx, inner_rect, GOvalScaleModeFitCircle,
        radius, DEG_TO_TRIGANGLE(90), DEG_TO_TRIGANGLE(270));

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
        prv_draw_icon_centered_at(ctx, data->icon_bitmap, icon_pos, GCompOpSet);
      }
      break;
    }

    case MINIVIEW_MODE_ICON_COLUMN: {
      int16_t h0 = data->column_bitmaps[0] ? gbitmap_get_bounds(data->column_bitmaps[0]).size.h : 0;
      int16_t h1 = data->column_bitmaps[1] ? gbitmap_get_bounds(data->column_bitmaps[1]).size.h : 0;
      int16_t h2 = data->column_bitmaps[2] ? gbitmap_get_bounds(data->column_bitmaps[2]).size.h : 0;

      int16_t cy[3] = {0};

      if (h1 > 0) {
        cy[1] = center.y;
        if (h0 > 0) {
          cy[0] = center.y - (h0 / 2 + h1 / 2 + MINIVIEW_ELEMENT_PADDING);
        }
        if (h2 > 0) {
          cy[2] = center.y + (h1 / 2 + h2 / 2 + MINIVIEW_ELEMENT_PADDING);
        }
      } else {
        if (h0 > 0 && h2 > 0) {
          int16_t total_h = h0 + h2 + MINIVIEW_ELEMENT_PADDING;
          int16_t top_y = center.y - total_h / 2;
          cy[0] = top_y + h0 / 2;
          cy[2] = top_y + h0 + MINIVIEW_ELEMENT_PADDING + h2 / 2;
        } else if (h0 > 0) {
          cy[0] = center.y;
        } else if (h2 > 0) {
          cy[2] = center.y;
        }
      }

      for (int i = 0; i < 3; i++) {
        if (data->column_bitmaps[i]) {
          prv_draw_icon_centered_at(ctx, data->column_bitmaps[i],
            GPoint(center.x, cy[i]), prv_miniview_icon_op());
        }
      }
      break;
    }
  }

  // Draw border outline and decoration over top of whatever is displayed
  prv_draw_border_outline(ctx, center, radius);
  prv_draw_border_decoration(ctx, center, radius, inner_radius);
}

Layer *miniview_create(MiniviewConfig config) {
  Layout *layout = providers_get_layout();
  GRect bounds = layout->miniview_bounds;
  Layer *layer = layer_create_with_data(bounds, sizeof(MiniviewData));
  MiniviewData *data = layer_get_data(layer);
  data->config = config;
  data->small_text_bounds = layout->miniview_small_text_bounds;
  data->medium_text_bounds = layout->miniview_medium_text_bounds;
  data->small_font = providers_get_font_small();
  data->medium_font = providers_get_font_medium();

  bool needs_single_icon = config.mode == MINIVIEW_MODE_ICON_TEXT
                        || config.mode == MINIVIEW_MODE_ICON_CENTER
                        || config.mode == MINIVIEW_MODE_CLOCK_DOTS;

  data->icon_bitmap = (needs_single_icon && config.icon_resource_id != 0)
    ? gbitmap_create_with_resource(config.icon_resource_id)
    : NULL;

  for (int i = 0; i < 3; i++) {
    data->column_bitmaps[i] = (config.mode == MINIVIEW_MODE_ICON_COLUMN
                               && config.column_icon_resource_ids[i] != 0)
      ? gbitmap_create_with_resource(config.column_icon_resource_ids[i])
      : NULL;
  }

  data->small_text[0] = '\0';
  data->medium_text[0] = '\0';

  layer_set_update_proc(layer, miniview_update_proc);
  return layer;
}

void miniview_set_small_text(Layer *layer, const char *text) {
  MiniviewData *data = layer_get_data(layer);
  strncpy(data->small_text, text, sizeof(data->small_text) - 1);
  data->small_text[sizeof(data->small_text) - 1] = '\0';
  for (char *p = data->small_text; *p; p++) {
    if (*p >= 'a' && *p <= 'z') *p -= 32;
  }
  layer_mark_dirty(layer);
}

void miniview_set_medium_text(Layer *layer, const char *text) {
  MiniviewData *data = layer_get_data(layer);
  strncpy(data->medium_text, text, sizeof(data->medium_text) - 1);
  data->medium_text[sizeof(data->medium_text) - 1] = '\0';
  for (char *p = data->medium_text; *p; p++) {
    if (*p >= 'a' && *p <= 'z') *p -= 32;
  }
  layer_mark_dirty(layer);
}

void miniview_set_icon_resource_id(Layer *layer, uint32_t resource_id) {
  MiniviewData *data = layer_get_data(layer);
  if (data->icon_bitmap) {
    gbitmap_destroy(data->icon_bitmap);
    data->icon_bitmap = NULL;
  }
  if (resource_id != 0) {
    data->icon_bitmap = gbitmap_create_with_resource(resource_id);
    data->config.icon_resource_id = resource_id;
  }
  layer_mark_dirty(layer);
}

void miniview_set_icon_angle(Layer *layer, int32_t angle) {
  MiniviewData *data = layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);
  uint16_t radius = bounds.size.w / 2;
  int16_t dot_ring_radius = (int16_t)(radius - MINIVIEW_BORDER_SIZE - DOT_RING_INSET);
  data->config.icon_offset = GPoint(
    (int16_t)(sin_lookup(angle) * dot_ring_radius / TRIG_MAX_RATIO),
    -(int16_t)(cos_lookup(angle) * dot_ring_radius / TRIG_MAX_RATIO)
  );
  layer_mark_dirty(layer);
}

void miniview_set_column_icon(Layer *layer, int index, uint32_t resource_id) {
  if (index < 0 || index >= 3) return;
  MiniviewData *data = layer_get_data(layer);
  if (data->column_bitmaps[index]) {
    gbitmap_destroy(data->column_bitmaps[index]);
    data->column_bitmaps[index] = NULL;
  }
  if (resource_id != 0) {
    data->column_bitmaps[index] = gbitmap_create_with_resource(resource_id);
  }
  layer_mark_dirty(layer);
}

void miniview_destroy(Layer *layer) {
  MiniviewData *data = layer_get_data(layer);
  if (data->icon_bitmap) {
    gbitmap_destroy(data->icon_bitmap);
  }
  for (int i = 0; i < 3; i++) {
    if (data->column_bitmaps[i]) {
      gbitmap_destroy(data->column_bitmaps[i]);
    }
  }
  layer_destroy(layer);
}
