#include "battery_provider.h"
#include "../complications/graph.h"
#include "../complications/miniview.h"

typedef struct {
    bool active;
    uint8_t option;
} SlotState;

static SlotState s_slots[COMPLICATION_COUNT];
static int16_t s_battery_history[GRAPH_MAX_VALUES];
static uint8_t s_battery_count;

void battery_provider_init(void) {
    memset(s_slots, 0, sizeof(s_slots));
    s_battery_count = 0;
}

void battery_provider_activate(ComplicationSlot slot, uint8_t option) {
    s_slots[slot].active = true;
    s_slots[slot].option = option;

    Layout *layout = providers_get_layout();
    GFont font_20 = providers_get_font_20();
    GFont font_28 = providers_get_font_28();
    Layer *window_layer = providers_get_window_layer();

    Layer *layer = NULL;

    switch (slot) {
        case COMPLICATION_GRAPH: {
            layer = graph_create(layout->graph_layer_bounds,
                                 layout->graph_plot_bounds, (GraphConfig) {
                .style = GRAPH_STYLE_LINE,
                .h_markers = 4,
                .v_markers = 3,
                .top_lip = true,
                .label_font = font_20,
                .icon_resource_id = RESOURCE_ID_ICON_STEPS,
                .label_text = "BAT",
            });
            BatteryChargeState state = battery_state_service_peek();
            s_battery_history[0] = state.charge_percent;
            s_battery_count = 1;
            graph_set_values(layer, s_battery_history, s_battery_count, 0, 100);
            break;
        }
        case COMPLICATION_MINIVIEW: {
            layer = miniview_create(layout->miniview_bounds, (MiniviewConfig) {
                .mode = MINIVIEW_MODE_TEXT_STACK,
                .tiny_text_bounds = layout->miniview_tiny_text_bounds,
                .small_text_bounds = layout->miniview_small_text_bounds,
                .tiny_font = font_20,
                .small_font = font_28,
            });
            BatteryChargeState state = battery_state_service_peek();
            char buf[8];
            if (option == MINIVIEW_OPTION_BATTERY_DND) {
                bool bt = connection_service_peek_pebble_app_connection();
                miniview_set_tiny_text(layer, bt ? "BT" : "NO BT");
            } else {
                miniview_set_tiny_text(layer, "BAT");
            }
            snprintf(buf, sizeof(buf), "%d%%", state.charge_percent);
            miniview_set_small_text(layer, buf);
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

void battery_provider_deactivate(ComplicationSlot slot) {
    s_slots[slot].active = false;
}

void battery_provider_tick(struct tm *tick_time) {
    (void)tick_time;

    BatteryChargeState state = battery_state_service_peek();

    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        if (!s_slots[i].active) continue;
        Layer *layer = providers_get_layer(i);
        if (!layer) continue;

        switch (i) {
            case COMPLICATION_GRAPH: {
                if (s_battery_count < GRAPH_MAX_VALUES) {
                    s_battery_history[s_battery_count++] = state.charge_percent;
                } else {
                    memmove(s_battery_history, s_battery_history + 1,
                            (GRAPH_MAX_VALUES - 1) * sizeof(int16_t));
                    s_battery_history[GRAPH_MAX_VALUES - 1] = state.charge_percent;
                }
                graph_set_values(layer, s_battery_history, s_battery_count, 0, 100);
                break;
            }
            case COMPLICATION_MINIVIEW: {
                char buf[8];
                if (s_slots[i].option == MINIVIEW_OPTION_BATTERY_DND) {
                    bool bt = connection_service_peek_pebble_app_connection();
                    miniview_set_tiny_text(layer, bt ? "BT" : "NO BT");
                } else {
                    miniview_set_tiny_text(layer, "BAT");
                }
                snprintf(buf, sizeof(buf), "%d%%", state.charge_percent);
                miniview_set_small_text(layer, buf);
                break;
            }
            default:
                break;
        }
    }
}

void battery_provider_deinit(void) {
    memset(s_slots, 0, sizeof(s_slots));
}
