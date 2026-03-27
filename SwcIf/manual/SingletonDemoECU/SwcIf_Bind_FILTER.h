#ifndef SWCIF_BIND_FILTER_H_
#define SWCIF_BIND_FILTER_H_
#include "SENSORBRIDGE.h"
#include "FILTER.h"
static inline void SwcIf_InputRoute_FILTER(void) { rx.x = IO.temperature_c; }
static inline void SwcIf_OutputRoute_FILTER(void) { (void)tx; }
#endif /* SWCIF_BIND_FILTER_H_ */
