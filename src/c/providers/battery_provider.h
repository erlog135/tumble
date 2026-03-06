#pragma once
#include "providers.h"

void battery_provider_init(void);
void battery_provider_activate(ComplicationSlot slot, uint8_t option);
void battery_provider_deactivate(ComplicationSlot slot);
void battery_provider_tick(struct tm *tick_time);
void battery_provider_deinit(void);
