#ifndef HEARTBEAT_H_
#define HEARTBEAT_H_
#include <stdint.h>
#include "std_types.h"
typedef struct {
    uint32_t ticks;
} HEARTBEAT_InstanceType;
void HEARTBEAT_initialize(HEARTBEAT_InstanceType * const self);
void HEARTBEAT_step(HEARTBEAT_InstanceType * const self);
uint32_t HEARTBEAT_GetTicks(HEARTBEAT_InstanceType * const self);
#endif /* HEARTBEAT_H_ */
