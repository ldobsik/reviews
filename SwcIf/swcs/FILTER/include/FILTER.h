#ifndef FILTER_H_
#define FILTER_H_
#include <stdint.h>
#include "std_types.h"

typedef struct FILTER_CfgType_tag { float alpha; } FILTER_CfgType;
extern const FILTER_CfgType * FILTER_cfg;

typedef struct { void (*LampDriver_SetIntensity)(uint8_t, float); void (*Reset)(void); } FILTER_Api_t;
extern FILTER_Api_t FILTER_Api;

typedef struct { float x; } FILTER_In_type;
typedef struct { float y; } FILTER_Out_type;
extern FILTER_In_type rx;
extern FILTER_Out_type tx;

void FILTER_init(void);
void FILTER_step(void);

#endif /* FILTER_H_ */
