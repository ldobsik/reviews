#include <stdint.h>
#include "RearLampECU_DefaultCfg.h"
const DRLPOS_CfgType RearLampECU_DRLPOS_DefaultCfg = { 2u, 250u };
DRLPOS_CONTROL_e RearLampECU_ReadStatusFallback(void){ return DRLPOS_CONTROL_OFF; }
