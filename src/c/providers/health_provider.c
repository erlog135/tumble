#include "health_provider.h"
#include "../complications/graph.h"
#include "../complications/miniview.h"
#include "../complications/bottom.h"
#include "../settings.h"

static uint32_t prv_icon_for_graph_option(uint8_t option) {
    (void)option;
    return RESOURCE_ID_ICON_STEPS;
}

static uint32_t prv_icon_for_miniview_option(uint8_t option) {
    switch (option) {
        case MINIVIEW_OPTION_CALORIES: return RESOURCE_ID_ICON_CALORIES;
        default:                       return RESOURCE_ID_ICON_STEPS;
    }
}

static uint32_t prv_icon_for_bottom_option(uint8_t option) {
    switch (option) {
        case BOTTOM_OPTION_CALORIES: return RESOURCE_ID_ICON_CALORIES;
        default:                     return RESOURCE_ID_ICON_STEPS;
    }
}

typedef struct {
    bool active;
    uint8_t option;
} SlotState;

static SlotState s_slots[COMPLICATION_COUNT];
static bool s_subscribed;

#if defined(PBL_HEALTH)
static void prv_health_handler(HealthEventType event, void *context) {
    (void)event;
    (void)context;
}
#endif

void health_provider_init(void) {
    memset(s_slots, 0, sizeof(s_slots));
    s_subscribed = false;
}

void health_provider_activate(ComplicationSlot slot, uint8_t option) {
    s_slots[slot].active = true;
    s_slots[slot].option = option;

#if defined(PBL_HEALTH)
    if (!s_subscribed) {
        health_service_events_subscribe(prv_health_handler, NULL);
        s_subscribed = true;
    }
#endif

    Layout *layout = providers_get_layout();
    GFont font_20 = providers_get_font_20();
    GFont font_28 = providers_get_font_28();
    Layer *window_layer = providers_get_window_layer();

    Layer *layer = NULL;

    switch (slot) {
        case COMPLICATION_GRAPH: {
            GraphStyle style = (option == GRAPH_OPTION_HEART_RATE)
                ? GRAPH_STYLE_LINE : GRAPH_STYLE_BARS;
            layer = graph_create(layout->graph_layer_bounds,
                                 layout->graph_plot_bounds, (GraphConfig) {
                .style = style,
                .h_markers = 4,
                .v_markers = 3,
                .top_lip = true,
                .label_font = font_20,
                .icon_resource_id = prv_icon_for_graph_option(option),
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
                .icon_resource_id = prv_icon_for_miniview_option(option),
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
                .icon_resource_id = prv_icon_for_bottom_option(option),
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

void health_provider_deactivate(ComplicationSlot slot) {
    s_slots[slot].active = false;

    bool any_active = false;
    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        if (s_slots[i].active) { any_active = true; break; }
    }
#if defined(PBL_HEALTH)
    if (!any_active && s_subscribed) {
        health_service_events_unsubscribe();
        s_subscribed = false;
    }
#endif
}

static void prv_update_graph(Layer *layer, uint8_t option) {
#if defined(PBL_HEALTH)
    HealthMinuteData *minute_data = malloc(GRAPH_MAX_VALUES * sizeof(HealthMinuteData));
    if (!minute_data) return;

    time_t end = time(NULL) - (15 * SECONDS_PER_MINUTE);
    time_t start = end - (GRAPH_MAX_VALUES * SECONDS_PER_MINUTE);

    uint32_t num = health_service_get_minute_history(
        minute_data, GRAPH_MAX_VALUES, &start, &end);

    int16_t values[GRAPH_MAX_VALUES];
    int16_t max_val = 1;
    uint8_t count = (num < GRAPH_MAX_VALUES) ? (uint8_t)num : GRAPH_MAX_VALUES;

    for (uint8_t i = 0; i < count; i++) {
        if (minute_data[i].is_invalid) {
            values[i] = 0;
        } else if (option == GRAPH_OPTION_HEART_RATE) {
            values[i] = (int16_t)minute_data[i].heart_rate_bpm;
        } else {
            values[i] = (int16_t)minute_data[i].steps;
        }
        if (values[i] > max_val) max_val = values[i];
    }

    graph_set_values(layer, values, count, 0, max_val);

    {
        char buf[GRAPH_LABEL_MAX];
        buf[0] = '\0';
        if (option == GRAPH_OPTION_HEART_RATE) {
            HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
            if (hr > 0) snprintf(buf, sizeof(buf), "%d bpm", (int)hr);
        } else {
            HealthValue steps = health_service_sum_today(HealthMetricStepCount);
            if (steps >= 1000)
                snprintf(buf, sizeof(buf), "%d,%03d",
                         (int)(steps / 1000), (int)(steps % 1000));
            else if (steps > 0)
                snprintf(buf, sizeof(buf), "%d", (int)steps);
        }
        graph_set_label_text(layer, buf[0] ? buf : NULL);
    }
    free(minute_data);
#else
    (void)layer;
    (void)option;
#endif
}

static void prv_update_miniview(Layer *layer, uint8_t option) {
    char buf[12];
    strncpy(buf, "--", sizeof(buf));

#if defined(PBL_HEALTH)
    switch (option) {
        case MINIVIEW_OPTION_HEART_RATE: {
            HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
            if (hr > 0) snprintf(buf, sizeof(buf), "%d", (int)hr);
            break;
        }
        case MINIVIEW_OPTION_STEPS: {
            HealthValue steps = health_service_sum_today(HealthMetricStepCount);
            if (steps >= 10000)
                snprintf(buf, sizeof(buf), "%d.%dk", (int)(steps / 1000),
                         (int)((steps % 1000) / 100));
            else if (steps > 0)
                snprintf(buf, sizeof(buf), "%d", (int)steps);
            break;
        }
        case MINIVIEW_OPTION_CALORIES: {
            HealthValue cal = health_service_sum_today(HealthMetricActiveKCalories);
            if (cal > 0) snprintf(buf, sizeof(buf), "%d", (int)cal);
            break;
        }
        default:
            break;
    }
#else
    (void)option;
#endif

    miniview_set_small_text(layer, buf);
}

static void prv_update_bottom(Layer *layer, uint8_t option) {
    char buf[20];
    strncpy(buf, "--", sizeof(buf));

#if defined(PBL_HEALTH)
    switch (option) {
        case BOTTOM_OPTION_STEPS: {
            HealthValue steps = health_service_sum_today(HealthMetricStepCount);
            if (steps >= 1000)
                snprintf(buf, sizeof(buf), "%d,%03d",
                         (int)(steps / 1000), (int)(steps % 1000));
            else
                snprintf(buf, sizeof(buf), "%d", (int)steps);
            break;
        }
        case BOTTOM_OPTION_HEART_RATE: {
            HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
            if (hr > 0)
                snprintf(buf, sizeof(buf), "%d bpm", (int)hr);
            break;
        }
        case BOTTOM_OPTION_CALORIES: {
            HealthValue cal = health_service_sum_today(HealthMetricActiveKCalories);
            snprintf(buf, sizeof(buf), "%d cal", (int)cal);
            break;
        }
        default:
            break;
    }
#else
    (void)option;
#endif

    bottom_complication_set_text(layer, buf);
}

void health_provider_tick(struct tm *tick_time) {
    (void)tick_time;

    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        if (!s_slots[i].active) continue;
        Layer *layer = providers_get_layer(i);
        if (!layer) continue;

        switch (i) {
            case COMPLICATION_GRAPH:
                prv_update_graph(layer, s_slots[i].option);
                break;
            case COMPLICATION_MINIVIEW:
                prv_update_miniview(layer, s_slots[i].option);
                break;
            case COMPLICATION_BOTTOM_LEFT:
            case COMPLICATION_BOTTOM_RIGHT:
                prv_update_bottom(layer, s_slots[i].option);
                break;
            default:
                break;
        }
    }
}

void health_provider_deinit(void) {
#if defined(PBL_HEALTH)
    if (s_subscribed) {
        health_service_events_unsubscribe();
        s_subscribed = false;
    }
#endif
    memset(s_slots, 0, sizeof(s_slots));
}
