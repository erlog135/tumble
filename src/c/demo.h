#pragma once

/**
 * demo.h — Tumble watchface demo mode
 *
 * To enable demo mode, define DEMO_MODE to one of the following values
 * (e.g. by uncommenting the line below, or passing -DDEMO_MODE=1 in the
 * build environment):
 *
 *   DEMO_MODE 1 — black bg / white miniview, heart-rate graph,
 *                 DOW+date miniview, calories (bot-left), steps (bot-right)
 *
 *   DEMO_MODE 2 — black bg / white miniview, temperature graph,
 *                 sun-position miniview, month+date (bot-left),
 *                 battery (bot-right)
 *
 *   DEMO_MODE 3 — white bg / black miniview, elevation graph,
 *                 custom TZ miniview (7:35 a), air-pressure (bot-left),
 *                 pressure trend "steady-rising" (bot-right)
 *
 * All demo modes share the same frozen display values:
 *   Time       12:35 PM
 *   Date       Thu Feb 12
 *   Weather    42 °F, partly cloudy
 *   Battery    80 %
 *   Sunset     7:54 PM  (474 min after midnight)
 *   Heart rate 84 bpm
 *   Steps      8352
 *   Calories   1984 kcal
 *   Elevation  760 ft
 *   Pressure   1005 hPa
 */

#define DEMO_MODE 2

#if defined(DEMO_MODE)

#include <pebble.h>

/** Must be called immediately after settings_load() in init(). */
void demo_apply_settings(void);

/** Must be called after providers_init() / providers_apply_settings().
 * Injects synthetic sensor data so every complication shows demo values.
 */
void demo_inject_data(void);

/**
 * Must be called after time_display_create() (and after demo_inject_data()).
 * Sets the time display to the frozen demo time: 12:35 PM.
 */
void demo_freeze_time(Layer *time_display_layer);

#endif /* DEMO_MODE */
