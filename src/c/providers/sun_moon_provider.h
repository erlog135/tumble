#pragma once
#include "providers.h"

void sun_moon_provider_init(void);
void sun_moon_provider_activate(ComplicationSlot slot, uint8_t option);
void sun_moon_provider_deactivate(ComplicationSlot slot);
void sun_moon_provider_tick(struct tm *tick_time);
void sun_moon_provider_on_solar_data(DictionaryIterator *iter);
void sun_moon_provider_on_lunar_data(DictionaryIterator *iter);
void sun_moon_provider_deinit(void);
