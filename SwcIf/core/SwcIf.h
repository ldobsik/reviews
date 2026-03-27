/* ========================================================================== */
/* FILE: SwcIf.h */
/* ========================================================================== */
#ifndef SWCIF_H_
#define SWCIF_H_

#include <stdint.h>
#include "SwcIf_GenCfg.h"
#include "SwcIf_Gen.h"
#include "SwcIf_Int.h"

/* Public runtime identifiers */
typedef int8_t SwcIf_InstanceType;
typedef int8_t SwcIf_ScheduleType;
typedef uint16_t SwcIf_EntryIdxType;
typedef uint16_t SwcIf_OvrSlotIdType;

/* Logical span descriptor */
typedef struct {
    SwcIf_InstanceType instance_id;
    SwcIf_EntryIdxType entry_idx;
    uint16_t byte_offset;
    uint16_t byte_len;
} SwcIf_EntrySpanType;

void SwcIf_Init(void);
void SwcIf_EnableInstance(SwcIf_InstanceType instance);
void SwcIf_DisableInstance(SwcIf_InstanceType instance);
void SwcIf_MainFunction(void);
Std_ReturnType SwcIf_SetNextSchedule(SwcIf_ScheduleType schedule);
SwcIf_ScheduleType SwcIf_GetActiveSchedule(void);

#if (SWCIF_ENABLE_INPUT_RAW_API == STD_ON)
Std_ReturnType SwcIf_GetInputRaw(const SwcIf_EntrySpanType * span, void * dst);
#endif
#if (SWCIF_ENABLE_OUTPUT_RAW_API == STD_ON)
Std_ReturnType SwcIf_GetOutputRaw(const SwcIf_EntrySpanType * span, void * dst);
#endif
#if (SWCIF_ENABLE_CONFIG_RAW_API == STD_ON)
Std_ReturnType SwcIf_GetConfigRaw(const SwcIf_EntrySpanType * span, void * dst);
#endif

#if (SWCIF_ENABLE_INPUT_OVR_API == STD_ON)
Std_ReturnType SwcIf_SetInputOvr(SwcIf_OvrSlotIdType ovr_slot_id, const SwcIf_EntrySpanType * span, const void * src);
#endif
#if (SWCIF_ENABLE_OUTPUT_OVR_API == STD_ON)
Std_ReturnType SwcIf_SetOutputOvr(SwcIf_OvrSlotIdType ovr_slot_id, const SwcIf_EntrySpanType * span, const void * src);
#endif
#if SWCIF_ANY_OVR
Std_ReturnType SwcIf_ClearOvr(SwcIf_OvrSlotIdType ovr_slot_id);
boolean SwcIf_IsOvrActive(SwcIf_OvrSlotIdType ovr_slot_id);
boolean SwcIf_IsInputOvrActive(SwcIf_InstanceType instance);
boolean SwcIf_IsOutputOvrActive(SwcIf_InstanceType instance);
#endif

#if (SWCIF_ENABLE_INSTANCE_INFO_API == STD_ON)
uint16_t SwcIf_GetInstanceCount(void);
const char * SwcIf_GetInstanceName(SwcIf_InstanceType instance);
#endif
#if (SWCIF_ENABLE_INPUT_INFO_API == STD_ON)
uint16_t SwcIf_GetInputEntryCount(SwcIf_InstanceType instance);
const char * SwcIf_GetInputEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
uint16_t SwcIf_GetInputEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
#endif
#if (SWCIF_ENABLE_OUTPUT_INFO_API == STD_ON)
uint16_t SwcIf_GetOutputEntryCount(SwcIf_InstanceType instance);
const char * SwcIf_GetOutputEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
uint16_t SwcIf_GetOutputEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
#endif
#if (SWCIF_ENABLE_CONFIG_INFO_API == STD_ON)
uint16_t SwcIf_GetConfigEntryCount(SwcIf_InstanceType instance);
const char * SwcIf_GetConfigEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
uint16_t SwcIf_GetConfigEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
#endif

#endif /* SWCIF_H_ */
