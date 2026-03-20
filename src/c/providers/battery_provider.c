#include "battery_provider.h"
#include "history.h"
#include "../complications/graph.h"
#include "../complications/miniview.h"
#include "../complications/bottom.h"

// Battery: 0–100 % → scaled 0–127.  Invalid = HISTORY_INVALID.
static int8_t prv_scale_battery(int pct) {
  if (pct < 0) return HISTORY_INVALID;
  return (int8_t)(pct > 100 ? 127 : pct * 127 / 100);
}

static uint32_t prv_battery_icon(int percent) {
  if (percent <= 10)  return RESOURCE_ID_ICON_BATTERY_0;
  if (percent <= 20) return RESOURCE_ID_ICON_BATTERY_20;
  if (percent <= 40) return RESOURCE_ID_ICON_BATTERY_40;
  if (percent <= 60) return RESOURCE_ID_ICON_BATTERY_60;
  if (percent <= 80) return RESOURCE_ID_ICON_BATTERY_80;
  return RESOURCE_ID_ICON_BATTERY_100;
}

typedef struct {
  bool active;
  uint8_t option;
} SlotState;

typedef struct {
  int8_t  points[HISTORY_24H_LEN]; // most-recent-first
  uint8_t count;
  uint8_t last_hour;  // 0–23; 0xFF = never sampled
} BatteryHistory;

static SlotState s_slots[COMPLICATION_COUNT];
static BatteryHistory s_history;

static void prv_update_graph(Layer *layer) {
  graph_set_values(layer, s_history.points, s_history.count);

  BatteryChargeState state = battery_state_service_peek();
  graph_set_icon(layer, prv_battery_icon(state.charge_percent));
  char buf[8];
  snprintf(buf, sizeof(buf), "%d%%", state.charge_percent);
  graph_set_label_text(layer, buf);
}

void battery_provider_init(void) {
  memset(s_slots, 0, sizeof(s_slots));
  memset(&s_history, 0, sizeof(s_history));
  s_history.last_hour = 0xFF;
  persist_read_data(PERSIST_KEY_BATTERY_HISTORY, &s_history, sizeof(s_history));
}

void battery_provider_activate(ComplicationSlot slot, uint8_t option) {
  s_slots[slot].active = true;
  s_slots[slot].option = option;

  Layout *layout = providers_get_layout();
  GFont font_20 = providers_get_font_20();
  Layer *window_layer = providers_get_window_layer();
  Layer *layer = NULL;

  switch (slot) {
    case COMPLICATION_GRAPH: {
      BatteryChargeState state = battery_state_service_peek();
      layer = graph_create(layout->graph_layer_bounds,
                           layout->graph_plot_bounds, (GraphConfig) {
        .style = GRAPH_STYLE_LINE,
        .h_markers = 5,
        .v_markers = 3,
        .top_lip = true,
        .label_font = font_20,
        .icon_resource_id = prv_battery_icon(state.charge_percent),
      });
      prv_update_graph(layer);
      break;
    }
    case COMPLICATION_MINIVIEW: {
      BatteryChargeState state = battery_state_service_peek();
      if (option == MINIVIEW_OPTION_BATTERY) {
        layer = miniview_create(layout->miniview_bounds, (MiniviewConfig) {
          .mode = MINIVIEW_MODE_ICON_CENTER,
          .icon_resource_id = prv_battery_icon(state.charge_percent),
        });
      } else {
        GFont font_28 = providers_get_font_28();
        layer = miniview_create(layout->miniview_bounds, (MiniviewConfig) {
          .mode = MINIVIEW_MODE_TEXT_STACK,
          .tiny_text_bounds = layout->miniview_tiny_text_bounds,
          .small_text_bounds = layout->miniview_small_text_bounds,
          .tiny_font = font_20,
          .small_font = font_28,
        });
        char buf[8];
        if (option == MINIVIEW_OPTION_BATTERY_DND) {
          bool bt = connection_service_peek_pebble_app_connection();
          miniview_set_tiny_text(layer, bt ? "BT" : "NO BT");
        }
        snprintf(buf, sizeof(buf), "%d%%", state.charge_percent);
        miniview_set_small_text(layer, buf);
      }
      break;
    }
    case COMPLICATION_BOTTOM_LEFT:
    case COMPLICATION_BOTTOM_RIGHT: {
      GRect bounds = (slot == COMPLICATION_BOTTOM_LEFT)
        ? layout->bottom_left_bounds : layout->bottom_right_bounds;
      BottomAlign align = (slot == COMPLICATION_BOTTOM_LEFT)
        ? BOTTOM_ALIGN_RIGHT : BOTTOM_ALIGN_LEFT;
      BatteryChargeState bstate = battery_state_service_peek();
      layer = bottom_complication_create(bounds, (BottomConfig) {
        .mode = BOTTOM_MODE_ICON_ONLY,
        .align = align,
        .font = font_20,
        .icon_resource_id = prv_battery_icon(bstate.charge_percent),
      });
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

void battery_provider_record_history(struct tm *tick_time) {
  uint8_t cur_hour = (uint8_t)tick_time->tm_hour;
  if (cur_hour == s_history.last_hour) return;

  BatteryChargeState state = battery_state_service_peek();
  int8_t scaled = prv_scale_battery(state.charge_percent);
  history_push(s_history.points, &s_history.count, HISTORY_24H_LEN, scaled);
  s_history.last_hour = cur_hour;
  persist_write_data(PERSIST_KEY_BATTERY_HISTORY, &s_history, sizeof(s_history));
}

void battery_provider_tick(struct tm *tick_time) {
  (void)tick_time;

  BatteryChargeState state = battery_state_service_peek();

  for (int i = 0; i < COMPLICATION_COUNT; i++) {
    if (!s_slots[i].active) continue;
    Layer *layer = providers_get_layer(i);
    if (!layer) continue;

    switch (i) {
      case COMPLICATION_GRAPH:
        prv_update_graph(layer);
        break;
      case COMPLICATION_MINIVIEW: {
        if (s_slots[i].option == MINIVIEW_OPTION_BATTERY) {
          miniview_set_icon_resource_id(layer, prv_battery_icon(state.charge_percent));
        } else if (s_slots[i].option == MINIVIEW_OPTION_BATTERY_DND) {
          char buf[8];
          bool bt = connection_service_peek_pebble_app_connection();
          miniview_set_tiny_text(layer, bt ? "BT" : "NO BT");
          snprintf(buf, sizeof(buf), "%d%%", state.charge_percent);
          miniview_set_small_text(layer, buf);
        }
        break;
      }
      case COMPLICATION_BOTTOM_LEFT:
      case COMPLICATION_BOTTOM_RIGHT:
        bottom_complication_set_icon(layer, prv_battery_icon(state.charge_percent));
        break;
      default:
        break;
    }
  }
}

void battery_provider_deinit(void) {
  memset(s_slots, 0, sizeof(s_slots));
}
