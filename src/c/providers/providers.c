#include "providers.h"
#include "time_provider.h"
#include "health_provider.h"
#include "weather_provider.h"
#include "battery_provider.h"
#include "sun_moon_provider.h"
#include "../complications/graph.h"
#include "../complications/miniview.h"
#include "../complications/bottom.h"

typedef enum {
    PROVIDER_NONE = 0,
    PROVIDER_TIME,
    PROVIDER_HEALTH,
    PROVIDER_WEATHER,
    PROVIDER_BATTERY,
    PROVIDER_SUN,
    PROVIDER_COUNT,
} ProviderId;

static Layer *s_window_layer;
static Layout *s_layout;
static GFont s_font_20;
static GFont s_font_28;
static Layer *s_layers[COMPLICATION_COUNT];
static ProviderId s_slot_owner[COMPLICATION_COUNT];
static uint8_t s_slot_option[COMPLICATION_COUNT];

Layer *providers_get_window_layer(void) { return s_window_layer; }
Layout *providers_get_layout(void)      { return s_layout; }
GFont providers_get_font_20(void)       { return s_font_20; }
GFont providers_get_font_28(void)       { return s_font_28; }

Layer *providers_get_layer(ComplicationSlot slot) {
    return s_layers[slot];
}

void providers_set_layer(ComplicationSlot slot, Layer *layer) {
    s_layers[slot] = layer;
}

static ProviderId prv_get_graph_provider(uint8_t option) {
    switch (option) {
        case GRAPH_OPTION_STEPS:
        case GRAPH_OPTION_HEART_RATE:   return PROVIDER_HEALTH;
        case GRAPH_OPTION_AIR_PRESSURE:
        case GRAPH_OPTION_ALTITUDE:
        case GRAPH_OPTION_TEMPERATURE:  return PROVIDER_WEATHER;
        case GRAPH_OPTION_BATTERY:      return PROVIDER_BATTERY;
        default:                        return PROVIDER_NONE;
    }
}

static ProviderId prv_get_miniview_provider(uint8_t option) {
    switch (option) {
        case MINIVIEW_OPTION_DATE_MONTH_DATE:
        case MINIVIEW_OPTION_DATE_DOW_DATE:
        case MINIVIEW_OPTION_CUSTOM_TZ:      return PROVIDER_TIME;
        case MINIVIEW_OPTION_HEART_RATE:
        case MINIVIEW_OPTION_STEPS:
        case MINIVIEW_OPTION_CALORIES:       return PROVIDER_HEALTH;
        case MINIVIEW_OPTION_ALTITUDE:
        case MINIVIEW_OPTION_AIR_PRESSURE:
        case MINIVIEW_OPTION_WEATHER:        return PROVIDER_WEATHER;
        case MINIVIEW_OPTION_SUNRISE_SUNSET:
        case MINIVIEW_OPTION_SUN_POSITION:
        case MINIVIEW_OPTION_MOON_PHASE:     return PROVIDER_SUN;
        case MINIVIEW_OPTION_BATTERY:
        case MINIVIEW_OPTION_BATTERY_DND:    return PROVIDER_BATTERY;
        default:                             return PROVIDER_NONE;
    }
}

static ProviderId prv_get_bottom_provider(uint8_t option) {
    switch (option) {
        case BOTTOM_OPTION_DATE_MONTH_DATE:
        case BOTTOM_OPTION_DATE_DOW_DATE:    return PROVIDER_TIME;
        case BOTTOM_OPTION_STEPS:
        case BOTTOM_OPTION_HEART_RATE:
        case BOTTOM_OPTION_CALORIES:         return PROVIDER_HEALTH;
        case BOTTOM_OPTION_ALTITUDE:
        case BOTTOM_OPTION_AIR_PRESSURE:
        case BOTTOM_OPTION_PRESSURE_TREND:
        case BOTTOM_OPTION_TEMPERATURE:      return PROVIDER_WEATHER;
        case BOTTOM_OPTION_SUNRISE_SUNSET:   return PROVIDER_SUN;
        default:                             return PROVIDER_NONE;
    }
}

static void prv_activate(ProviderId id, ComplicationSlot slot, uint8_t option) {
    switch (id) {
        case PROVIDER_TIME:    time_provider_activate(slot, option);    break;
        case PROVIDER_HEALTH:  health_provider_activate(slot, option);  break;
        case PROVIDER_WEATHER: weather_provider_activate(slot, option); break;
        case PROVIDER_BATTERY: battery_provider_activate(slot, option); break;
        case PROVIDER_SUN:     sun_moon_provider_activate(slot, option);     break;
        default: break;
    }
}

static void prv_deactivate(ProviderId id, ComplicationSlot slot) {
    switch (id) {
        case PROVIDER_TIME:    time_provider_deactivate(slot);    break;
        case PROVIDER_HEALTH:  health_provider_deactivate(slot);  break;
        case PROVIDER_WEATHER: weather_provider_deactivate(slot); break;
        case PROVIDER_BATTERY: battery_provider_deactivate(slot); break;
        case PROVIDER_SUN:     sun_moon_provider_deactivate(slot);     break;
        default: break;
    }
}

static void prv_destroy_layer(ComplicationSlot slot) {
    Layer *layer = s_layers[slot];
    if (!layer) return;
    switch (slot) {
        case COMPLICATION_GRAPH:
            graph_destroy(layer);
            break;
        case COMPLICATION_MINIVIEW:
            miniview_destroy(layer);
            break;
        case COMPLICATION_BOTTOM_LEFT:
        case COMPLICATION_BOTTOM_RIGHT:
            bottom_complication_destroy(layer);
            break;
        default:
            break;
    }
    s_layers[slot] = NULL;
}

void providers_init(Layer *window_layer, Layout *layout,
                    GFont font_20, GFont font_28) {
    s_window_layer = window_layer;
    s_layout = layout;
    s_font_20 = font_20;
    s_font_28 = font_28;

    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        s_layers[i] = NULL;
        s_slot_owner[i] = PROVIDER_NONE;
        s_slot_option[i] = 0;
    }

    time_provider_init();
    health_provider_init();
    weather_provider_init();
    battery_provider_init();
    sun_moon_provider_init();
}

void providers_apply_settings(void) {
    ClaySettings *cfg = settings_get();

    ProviderId new_providers[COMPLICATION_COUNT];
    uint8_t new_options[COMPLICATION_COUNT];

    new_providers[COMPLICATION_GRAPH]        = prv_get_graph_provider(cfg->graph_option);
    new_options[COMPLICATION_GRAPH]          = cfg->graph_option;

    new_providers[COMPLICATION_MINIVIEW]     = prv_get_miniview_provider(cfg->miniview_option);
    new_options[COMPLICATION_MINIVIEW]       = cfg->miniview_option;

    new_providers[COMPLICATION_BOTTOM_LEFT]  = prv_get_bottom_provider(cfg->bottom_left_option);
    new_options[COMPLICATION_BOTTOM_LEFT]    = cfg->bottom_left_option;

    new_providers[COMPLICATION_BOTTOM_RIGHT] = prv_get_bottom_provider(cfg->bottom_right_option);
    new_options[COMPLICATION_BOTTOM_RIGHT]   = cfg->bottom_right_option;

    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        if (s_slot_owner[i] != new_providers[i] ||
            s_slot_option[i] != new_options[i]) {
            if (s_slot_owner[i] != PROVIDER_NONE) {
                prv_deactivate(s_slot_owner[i], (ComplicationSlot)i);
                prv_destroy_layer((ComplicationSlot)i);
            }
            s_slot_owner[i] = new_providers[i];
            s_slot_option[i] = new_options[i];
            if (new_providers[i] != PROVIDER_NONE) {
                prv_activate(new_providers[i], (ComplicationSlot)i, new_options[i]);
            }
        }
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    providers_on_minute_tick(t);
}

void providers_on_minute_tick(struct tm *tick_time) {
    battery_provider_record_history(tick_time);
    weather_provider_record_history(tick_time);

    bool has[PROVIDER_COUNT];
    memset(has, 0, sizeof(has));

    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        if (s_slot_owner[i] > PROVIDER_NONE && s_slot_owner[i] < PROVIDER_COUNT) {
            has[s_slot_owner[i]] = true;
        }
    }

    if (has[PROVIDER_TIME])    time_provider_tick(tick_time);
    if (has[PROVIDER_HEALTH])  health_provider_tick(tick_time);
    if (has[PROVIDER_WEATHER]) weather_provider_tick(tick_time);
    if (has[PROVIDER_BATTERY]) battery_provider_tick(tick_time);
    if (has[PROVIDER_SUN])     sun_moon_provider_tick(tick_time);
}

void providers_on_weather_data(DictionaryIterator *iter) {
    weather_provider_on_data(iter);
    sun_moon_provider_on_solar_data(iter);
    sun_moon_provider_on_lunar_data(iter);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    bool has_weather = false, has_sun = false;
    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        if (s_slot_owner[i] == PROVIDER_WEATHER) has_weather = true;
        if (s_slot_owner[i] == PROVIDER_SUN) has_sun = true;
    }
    if (has_weather) weather_provider_tick(t);
    if (has_sun) sun_moon_provider_tick(t);
}

static void prv_tick_sun_if_active(void) {
    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        if (s_slot_owner[i] == PROVIDER_SUN) {
            time_t now = time(NULL);
            sun_moon_provider_tick(localtime(&now));
            return;
        }
    }
}

void providers_on_solar_data(DictionaryIterator *iter) {
    sun_moon_provider_on_solar_data(iter);
    prv_tick_sun_if_active();
}

void providers_on_lunar_data(DictionaryIterator *iter) {
    sun_moon_provider_on_lunar_data(iter);
    prv_tick_sun_if_active();
}

void providers_deinit(void) {
    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        if (s_slot_owner[i] != PROVIDER_NONE) {
            prv_deactivate(s_slot_owner[i], (ComplicationSlot)i);
            prv_destroy_layer((ComplicationSlot)i);
            s_slot_owner[i] = PROVIDER_NONE;
        }
    }

    time_provider_deinit();
    health_provider_deinit();
    weather_provider_deinit();
    battery_provider_deinit();
    sun_moon_provider_deinit();
}
