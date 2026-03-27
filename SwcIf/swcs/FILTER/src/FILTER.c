#include "FILTER.h"
FILTER_In_type rx = {0};
FILTER_Out_type tx = {0};
const FILTER_CfgType * FILTER_cfg = 0;
FILTER_ClientBindings_type FILTER_Api = {0};
void FILTER_init(void) { tx.y = 0.0f; }
void FILTER_step(void)
{
    float gain = (FILTER_cfg != 0) ? FILTER_cfg->gain : 1.0f;
    tx.y = rx.x * gain;
    {
        uint16_t duty = (uint16_t)(tx.y * 100.0f);
        if (FILTER_Api.Pwm_SetDutyCycle[0]) FILTER_Api.Pwm_SetDutyCycle[0](duty);
        if (FILTER_Api.Pwm_SetDutyCycle[1]) FILTER_Api.Pwm_SetDutyCycle[1](duty);
    }
}
