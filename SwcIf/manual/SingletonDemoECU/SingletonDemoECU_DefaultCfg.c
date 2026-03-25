#include "FILTER.h"
#include "SingletonDemoECU_DefaultCfg.h"
const FILTER_CfgType SingletonDemoECU_FILTER_DefaultCfg = { .alpha = 0.5f };
void SingletonDemoECU_Reset(void){ tx.y = 0.0f; }
