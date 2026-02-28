#include <pebble.h>
#include "layout.h"
#include "complications/graph.h"
#include "complications/miniview.h"
#include "complications/bottom.h"

static Window *s_main_window;

static Layer *s_graph_layer;
static Layer *s_miniview_layer;
static TextLayer *s_time_layer;
static TextLayer *s_seconds_layer;
static Layer *s_bottom_left_layer;
static Layer *s_bottom_right_layer;
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

  s_graph_layer = graph_create(s_layout.graph_layer_bounds, s_layout.graph_plot_bounds, (GraphConfig) {
    .style = GRAPH_STYLE_BARS,
    .h_markers = 4,
    .v_markers = 3,
    .top_lip = true,
    .label_font = s_font_20,
    .icon_resource_id = RESOURCE_ID_ICON_STEPS,
    .label_text = "STEPS",
  });
  layer_add_child(window_layer, s_graph_layer);

  MiniviewConfig miniview_config = {
    .mode = MINIVIEW_MODE_CLOCK_DOTS,
    .tiny_text_bounds = s_layout.miniview_tiny_text_bounds,
    .small_text_bounds = s_layout.miniview_small_text_bounds,
    .tiny_font = s_font_20,
    .small_font = s_font_28,
    .icon_resource_id = RESOURCE_ID_ICON_STEPS,
    .icon_offset = GPoint(0, -15),
  };
  s_miniview_layer = miniview_create(s_layout.miniview_bounds, miniview_config);
  layer_add_child(window_layer, s_miniview_layer);

  s_time_layer = make_placeholder(s_layout.time_layer_bounds, "TIME", s_font_49);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_seconds_layer = make_placeholder(s_layout.seconds_layer_bounds, "00", s_font_20);
  layer_add_child(window_layer, text_layer_get_layer(s_seconds_layer));

  s_bottom_left_layer = bottom_complication_create(s_layout.bottom_left_bounds, (BottomConfig) {
    .mode = BOTTOM_MODE_ICON_TEXT,
    .align = BOTTOM_ALIGN_RIGHT,
    .font = s_font_20,
    .icon_resource_id = RESOURCE_ID_ICON_STEPS,
  });
  bottom_complication_set_text(s_bottom_left_layer, "8,432");
  layer_add_child(window_layer, s_bottom_left_layer);

  s_bottom_right_layer = bottom_complication_create(s_layout.bottom_right_bounds, (BottomConfig) {
    .mode = BOTTOM_MODE_ICON_TEXT,
    .align = BOTTOM_ALIGN_LEFT,
    .font = s_font_20,
    .icon_resource_id = RESOURCE_ID_ICON_STEPS,
  });
  bottom_complication_set_text(s_bottom_right_layer, "512 cal");
  layer_add_child(window_layer, s_bottom_right_layer);

  s_debug_layer = layer_create(bounds);
  layer_set_update_proc(s_debug_layer, debug_layer_update_proc);
  layer_add_child(window_layer, s_debug_layer);

  update_time();
}

static void main_window_unload(Window *window) {
  layer_destroy(s_debug_layer);
  graph_destroy(s_graph_layer);
  miniview_destroy(s_miniview_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_seconds_layer);
  bottom_complication_destroy(s_bottom_left_layer);
  bottom_complication_destroy(s_bottom_right_layer);
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
