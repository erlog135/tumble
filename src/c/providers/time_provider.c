#include "time_provider.h"
#include "../complications/miniview.h"
#include "../complications/bottom.h"

typedef struct {
    bool active;
    uint8_t option;
} SlotState;

static SlotState s_slots[COMPLICATION_COUNT];

void time_provider_init(void) {
    memset(s_slots, 0, sizeof(s_slots));
}

void time_provider_activate(ComplicationSlot slot, uint8_t option) {
    s_slots[slot].active = true;
    s_slots[slot].option = option;

    Layout *layout = providers_get_layout();
    GFont font_small = providers_get_font_small();
    GFont font_medium = providers_get_font_medium();
    Layer *window_layer = providers_get_window_layer();

    Layer *layer = NULL;

    switch (slot) {
        case COMPLICATION_MINIVIEW: {
            layer = miniview_create(layout->miniview_bounds, (MiniviewConfig) {
                .mode = MINIVIEW_MODE_TEXT_STACK,
                .small_text_bounds = layout->miniview_small_text_bounds,
                .medium_text_bounds = layout->miniview_medium_text_bounds,
                .small_font = font_small,
                .medium_font = font_medium,
            });
            break;
        }
        case COMPLICATION_BOTTOM_LEFT:
        case COMPLICATION_BOTTOM_RIGHT: {
            GRect bounds = (slot == COMPLICATION_BOTTOM_LEFT)
                ? layout->bottom_left_bounds : layout->bottom_right_bounds;
            BottomAlign align = (slot == COMPLICATION_BOTTOM_LEFT)
                ? BOTTOM_ALIGN_RIGHT : BOTTOM_ALIGN_LEFT;
            layer = bottom_complication_create(bounds, (BottomConfig) {
                .mode = BOTTOM_MODE_TEXT_ONLY,
                .align = align,
                .font = font_small,
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

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    time_provider_tick(t);
}

void time_provider_deactivate(ComplicationSlot slot) {
    s_slots[slot].active = false;
}

static void prv_upper(char *s) {
    for (; *s; s++) {
        if (*s >= 'a' && *s <= 'z') *s -= 32;
    }
}

void time_provider_tick(struct tm *tick_time) {
    for (int i = 0; i < COMPLICATION_COUNT; i++) {
        if (!s_slots[i].active) continue;

        Layer *layer = providers_get_layer(i);
        if (!layer) continue;

        switch (i) {
            case COMPLICATION_MINIVIEW: {
                char tiny[8];
                char small[8];
                switch (s_slots[i].option) {
                    case MINIVIEW_OPTION_DATE_MONTH_DATE:
                        strftime(tiny, sizeof(tiny), "%b", tick_time);
                        prv_upper(tiny);
                        strftime(small, sizeof(small), "%d", tick_time);
                        break;
                    case MINIVIEW_OPTION_DATE_DOW_DATE:
                        strftime(tiny, sizeof(tiny), "%a", tick_time);
                        prv_upper(tiny);
                        strftime(small, sizeof(small), "%d", tick_time);
                        break;
                    case MINIVIEW_OPTION_CUSTOM_TZ:
                        strncpy(tiny, "UTC", sizeof(tiny));
                        strftime(small, sizeof(small), "%H:%M", tick_time);
                        break;
                    default:
                        tiny[0] = '\0';
                        small[0] = '\0';
                        break;
                }
                miniview_set_small_text(layer, tiny);
                miniview_set_medium_text(layer, small);
                break;
            }
            case COMPLICATION_BOTTOM_LEFT:
            case COMPLICATION_BOTTOM_RIGHT: {
                char buf[16];
                switch (s_slots[i].option) {
                    case BOTTOM_OPTION_DATE_MONTH_DATE:
                        strftime(buf, sizeof(buf), "%b %d", tick_time);
                        break;
                    case BOTTOM_OPTION_DATE_DOW_DATE:
                        strftime(buf, sizeof(buf), "%a %d", tick_time);
                        break;
                    default:
                        buf[0] = '\0';
                        break;
                }
                bottom_complication_set_text(layer, buf);
                break;
            }
            default:
                break;
        }
    }
}

void time_provider_deinit(void) {
    memset(s_slots, 0, sizeof(s_slots));
}
