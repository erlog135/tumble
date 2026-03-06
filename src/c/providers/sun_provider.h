#pragma once
#include "providers.h"

void sun_provider_init(void);
void sun_provider_activate(ComplicationSlot slot, uint8_t option);
void sun_provider_deactivate(ComplicationSlot slot);
void sun_provider_tick(struct tm *tick_time);
void sun_provider_on_data(DictionaryIterator *iter);
void sun_provider_deinit(void);
