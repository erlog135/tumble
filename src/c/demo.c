#include "demo.h"

#if defined(DEMO_MODE)

#include <pebble.h>
#include "settings.h"
#include "providers/providers.h"
#include "complications/graph.h"
#include "complications/miniview.h"
#include "complications/bottom.h"
#include "time_display.h"

/* ─── Shared demo values ─────────────────────────────────────────────────── */

/* Time: 12:35 PM  →  used to configure custom-TZ offset so 7:35 A is shown.
 * Local time is frozen at 12:35; the TZ miniview offset is set so that
 * (12:35 local + offset) mod 24h == 07:35, i.e. offset = -5*60 = -300 min. */
#define DEMO_WEATHER_TEMP_F   42      /* °F, partly cloudy */
#define DEMO_WEATHER_PRESSURE 1005    /* hPa */
#define DEMO_WEATHER_ALTITUDE_M 232   /* meters ≈ 760 ft */
#define DEMO_WEATHER_CONDITION 2      /* WEATHER_COND_PARTLY_CLOUDY */

#define DEMO_BATTERY_PCT      80
#define DEMO_SUNSET_MIN       (19*60 + 54)  /* 7:54 PM = 1194 min after midnight */
#define DEMO_SUNRISE_MIN      (6*60  + 30)  /* plausible sunrise companion */
#define DEMO_HEART_RATE       84     /* bpm */
#define DEMO_STEPS            8352
#define DEMO_CALORIES         1984
#define DEMO_ELEVATION_FT     760    /* feet */

/* History graph: 24 hourly points, most-recent-first.
 * We fill a gentle, realistic-looking curve for each metric. */

/* Heart-rate graph points (scaled 0–127, where 127 = 200 bpm).
 * 84 bpm → 127*84/200 = 53.  We draw a gentle wave around that. */
#if DEMO_MODE == 1
static const int8_t DEMO_HR_GRAPH[24] = {
    53, 51, 50, 52, 56, 60, 57, 54,
    51, 49, 48, 50, 53, 58, 62, 65,
    61, 57, 54, 51, 49, 50, 52, 53,
};
#endif

/* Temperature graph points (scaled: temp_f → temp_c → val = temp_c + 50).
 * 42°F = 5.6°C → val ≈ 56.
 * We draw a daily temperature arc peaking at midday. */
#if DEMO_MODE == 2
static const int8_t DEMO_TEMP_GRAPH[24] = {
    56, 53, 52, 55, 58, 60, 58, 56,
    55, 54, 53, 54, 55, 58, 62, 65,
    61, 57, 54, 51, 49, 50, 52, 53,
};
#endif

/* Elevation graph points (scaled: alt_m in [-500,+5000] → 0–127).
 * 232 m → (232+500)*127/5500 ≈ 17. */
#if DEMO_MODE == 3
static const int8_t DEMO_ELEVATION_GRAPH[24] = {
    17, 17, 16, 16, 15, 15, 14, 15,
    16, 18, 20, 21, 19, 18, 17, 17,
    16, 15, 15, 16, 17, 17, 17, 17,
};
#endif

/* ─── Helpers ────────────────────────────────────────────────────────────── */

/* Sun-position angle: slightly ahead of solar noon (0° = 12 o'clock on dial).
 * Solar noon maps to DEG_TO_TRIGANGLE(0). "Slightly ahead" ≈ +10°. */
#define DEMO_SUN_ANGLE_DEG 15

/* Pressure trend icon for "steady-rising": older window steady, recent rising. */
#ifndef RESOURCE_ID_ICON_PRESSURE_TREND_SU
#define RESOURCE_ID_ICON_PRESSURE_TREND_SU RESOURCE_ID_ICON_PRESSURE_TREND_SS
#endif

/* Battery icon for 80 % */
static uint32_t prv_battery_icon_80(void) {
    return RESOURCE_ID_ICON_BATTERY_80;
}

/* ─── demo_apply_settings ────────────────────────────────────────────────── */

/**
 * Override ClaySettings for the chosen DEMO_MODE.
 * Must be called immediately after settings_load() so that
 * providers_apply_settings() picks up the correct complication layout.
 */
void demo_apply_settings(void) {
    ClaySettings *cfg = settings_get();

    /* Units: imperial */
    cfg->unit_temp     = UNIT_TEMP_F;
    cfg->unit_altitude = UNIT_ALTITUDE_FT;
    cfg->unit_pressure = UNIT_PRESSURE_MB;

    /* Seconds always off in demo (time is effectively frozen) */
    cfg->seconds_option = SECONDS_OPTION_ALWAYS_OFF;

#if DEMO_MODE == 1
    /* ── Mode 1: black bg / white miniview ──────────────────────────────── */
    cfg->black_bg          = true;
    cfg->black_miniview_bg = false;   /* white miniview */
    cfg->graph_option      = GRAPH_OPTION_HEART_RATE;
    cfg->miniview_option   = MINIVIEW_OPTION_DATE_DOW_DATE;
    cfg->bottom_left_option  = BOTTOM_OPTION_CALORIES;
    cfg->bottom_right_option = BOTTOM_OPTION_STEPS;

#elif DEMO_MODE == 2
    /* ── Mode 2: black bg / white miniview ──────────────────────────────── */
    cfg->black_bg          = true;
    cfg->black_miniview_bg = false;   /* white miniview */
    cfg->graph_option      = GRAPH_OPTION_TEMPERATURE;
    cfg->miniview_option   = MINIVIEW_OPTION_SUN_POSITION;
    cfg->bottom_left_option  = BOTTOM_OPTION_DATE_MONTH_DATE;
    cfg->bottom_right_option = BOTTOM_OPTION_BATTERY;

#elif DEMO_MODE == 3
    /* ── Mode 3: white bg / black miniview ──────────────────────────────── */
    cfg->black_bg          = false;
    cfg->black_miniview_bg = true;    /* black miniview */
    cfg->graph_option      = GRAPH_OPTION_ALTITUDE;
    cfg->miniview_option   = MINIVIEW_OPTION_CUSTOM_TZ;
    /* Custom TZ: local 12:35 → 07:35, offset = -5h = -300 min */
    cfg->tz_offset_minutes   = -300;
    cfg->bottom_left_option  = BOTTOM_OPTION_AIR_PRESSURE;
    cfg->bottom_right_option = BOTTOM_OPTION_PRESSURE_TREND;

#endif /* DEMO_MODE */
}

/* ─── demo_inject_data ───────────────────────────────────────────────────── */

/**
 * Push synthetic sensor values into the already-created complication layers.
 * Must be called after providers_init() and providers_apply_settings().
 *
 * Rather than faking Pebble sensor APIs (health_service_*, battery_state_*,
 * etc.), we call the same complication-layer setters that each provider's
 * tick function uses.  This keeps demo.c fully self-contained.
 */
void demo_inject_data(void) {
    Layer *graph_layer = providers_get_layer(COMPLICATION_GRAPH);
    Layer *miniview_layer = providers_get_layer(COMPLICATION_MINIVIEW);
    Layer *bot_left_layer  = providers_get_layer(COMPLICATION_BOTTOM_LEFT);
    Layer *bot_right_layer = providers_get_layer(COMPLICATION_BOTTOM_RIGHT);

    /* ── Graph ──────────────────────────────────────────────────────────── */
    if (graph_layer) {
#if DEMO_MODE == 1
        /* Heart-rate graph (4h, 24 × 10-min buckets, line style) */
        graph_set_values(graph_layer, DEMO_HR_GRAPH, 24);
        graph_set_label_text(graph_layer, "84");

#elif DEMO_MODE == 2
        /* Temperature graph (24h, hourly, line style) */
        graph_set_values(graph_layer, DEMO_TEMP_GRAPH, 24);
        graph_set_label_text(graph_layer, "42\xc2\xb0");
        /* Show partly-cloudy icon in the graph header */
        graph_set_icon(graph_layer, RESOURCE_ID_ICON_WEATHER_PARTLY_CLOUDY);

#elif DEMO_MODE == 3
        /* Elevation graph (24h, hourly, filled style) */
        graph_set_values(graph_layer, DEMO_ELEVATION_GRAPH, 24);
        /* 760 ft */
        graph_set_label_text(graph_layer, "760");

#endif /* DEMO_MODE */
    }

    /* ── Miniview ───────────────────────────────────────────────────────── */
    if (miniview_layer) {
#if DEMO_MODE == 1
        /* DOW + date: "THU" over "12" */
        miniview_set_small_text(miniview_layer, "THU");
        miniview_set_medium_text(miniview_layer, "12");

#elif DEMO_MODE == 2
        /* Sun position: slightly ahead of solar noon */
        miniview_set_icon_angle(miniview_layer,
            DEG_TO_TRIGANGLE(DEMO_SUN_ANGLE_DEG));

#elif DEMO_MODE == 3
        /* Custom TZ: local 12:35 PM → 07:35 A in the target zone.
         * The TZ offset is already baked into settings so time_provider
         * will compute the right string on its first tick.  We also set
         * an initial value here for an instant display. */
        miniview_set_small_text(miniview_layer, "UTC");
        miniview_set_medium_text(miniview_layer, "7:35 A");

#endif /* DEMO_MODE */
    }

    /* ── Bottom left ────────────────────────────────────────────────────── */
    if (bot_left_layer) {
#if DEMO_MODE == 1
        /* Calories: 1984 */
        bottom_complication_set_text(bot_left_layer, "1984");

#elif DEMO_MODE == 2
        /* Month + date: "FEB 12" */
        bottom_complication_set_text(bot_left_layer, "FEB 12");

#elif DEMO_MODE == 3
        /* Air pressure: 1005 mb */
        bottom_complication_set_text(bot_left_layer, "1005");

#endif /* DEMO_MODE */
    }

    /* ── Bottom right ───────────────────────────────────────────────────── */
    if (bot_right_layer) {
#if DEMO_MODE == 1
        /* Steps: 8352 */
        bottom_complication_set_text(bot_right_layer, "8352");

#elif DEMO_MODE == 2
        /* Battery icon at 80 % */
        bottom_complication_set_icon(bot_right_layer, prv_battery_icon_80());

#elif DEMO_MODE == 3
        /* Pressure trend: steady-rising (SU icon) */
        bottom_complication_set_icon(bot_right_layer,
            RESOURCE_ID_ICON_PRESSURE_TREND_SU);

#endif /* DEMO_MODE */
    }

    /* Force a redraw of every complication layer. */
    providers_mark_layers_dirty();
}

/* ─── demo_freeze_time ───────────────────────────────────────────────────── */

/**
 * Sets the time display to the frozen demo time: 12:35 PM, Thu Feb 12.
 * Uses a synthetic struct tm so no real wall-clock call is needed.
 * Must be called after time_display_create().
 */
void demo_freeze_time(Layer *time_display_layer) {
    if (!time_display_layer) return;

    /* struct tm: months are 0-based, years relative to 1900.
     * Feb 12 = month 1, year 2026 = 126.
     * Thursday = tm_wday 4 (0=Sun, 1=Mon, ..., 4=Thu).
     * Day-of-year: Jan(31) + 12 - 1 = 42 → tm_yday 42. */
    struct tm demo_tm = {
        .tm_sec  = 0,
        .tm_min  = 35,
        .tm_hour = 12,   /* 12:35 PM (12-hour and 24-hour both read correctly) */
        .tm_mday = 12,
        .tm_mon  = 1,    /* February */
        .tm_year = 126,  /* 2026 */
        .tm_wday = 4,    /* Thursday */
        .tm_yday = 42,
        .tm_isdst = 0,
    };

    /* update_tick_subscription() is suppressed in demo mode, so we must
     * hide seconds ourselves — equivalent to SECONDS_OPTION_ALWAYS_OFF. */
    time_display_set_seconds_reserved(time_display_layer, false);
    time_display_set_seconds_visible(time_display_layer, false);
    time_display_set_time(time_display_layer, &demo_tm);
}

#endif /* DEMO_MODE */
