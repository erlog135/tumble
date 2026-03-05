#include <pebble.h>
#include "layout.h"
#include "time_display.h"
#include "complications/graph.h"
#include "complications/miniview.h"
#include "complications/bottom.h"

static Window *s_main_window;

static Layer *s_graph_layer;
static Layer *s_miniview_layer;
static Layer *s_time_display_layer;
static Layer *s_bottom_left_layer;
static Layer *s_bottom_right_layer;
#if DRAW_DEBUG_RECTANGLES
static Layer *s_debug_layer;
#endif

static Layout s_layout;

static GFont s_font_28;
static GFont s_font_20;

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  time_display_set_time(s_time_display_layer, tick_time);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

#if DRAW_DEBUG_RECTANGLES
static void debug_layer_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, s_layout.graph_layer_bounds);
  graphics_draw_rect(ctx, s_layout.miniview_bounds);
  graphics_draw_rect(ctx, s_layout.time_layer_bounds);
  graphics_draw_rect(ctx, s_layout.bottom_left_bounds);
  graphics_draw_rect(ctx, s_layout.bottom_right_bounds);
}
#endif

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

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

  s_time_display_layer = time_display_create(s_layout.time_layer_bounds, s_font_20);
  layer_add_child(window_layer, s_time_display_layer);

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

#if DRAW_DEBUG_RECTANGLES
  s_debug_layer = layer_create(bounds);
  layer_set_update_proc(s_debug_layer, debug_layer_update_proc);
  layer_add_child(window_layer, s_debug_layer);
#endif

  update_time();
}

static void main_window_unload(Window *window) {
#if DRAW_DEBUG_RECTANGLES
  layer_destroy(s_debug_layer);
#endif
  graph_destroy(s_graph_layer);
  miniview_destroy(s_miniview_layer);
  time_display_destroy(s_time_display_layer);
  bottom_complication_destroy(s_bottom_left_layer);
  bottom_complication_destroy(s_bottom_right_layer);
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
