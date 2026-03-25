#ifndef DRLPOS_H_
#define DRLPOS_H_
#include <stdint.h>
#include "std_types.h"
typedef struct DRLPOS_CfgType_tag {
    uint8_t variant_id;
    uint16_t timeout_ms;
} DRLPOS_CfgType;
typedef enum { DRLPOS_CONTROL_OFF = 0, DRLPOS_CONTROL_ON = 1 } DRLPOS_CONTROL_e;
typedef struct {
    void (*LampDriver_SetIntensity)(uint8_t channel, float intensity);
    DRLPOS_CONTROL_e (*RearLampECU_ReadStatusFallback)(void);
} DRLPOS_ClientBindings_type;
typedef struct {
    DRLPOS_CONTROL_e drlpos_stadiumActivationRequest;
    DRLPOS_CONTROL_e drlpos_wingActivationRequest;
    uint8_t drlpos_suppressDrl;
    uint8_t drlpos_suppressPos;
} DRLPOS_In_type;
typedef struct {
    float drlpos_stadiumSegmentsIntensity;
    float drlpos_wingSegmentsIntensity[8u];
    DRLPOS_CONTROL_e drlpos_stadiumStatus;
    DRLPOS_CONTROL_e drlpos_wingStatus;
} DRLPOS_Out_type;
typedef struct {
    const DRLPOS_CfgType * cfg;
    DRLPOS_ClientBindings_type api;
    DRLPOS_In_type rx;
    DRLPOS_Out_type tx;
} DRLPOS_InstanceType;
void DRLPOS_initialize(DRLPOS_InstanceType * const self);
void DRLPOS_step(DRLPOS_InstanceType * const self);
DRLPOS_CONTROL_e DRLPOS_GetStatus(DRLPOS_InstanceType * const self);
void DRLPOS_ForceOff(DRLPOS_InstanceType * const self);
#endif /* DRLPOS_H_ */
