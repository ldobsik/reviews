#include "FILTER.h"
const FILTER_CfgType * FILTER_cfg = 0;
FILTER_Api_t FILTER_Api = { 0 };
FILTER_In_type rx = { 0.0f };
FILTER_Out_type tx = { 0.0f };
void FILTER_init(void){ tx.y = 0.0f; }
void FILTER_step(void){ float a = FILTER_cfg ? FILTER_cfg->alpha : 1.0f; tx.y = a*rx.x + (1.0f-a)*tx.y; if(FILTER_Api.LampDriver_SetIntensity){ FILTER_Api.LampDriver_SetIntensity(0u, tx.y);} }
