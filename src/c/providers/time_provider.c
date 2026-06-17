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
    Layer *window_layer = providers_get_window_layer();

    Layer *layer = NULL;

    switch (slot) {
        case COMPLICATION_MINIVIEW: {
            MiniviewConfig cfg = { 0 };
            cfg.mode = MINIVIEW_MODE_TEXT_STACK;
            layer = miniview_create(cfg);
            break;
        }
        case COMPLICATION_BOTTOM_LEFT:
        case COMPLICATION_BOTTOM_RIGHT: {
            GRect bounds = (slot == COMPLICATION_BOTTOM_LEFT)
                ? layout->bottom_left_bounds : layout->bottom_right_bounds;
            BottomAlign align = (slot == COMPLICATION_BOTTOM_LEFT)
                ? BOTTOM_ALIGN_LEFT : BOTTOM_ALIGN_RIGHT;
            BottomConfig cfg = {
                .align = align,
                .font = font_small,
                .mode = BOTTOM_MODE_TEXT_ONLY,
            };
            layer = bottom_complication_create(bounds, cfg);
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

/** Map a UTC offset (minutes) to an iconic abbreviation, or fall back to ±H:MM. */
static void prv_tz_abbr_for_offset(int16_t off_min, char *buf, size_t sz) {
    typedef struct { int16_t off; const char *abbr; } TzEntry;
    static const TzEntry s_tz[] = {
        { -720, "BIT"  },  /* UTC-12  Baker Island Time */
        { -660, "SST"  },  /* UTC-11  Samoa Standard Time */
        { -600, "HST"  },  /* UTC-10  Hawaii Standard Time */
        { -540, "AKST" },  /* UTC-9   Alaska Standard Time */
        { -480, "PST"  },  /* UTC-8   Pacific Standard / Alaska Daylight */
        { -420, "MST"  },  /* UTC-7   Mountain Standard / Pacific Daylight */
        { -360, "CST"  },  /* UTC-6   Central Standard / Mountain Daylight */
        { -300, "EST"  },  /* UTC-5   Eastern Standard / Central Daylight */
        { -240, "AST"  },  /* UTC-4   Atlantic Standard / Eastern Daylight */
        { -210, "NST"  },  /* UTC-3:30 Newfoundland Standard */
        { -180, "BRT"  },  /* UTC-3   Brasilia / Atlantic Daylight */
        { -120, "GST"  },  /* UTC-2   South Georgia */
        {  -60, "CVT"  },  /* UTC-1   Cape Verde / Azores Standard */
        {    0, "UTC"  },  /* UTC+0   Coordinated Universal / GMT / WET */
        {   60, "CET"  },  /* UTC+1   Central European / BST / WAT */
        {  120, "EET"  },  /* UTC+2   Eastern European / CEST / CAT */
        {  180, "MSK"  },  /* UTC+3   Moscow / EAT */
        {  210, "IRST" },  /* UTC+3:30 Iran Standard */
        {  240, "GST"  },  /* UTC+4   Gulf Standard / AMT */
        {  270, "AFT"  },  /* UTC+4:30 Afghanistan */
        {  300, "PKT"  },  /* UTC+5   Pakistan / Yekaterinburg */
        {  330, "IST"  },  /* UTC+5:30 India Standard */
        {  345, "NPT"  },  /* UTC+5:45 Nepal */
        {  360, "BST"  },  /* UTC+6   Bangladesh / Omsk */
        {  390, "MMT"  },  /* UTC+6:30 Myanmar */
        {  420, "ICT"  },  /* UTC+7   Indochina / Krasnoyarsk */
        {  480, "CST"  },  /* UTC+8   China / HKT / SGT / AWST */
        {  525, "ACWST"},  /* UTC+8:45 Australian Central Western */
        {  540, "JST"  },  /* UTC+9   Japan / KST / YAKT */
        {  570, "ACST" },  /* UTC+9:30 Australian Central Standard */
        {  600, "AEST" },  /* UTC+10  Australian Eastern / VLAT */
        {  630, "LHST" },  /* UTC+10:30 Lord Howe Standard */
        {  660, "SBT"  },  /* UTC+11  Solomon Islands / Australian Eastern Daylight */
        {  720, "NZST" },  /* UTC+12  New Zealand Standard / ANAT */
        {  765, "CHAST"},  /* UTC+12:45 Chatham Standard */
        {  780, "TOT"  },  /* UTC+13  Tonga / New Zealand Daylight */
        {  840, "LINT" },  /* UTC+14  Line Islands */
    };
    for (size_t i = 0; i < sizeof(s_tz)/sizeof(s_tz[0]); i++) {
        if (s_tz[i].off == off_min) {
            strncpy(buf, s_tz[i].abbr, sz - 1);
            buf[sz - 1] = '\0';
            return;
        }
    }
    /* Fallback: ±H:MM */
    int16_t abs_off = off_min < 0 ? -off_min : off_min;
    int16_t hh = abs_off / 60;
    int16_t mm = abs_off % 60;
    if (mm == 0) {
        snprintf(buf, sz, "%c%d", off_min < 0 ? '-' : '+', (int)hh);
    } else {
        snprintf(buf, sz, "%c%d:%02d", off_min < 0 ? '-' : '+', (int)hh, (int)mm);
    }
}

static void prv_format_tz_time(char *buf, size_t sz, struct tm *tz_tm) {
    if (clock_is_24h_style()) {
        strftime(buf, sz, "%H:%M", tz_tm);
    } else {
        char tmp[8];
        strftime(tmp, sizeof(tmp), "%I:%M", tz_tm);
        const char *h = (tmp[0] == '0') ? tmp + 1 : tmp;
        const char *ap = (tz_tm->tm_hour < 12) ? "A" : "P";
        snprintf(buf, sz, "%s %s", h, ap);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "tz_time: %s %s %s", h, ap, buf);
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
                    case MINIVIEW_OPTION_CUSTOM_TZ: {
                        int16_t off = settings_get()->tz_offset_minutes;
                        time_t adjusted = time(NULL) + (time_t)(off * 60);
                        struct tm *tz_tm = gmtime(&adjusted);
                        prv_format_tz_time(small, sizeof(small), tz_tm);
                        prv_tz_abbr_for_offset(off, tiny, sizeof(tiny));
                        break;
                    }
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
                    case BOTTOM_OPTION_CUSTOM_TZ: {
                        int16_t off = settings_get()->tz_offset_minutes;
                        time_t adjusted = time(NULL) + (time_t)(off * 60);
                        struct tm *tz_tm = gmtime(&adjusted);
                        char tz_abbr[8];
                        prv_tz_abbr_for_offset(off, tz_abbr, sizeof(tz_abbr));
                        char tz_time[12];
                        prv_format_tz_time(tz_time, sizeof(tz_time), tz_tm);
                        snprintf(buf, sizeof(buf), "%s %s", tz_abbr, tz_time);
                        break;
                    }
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
