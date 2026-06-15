#include "health_provider.h"
#include "history.h"
#include "../complications/graph.h"
#include "../complications/miniview.h"
#include "../complications/bottom.h"
#include "../settings.h"
#include <string.h>

typedef struct {
  bool active;
  uint8_t option;
} SlotState;

static SlotState s_slots[COMPLICATION_COUNT];

#if defined(PBL_HEALTH)
// Steps (per hour): 0–3000 → 0–127 for GraphConfig (fixed_range 0–127).
static int8_t prv_scale_steps(int steps) {
  if (steps <= 0) return HISTORY_INVALID;
  if (steps > 1000) steps = 1000;
  return (int8_t)((steps * 127) / 1000);
}

// HR graph: 0–200 bpm → 0–127 for GraphConfig (fixed_range 0–127 ≡ 0–200 bpm).
static int8_t prv_scale_hr_graph(int bpm) {
  if (bpm <= 0) return HISTORY_INVALID;
  if (bpm > 200) bpm = 200;
  return (int8_t)((bpm * 127) / 200);
}

// Steps graph: local clock hours, filled from sum_today deltas while any steps widget is
// active (same idea as peek-based HR). Not read from minute history.
typedef struct {
  int8_t    points[HISTORY_24H_LEN];
  time_t    hour_start_epoch;
  HealthValue baseline;
  uint8_t   hist_ready;
} StepsGraphHistory;

static StepsGraphHistory s_steps_graph;

static time_t prv_local_hour_start(time_t now) {
  struct tm *lt_ptr = localtime(&now);
  if (!lt_ptr) {
    return now - (now % 3600);
  }
  struct tm lt = *lt_ptr;
  lt.tm_min = 0;
  lt.tm_sec = 0;
  return mktime(&lt);
}

static void prv_steps_graph_persist(void) {
  persist_write_data(PERSIST_KEY_STEPS_HISTORY, &s_steps_graph, sizeof(s_steps_graph));
}

// Called once per tick when any complication shows steps — keeps graph in sync without
// health_service_get_minute_history.
static void prv_steps_graph_history_update(void) {
  time_t now = time(NULL);
  time_t hs_now = prv_local_hour_start(now);
  HealthValue now_total = health_service_sum_today(HealthMetricStepCount);

  if (!s_steps_graph.hist_ready) {
    memset(s_steps_graph.points, HISTORY_INVALID, sizeof(s_steps_graph.points));
    s_steps_graph.hour_start_epoch = hs_now;
    s_steps_graph.baseline = now_total;
    s_steps_graph.hist_ready = 1;
  } else if (hs_now < s_steps_graph.hour_start_epoch) {
    memset(s_steps_graph.points, HISTORY_INVALID, sizeof(s_steps_graph.points));
    s_steps_graph.hour_start_epoch = hs_now;
    s_steps_graph.baseline = now_total;
  } else {
    time_t diff = hs_now - s_steps_graph.hour_start_epoch;
    int dh = (int)(diff / 3600);
    if (dh > 0) {
      if (dh >= HISTORY_24H_LEN) {
        memset(s_steps_graph.points, HISTORY_INVALID, sizeof(s_steps_graph.points));
      } else {
        for (int k = 0; k < dh; k++) {
          memmove(&s_steps_graph.points[1], &s_steps_graph.points[0],
                  (HISTORY_24H_LEN - 1) * sizeof(int8_t));
          s_steps_graph.points[0] = HISTORY_INVALID;
        }
      }
      s_steps_graph.hour_start_epoch = hs_now;
      s_steps_graph.baseline = now_total;
    }
  }

  int cur = (int)now_total - (int)s_steps_graph.baseline;
  if (cur < 0) {
    cur = 0;
  }
  s_steps_graph.points[0] = prv_scale_steps(cur);
  prv_steps_graph_persist();
}

static bool prv_any_steps_slot_active(void) {
  for (int i = 0; i < COMPLICATION_COUNT; i++) {
    if (!s_slots[i].active) {
      continue;
    }
    uint8_t o = s_slots[i].option;
    if (i == COMPLICATION_GRAPH && o == GRAPH_OPTION_STEPS) {
      return true;
    }
    if (i == COMPLICATION_MINIVIEW && o == MINIVIEW_OPTION_STEPS) {
      return true;
    }
    if ((i == COMPLICATION_BOTTOM_LEFT || i == COMPLICATION_BOTTOM_RIGHT)
        && o == BOTTOM_OPTION_STEPS) {
      return true;
    }
  }
  return false;
}

static bool prv_any_hr_slot_active(void) {
  for (int i = 0; i < COMPLICATION_COUNT; i++) {
    if (!s_slots[i].active) {
      continue;
    }
    uint8_t o = s_slots[i].option;
    if (i == COMPLICATION_GRAPH && o == GRAPH_OPTION_HEART_RATE) {
      return true;
    }
    if (i == COMPLICATION_MINIVIEW && o == MINIVIEW_OPTION_HEART_RATE) {
      return true;
    }
    if ((i == COMPLICATION_BOTTOM_LEFT || i == COMPLICATION_BOTTOM_RIGHT)
        && o == BOTTOM_OPTION_HEART_RATE) {
      return true;
    }
  }
  return false;
}

// Peek-based HR graph: one value per 10-minute bucket (4h window). Persisted like
// battery history; updated while any complication shows heart rate.
typedef struct {
  int8_t   points[HISTORY_4H_LEN];
  uint32_t bucket_epoch;
  uint32_t sum_bpm;
  uint8_t  peek_count;
  uint8_t  hist_ready;
} HeartRateGraphHistory;

static HeartRateGraphHistory s_hr_graph;

static void prv_hr_graph_history_persist(void) {
  persist_write_data(PERSIST_KEY_HEALTH_HISTORY, &s_hr_graph, sizeof(s_hr_graph));
}

static void prv_hr_graph_history_update_from_peek(void) {
  time_t now = time(NULL);
  uint32_t bucket = (uint32_t)(now / 600);

  if (!s_hr_graph.hist_ready) {
    memset(s_hr_graph.points, HISTORY_INVALID, sizeof(s_hr_graph.points));
    s_hr_graph.bucket_epoch = bucket;
    s_hr_graph.hist_ready = 1;
    s_hr_graph.sum_bpm = 0;
    s_hr_graph.peek_count = 0;
  } else if (bucket < s_hr_graph.bucket_epoch) {
    memset(s_hr_graph.points, HISTORY_INVALID, sizeof(s_hr_graph.points));
    s_hr_graph.bucket_epoch = bucket;
    s_hr_graph.sum_bpm = 0;
    s_hr_graph.peek_count = 0;
    s_hr_graph.hist_ready = 1;
  } else {
    uint32_t delta = bucket - s_hr_graph.bucket_epoch;
    if (delta > 0) {
      if (delta >= HISTORY_4H_LEN) {
        memset(s_hr_graph.points, HISTORY_INVALID, sizeof(s_hr_graph.points));
      } else {
        for (uint32_t k = 0; k < delta; k++) {
          memmove(&s_hr_graph.points[1], &s_hr_graph.points[0],
                  (HISTORY_4H_LEN - 1) * sizeof(int8_t));
          s_hr_graph.points[0] = HISTORY_INVALID;
        }
      }
      s_hr_graph.bucket_epoch = bucket;
      s_hr_graph.sum_bpm = 0;
      s_hr_graph.peek_count = 0;
    }
  }

  HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
  if (hr > 0) {
    s_hr_graph.sum_bpm += (uint32_t)hr;
    s_hr_graph.peek_count++;
    int avg_bpm = (int)(s_hr_graph.sum_bpm / s_hr_graph.peek_count);
    s_hr_graph.points[0] = prv_scale_hr_graph(avg_bpm);
  }

  prv_hr_graph_history_persist();
}
#endif

static uint32_t prv_icon_for_graph_option(uint8_t option) {
  switch (option) {
    case GRAPH_OPTION_HEART_RATE: return RESOURCE_ID_ICON_HEART_RATE;
    default:                      return RESOURCE_ID_ICON_STEPS;
  }
}

static uint32_t prv_icon_for_miniview_option(uint8_t option) {
  switch (option) {
    case MINIVIEW_OPTION_CALORIES:  return RESOURCE_ID_ICON_CALORIES;
    case MINIVIEW_OPTION_HEART_RATE: return RESOURCE_ID_ICON_HEART_RATE;
    default:                        return RESOURCE_ID_ICON_STEPS;
  }
}

static uint32_t prv_icon_for_bottom_option(uint8_t option) {
  switch (option) {
    case BOTTOM_OPTION_CALORIES:  return RESOURCE_ID_ICON_CALORIES;
    case BOTTOM_OPTION_HEART_RATE: return RESOURCE_ID_ICON_HEART_RATE;
    default:                       return RESOURCE_ID_ICON_STEPS;
  }
}

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
  int8_t vals[HISTORY_24H_LEN] = {0};
  uint8_t count = 0;
#if defined(PBL_HEALTH)
  if (option == GRAPH_OPTION_HEART_RATE) {
    memcpy(vals, s_hr_graph.points, sizeof(s_hr_graph.points));
    count = HISTORY_4H_LEN;
  } else {
    memcpy(vals, s_steps_graph.points, sizeof(s_steps_graph.points));
    count = HISTORY_24H_LEN;
  }
#endif
  graph_set_values(layer, vals, count);

  char buf[GRAPH_LABEL_MAX];
  buf[0] = '\0';
#if defined(PBL_HEALTH)
  if (option == GRAPH_OPTION_HEART_RATE) {
    HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
    if (hr > 0) snprintf(buf, sizeof(buf), "%d", (int)hr);
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
  memset(&s_hr_graph, 0, sizeof(s_hr_graph));
  memset(s_hr_graph.points, HISTORY_INVALID, sizeof(s_hr_graph.points));
  persist_read_data(PERSIST_KEY_HEALTH_HISTORY, &s_hr_graph, sizeof(s_hr_graph));
  memset(&s_steps_graph, 0, sizeof(s_steps_graph));
  memset(s_steps_graph.points, HISTORY_INVALID, sizeof(s_steps_graph.points));
  persist_read_data(PERSIST_KEY_STEPS_HISTORY, &s_steps_graph, sizeof(s_steps_graph));
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
  Layer *window_layer = providers_get_window_layer();
  Layer *layer = NULL;

  switch (slot) {
    case COMPLICATION_GRAPH: {
      GraphStyle style = (option == GRAPH_OPTION_HEART_RATE)
        ? GRAPH_STYLE_LINE : GRAPH_STYLE_BARS;
      uint8_t h_markers = (option == GRAPH_OPTION_HEART_RATE) ? 3 : 0;
      layer = graph_create(layout->graph_layer_bounds,
                           layout->graph_plot_bounds, (GraphConfig) {
        .style = style,
        .h_markers = h_markers,
        .v_markers = 0,
        .top_lip = (option == GRAPH_OPTION_STEPS),
        .fixed_range = (option == GRAPH_OPTION_STEPS
                        || option == GRAPH_OPTION_HEART_RATE),
        .fixed_min = 0,
        .fixed_max = 127,
        .label_font = font_small,
        .icon_resource_id = prv_icon_for_graph_option(option),
      });
      prv_update_graph(layer, option);
      break;
    }
    case COMPLICATION_MINIVIEW: {
      layer = miniview_create((MiniviewConfig) {
        .mode = MINIVIEW_MODE_ICON_TEXT,
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
        ? BOTTOM_ALIGN_LEFT : BOTTOM_ALIGN_RIGHT;
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

#if defined(PBL_HEALTH)
  if (prv_any_steps_slot_active()) {
    prv_steps_graph_history_update();
  }
  if (prv_any_hr_slot_active()) {
    prv_hr_graph_history_update_from_peek();
  }
  Layer *graph_layer = providers_get_layer(COMPLICATION_GRAPH);
  if (graph_layer && s_slots[COMPLICATION_GRAPH].active) {
    uint8_t go = s_slots[COMPLICATION_GRAPH].option;
    if (go == GRAPH_OPTION_STEPS && prv_any_steps_slot_active()) {
      prv_update_graph(graph_layer, go);
    } else if (go == GRAPH_OPTION_HEART_RATE && prv_any_hr_slot_active()) {
      prv_update_graph(graph_layer, go);
    }
  }
#endif
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
      if (steps >= 100000) {
        snprintf(buf, sizeof(buf), "%d.%dk", (int)(steps / 1000),
                 (int)((steps % 1000) / 100));
      } else if (steps > 0) {
        snprintf(buf, sizeof(buf), "%d", (int)steps);
      }
      break;
    }
    case MINIVIEW_OPTION_CALORIES: {
      HealthValue cal = health_service_sum_today(HealthMetricRestingKCalories) + health_service_sum_today(HealthMetricActiveKCalories);
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
      if (hr > 0) snprintf(buf, sizeof(buf), "%d", (int)hr);
      break;
    }
    case BOTTOM_OPTION_CALORIES: {
      HealthValue cal = health_service_sum_today(HealthMetricRestingKCalories) + health_service_sum_today(HealthMetricActiveKCalories);
      if (cal > 0) snprintf(buf, sizeof(buf), "%d", (int)cal);
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

#if defined(PBL_HEALTH)
  if (prv_any_steps_slot_active()) {
    prv_steps_graph_history_update();
  }
  if (prv_any_hr_slot_active()) {
    prv_hr_graph_history_update_from_peek();
  }
#endif

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
