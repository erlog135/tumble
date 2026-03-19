#pragma once
#include "providers.h"

void health_provider_init(void);
void health_provider_activate(ComplicationSlot slot, uint8_t option);
void health_provider_deactivate(ComplicationSlot slot);
void health_provider_tick(struct tm *tick_time);
void health_provider_record_history(struct tm *tick_time);
void health_provider_deinit(void);
