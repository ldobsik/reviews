#ifndef LAMPDRV_H_
#define LAMPDRV_H_
#include <stdint.h>
#include "std_types.h"
typedef struct {
    float lastIntensity;
} LAMPDRV_Out_type;
typedef struct {
    LAMPDRV_Out_type Signals;
} LAMPDRV_InstanceType;
void LAMPDRV_initialize(LAMPDRV_InstanceType * const self);
void LAMPDRV_step(LAMPDRV_InstanceType * const self);
void LAMPDRV_SetIntensity(LAMPDRV_InstanceType * const self, uint8_t channel, float intensity);
#endif /* LAMPDRV_H_ */
