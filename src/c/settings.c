#include "settings.h"

#define SETTINGS_KEY 1

static ClaySettings settings;

static void prv_default_settings(void) {
    settings.black_bg = true;
    settings.invert_miniview = true;
    settings.weather_option = 0; //WeatherKit
    settings.weather_refresh_interval = 30; // 30 minutes
    settings.miniview_option = 2; //DOTW and date
    settings.graph_option = 0; //Steps
    settings.seconds_option = 0; //Seconds always on
    settings.bottom_left_option = 7; //Sunrise/sunset
    settings.bottom_right_option = 0; //Battery
}

void settings_init(void) {
    prv_default_settings();
}

void settings_load(void) {
    // Load the default settings
    prv_default_settings();
    // Read settings from persistent storage, if they exist
    persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

void settings_save(void) {
    persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

ClaySettings* settings_get(void) {
    return &settings;
}