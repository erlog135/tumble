#include "health_provider.h"
#include "history.h"
#include "../complications/graph.h"
#include "../complications/miniview.h"
#include "../complications/bottom.h"
#include "../settings.h"

#if defined(PBL_HEALTH)
// Steps (per hour): 0–1270 → 0–127  (1 unit = 10 steps)
static int8_t prv_scale_steps(int steps) {
  if (steps <= 0) return HISTORY_INVALID;
  return (int8_t)(steps > 1270 ? 127 : steps / 10);
}

// Heart rate: 0–254 bpm → 0–127  (1 unit = 2 bpm).  0 bpm = no reading → invalid.
static int8_t prv_scale_hr(int bpm) {
  if (bpm <= 0) return HISTORY_INVALID;
  return (int8_t)(bpm > 254 ? 127 : bpm / 2);
}

static void prv_build_steps_history(int8_t *out, uint8_t *out_count) {
  time_t now = time(NULL);
  for (int i = 0; i < HISTORY_24H_LEN; i++) {
    time_t end   = now - (time_t)(i * 3600);
    time_t start = end - 3600;
    HealthValue v = health_service_sum(HealthMetricStepCount, start, end);
    out[i] = prv_scale_steps((int)v);
  }
  *out_count = HISTORY_24H_LEN;
}

static void prv_build_hr_history(int8_t *out, uint8_t *out_count) {
  time_t now = time(NULL);
  for (int i = 0; i < HISTORY_4H_LEN; i++) {
    time_t end   = now - (time_t)(i * 10 * 60);
    time_t start = end - (10 * 60);
    HealthValue avg = health_service_aggregate_averaged(
        HealthMetricHeartRateBPM, start, end,
        HealthAggregationAvg, HealthServiceTimeScopeOnce);
    out[i] = prv_scale_hr((int)avg);
  }
  *out_count = HISTORY_4H_LEN;
}
#endif

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
#if defined(PBL_HEALTH)
static bool s_subscribed;
#endif

#if defined(PBL_HEALTH)
static void prv_health_handler(HealthEventType event, void *context) {
  (void)event;
  (void)context;
}
#endif

static void prv_update_graph(Layer *layer, uint8_t option) {
  int8_t vals[HISTORY_24H_LEN];
  uint8_t count = 0;
#if defined(PBL_HEALTH)
  if (option == GRAPH_OPTION_HEART_RATE) {
    prv_build_hr_history(vals, &count);
  } else {
    prv_build_steps_history(vals, &count);
  }
#endif
  graph_set_values(layer, vals, count);

  char buf[GRAPH_LABEL_MAX];
  buf[0] = '\0';
#if defined(PBL_HEALTH)
  if (option == GRAPH_OPTION_HEART_RATE) {
    HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
    if (hr > 0) snprintf(buf, sizeof(buf), "%d bpm", (int)hr);
  } else {
    HealthValue steps = health_service_sum_today(HealthMetricStepCount);
    if (steps > 0) snprintf(buf, sizeof(buf), "%d", (int)steps);
  }
#endif
  const char *dash = (option == GRAPH_OPTION_HEART_RATE) ? "--" : "-----";
  graph_set_label_text(layer, buf[0] ? buf : dash);
}

void health_provider_init(void) {
  memset(s_slots, 0, sizeof(s_slots));
#if defined(PBL_HEALTH)
  s_subscribed = false;
#endif
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
  GFont font_small = providers_get_font_small();
  GFont font_medium = providers_get_font_medium();
  Layer *window_layer = providers_get_window_layer();
  Layer *layer = NULL;

  switch (slot) {
    case COMPLICATION_GRAPH: {
      GraphStyle style = (option == GRAPH_OPTION_HEART_RATE)
        ? GRAPH_STYLE_LINE : GRAPH_STYLE_BARS;
      layer = graph_create(layout->graph_layer_bounds,
                           layout->graph_plot_bounds, (GraphConfig) {
        .style = style,
        .h_markers = 3,
        .v_markers = 3,
        .top_lip = true,
        .label_font = font_small,
        .icon_resource_id = prv_icon_for_graph_option(option),
      });
      prv_update_graph(layer, option);
      break;
    }
    case COMPLICATION_MINIVIEW: {
      layer = miniview_create(layout->miniview_bounds, (MiniviewConfig) {
        .mode = MINIVIEW_MODE_ICON_TEXT,
        .small_text_bounds = layout->miniview_small_text_bounds,
        .medium_text_bounds = layout->miniview_medium_text_bounds,
        .small_font = font_small,
        .medium_font = font_medium,
        .icon_resource_id = prv_icon_for_miniview_option(option),
      });
      const char *miniview_init = (option == MINIVIEW_OPTION_CALORIES) ? "----"
                               : (option == MINIVIEW_OPTION_STEPS)    ? "-----"
                               : "--";
      miniview_set_medium_text(layer, miniview_init);
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
        .font = font_small,
        .icon_resource_id = prv_icon_for_bottom_option(option),
      });
      const char *bottom_init = (option == BOTTOM_OPTION_CALORIES) ? "----"
                              : (option == BOTTOM_OPTION_STEPS)    ? "-----"
                              : "--";
      bottom_complication_set_text(layer, bottom_init);
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

#if defined(PBL_HEALTH)
  bool any_active = false;
  for (int i = 0; i < COMPLICATION_COUNT; i++) {
    if (s_slots[i].active) { any_active = true; break; }
  }
  if (!any_active && s_subscribed) {
    health_service_events_unsubscribe();
    s_subscribed = false;
  }
#endif
}

static void prv_update_miniview(Layer *layer, uint8_t option) {
  char buf[12];
  if (option == MINIVIEW_OPTION_CALORIES)      strncpy(buf, "----",  sizeof(buf));
  else if (option == MINIVIEW_OPTION_STEPS)    strncpy(buf, "-----", sizeof(buf));
  else                                         strncpy(buf, "--",    sizeof(buf));

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

  miniview_set_medium_text(layer, buf);
}

static void prv_update_bottom(Layer *layer, uint8_t option) {
  char buf[20];
  if (option == BOTTOM_OPTION_CALORIES)      strncpy(buf, "----",  sizeof(buf));
  else if (option == BOTTOM_OPTION_STEPS)    strncpy(buf, "-----", sizeof(buf));
  else                                       strncpy(buf, "--",    sizeof(buf));

#if defined(PBL_HEALTH)
  switch (option) {
    case BOTTOM_OPTION_STEPS: {
      HealthValue steps = health_service_sum_today(HealthMetricStepCount);
      if (steps > 0) snprintf(buf, sizeof(buf), "%d", (int)steps);
      break;
    }
    case BOTTOM_OPTION_HEART_RATE: {
      HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
      if (hr > 0) snprintf(buf, sizeof(buf), "%d bpm", (int)hr);
      break;
    }
    case BOTTOM_OPTION_CALORIES: {
      HealthValue cal = health_service_sum_today(HealthMetricActiveKCalories);
      if (cal > 0) snprintf(buf, sizeof(buf), "%d cal", (int)cal);
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
