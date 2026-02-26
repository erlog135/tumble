#include <pebble.h>
#include "layout.h"

static Window *s_main_window;

static Layer *s_graph_layer;
static Layer *s_miniview_layer;
static TextLayer *s_time_layer;
static TextLayer *s_seconds_layer;
static TextLayer *s_bottom_left_layer;
static TextLayer *s_bottom_right_layer;
static Layer *s_debug_layer;

static Layout s_layout;

static GFont s_font_49;
static GFont s_font_28;
static GFont s_font_20;

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);

  static char s_seconds_buffer[3];
  strftime(s_seconds_buffer, sizeof(s_seconds_buffer), "%S", tick_time);
  text_layer_set_text(s_seconds_layer, s_seconds_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void debug_layer_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, s_layout.graph_layer_bounds);
  graphics_draw_rect(ctx, s_layout.miniview_bounds);
  graphics_draw_rect(ctx, s_layout.time_layer_bounds);
  graphics_draw_rect(ctx, s_layout.seconds_layer_bounds);
  graphics_draw_rect(ctx, s_layout.bottom_left_bounds);
  graphics_draw_rect(ctx, s_layout.bottom_right_bounds);
}

static void miniview_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  uint16_t radius = bounds.size.w / 2;

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, radius);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, radius - MINIVIEW_BORDER_SIZE);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, "THU", s_font_20,
    s_layout.miniview_tiny_text_bounds,
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "12", s_font_28,
    s_layout.miniview_small_text_bounds,
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void graph_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GRect plot_bounds = s_layout.graph_plot_bounds;

  // Draw the label using the layer's bounds
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, "GRAPH", s_font_20,
    GRect(0, 0, bounds.size.w, TINY_FONT_HEIGHT),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Use plot bounds for the graph lines
  int16_t y_top = plot_bounds.origin.y;
  int16_t y_bot = plot_bounds.origin.y + plot_bounds.size.h - 1;
  int16_t x1 = plot_bounds.origin.x;
  int16_t x2 = plot_bounds.origin.x + plot_bounds.size.w - 1;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, GPoint(x1, y_top), GPoint(x1, y_bot));
  graphics_draw_line(ctx, GPoint(x2, y_top), GPoint(x2, y_bot));
  graphics_draw_line(ctx, GPoint(x1, y_bot), GPoint(x2, y_bot));
}

static TextLayer *make_placeholder(GRect bounds, const char *text, GFont font) {
  TextLayer *layer = text_layer_create(bounds);
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, GTextAlignmentCenter);
  text_layer_set_text(layer, text);
  return layer;
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_font_49 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_NEUE_49));
  s_font_28 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_NEUE_28));
  s_font_20 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_NEUE_20));

  layout_init(&s_layout, bounds);

  s_graph_layer = layer_create(s_layout.graph_layer_bounds);
  layer_set_update_proc(s_graph_layer, graph_layer_update_proc);
  layer_add_child(window_layer, s_graph_layer);

  s_miniview_layer = layer_create(s_layout.miniview_bounds);
  layer_set_update_proc(s_miniview_layer, miniview_layer_update_proc);
  layer_add_child(window_layer, s_miniview_layer);

  s_time_layer = make_placeholder(s_layout.time_layer_bounds, "TIME", s_font_49);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_seconds_layer = make_placeholder(s_layout.seconds_layer_bounds, "00", s_font_20);
  layer_add_child(window_layer, text_layer_get_layer(s_seconds_layer));

  s_bottom_left_layer = make_placeholder(s_layout.bottom_left_bounds, "BOT L", s_font_20);
  layer_add_child(window_layer, text_layer_get_layer(s_bottom_left_layer));

  s_bottom_right_layer = make_placeholder(s_layout.bottom_right_bounds, "BOT R", s_font_20);
  layer_add_child(window_layer, text_layer_get_layer(s_bottom_right_layer));

  s_debug_layer = layer_create(bounds);
  layer_set_update_proc(s_debug_layer, debug_layer_update_proc);
  layer_add_child(window_layer, s_debug_layer);

  update_time();
}

static void main_window_unload(Window *window) {
  layer_destroy(s_debug_layer);
  layer_destroy(s_graph_layer);
  layer_destroy(s_miniview_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_seconds_layer);
  text_layer_destroy(s_bottom_left_layer);
  text_layer_destroy(s_bottom_right_layer);
  fonts_unload_custom_font(s_font_49);
  fonts_unload_custom_font(s_font_28);
  fonts_unload_custom_font(s_font_20);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });

  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
