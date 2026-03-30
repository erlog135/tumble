#include "time_display.h"
#include "layout.h"

#define NUM_GLYPHS 11  /* 0-9 + colon */

typedef struct {
  GBitmap *glyph_sheet;
  GBitmap *glyph_sub[NUM_GLYPHS];
  TextLayer *seconds_layer;
  Layer *glyph_layer;
  char time_str[8];
  bool seconds_visible;
  bool seconds_reserved;  /* keep seconds column space even when hidden */
} TimeDisplayData;

static int8_t prv_char_to_glyph_index(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c == ':') return 10;
  return -1;
}

static int16_t prv_glyph_display_width(int8_t glyph_idx) {
  if (glyph_idx >= 0 && glyph_idx <= 9) return GLYPH_NUMERAL_DISPLAY_WIDTH;
  if (glyph_idx == 10) return GLYPH_COLON_DISPLAY_WIDTH;
  return 0;
}

/* Returns the x offset for the time glyphs given the layer width.
   When seconds are present, prefers centering the time alone if the seconds
   column still fits to the right; otherwise centres the combined block. */
static int16_t prv_compute_time_x(int16_t layer_w, int16_t glyph_w, bool has_seconds) {
  if (has_seconds) {
    int16_t centered_x = (layer_w - glyph_w) / 2;
    if (centered_x + glyph_w + GLYPH_SPACING_X + SECONDS_LAYER_WIDTH <= layer_w) {
      return centered_x;
    }
    return (layer_w - (glyph_w + GLYPH_SPACING_X + SECONDS_LAYER_WIDTH)) / 2;
  }
  return (layer_w - glyph_w) / 2;
}

static int16_t prv_glyph_total_width(const char *time_str) {
  int16_t total = 0;
  for (const char *p = time_str; *p; p++) {
    int8_t idx = prv_char_to_glyph_index(*p);
    if (idx >= 0) {
      total += prv_glyph_display_width(idx);
      if (p[1]) total += GLYPH_SPACING_X;
    }
  }
  return total;
}

static void glyph_layer_update_proc(Layer *layer, GContext *ctx) {
  TimeDisplayData *data = *(TimeDisplayData **)layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_antialiased(ctx, false);

  int16_t glyph_w = prv_glyph_total_width(data->time_str);
  int16_t x = prv_compute_time_x(bounds.size.w, glyph_w,
    data->seconds_visible || data->seconds_reserved);
  int16_t y = (bounds.size.h - BITMAP_GLYPH_HEIGHT) / 2;

  graphics_context_set_compositing_mode(ctx, GCompOpSet);

  for (const char *p = data->time_str; *p; p++) {
    int8_t idx = prv_char_to_glyph_index(*p);
    if (idx < 0 || !data->glyph_sub[idx]) continue;

    int16_t w = prv_glyph_display_width(idx);
    GRect dest = GRect(x, y, w, BITMAP_GLYPH_HEIGHT);
    graphics_draw_bitmap_in_rect(ctx, data->glyph_sub[idx], dest);

    x += w + GLYPH_SPACING_X;
  }
}

static void container_update_proc(Layer *layer, GContext *ctx) {
  (void)layer;
  (void)ctx;
}

Layer *time_display_create(GRect bounds, GFont seconds_font) {
  Layer *layer = layer_create_with_data(bounds, sizeof(TimeDisplayData));
  TimeDisplayData *data = layer_get_data(layer);

  data->glyph_sheet = gbitmap_create_with_resource(TIME_GLYPH_SHEET_RESOURCE_ID);
  if (!data->glyph_sheet) {
    layer_destroy(layer);
    return NULL;
  }

  for (int i = 0; i < 10; i++) {
    GRect src = GRect(
      i * BITMAP_GLYPH_WIDTH + GLYPH_NUMERAL_MARGIN_X,
      0,
      GLYPH_NUMERAL_DISPLAY_WIDTH,
      BITMAP_GLYPH_HEIGHT
    );
    data->glyph_sub[i] = gbitmap_create_as_sub_bitmap(data->glyph_sheet, src);
  }
  {
    GRect src = GRect(
      10 * BITMAP_GLYPH_WIDTH + GLYPH_COLON_MARGIN_X,
      0,
      GLYPH_COLON_DISPLAY_WIDTH,
      BITMAP_GLYPH_HEIGHT
    );
    data->glyph_sub[10] = gbitmap_create_as_sub_bitmap(data->glyph_sheet, src);
  }

  data->time_str[0] = '\0';
  data->seconds_visible = true;
  data->seconds_reserved = true;

  /* Seconds layer (drawn first, behind) */
  int16_t glyph_top = (bounds.size.h - BITMAP_GLYPH_HEIGHT) / 2;
  GRect seconds_bounds = GRect(
    bounds.size.w - SECONDS_LAYER_WIDTH,
    glyph_top,
    SECONDS_LAYER_WIDTH,
    SMALL_FONT_HEIGHT
  );
  data->seconds_layer = text_layer_create(seconds_bounds);
  text_layer_set_background_color(data->seconds_layer, GColorClear);
  text_layer_set_text_color(data->seconds_layer, GColorWhite);
  text_layer_set_font(data->seconds_layer, seconds_font);
  text_layer_set_text_alignment(data->seconds_layer, GTextAlignmentCenter);
  text_layer_set_text(data->seconds_layer, "00");
  layer_add_child(layer, text_layer_get_layer(data->seconds_layer));

  /* Glyph layer (drawn second, on top so numerals are not covered) */
  data->glyph_layer = layer_create_with_data(GRect(0, 0, bounds.size.w, bounds.size.h),
    sizeof(TimeDisplayData *));
  *(TimeDisplayData **)layer_get_data(data->glyph_layer) = data;
  layer_set_update_proc(data->glyph_layer, glyph_layer_update_proc);
  layer_add_child(layer, data->glyph_layer);

  layer_set_update_proc(layer, container_update_proc);
  return layer;
}

void time_display_destroy(Layer *layer) {
  TimeDisplayData *data = layer_get_data(layer);
  if (data->glyph_layer) {
    layer_destroy(data->glyph_layer);
    data->glyph_layer = NULL;
  }
  if (data->seconds_layer) {
    text_layer_destroy(data->seconds_layer);
    data->seconds_layer = NULL;
  }
  for (int i = 0; i < NUM_GLYPHS; i++) {
    if (data->glyph_sub[i]) {
      gbitmap_destroy(data->glyph_sub[i]);
      data->glyph_sub[i] = NULL;
    }
  }
  if (data->glyph_sheet) {
    gbitmap_destroy(data->glyph_sheet);
    data->glyph_sheet = NULL;
  }
  layer_destroy(layer);
}

void time_display_set_time(Layer *layer, struct tm *tick_time) {
  TimeDisplayData *data = layer_get_data(layer);
  strftime(data->time_str, sizeof(data->time_str),
    clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  if (!clock_is_24h_style() && data->time_str[0] == '0') {
    memmove(data->time_str, data->time_str + 1, strlen(data->time_str));
  }

  if (data->seconds_visible || data->seconds_reserved) {
    GRect lbounds = layer_get_bounds(layer);
    int16_t glyph_w = prv_glyph_total_width(data->time_str);
    int16_t start_x = prv_compute_time_x(lbounds.size.w, glyph_w, true);
    int16_t glyph_top = (lbounds.size.h - BITMAP_GLYPH_HEIGHT) / 2;
    layer_set_frame(text_layer_get_layer(data->seconds_layer),
      GRect(start_x + glyph_w + GLYPH_SPACING_X, glyph_top, SECONDS_LAYER_WIDTH, SMALL_FONT_HEIGHT));

    if (data->seconds_visible) {
      static char seconds_buf[3];
      strftime(seconds_buf, sizeof(seconds_buf), "%S", tick_time);
      text_layer_set_text(data->seconds_layer, seconds_buf);
    }
  }

  layer_mark_dirty(data->glyph_layer);
}

void time_display_set_seconds_visible(Layer *layer, bool visible) {
  TimeDisplayData *data = layer_get_data(layer);
  data->seconds_visible = visible;
  layer_set_hidden(text_layer_get_layer(data->seconds_layer), !visible);
  layer_mark_dirty(data->glyph_layer);
}

void time_display_set_seconds_reserved(Layer *layer, bool reserved) {
  TimeDisplayData *data = layer_get_data(layer);
  data->seconds_reserved = reserved;
  layer_mark_dirty(data->glyph_layer);
}
