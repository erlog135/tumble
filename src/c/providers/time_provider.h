#pragma once
#include "providers.h"

void time_provider_init(void);
void time_provider_activate(ComplicationSlot slot, uint8_t option);
void time_provider_deactivate(ComplicationSlot slot);
void time_provider_tick(struct tm *tick_time);
void time_provider_deinit(void);
