#include "weather_provider.h"
#include "../complications/graph.h"
#include "../complications/miniview.h"
#include "../complications/bottom.h"

typedef struct {
    bool active;
    uint8_t option;
} SlotState;

static SlotState s_slots[COMPLICATION_COUNT];
static int16_t s_temperature;
static int16_t s_pressure;
static int16_t s_pressure_trend;
static int16_t s_altitude;
static int16_t s_condition;
static bool s_has_data;

void weather_provider_init(void) {
    memset(s_slots, 0, sizeof(s_slots));
    s_has_data = false;
}

void weather_provider_activate(ComplicationSlot slot, uint8_t option) {
    s_slots[slot].active = true;
    s_slots[slot].option = option;

    Layout *layout = providers_get_layout();
    GFont font_20 = providers_get_font_20();
    GFont font_28 = providers_get_font_28();
    Layer *window_layer = providers_get_window_layer();

    Layer *layer = NULL;

    switch (slot) {
        case COMPLICATION_GRAPH: {
            const char *label;
            switch (option) {
                case GRAPH_OPTION_AIR_PRESSURE: label = "PRES"; break;
                case GRAPH_OPTION_ALTITUDE:     label = "ALT";  break;
                case GRAPH_OPTION_TEMPERATURE:  label = "TEMP"; break;
                default:                        label = "??";   break;
            }
            layer = graph_create(layout->graph_layer_bounds,
                                 layout->graph_plot_bounds, (GraphConfig) {
                .style = GRAPH_STYLE_LINE,
                .h_markers = 4,
                .v_markers = 3,
                .top_lip = true,
                .label_font = font_20,
                .icon_resource_id = RESOURCE_ID_ICON_STEPS,
                .label_text = label,
            });
            break;
        }
        case COMPLICATION_MINIVIEW: {
            layer = miniview_create(layout->miniview_bounds, (MiniviewConfig) {
                .mode = MINIVIEW_MODE_ICON_TEXT,
                .tiny_text_bounds = layout->miniview_tiny_text_bounds,
                .small_text_bounds = layout->miniview_small_text_bounds,
                .tiny_font = font_20,
                .small_font = font_28,
                .icon_resource_id = RESOURCE_ID_ICON_STEPS,
            });
            miniview_set_small_text(layer, "--");
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
                .icon_resource_id = RESOURCE_ID_ICON_STEPS,
            });
            bottom_complication_set_text(layer, "--");
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

void weather_provider_deactivate(ComplicationSlot slot) {
    s_slots[slot].active = false;
}

void weather_provider_tick(struct tm *tick_time) {
    (void)tick_time;

    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        if (!s_slots[i].active) continue;
        Layer *layer = providers_get_layer(i);
        if (!layer) continue;

        switch (i) {
            case COMPLICATION_MINIVIEW: {
                char buf[12];
                if (!s_has_data) {
                    strncpy(buf, "--", sizeof(buf));
                } else {
                    switch (s_slots[i].option) {
                        case MINIVIEW_OPTION_ALTITUDE:
                            snprintf(buf, sizeof(buf), "%d", s_altitude);
                            break;
                        case MINIVIEW_OPTION_AIR_PRESSURE:
                            snprintf(buf, sizeof(buf), "%d", s_pressure);
                            break;
                        case MINIVIEW_OPTION_WEATHER:
                            snprintf(buf, sizeof(buf), "%d", s_temperature);
                            break;
                        default:
                            strncpy(buf, "--", sizeof(buf));
                            break;
                    }
                }
                miniview_set_small_text(layer, buf);
                break;
            }
            case COMPLICATION_BOTTOM_LEFT:
            case COMPLICATION_BOTTOM_RIGHT: {
                char buf[20];
                if (!s_has_data) {
                    strncpy(buf, "--", sizeof(buf));
                } else {
                    switch (s_slots[i].option) {
                        case BOTTOM_OPTION_ALTITUDE:
                            snprintf(buf, sizeof(buf), "%d m", s_altitude);
                            break;
                        case BOTTOM_OPTION_AIR_PRESSURE:
                            snprintf(buf, sizeof(buf), "%d hPa", s_pressure);
                            break;
                        case BOTTOM_OPTION_PRESSURE_TREND:
                            snprintf(buf, sizeof(buf), "%+d", s_pressure_trend);
                            break;
                        case BOTTOM_OPTION_TEMPERATURE:
                            snprintf(buf, sizeof(buf), "%d", s_temperature);
                            break;
                        default:
                            strncpy(buf, "--", sizeof(buf));
                            break;
                    }
                }
                bottom_complication_set_text(layer, buf);
                break;
            }
            default:
                break;
        }
    }
}

void weather_provider_on_data(DictionaryIterator *iter) {
    Tuple *t;
    if ((t = dict_find(iter, MESSAGE_KEY_WEATHER_TEMPERATURE)))
        s_temperature = (int16_t)t->value->int32;
    if ((t = dict_find(iter, MESSAGE_KEY_WEATHER_PRESSURE)))
        s_pressure = (int16_t)t->value->int32;
    if ((t = dict_find(iter, MESSAGE_KEY_WEATHER_PRESSURE_TREND)))
        s_pressure_trend = (int16_t)t->value->int32;
    if ((t = dict_find(iter, MESSAGE_KEY_WEATHER_ALTITUDE)))
        s_altitude = (int16_t)t->value->int32;
    if ((t = dict_find(iter, MESSAGE_KEY_WEATHER_CONDITION)))
        s_condition = (int16_t)t->value->int32;

    s_has_data = true;
}

void weather_provider_deinit(void) {
    memset(s_slots, 0, sizeof(s_slots));
}
