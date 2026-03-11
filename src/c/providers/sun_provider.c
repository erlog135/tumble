#include "sun_provider.h"
#include "../complications/miniview.h"
#include "../complications/bottom.h"
#include <time.h>

typedef struct {
    bool active;
    uint8_t option;
} SlotState;

static SlotState s_slots[COMPLICATION_COUNT];
static int16_t s_sunrise_min;
static int16_t s_sunset_min;
static int16_t s_moon_phase;
static bool s_has_data;

void sun_provider_init(void) {
    memset(s_slots, 0, sizeof(s_slots));
    s_has_data = false;
}

void sun_provider_activate(ComplicationSlot slot, uint8_t option) {
    s_slots[slot].active = true;
    s_slots[slot].option = option;

    Layout *layout = providers_get_layout();
    GFont font_20 = providers_get_font_20();
    GFont font_28 = providers_get_font_28();
    Layer *window_layer = providers_get_window_layer();

    Layer *layer = NULL;

    switch (slot) {
        case COMPLICATION_MINIVIEW: {
            switch (option) {
                case MINIVIEW_OPTION_SUNRISE_SUNSET:
                    layer = miniview_create(layout->miniview_bounds, (MiniviewConfig) {
                        .mode = MINIVIEW_MODE_TEXT_STACK,
                        .tiny_text_bounds = layout->miniview_tiny_text_bounds,
                        .small_text_bounds = layout->miniview_small_text_bounds,
                        .tiny_font = font_20,
                        .small_font = font_28,
                    });
                    miniview_set_tiny_text(layer, "SUN");
                    miniview_set_small_text(layer, "--:--");
                    break;
                case MINIVIEW_OPTION_SUN_POSITION:
                    layer = miniview_create(layout->miniview_bounds, (MiniviewConfig) {
                        .mode = MINIVIEW_MODE_CLOCK_DOTS,
                        .icon_resource_id = RESOURCE_ID_ICON_SUN,
                        .icon_offset = GPoint(0, -15),
                    });
                    break;
                case MINIVIEW_OPTION_MOON_PHASE:
                    layer = miniview_create(layout->miniview_bounds, (MiniviewConfig) {
                        .mode = MINIVIEW_MODE_ICON_CENTER,
                        .icon_resource_id = 0,
                    });
                    break;
                default:
                    break;
            }
            break;
        }
        case COMPLICATION_BOTTOM_LEFT:
        case COMPLICATION_BOTTOM_RIGHT: {
            GRect bounds = (slot == COMPLICATION_BOTTOM_LEFT)
                ? layout->bottom_left_bounds : layout->bottom_right_bounds;
            BottomAlign align = (slot == COMPLICATION_BOTTOM_LEFT)
                ? BOTTOM_ALIGN_RIGHT : BOTTOM_ALIGN_LEFT;
            time_t now = time(NULL);
            struct tm *now_tm = localtime(&now);
            uint32_t init_icon = (now_tm->tm_hour >= 12)
                ? RESOURCE_ID_ICON_SUNSET : RESOURCE_ID_ICON_SUNRISE;
            layer = bottom_complication_create(bounds, (BottomConfig) {
                .mode = BOTTOM_MODE_ICON_TEXT,
                .align = align,
                .font = font_20,
                .icon_resource_id = init_icon,
            });
            bottom_complication_set_text(layer, "--:--");
            break;
        }
        default:
            break;
    }

    if (layer) {
        layer_add_child(window_layer, layer);
        providers_set_layer(slot, layer);
    }
}

void sun_provider_deactivate(ComplicationSlot slot) {
    s_slots[slot].active = false;
}

static void prv_format_sun_time(char *buf, size_t size, int16_t total_minutes) {
    int h = total_minutes / 60;
    int m = total_minutes % 60;
    if (!clock_is_24h_style()) {
        h = h % 12;
        if (h == 0) h = 12;
    }
    snprintf(buf, size, "%d:%02d", h, m);
}

// Maps current time to a clock-face trig angle for the sun icon:
//   Day   (sunrise→sunset):        9 o'clock (270°) → 12 (0°) → 3 (90°)
//   Night (sunset→next sunrise):   3 o'clock (90°)  → 6 (180°) → 9 (270°)
static int32_t prv_sun_trig_angle(struct tm *tick_time) {
    int16_t current_min = (int16_t)(tick_time->tm_hour * 60 + tick_time->tm_min);
    int16_t total_day = s_sunset_min - s_sunrise_min;
    int32_t angle_deg;

    if (total_day > 0 && current_min >= s_sunrise_min && current_min < s_sunset_min) {
        angle_deg = 270 + (int32_t)(current_min - s_sunrise_min) * 180 / total_day;
    } else {
        int16_t total_night = 1440 - total_day;
        int16_t night_elapsed = (current_min >= s_sunset_min)
            ? current_min - s_sunset_min
            : current_min + 1440 - s_sunset_min;
        angle_deg = 90 + (int32_t)night_elapsed * 180 / (total_night > 0 ? total_night : 1);
    }

    return DEG_TO_TRIGANGLE(angle_deg % 360);
}

void sun_provider_tick(struct tm *tick_time) {
    if (!s_has_data) return;

    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        if (!s_slots[i].active) continue;
        Layer *layer = providers_get_layer(i);
        if (!layer) continue;

        switch (i) {
            case COMPLICATION_MINIVIEW: {
                if (s_slots[i].option == MINIVIEW_OPTION_SUNRISE_SUNSET) {
                    bool past_noon = tick_time->tm_hour >= 12;
                    char buf[8];
                    prv_format_sun_time(buf, sizeof(buf),
                                        past_noon ? s_sunset_min : s_sunrise_min);
                    miniview_set_tiny_text(layer, past_noon ? "SET" : "RISE");
                    miniview_set_small_text(layer, buf);
                } else if (s_slots[i].option == MINIVIEW_OPTION_SUN_POSITION) {
                    miniview_set_icon_angle(layer, prv_sun_trig_angle(tick_time));
                }
                break;
            }
            case COMPLICATION_BOTTOM_LEFT:
            case COMPLICATION_BOTTOM_RIGHT: {
                if (s_slots[i].option == BOTTOM_OPTION_SUNRISE_SUNSET) {
                    int16_t current_min = tick_time->tm_hour * 60 + tick_time->tm_min;
                    bool show_sunset;
                    if (s_sunrise_min <= s_sunset_min) {
                        // Normal: sunrise before sunset (typical case)
                        show_sunset = (current_min >= s_sunrise_min && current_min < s_sunset_min);
                    } else {
                        // Sunrise after sunset (rare, e.g., polar)
                        show_sunset = (current_min >= s_sunrise_min || current_min < s_sunset_min);
                    }

                    char buf[8];
                    if (show_sunset) {
                        prv_format_sun_time(buf, sizeof(buf), s_sunset_min);
                        bottom_complication_set_icon(layer, RESOURCE_ID_ICON_SUNSET);
                    } else {
                        prv_format_sun_time(buf, sizeof(buf), s_sunrise_min);
                        bottom_complication_set_icon(layer, RESOURCE_ID_ICON_SUNRISE);
                    }
                    bottom_complication_set_text(layer, buf);
                }
                break;
            }
            default:
                break;
        }
    }
}

void sun_provider_on_data(DictionaryIterator *iter) {
    Tuple *t;
    if ((t = dict_find(iter, MESSAGE_KEY_WEATHER_SUN_RISE_SET))) {
        int32_t packed = t->value->int32;
        s_sunrise_min = (int16_t)((packed >> 16) & 0xFFFF);
        s_sunset_min  = (int16_t)(packed & 0xFFFF);
    }
    if ((t = dict_find(iter, MESSAGE_KEY_WEATHER_MOON_PHASE)))
        s_moon_phase = (int16_t)t->value->int32;

    s_has_data = true;
}

void sun_provider_deinit(void) {
    memset(s_slots, 0, sizeof(s_slots));
}
