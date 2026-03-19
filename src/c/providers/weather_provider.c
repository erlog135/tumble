#include "weather_provider.h"
#include "history.h"
#include "../complications/graph.h"
#include "../complications/miniview.h"
#include "../complications/bottom.h"
#include "../settings.h"

// Temperature: −50°C to +77°C → 0–127  (offset +50, 1 unit = 1°C)
static int8_t prv_scale_temperature(int16_t temp) {
  int16_t v = temp + 50;
  if (v < 0) v = 0;
  if (v > 127) v = 127;
  return (int8_t)v;
}

// Pressure: 870–1085 hPa → 0–127
static int8_t prv_scale_pressure(int16_t hpa) {
  int32_t v = ((int32_t)(hpa - 870) * 127) / 215;
  if (v < 0) v = 0;
  if (v > 127) v = 127;
  return (int8_t)v;
}

// Altitude: −500 to +5000 m → 0–127
static int8_t prv_scale_altitude(int16_t m) {
  int32_t v = ((int32_t)(m + 500) * 127) / 5500;
  if (v < 0) v = 0;
  if (v > 127) v = 127;
  return (int8_t)v;
}

static uint32_t prv_icon_for_graph_option(uint8_t option) {
  switch (option) {
    case GRAPH_OPTION_AIR_PRESSURE: return RESOURCE_ID_ICON_PRESSURE;
    case GRAPH_OPTION_ALTITUDE:     return RESOURCE_ID_ICON_ALTITUDE;
    default:                        return 0;
  }
}

static uint32_t prv_icon_for_miniview_option(uint8_t option) {
  switch (option) {
    case MINIVIEW_OPTION_AIR_PRESSURE: return RESOURCE_ID_ICON_PRESSURE;
    case MINIVIEW_OPTION_ALTITUDE:     return RESOURCE_ID_ICON_ALTITUDE;
    default:                           return 0;
  }
}

static uint32_t prv_icon_for_bottom_option(uint8_t option) {
  switch (option) {
    case BOTTOM_OPTION_AIR_PRESSURE:   return RESOURCE_ID_ICON_PRESSURE;
    case BOTTOM_OPTION_PRESSURE_TREND: return RESOURCE_ID_ICON_PRESSURE_TREND_SS;
    case BOTTOM_OPTION_ALTITUDE:       return RESOURCE_ID_ICON_ALTITUDE;
    default:                           return 0;
  }
}

static uint32_t prv_pressure_trend_icon(int16_t trend) {
  if (trend <= -3) return RESOURCE_ID_ICON_PRESSURE_TREND_DD;
  if (trend == -2) return RESOURCE_ID_ICON_PRESSURE_TREND_DS;
  if (trend == -1) return RESOURCE_ID_ICON_PRESSURE_TREND_SD;
  if (trend ==  0) return RESOURCE_ID_ICON_PRESSURE_TREND_SS;
  if (trend ==  1) return RESOURCE_ID_ICON_PRESSURE_TREND_SU;
  if (trend ==  2) return RESOURCE_ID_ICON_PRESSURE_TREND_US;
  return RESOURCE_ID_ICON_PRESSURE_TREND_UU;
}

typedef struct {
  bool active;
  uint8_t option;
} SlotState;

// All three arrays are always pushed together so a single count/last_hour covers all.
typedef struct {
  int8_t  pressure[HISTORY_24H_LEN];    // most-recent-first
  int8_t  altitude[HISTORY_24H_LEN];
  int8_t  temperature[HISTORY_24H_LEN];
  uint8_t count;
  uint8_t last_hour;  // 0–23; 0xFF = never sampled
} WeatherHistory;

static SlotState s_slots[COMPLICATION_COUNT];
static int16_t s_temperature;
static int16_t s_pressure;
static int16_t s_pressure_trend;
static int16_t s_altitude;
static int16_t s_condition;
static bool s_has_data;
static WeatherHistory s_history;

// Push one sample for all three weather series simultaneously.
static void prv_history_push_all(int8_t pres, int8_t alt, int8_t temp) {
  uint8_t n = s_history.count;
  uint8_t max = HISTORY_24H_LEN;
  if (n < max) {
    memmove(s_history.pressure    + 1, s_history.pressure,    n * sizeof(int8_t));
    memmove(s_history.altitude    + 1, s_history.altitude,    n * sizeof(int8_t));
    memmove(s_history.temperature + 1, s_history.temperature, n * sizeof(int8_t));
    s_history.count = n + 1;
  } else {
    memmove(s_history.pressure    + 1, s_history.pressure,    (max - 1) * sizeof(int8_t));
    memmove(s_history.altitude    + 1, s_history.altitude,    (max - 1) * sizeof(int8_t));
    memmove(s_history.temperature + 1, s_history.temperature, (max - 1) * sizeof(int8_t));
  }
  s_history.pressure[0]    = pres;
  s_history.altitude[0]    = alt;
  s_history.temperature[0] = temp;
}

static void prv_update_graph(Layer *layer, uint8_t option) {
  const int8_t *src = NULL;
  switch (option) {
    case GRAPH_OPTION_AIR_PRESSURE: src = s_history.pressure;    break;
    case GRAPH_OPTION_ALTITUDE:     src = s_history.altitude;    break;
    case GRAPH_OPTION_TEMPERATURE:  src = s_history.temperature; break;
    default: break;
  }
  if (src) graph_set_values(layer, src, s_history.count);

  char buf[12];
  if (!s_has_data) {
    strncpy(buf, "--", sizeof(buf));
  } else {
    switch (option) {
      case GRAPH_OPTION_AIR_PRESSURE:
        snprintf(buf, sizeof(buf), "%d hPa", s_pressure);   break;
      case GRAPH_OPTION_ALTITUDE:
        snprintf(buf, sizeof(buf), "%d m",   s_altitude);   break;
      case GRAPH_OPTION_TEMPERATURE:
        snprintf(buf, sizeof(buf), "%d\u00b0", s_temperature); break;
      default:
        strncpy(buf, "--", sizeof(buf));                     break;
    }
  }
  graph_set_label_text(layer, buf);
}

void weather_provider_init(void) {
  memset(s_slots, 0, sizeof(s_slots));
  s_has_data = false;
  memset(&s_history, 0, sizeof(s_history));
  s_history.last_hour = 0xFF;
  persist_read_data(PERSIST_KEY_WEATHER_HISTORY, &s_history, sizeof(s_history));
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
      layer = graph_create(layout->graph_layer_bounds,
                           layout->graph_plot_bounds, (GraphConfig) {
        .style = GRAPH_STYLE_LINE,
        .h_markers = 5,
        .v_markers = 3,
        .top_lip = true,
        .label_font = font_20,
        .icon_resource_id = prv_icon_for_graph_option(option),
      });
      prv_update_graph(layer, option);
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

void weather_provider_deactivate(ComplicationSlot slot) {
  s_slots[slot].active = false;
}

void weather_provider_record_history(struct tm *tick_time) {
  uint8_t cur_hour = (uint8_t)tick_time->tm_hour;
  if (cur_hour == s_history.last_hour) return;

  int8_t pres = s_has_data ? prv_scale_pressure(s_pressure)       : HISTORY_INVALID;
  int8_t alt  = s_has_data ? prv_scale_altitude(s_altitude)       : HISTORY_INVALID;
  int8_t temp = s_has_data ? prv_scale_temperature(s_temperature) : HISTORY_INVALID;

  prv_history_push_all(pres, alt, temp);
  s_history.last_hour = cur_hour;
  persist_write_data(PERSIST_KEY_WEATHER_HISTORY, &s_history, sizeof(s_history));
}

void weather_provider_tick(struct tm *tick_time) {
  (void)tick_time;

  for (int i = 0; i < COMPLICATION_COUNT; i++) {
    if (!s_slots[i].active) continue;
    Layer *layer = providers_get_layer(i);
    if (!layer) continue;

    switch (i) {
      case COMPLICATION_GRAPH:
        prv_update_graph(layer, s_slots[i].option);
        break;
      case COMPLICATION_MINIVIEW: {
        char buf[12];
        if (!s_has_data) {
          strncpy(buf, "--", sizeof(buf));
        } else {
          switch (s_slots[i].option) {
            case MINIVIEW_OPTION_ALTITUDE:
              snprintf(buf, sizeof(buf), "%d",   s_altitude);   break;
            case MINIVIEW_OPTION_AIR_PRESSURE:
              snprintf(buf, sizeof(buf), "%d",   s_pressure);   break;
            case MINIVIEW_OPTION_WEATHER:
              snprintf(buf, sizeof(buf), "%d",   s_temperature); break;
            default:
              strncpy(buf, "--", sizeof(buf));                   break;
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
              snprintf(buf, sizeof(buf), "%d m",   s_altitude);         break;
            case BOTTOM_OPTION_AIR_PRESSURE:
              snprintf(buf, sizeof(buf), "%d hPa", s_pressure);         break;
            case BOTTOM_OPTION_PRESSURE_TREND:
              snprintf(buf, sizeof(buf), "%+d",    s_pressure_trend);
              bottom_complication_set_icon(layer, prv_pressure_trend_icon(s_pressure_trend));
              break;
            case BOTTOM_OPTION_TEMPERATURE:
              snprintf(buf, sizeof(buf), "%d",     s_temperature);      break;
            default:
              strncpy(buf, "--", sizeof(buf));                           break;
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
