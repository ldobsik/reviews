/* ========================================================================== */
/* FILE: SwcIf_Priv.h */
/* ========================================================================== */
#ifndef SWCIF_PRIV_H_
#define SWCIF_PRIV_H_

#include <stdint.h>
#include "SwcIf.h"

typedef void (*SwcIf_GenericInstanceFnType)(void * const pself);
typedef void (*SwcIf_GenericRouteFnType)(void * const pself);

#if SWCIF_OVR_SLOT_COUNT <= 8
typedef uint8_t SwcIf_OvrMask_type;
#elif SWCIF_OVR_SLOT_COUNT <= 16
typedef uint16_t SwcIf_OvrMask_type;
#else
typedef uint32_t SwcIf_OvrMask_type;
#endif

typedef struct {
    uint16_t offset;
    uint16_t size;
} SwcIf_RawEntryMetaType;

typedef struct {
    uint8_t * target_ptr;
    uint8_t mask_idx;
    uint16_t byte_len;
    uint8_t data[SWCIF_OVR_BUFFER_LEN];
} SwcIf_OvrSlotType;

typedef enum {
    SWCIF_SCHEDULE_SWITCH_DEFERRED = 0,
    SWCIF_SCHEDULE_SWITCH_IMMEDIATE = 1
} SwcIf_ScheduleSwitchModeType;

typedef struct {
    uint8_t offset_tick;
    uint8_t run_count;
    const SwcIf_InstanceType * run_list;
} SwcIf_ScheduleExpiryPointType;

#if SWCIF_ANY_OVR
extern SwcIf_OvrMask_type SwcIf_InstanceMasks_[];
extern void SwcIf_ApplyOverrides_(SwcIf_OvrMask_type active_mask);
#endif
extern void SwcIf_ExecScheduleTick_(SwcIf_ScheduleType schedule, uint8_t tick);
extern const SwcIf_GenericInstanceFnType SwcIf_BindFnTable_[SWCIF_INSTANCE_COUNT];
extern const SwcIf_GenericInstanceFnType SwcIf_InitFnTable_[SWCIF_INSTANCE_COUNT];
extern void * const SwcIf_InstanceObjectTable_[SWCIF_INSTANCE_COUNT];
extern const SwcIf_RawEntryMetaType * const SwcIf_InputMetaTable_[SWCIF_INSTANCE_COUNT];
extern const SwcIf_RawEntryMetaType * const SwcIf_OutputMetaTable_[SWCIF_INSTANCE_COUNT];
extern const SwcIf_RawEntryMetaType * const SwcIf_ConfigMetaTable_[SWCIF_INSTANCE_COUNT];
extern const uint16_t SwcIf_InputEntryCountTable_[SWCIF_INSTANCE_COUNT];
extern const uint16_t SwcIf_OutputEntryCountTable_[SWCIF_INSTANCE_COUNT];
extern const uint16_t SwcIf_ConfigEntryCountTable_[SWCIF_INSTANCE_COUNT];
extern uint8_t * const SwcIf_InputBaseTable_[SWCIF_INSTANCE_COUNT];
extern uint8_t * const SwcIf_OutputBaseTable_[SWCIF_INSTANCE_COUNT];
extern const void * const * const SwcIf_ConfigPtrAddressTable_[SWCIF_INSTANCE_COUNT];
extern const SwcIf_ScheduleSwitchModeType SwcIf_ScheduleSwitchMode_[SWCIF_SCHEDULE_COUNT];
extern const SwcIf_ScheduleExpiryPointType * const SwcIf_ScheduleExpiryPointTable_[SWCIF_SCHEDULE_COUNT];
extern const SwcIf_ScheduleType SwcIf_DefaultSchedule_;
extern boolean SwcIf_InstanceEnabled_[SWCIF_INSTANCE_COUNT];

#endif /* SWCIF_PRIV_H_ */
