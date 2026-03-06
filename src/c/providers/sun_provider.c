#include "sun_provider.h"
#include "../complications/miniview.h"
#include "../complications/bottom.h"

typedef struct {
    bool active;
    uint8_t option;
} SlotState;

static SlotState s_slots[COMPLICATION_COUNT];
static int32_t s_sunrise;
static int32_t s_sunset;
static int16_t s_sun_angle;
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
                        .icon_resource_id = RESOURCE_ID_ICON_STEPS,
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
            layer = bottom_complication_create(bounds, (BottomConfig) {
                .mode = BOTTOM_MODE_ICON_TEXT,
                .align = align,
                .font = font_20,
                .icon_resource_id = RESOURCE_ID_ICON_SUN,
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
                    time_t display_time = past_noon ? s_sunset : s_sunrise;
                    struct tm *sun_tm = localtime(&display_time);
                    char buf[8];
                    strftime(buf, sizeof(buf),
                             clock_is_24h_style() ? "%H:%M" : "%I:%M", sun_tm);
                    miniview_set_tiny_text(layer, past_noon ? "SET" : "RISE");
                    miniview_set_small_text(layer, buf);
                }
                break;
            }
            case COMPLICATION_BOTTOM_LEFT:
            case COMPLICATION_BOTTOM_RIGHT: {
                if (s_slots[i].option == BOTTOM_OPTION_SUNRISE_SUNSET) {
                    bool past_noon = tick_time->tm_hour >= 12;
                    time_t display_time = past_noon ? s_sunset : s_sunrise;
                    struct tm *sun_tm = localtime(&display_time);
                    char buf[8];
                    strftime(buf, sizeof(buf),
                             clock_is_24h_style() ? "%H:%M" : "%I:%M", sun_tm);
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
    if ((t = dict_find(iter, MESSAGE_KEY_WEATHER_SUNRISE)))
        s_sunrise = t->value->int32;
    if ((t = dict_find(iter, MESSAGE_KEY_WEATHER_SUNSET)))
        s_sunset = t->value->int32;
    if ((t = dict_find(iter, MESSAGE_KEY_WEATHER_SUN_ANGLE)))
        s_sun_angle = (int16_t)t->value->int32;
    if ((t = dict_find(iter, MESSAGE_KEY_WEATHER_MOON_PHASE)))
        s_moon_phase = (int16_t)t->value->int32;

    s_has_data = true;
}

void sun_provider_deinit(void) {
    memset(s_slots, 0, sizeof(s_slots));
}
