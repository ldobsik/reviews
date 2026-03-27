#ifndef SINGLETONDEMOECU_DEFAULTCFG_H_
#define SINGLETONDEMOECU_DEFAULTCFG_H_
#include <stdint.h>
#include "FILTER.h"
#include "SwcIf_Int.h"
extern const FILTER_CfgType SingletonDemoECU_FILTER_DefaultCfg;
static inline void SingletonDemoECU_Pwm_SetDutyCycle_A(uint16_t duty) { float intensity=((float)duty)/100.0f; SWCIF_SERVER(LAMPDRV_0, LAMPDRV_SetIntensity)(0u, intensity); }
static inline void SingletonDemoECU_Pwm_SetDutyCycle_B(uint16_t duty) { float intensity=((float)duty)/100.0f; SWCIF_SERVER(LAMPDRV_0, LAMPDRV_SetIntensity)(1u, intensity); }
static inline void SingletonDemoECU_Reset(void) { tx.y = 0.0f; }
#endif /* SINGLETONDEMOECU_DEFAULTCFG_H_ */
