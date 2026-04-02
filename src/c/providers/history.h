#pragma once
#include <pebble.h>

// 24 hourly points  = 24h window
// 24 per-10min points = 4h window
#define HISTORY_24H_LEN 24
#define HISTORY_4H_LEN  24

// Sentinel: stored in history arrays when data is unavailable / invalid.
// All valid values are in [0, 127].
#define HISTORY_INVALID ((int8_t)(-1))

// Persist keys — keep away from SETTINGS_KEY (1)
#define PERSIST_KEY_BATTERY_HISTORY  10
#define PERSIST_KEY_HEALTH_HISTORY   11
#define PERSIST_KEY_WEATHER_HISTORY  12
#define PERSIST_KEY_WEATHER_CACHE    13
#define PERSIST_KEY_STEPS_HISTORY    14

// Push a new sample into a most-recent-first ring buffer.
// hist[0] will always be the newest value after the push.
static inline void history_push(int8_t *hist, uint8_t *count, uint8_t max, int8_t val) {
  uint8_t n = *count;
  if (n < max) {
    memmove(hist + 1, hist, n * sizeof(int8_t));
    *count = n + 1;
  } else {
    memmove(hist + 1, hist, (max - 1) * sizeof(int8_t));
  }
  hist[0] = val;
}

