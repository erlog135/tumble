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

static void prv_format_temperature(char *buf, size_t n, int16_t temp_f, uint8_t unit) {
  if (unit == UNIT_TEMP_C) {
    int16_t temp_c = (int16_t)((temp_f - 32) * 5 / 9);
    snprintf(buf, n, "%d\u00b0", temp_c);
  } else {
    snprintf(buf, n, "%d\u00b0", temp_f);
  }
}

static void prv_format_pressure(char *buf, size_t n, int16_t hpa, uint8_t unit) {
  if (unit == UNIT_PRESSURE_HPA) {
    snprintf(buf, n, "%d", hpa);
  } else if (unit == UNIT_PRESSURE_INHG) {
    int32_t hundredths = ((int32_t)hpa * 2953) / 1000;
    snprintf(buf, n, "%d.%02d", (int)(hundredths / 100), (int)(hundredths % 100));
  } else {
    snprintf(buf, n, "%d", hpa);
  }
}

static void prv_format_altitude(char *buf, size_t n, int16_t m, uint8_t unit) {
  if (unit == UNIT_ALTITUDE_FT) {
    int32_t ft = ((int32_t)m * 328) / 100;
    snprintf(buf, n, "%d", (int)ft);
  } else {
    snprintf(buf, n, "%d", m);
  }
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
    case BOTTOM_OPTION_AIR_PRESSURE: return RESOURCE_ID_ICON_PRESSURE;
    case BOTTOM_OPTION_ALTITUDE:     return RESOURCE_ID_ICON_ALTITUDE;
    default:                         return 0;
  }
}

// Compact condition enum — must stay in sync with wmoToCondition() in open_meteo.js
typedef enum {
  WEATHER_COND_UNKNOWN             = 0,
  WEATHER_COND_CLEAR               = 1,
  WEATHER_COND_PARTLY_CLOUDY       = 2,  // WMO 1–2
  WEATHER_COND_CLOUDY              = 3,  // WMO 3 overcast
  WEATHER_COND_FOGGY               = 4,
  WEATHER_COND_RAINY               = 5,  // drizzle, rain, rain showers
  WEATHER_COND_SNOWY_RAINY         = 6,  // freezing drizzle / freezing rain
  WEATHER_COND_SNOWY               = 7,  // snow fall, snow grains, snow showers
  WEATHER_COND_STORMY              = 8,  // thunderstorm ± hail
  WEATHER_COND_CLEAR_NIGHT         = 9,
  WEATHER_COND_PARTLY_CLOUDY_NIGHT = 10, // WMO 1–2 at night
} WeatherCondition;

static uint32_t prv_icon_for_condition(int16_t condition) {
  switch ((WeatherCondition)condition) {
    case WEATHER_COND_UNKNOWN:             return RESOURCE_ID_ICON_WEATHER_UNKNOWN;
    case WEATHER_COND_CLEAR:               return RESOURCE_ID_ICON_WEATHER_CLEAR;
    case WEATHER_COND_PARTLY_CLOUDY:       return RESOURCE_ID_ICON_WEATHER_PARTLY_CLOUDY;
    case WEATHER_COND_CLOUDY:              return RESOURCE_ID_ICON_WEATHER_CLOUDY;
    case WEATHER_COND_FOGGY:              return RESOURCE_ID_ICON_WEATHER_FOGGY;
    case WEATHER_COND_RAINY:              return RESOURCE_ID_ICON_WEATHER_RAINY;
    case WEATHER_COND_SNOWY_RAINY:        return RESOURCE_ID_ICON_WEATHER_SNOWY_RAINY;
    case WEATHER_COND_SNOWY:              return RESOURCE_ID_ICON_WEATHER_SNOWY;
    case WEATHER_COND_STORMY:             return RESOURCE_ID_ICON_WEATHER_STORMY;
    case WEATHER_COND_CLEAR_NIGHT:        return RESOURCE_ID_ICON_WEATHER_CLEAR_NIGHT;
    case WEATHER_COND_PARTLY_CLOUDY_NIGHT: return RESOURCE_ID_ICON_WEATHER_PARTLY_CLOUDY_NIGHT;
    default:                              return RESOURCE_ID_ICON_WEATHER_UNKNOWN;
  }
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

// Last received values. Restored on boot if less than one hour old.
typedef struct {
  int16_t temperature;
  int16_t pressure;
  int16_t pressure_trend;
  int16_t altitude;
  int16_t condition;
  time_t  timestamp;  // 0 = never written
} WeatherCache;

static SlotState s_slots[COMPLICATION_COUNT];
static int16_t s_temperature;
static int16_t s_pressure;
static int16_t s_pressure_trend;
static int16_t s_altitude;
static int16_t s_condition;
static bool s_has_data;
static WeatherHistory s_history;

// Derive the pressure-trend icon from the local 6-hour history.
// Splits the last 6 hourly samples into two 3-hour windows, classifies each
// as up / steady / down, and returns the matching icon.
// Returns RESOURCE_ID_PRESSURE_TREND_SAMPLING when fewer than 6 valid
// samples are available.
static uint32_t prv_compute_pressure_trend_icon(void) {
  if (s_history.count < 6) return RESOURCE_ID_PRESSURE_TREND_SAMPLING;
  for (int i = 0; i < 6; i++) {
    if (s_history.pressure[i] == HISTORY_INVALID) return RESOURCE_ID_PRESSURE_TREND_SAMPLING;
  }

  // history is most-recent-first: [0] = now, [2] = 3 h ago, [5] = 6 h ago
  // Positive delta means pressure rose over the window.
#define TREND_THRESHOLD 1
  int recent = 0, older = 0;
  int8_t rd = s_history.pressure[0] - s_history.pressure[2];
  int8_t od = s_history.pressure[3] - s_history.pressure[5];
  if      (rd >  TREND_THRESHOLD) recent =  1;
  else if (rd < -TREND_THRESHOLD) recent = -1;
  if      (od >  TREND_THRESHOLD) older  =  1;
  else if (od < -TREND_THRESHOLD) older  = -1;
#undef TREND_THRESHOLD

  // Icon name encodes [older window][recent window].
  if (older == -1 && recent == -1) return RESOURCE_ID_ICON_PRESSURE_TREND_DD;
  if (older == -1 && recent ==  0) return RESOURCE_ID_ICON_PRESSURE_TREND_DS;
  if (older ==  0 && recent == -1) return RESOURCE_ID_ICON_PRESSURE_TREND_SD;
  if (older ==  0 && recent ==  0) return RESOURCE_ID_ICON_PRESSURE_TREND_SS;
  if (older ==  0 && recent ==  1) return RESOURCE_ID_ICON_PRESSURE_TREND_SU;
  if (older ==  1 && recent ==  0) return RESOURCE_ID_ICON_PRESSURE_TREND_US;
  if (older ==  1 && recent ==  1) return RESOURCE_ID_ICON_PRESSURE_TREND_UU;
  return RESOURCE_ID_ICON_PRESSURE_TREND_SS;
}

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

  char buf[16];
  if (!s_has_data) {
    strncpy(buf, "--", sizeof(buf));
  } else {
    ClaySettings *cfg = settings_get();
    switch (option) {
      case GRAPH_OPTION_AIR_PRESSURE:
        prv_format_pressure(buf, sizeof(buf), s_pressure, cfg->unit_pressure);    break;
      case GRAPH_OPTION_ALTITUDE:
        prv_format_altitude(buf, sizeof(buf), s_altitude, cfg->unit_altitude);    break;
      case GRAPH_OPTION_TEMPERATURE:
        prv_format_temperature(buf, sizeof(buf), s_temperature, cfg->unit_temp);  break;
      default:
        strncpy(buf, "--", sizeof(buf));                                           break;
    }
  }
  graph_set_label_text(layer, buf);
  if (option == GRAPH_OPTION_TEMPERATURE && s_has_data) {
    graph_set_icon(layer, prv_icon_for_condition(s_condition));
  }
}

void weather_provider_init(void) {
  memset(s_slots, 0, sizeof(s_slots));
  s_has_data = false;
  memset(&s_history, 0, sizeof(s_history));
  s_history.last_hour = 0xFF;
  persist_read_data(PERSIST_KEY_WEATHER_HISTORY, &s_history, sizeof(s_history));

  WeatherCache cache;
  memset(&cache, 0, sizeof(cache));
  if (persist_read_data(PERSIST_KEY_WEATHER_CACHE, &cache, sizeof(cache)) > 0
      && cache.timestamp != 0
      && (time(NULL) - cache.timestamp) <= 3600) {
    s_temperature   = cache.temperature;
    s_pressure      = cache.pressure;
    s_pressure_trend = cache.pressure_trend;
    s_altitude      = cache.altitude;
    s_condition     = cache.condition;
    s_has_data      = true;
  }
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
      GraphStyle style = (option == GRAPH_OPTION_ALTITUDE)
        ? GRAPH_STYLE_FILLED : GRAPH_STYLE_LINE;
      layer = graph_create(layout->graph_layer_bounds,
                           layout->graph_plot_bounds, (GraphConfig) {
        .style = style,
        .h_markers = 3,
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
      if (option == MINIVIEW_OPTION_WEATHER && s_has_data) {
        miniview_set_icon_resource_id(layer, prv_icon_for_condition(s_condition));
      }
      break;
    }
    case COMPLICATION_BOTTOM_LEFT:
    case COMPLICATION_BOTTOM_RIGHT: {
      GRect bounds = (slot == COMPLICATION_BOTTOM_LEFT)
        ? layout->bottom_left_bounds : layout->bottom_right_bounds;
      BottomAlign align = (slot == COMPLICATION_BOTTOM_LEFT)
        ? BOTTOM_ALIGN_RIGHT : BOTTOM_ALIGN_LEFT;
      bool is_trend = (option == BOTTOM_OPTION_PRESSURE_TREND);
      layer = bottom_complication_create(bounds, (BottomConfig) {
        .mode = is_trend ? BOTTOM_MODE_ICON_ONLY : BOTTOM_MODE_ICON_TEXT,
        .align = align,
        .font = font_20,
        .icon_resource_id = is_trend
          ? prv_compute_pressure_trend_icon()
          : prv_icon_for_bottom_option(option),
      });
      if (!is_trend) bottom_complication_set_text(layer, "--");
      if (option == BOTTOM_OPTION_TEMPERATURE && s_has_data) {
        bottom_complication_set_icon(layer, prv_icon_for_condition(s_condition));
      }
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
        char buf[16];
        if (!s_has_data) {
          strncpy(buf, "--", sizeof(buf));
        } else {
          ClaySettings *cfg = settings_get();
          switch (s_slots[i].option) {
            case MINIVIEW_OPTION_ALTITUDE:
              prv_format_altitude(buf, sizeof(buf), s_altitude, cfg->unit_altitude);      break;
            case MINIVIEW_OPTION_AIR_PRESSURE:
              prv_format_pressure(buf, sizeof(buf), s_pressure, cfg->unit_pressure);      break;
            case MINIVIEW_OPTION_WEATHER:
              prv_format_temperature(buf, sizeof(buf), s_temperature, cfg->unit_temp);
              miniview_set_icon_resource_id(layer, prv_icon_for_condition(s_condition));
              break;
            default:
              strncpy(buf, "--", sizeof(buf));                                            break;
          }
        }
        miniview_set_small_text(layer, buf);
        break;
      }
      case COMPLICATION_BOTTOM_LEFT:
      case COMPLICATION_BOTTOM_RIGHT: {
        uint8_t opt = s_slots[i].option;
        if (opt == BOTTOM_OPTION_PRESSURE_TREND) {
          bottom_complication_set_icon(layer, prv_compute_pressure_trend_icon());
          break;
        }
        char buf[20];
        if (!s_has_data) {
          strncpy(buf, "--", sizeof(buf));
        } else {
          ClaySettings *cfg = settings_get();
          switch (opt) {
            case BOTTOM_OPTION_ALTITUDE:
              prv_format_altitude(buf, sizeof(buf), s_altitude, cfg->unit_altitude);      break;
            case BOTTOM_OPTION_AIR_PRESSURE:
              prv_format_pressure(buf, sizeof(buf), s_pressure, cfg->unit_pressure);      break;
            case BOTTOM_OPTION_TEMPERATURE:
              prv_format_temperature(buf, sizeof(buf), s_temperature, cfg->unit_temp);
              bottom_complication_set_icon(layer, prv_icon_for_condition(s_condition));
              break;
            default:
              strncpy(buf, "--", sizeof(buf));                                            break;
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

  WeatherCache cache = {
    .temperature   = s_temperature,
    .pressure      = s_pressure,
    .pressure_trend = s_pressure_trend,
    .altitude      = s_altitude,
    .condition     = s_condition,
    .timestamp     = time(NULL),
  };
  persist_write_data(PERSIST_KEY_WEATHER_CACHE, &cache, sizeof(cache));
}

void weather_provider_deinit(void) {
  memset(s_slots, 0, sizeof(s_slots));
}
