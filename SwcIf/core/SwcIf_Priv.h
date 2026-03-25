/* ========================================================================== */
/* FILE: SwcIf_Priv.h */
/* ========================================================================== */
/**
 * @file SwcIf_Priv.h
 * @brief Private SwcIf declarations shared by static core and generated code.
 */
#ifndef SWCIF_PRIV_H_
#define SWCIF_PRIV_H_

#include <stdint.h>
#include "SwcIf.h"

/* ========================================================================== */
/* Private Function Pointer Types */
/* ========================================================================== */
typedef void (*SwcIf_GenericInstanceFnType)(void * const pself);
typedef void (*SwcIf_GenericRouteFnType)(void * const pself);

/* ========================================================================== */
/* Private Metadata / Schedule Types */
/* ========================================================================== */
typedef struct
{
    uint16_t offset;
    uint16_t size;
} SwcIf_RawEntryMetaType;

typedef struct
{
    uint8_t * target_ptr;
    SwcIf_InstanceType instance_id;
    uint16_t byte_len;
    uint8_t data[SWCIF_OVR_BUFFER_LEN];
} SwcIf_OvrSlotType;

typedef enum
{
    SWCIF_SCHEDULE_SWITCH_DEFERRED = 0,
    SWCIF_SCHEDULE_SWITCH_IMMEDIATE = 1
} SwcIf_ScheduleSwitchModeType;

typedef struct
{
    uint8_t offset_tick;
    uint8_t run_count;
    const SwcIf_InstanceType * run_list;
} SwcIf_ScheduleExpiryPointType;

/* ========================================================================== */
/* Generated Tables (provided by generator output) */
/* ========================================================================== */
extern const SwcIf_GenericInstanceFnType SwcIf_BindFnTable_[SWCIF_INSTANCE_COUNT];
extern const SwcIf_GenericInstanceFnType SwcIf_InitFnTable_[SWCIF_INSTANCE_COUNT];
extern const SwcIf_GenericInstanceFnType SwcIf_StepFnTable_[SWCIF_INSTANCE_COUNT];
extern void * const SwcIf_InstanceObjectTable_[SWCIF_INSTANCE_COUNT];
extern const SwcIf_GenericRouteFnType SwcIf_InputUpdateTable_[SWCIF_INSTANCE_COUNT];
extern const SwcIf_GenericRouteFnType SwcIf_OutputUpdateTable_[SWCIF_INSTANCE_COUNT];
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

#endif /* SWCIF_PRIV_H_ */
