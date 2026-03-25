#include "HEARTBEAT.h"
void HEARTBEAT_initialize(HEARTBEAT_InstanceType * const self){ self->ticks = 0u; }
void HEARTBEAT_step(HEARTBEAT_InstanceType * const self){ self->ticks++; }
uint32_t HEARTBEAT_GetTicks(HEARTBEAT_InstanceType * const self){ return self->ticks; }
