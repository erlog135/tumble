#include <pebble.h>
#include "layout.h"
#include "settings.h"
#include "time_display.h"
#include "providers/providers.h"

static Window *s_main_window;
static Layer *s_time_display_layer;
static AppTimer *s_shake_timer = NULL;
#if DRAW_DEBUG_RECTANGLES
static Layer *s_debug_layer;
#endif

static Layout s_layout;
static GFont s_font_medium;
static GFont s_font_small;
static GFont s_font_seconds;

static void update_time(void) {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    time_display_set_time(s_time_display_layer, tick_time);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
#ifdef TEST_SUN_CLOCK_SECONDS
    providers_on_minute_tick(tick_time);
#else
    if (units_changed & MINUTE_UNIT) {
        providers_on_minute_tick(tick_time);
    }
#endif
}

static void prv_shake_timer_callback(void *context) {
    s_shake_timer = NULL;
    time_display_set_seconds_visible(s_time_display_layer, false);
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    update_time();
}

static void prv_tap_handler(AccelAxisType axis, int32_t direction) {
    ClaySettings *cfg = settings_get();
    uint32_t duration_ms = (cfg->seconds_option == SECONDS_OPTION_SHAKE_30S) ? 30000 : 15000;

    if (s_shake_timer) {
        app_timer_reschedule(s_shake_timer, duration_ms);
    } else {
        time_display_set_seconds_visible(s_time_display_layer, true);
        tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
        update_time();
        s_shake_timer = app_timer_register(duration_ms, prv_shake_timer_callback, NULL);
    }
}

static void update_tick_subscription(void) {
#ifdef TEST_SUN_CLOCK_SECONDS
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
#else
    ClaySettings *cfg = settings_get();

    if (s_shake_timer) {
        app_timer_cancel(s_shake_timer);
        s_shake_timer = NULL;
    }

    bool is_shake = (cfg->seconds_option == SECONDS_OPTION_SHAKE_30S ||
                     cfg->seconds_option == SECONDS_OPTION_SHAKE_15S);

    if (is_shake) {
        accel_tap_service_subscribe(prv_tap_handler);
        time_display_set_seconds_reserved(s_time_display_layer, true);
        time_display_set_seconds_visible(s_time_display_layer, false);
        tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    } else {
        accel_tap_service_unsubscribe();
        bool show = (cfg->seconds_option == SECONDS_OPTION_ALWAYS_ON);
        time_display_set_seconds_reserved(s_time_display_layer, show);
        time_display_set_seconds_visible(s_time_display_layer, show);
        tick_timer_service_subscribe(show ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
    }
#endif
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
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_UNIT_TEMP)) && t->type == TUPLE_CSTRING)
            s->unit_temp = (strcmp(t->value->cstring, "c") == 0) ? UNIT_TEMP_C : UNIT_TEMP_F;
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_UNIT_ALTITUDE)) && t->type == TUPLE_CSTRING)
            s->unit_altitude = (strcmp(t->value->cstring, "m") == 0) ? UNIT_ALTITUDE_M : UNIT_ALTITUDE_FT;
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_UNIT_PRESSURE)) && t->type == TUPLE_CSTRING) {
            const char *v = t->value->cstring;
            if (strcmp(v, "hpa") == 0)        s->unit_pressure = UNIT_PRESSURE_HPA;
            else if (strcmp(v, "inhg") == 0)  s->unit_pressure = UNIT_PRESSURE_INHG;
            else                              s->unit_pressure = UNIT_PRESSURE_MB;
        }
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
        if ((t = dict_find(iter, MESSAGE_KEY_CFG_TZ_OFFSET)))
            s->tz_offset_minutes = (int16_t)prv_tuple_int(t);

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

    if (dict_find(iter, MESSAGE_KEY_WEATHER_SUN_RISE_SET)) {
        providers_on_solar_data(iter);
        return;
    }

    if (dict_find(iter, MESSAGE_KEY_WEATHER_MOON_PHASE)) {
        providers_on_lunar_data(iter);
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

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
    s_font_medium = fonts_load_custom_font(
        resource_get_handle(RESOURCE_ID_BEBAS_NEUE_36));
    s_font_small = fonts_load_custom_font(
        resource_get_handle(RESOURCE_ID_BEBAS_NEUE_28));
#else
    s_font_medium = fonts_load_custom_font(
        resource_get_handle(RESOURCE_ID_BEBAS_NEUE_28));
    s_font_small = fonts_load_custom_font(
        resource_get_handle(RESOURCE_ID_BEBAS_NEUE_20));
#endif
    s_font_seconds = fonts_load_custom_font(
        resource_get_handle(RESOURCE_ID_NUMS_THIN_16));

    layout_init(&s_layout, bounds);

    s_time_display_layer = time_display_create(
        s_layout.time_layer_bounds, s_font_seconds);
    layer_add_child(window_layer, s_time_display_layer);

    providers_init(window_layer, &s_layout, s_font_small, s_font_medium);
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
    fonts_unload_custom_font(s_font_medium);
    fonts_unload_custom_font(s_font_small);
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
    accel_tap_service_unsubscribe();
    if (s_shake_timer) {
        app_timer_cancel(s_shake_timer);
        s_shake_timer = NULL;
    }
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
