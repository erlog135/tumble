#include "settings.h"

#define SETTINGS_KEY 1

static ClaySettings settings;

static void prv_default_settings(void) {
    settings.black_bg = true;
    settings.invert_miniview = true;
    settings.weather_option = WEATHER_OPTION_OPEN_METEO;
    settings.weather_refresh_interval = 30;
    settings.unit_temp = UNIT_TEMP_F;
    settings.unit_altitude = UNIT_ALTITUDE_FT;
    settings.unit_pressure = UNIT_PRESSURE_MB;
    settings.tz_offset_minutes = 0;
    settings.miniview_option = MINIVIEW_OPTION_DATE_DOW_DATE;
    settings.graph_option = GRAPH_OPTION_TEMPERATURE;
    settings.seconds_option = SECONDS_OPTION_ALWAYS_ON;
    settings.bottom_left_option = BOTTOM_OPTION_SUNRISE_SUNSET;
    settings.bottom_right_option = BOTTOM_OPTION_BATTERY;
}

void settings_init(void) {
    prv_default_settings();
}

void settings_load(void) {
    // Load the default settings
    prv_default_settings();
    // Read settings from persistent storage, if they exist
    persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
    settings.weather_option = WEATHER_OPTION_OPEN_METEO;
}

void settings_save(void) {
    persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

ClaySettings* settings_get(void) {
    return &settings;
}