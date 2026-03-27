#ifndef FILTER_H_
#define FILTER_H_
#include <stdint.h>
typedef struct { float x; } FILTER_In_type;
typedef struct { float y; } FILTER_Out_type;
typedef struct { float gain; } FILTER_CfgType;
typedef struct {
    void (*Pwm_SetDutyCycle[2])(uint16_t duty);
    void (*Reset)(void);
} FILTER_ClientBindings_type;
extern FILTER_In_type rx;
extern FILTER_Out_type tx;
extern const FILTER_CfgType * FILTER_cfg;
extern FILTER_ClientBindings_type FILTER_Api;
void FILTER_init(void);
void FILTER_step(void);
#endif /* FILTER_H_ */
