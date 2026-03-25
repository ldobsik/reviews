#include <stdint.h>
#include "std_types.h"
#include "SwcIf.h"
#define SWCIF_LOCAL_INSTANCE DRLPOS_0
void SWCIF_STATIC_BIND_FN(SWCIF_LOCAL_INSTANCE)(DRLPOS_InstanceType * const self){ (void)self; }
void SWCIF_IN_FN(SWCIF_LOCAL_INSTANCE)(DRLPOS_InstanceType * const self){
    self->rx.drlpos_stadiumActivationRequest = DRLPOS_CONTROL_OFF;
    self->rx.drlpos_wingActivationRequest = DRLPOS_CONTROL_OFF;
    self->rx.drlpos_suppressDrl = FALSE;
    self->rx.drlpos_suppressPos = FALSE;
}
void SWCIF_OUT_FN(SWCIF_LOCAL_INSTANCE)(DRLPOS_InstanceType * const self){
    (void)self->tx.drlpos_stadiumStatus;
    (void)self->tx.drlpos_wingStatus;
}
#undef SWCIF_LOCAL_INSTANCE
