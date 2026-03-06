#pragma once
#include "providers.h"

void weather_provider_init(void);
void weather_provider_activate(ComplicationSlot slot, uint8_t option);
void weather_provider_deactivate(ComplicationSlot slot);
void weather_provider_tick(struct tm *tick_time);
void weather_provider_on_data(DictionaryIterator *iter);
void weather_provider_deinit(void);
