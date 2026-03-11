#include <pebble.h>
#include "layout.h"
#include "settings.h"
#include "time_display.h"
#include "providers/providers.h"

static Window *s_main_window;
static Layer *s_time_display_layer;
#if DRAW_DEBUG_RECTANGLES
static Layer *s_debug_layer;
#endif

static Layout s_layout;
static GFont s_font_28;
static GFont s_font_20;
static GFont s_font_seconds;

static void update_time(void) {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    time_display_set_time(s_time_display_layer, tick_time);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
    if (units_changed & MINUTE_UNIT) {
        providers_on_minute_tick(tick_time);
    }
}

static void update_tick_subscription(void) {
    ClaySettings *cfg = settings_get();
    TimeUnits units = (cfg->seconds_option != SECONDS_OPTION_ALWAYS_OFF)
        ? SECOND_UNIT : MINUTE_UNIT;
    tick_timer_service_subscribe(units, tick_handler);
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

static int32_t prv_tuple_int(Tuple *t) {
    if (!t) return 0;
    if (t->type == TUPLE_CSTRING) {
        int32_t result = 0;
        bool negative = false;
        const char *s = t->value->cstring;
        if (*s == '-') { negative = true; s++; }
        while (*s >= '0' && *s <= '9') {
            result = result * 10 + (*s - '0');
            s++;
        }
        return negative ? -result : result;
    }
    return t->value->int32;
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
    (void)context;

    Tuple *cfg_t = dict_find(iter, MESSAGE_KEY_CFG_BLACK_BG);
    if (cfg_t) {
        ClaySettings *s = settings_get();
        s->black_bg = (bool)prv_tuple_int(cfg_t);

        Tuple *t;
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_INVERT_MINIVIEW)))
            s->invert_miniview = (bool)prv_tuple_int(t);
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_WEATHER_OPTION)))
            s->weather_option = (uint8_t)prv_tuple_int(t);
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_WEATHER_REFRESH_INTERVAL)))
            s->weather_refresh_interval = (uint8_t)prv_tuple_int(t);
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_MINIVIEW_OPTION)))
            s->miniview_option = (uint8_t)prv_tuple_int(t);
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_GRAPH_OPTION)))
            s->graph_option = (uint8_t)prv_tuple_int(t);
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_SECONDS_OPTION)))
            s->seconds_option = (uint8_t)prv_tuple_int(t);
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_BOTTOM_LEFT_OPTION)))
            s->bottom_left_option = (uint8_t)prv_tuple_int(t);
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_BOTTOM_RIGHT_OPTION)))
            s->bottom_right_option = (uint8_t)prv_tuple_int(t);

        settings_save();

        window_set_background_color(s_main_window,
            s->black_bg ? GColorBlack : GColorWhite);

        providers_apply_settings();
        update_tick_subscription();
        return;
    }

    Tuple *weather_t = dict_find(iter, MESSAGE_KEY_WEATHER_TEMPERATURE);
    if (weather_t) {
        providers_on_weather_data(iter);
        return;
    }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
    (void)context;
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", reason);
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_font_28 = fonts_load_custom_font(
        resource_get_handle(RESOURCE_ID_BEBAS_NEUE_28));
    s_font_20 = fonts_load_custom_font(
        resource_get_handle(RESOURCE_ID_BEBAS_NEUE_20));
    s_font_seconds = fonts_load_custom_font(
        resource_get_handle(RESOURCE_ID_NUMS_THIN_16));

    layout_init(&s_layout, bounds);

    s_time_display_layer = time_display_create(
        s_layout.time_layer_bounds, s_font_seconds);
    layer_add_child(window_layer, s_time_display_layer);

    providers_init(window_layer, &s_layout, s_font_20, s_font_28);
    providers_apply_settings();

#if DRAW_DEBUG_RECTANGLES
    s_debug_layer = layer_create(bounds);
    layer_set_update_proc(s_debug_layer, debug_layer_update_proc);
    layer_add_child(window_layer, s_debug_layer);
#endif

    update_time();
}

static void main_window_unload(Window *window) {
    (void)window;

#if DRAW_DEBUG_RECTANGLES
    layer_destroy(s_debug_layer);
#endif
    providers_deinit();
    time_display_destroy(s_time_display_layer);
    fonts_unload_custom_font(s_font_28);
    fonts_unload_custom_font(s_font_20);
    fonts_unload_custom_font(s_font_seconds);
}

static void init(void) {
    settings_load();

    s_main_window = window_create();

    ClaySettings *cfg = settings_get();
    window_set_background_color(s_main_window,
        cfg->black_bg ? GColorBlack : GColorWhite);

    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload,
    });

    window_stack_push(s_main_window, true);

    update_tick_subscription();

    app_message_register_inbox_received(inbox_received_handler);
    app_message_register_inbox_dropped(inbox_dropped_handler);
    app_message_open(
        240, 240);
}

static void deinit(void) {
    tick_timer_service_unsubscribe();
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
