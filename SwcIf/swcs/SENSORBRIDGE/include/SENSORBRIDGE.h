#ifndef SENSORBRIDGE_H_
#define SENSORBRIDGE_H_
#include <stdint.h>
#include "std_types.h"

typedef struct { float temperature_c; float supply_v; } SENSORBRIDGE_IO_type;
extern SENSORBRIDGE_IO_type IO;
void SENSORBRIDGE_step(void);

#endif /* SENSORBRIDGE_H_ */
