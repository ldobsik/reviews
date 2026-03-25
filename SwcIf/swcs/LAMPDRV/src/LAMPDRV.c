#include "LAMPDRV.h"
void LAMPDRV_initialize(LAMPDRV_InstanceType * const self){ self->Signals.lastIntensity = 0.0f; }
void LAMPDRV_step(LAMPDRV_InstanceType * const self){ (void)self; }
void LAMPDRV_SetIntensity(LAMPDRV_InstanceType * const self, uint8_t channel, float intensity){ (void)channel; self->Signals.lastIntensity = intensity; }
