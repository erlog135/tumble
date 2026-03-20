#pragma once
#include <pebble.h>

typedef enum {
    WEATHER_OPTION_WEATHERKIT  = 0,
    WEATHER_OPTION_OPEN_METEO  = 1,
} WeatherOption;

typedef enum {
    UNIT_TEMP_F = 0,
    UNIT_TEMP_C = 1,
} UnitTemp;

typedef enum {
    UNIT_ALTITUDE_FT = 0,
    UNIT_ALTITUDE_M  = 1,
} UnitAltitude;

typedef enum {
    UNIT_PRESSURE_MB   = 0,
    UNIT_PRESSURE_HPA  = 1,
    UNIT_PRESSURE_INHG = 2,
} UnitPressure;

typedef enum {
    MINIVIEW_OPTION_DATE_MONTH_DATE = 0,
    MINIVIEW_OPTION_DATE_DOW_DATE   = 1,
    MINIVIEW_OPTION_HEART_RATE      = 2,
    MINIVIEW_OPTION_STEPS           = 3,
    MINIVIEW_OPTION_CALORIES        = 4,
    MINIVIEW_OPTION_ALTITUDE        = 5,
    MINIVIEW_OPTION_AIR_PRESSURE    = 6,
    MINIVIEW_OPTION_SUNRISE_SUNSET  = 7,
    MINIVIEW_OPTION_WEATHER         = 8,
    MINIVIEW_OPTION_CUSTOM_TZ       = 9,
    MINIVIEW_OPTION_SUN_POSITION    = 10,
    MINIVIEW_OPTION_MOON_PHASE      = 11,
    MINIVIEW_OPTION_BATTERY         = 12,
    MINIVIEW_OPTION_BATTERY_DND     = 13,
} MiniviewOption;

typedef enum {
    GRAPH_OPTION_STEPS        = 0,
    GRAPH_OPTION_HEART_RATE   = 1,
    GRAPH_OPTION_AIR_PRESSURE = 2,
    GRAPH_OPTION_ALTITUDE     = 3,
    GRAPH_OPTION_BATTERY      = 4,
    GRAPH_OPTION_TEMPERATURE  = 5,
} GraphOption;

typedef enum {
    SECONDS_OPTION_ALWAYS_ON    = 0,
    SECONDS_OPTION_SHAKE_30S    = 1,
    SECONDS_OPTION_SHAKE_15S    = 2,
    SECONDS_OPTION_ALWAYS_OFF   = 3,
} SecondsOption;

typedef enum {
    BOTTOM_OPTION_DATE_MONTH_DATE  = 0,
    BOTTOM_OPTION_DATE_DOW_DATE    = 1,
    BOTTOM_OPTION_STEPS            = 2,
    BOTTOM_OPTION_HEART_RATE       = 3,
    BOTTOM_OPTION_CALORIES         = 4,
    BOTTOM_OPTION_ALTITUDE         = 5,
    BOTTOM_OPTION_AIR_PRESSURE     = 6,
    BOTTOM_OPTION_PRESSURE_TREND   = 7,
    BOTTOM_OPTION_SUNRISE_SUNSET   = 8,
    BOTTOM_OPTION_TEMPERATURE      = 9,
} BottomOption;

typedef struct {
    bool black_bg;
    bool invert_miniview;
    uint8_t weather_option;
    uint8_t weather_refresh_interval;
    uint8_t unit_temp;
    uint8_t unit_altitude;
    uint8_t unit_pressure;
    uint8_t miniview_option;
    uint8_t graph_option;
    uint8_t seconds_option;
    uint8_t bottom_left_option;
    uint8_t bottom_right_option;
} ClaySettings;

void settings_init(void);

void settings_load(void);

void settings_save(void);

ClaySettings* settings_get(void);