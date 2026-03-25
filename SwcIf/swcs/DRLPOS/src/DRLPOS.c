#include "DRLPOS.h"
void DRLPOS_initialize(DRLPOS_InstanceType * const self){
    self->tx.drlpos_stadiumSegmentsIntensity = 0.0f;
    for (uint8_t i = 0u; i < 8u; i++) { self->tx.drlpos_wingSegmentsIntensity[i] = 0.0f; }
    self->tx.drlpos_stadiumStatus = DRLPOS_CONTROL_OFF;
    self->tx.drlpos_wingStatus = DRLPOS_CONTROL_OFF;
}
void DRLPOS_step(DRLPOS_InstanceType * const self){
    DRLPOS_CONTROL_e status = self->api.RearLampECU_ReadStatusFallback();
    self->api.LampDriver_SetIntensity(0u, self->tx.drlpos_stadiumSegmentsIntensity);
    self->tx.drlpos_stadiumStatus = status;
}
DRLPOS_CONTROL_e DRLPOS_GetStatus(DRLPOS_InstanceType * const self){ return self->tx.drlpos_stadiumStatus; }
void DRLPOS_ForceOff(DRLPOS_InstanceType * const self){
    self->tx.drlpos_stadiumSegmentsIntensity = 0.0f;
    self->tx.drlpos_stadiumStatus = DRLPOS_CONTROL_OFF;
    self->tx.drlpos_wingStatus = DRLPOS_CONTROL_OFF;
}
