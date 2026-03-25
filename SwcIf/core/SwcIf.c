/* FILE: SwcIf.c - Static SwcIf runtime implementation */
#include <stdint.h>
#include <string.h>
#include "std_types.h"
#include "SwcIf_IntCfg.h"
#include "ext.h"
#include "SwcIf.h"
#include "SwcIf_Priv.h"
#include "SwcIf_GenMeta.h"

typedef enum {
    SWCIF_AREA_INPUT = 0,
    SWCIF_AREA_OUTPUT,
    SWCIF_AREA_CONFIG,
    SWCIF_AREA_COUNT
} SwcIf_AreaType;

static uint16_t SwcIf_ScheduleTick_;
static const SwcIf_ScheduleExpiryPointType * SwcIf_CurrentExpiryPoint_;
static SwcIf_ScheduleType SwcIf_ActiveSchedule_;
static SwcIf_ScheduleType SwcIf_NextSchedule_;
static boolean SwcIf_ScheduleSwitchPending_;
static boolean SwcIf_InstanceEnabled_[SWCIF_INSTANCE_COUNT];
static uint16_t SwcIf_ActiveOverrideCount_;
static SwcIf_OvrSlotType SwcIf_OvrSlots_[SWCIF_OVR_SLOT_COUNT];

static boolean SwcIf_IsValidInstance_(SwcIf_InstanceType instance)
{
    return (boolean)((instance >= 0) && (instance < SWCIF_INSTANCE_COUNT));
}

static boolean SwcIf_IsValidOvrSlot_(SwcIf_OvrSlotIdType ovr_slot_id)
{
    return (boolean)(ovr_slot_id < SWCIF_OVR_SLOT_COUNT);
}

static boolean SwcIf_IsValidSchedule_(SwcIf_ScheduleType schedule)
{
    return (boolean)((schedule >= 0) && (schedule < SWCIF_SCHEDULE_COUNT));
}

static void SwcIf_ActivateSchedule_(SwcIf_ScheduleType schedule)
{
    SwcIf_ActiveSchedule_ = schedule;
    SwcIf_CurrentExpiryPoint_ = SwcIf_ScheduleExpiryPointTable_[(uint8_t)schedule];
    SwcIf_ScheduleTick_ = 0u;
    SwcIf_NextSchedule_ = schedule;
}

#if (SWCIF_ANY_RAW_OR_OVR)
static uint8_t * SwcIf_ResolveRawPtr_(const SwcIf_EntrySpanType * span, SwcIf_AreaType area)
{
    uint8_t * ret_ptr = NULL_PTR;
    const uint8_t * base_ptr = NULL_PTR;
    const SwcIf_RawEntryMetaType * meta_table = NULL_PTR;
    uint16_t entry_count = 0u;

    if ((span == NULL_PTR) || (SwcIf_IsValidInstance_(span->instance_id) == FALSE) || (area >= SWCIF_AREA_COUNT)) {
        return NULL_PTR;
    }

    switch (area) {
        case SWCIF_AREA_INPUT:
            base_ptr = SwcIf_InputBaseTable_[(uint8_t)span->instance_id];
            meta_table = SwcIf_InputMetaTable_[(uint8_t)span->instance_id];
            entry_count = SwcIf_InputEntryCountTable_[(uint8_t)span->instance_id];
            break;
        case SWCIF_AREA_OUTPUT:
            base_ptr = SwcIf_OutputBaseTable_[(uint8_t)span->instance_id];
            meta_table = SwcIf_OutputMetaTable_[(uint8_t)span->instance_id];
            entry_count = SwcIf_OutputEntryCountTable_[(uint8_t)span->instance_id];
            break;
        case SWCIF_AREA_CONFIG:
            if (SwcIf_ConfigPtrAddressTable_[(uint8_t)span->instance_id] != NULL_PTR) {
                base_ptr = (const uint8_t *)(*SwcIf_ConfigPtrAddressTable_[(uint8_t)span->instance_id]);
            }
            meta_table = SwcIf_ConfigMetaTable_[(uint8_t)span->instance_id];
            entry_count = SwcIf_ConfigEntryCountTable_[(uint8_t)span->instance_id];
            break;
        default:
            break;
    }

    if ((base_ptr != NULL_PTR) && (meta_table != NULL_PTR) && (span->entry_idx < entry_count)) {
        const SwcIf_RawEntryMetaType * meta = &meta_table[span->entry_idx];
        if (((uint32_t)span->byte_offset + span->byte_len) <= meta->size) {
            ret_ptr = (uint8_t *)base_ptr + meta->offset + span->byte_offset;
        }
    }

    return ret_ptr;
}

static Std_ReturnType SwcIf_GetRaw_(const SwcIf_EntrySpanType * span, void * dst, SwcIf_AreaType area)
{
    Std_ReturnType ret = E_NOT_OK;

    if (dst != NULL_PTR) {
        const uint8_t * src_ptr = SwcIf_ResolveRawPtr_(span, area);
        if (src_ptr != NULL_PTR) {
            SWCIF_ENTER_CRITICAL();
            (void)memcpy(dst, src_ptr, span->byte_len);
            SWCIF_EXIT_CRITICAL();
            ret = E_OK;
        }
    }

    return ret;
}

#if ((SWCIF_ENABLE_INPUT_OVR_API == STD_ON) || (SWCIF_ENABLE_OUTPUT_OVR_API == STD_ON))
static Std_ReturnType SwcIf_SetOvr_(SwcIf_OvrSlotIdType slot, SwcIf_AreaType area, const SwcIf_EntrySpanType * span, const void * src)
{
    Std_ReturnType ret = E_NOT_OK;

    if ((src != NULL_PTR) && (SwcIf_IsValidOvrSlot_(slot) == TRUE) && (area != SWCIF_AREA_CONFIG)) {
        uint8_t * target_ptr = SwcIf_ResolveRawPtr_(span, area);
        if ((target_ptr != NULL_PTR) && (span->byte_len <= SWCIF_OVR_BUFFER_LEN)) {
            if (SwcIf_OvrSlots_[slot].target_ptr == NULL_PTR) {
                SwcIf_ActiveOverrideCount_++;
            }
            SwcIf_OvrSlots_[slot].target_ptr = target_ptr;
            SwcIf_OvrSlots_[slot].instance_id = span->instance_id;
            SwcIf_OvrSlots_[slot].byte_len = span->byte_len;
            (void)memcpy(SwcIf_OvrSlots_[slot].data, src, span->byte_len);
            ret = E_OK;
        }
    }

    return ret;
}

#endif /* SWCIF_ANY_RAW_OR_OVR */
static void SwcIf_ApplyOverrides_(SwcIf_InstanceType instance, SwcIf_AreaType area)
{
    SwcIf_OvrSlotIdType slot;

    if (SwcIf_ActiveOverrideCount_ > 0u) {
        for (slot = 0u; slot < SWCIF_OVR_SLOT_COUNT; slot++) {
            if ((SwcIf_OvrSlots_[slot].target_ptr != NULL_PTR) &&
                (SwcIf_OvrSlots_[slot].instance_id == instance)) {
                SWCIF_ENTER_CRITICAL();
                (void)memcpy(SwcIf_OvrSlots_[slot].target_ptr,
                             SwcIf_OvrSlots_[slot].data,
                             SwcIf_OvrSlots_[slot].byte_len);
                SWCIF_EXIT_CRITICAL();
            }
        }
    }
    (void)area;
}
#endif

static void SwcIf_RunInstance_(SwcIf_InstanceType instance)
{
    if (SwcIf_InstanceEnabled_[(uint8_t)instance] == TRUE) {
        SwcIf_InputUpdateTable_[(uint8_t)instance](SwcIf_InstanceObjectTable_[(uint8_t)instance]);
#if (SWCIF_ENABLE_INPUT_OVR_API == STD_ON)
        SwcIf_ApplyOverrides_(instance, SWCIF_AREA_INPUT);
#endif
        SwcIf_StepFnTable_[(uint8_t)instance](SwcIf_InstanceObjectTable_[(uint8_t)instance]);
#if (SWCIF_ENABLE_OUTPUT_OVR_API == STD_ON)
        SwcIf_ApplyOverrides_(instance, SWCIF_AREA_OUTPUT);
#endif
        SwcIf_OutputUpdateTable_[(uint8_t)instance](SwcIf_InstanceObjectTable_[(uint8_t)instance]);
    }
}

void SwcIf_Init(void)
{
    SwcIf_InstanceType instance;
    SwcIf_OvrSlotIdType slot;

    SwcIf_ActivateSchedule_(SwcIf_DefaultSchedule_);
    SwcIf_ScheduleSwitchPending_ = FALSE;
    SwcIf_ActiveOverrideCount_ = 0u;

    for (instance = 0; instance < SWCIF_INSTANCE_COUNT; instance++) {
        SwcIf_InstanceEnabled_[(uint8_t)instance] = TRUE;
        SwcIf_BindFnTable_[(uint8_t)instance](SwcIf_InstanceObjectTable_[(uint8_t)instance]);
        SwcIf_InitFnTable_[(uint8_t)instance](SwcIf_InstanceObjectTable_[(uint8_t)instance]);
    }

    for (slot = 0u; slot < SWCIF_OVR_SLOT_COUNT; slot++) {
        SwcIf_OvrSlots_[slot].target_ptr = NULL_PTR;
        SwcIf_OvrSlots_[slot].instance_id = -1;
        SwcIf_OvrSlots_[slot].byte_len = 0u;
    }
}

void SwcIf_EnableInstance(SwcIf_InstanceType instance)
{
    if (SwcIf_IsValidInstance_(instance) == TRUE) {
        SwcIf_InstanceEnabled_[(uint8_t)instance] = TRUE;
    }
}

void SwcIf_DisableInstance(SwcIf_InstanceType instance)
{
    if (SwcIf_IsValidInstance_(instance) == TRUE) {
        SwcIf_InstanceEnabled_[(uint8_t)instance] = FALSE;
    }
}

void SwcIf_MainFunction(void)
{
    uint16_t run_idx;

    if (((SwcIf_ScheduleSwitchPending_ == TRUE) &&
         (SwcIf_ScheduleSwitchMode_[(uint8_t)SwcIf_NextSchedule_] == SWCIF_SCHEDULE_SWITCH_IMMEDIATE)) ||
        ((SwcIf_CurrentExpiryPoint_->offset_tick == SwcIf_ScheduleTick_) &&
         (SwcIf_CurrentExpiryPoint_->run_count == 0u))) {
        SwcIf_ActivateSchedule_(SwcIf_NextSchedule_);
        SwcIf_ScheduleSwitchPending_ = FALSE;
    }

    if (SwcIf_CurrentExpiryPoint_->offset_tick == SwcIf_ScheduleTick_) {
        for (run_idx = 0u; run_idx < SwcIf_CurrentExpiryPoint_->run_count; run_idx++) {
            SwcIf_RunInstance_(SwcIf_CurrentExpiryPoint_->run_list[run_idx]);
        }
        SwcIf_CurrentExpiryPoint_++;
    }

    SwcIf_ScheduleTick_++;

    if ((SwcIf_ScheduleSwitchPending_ == TRUE) &&
        (SwcIf_ScheduleSwitchMode_[(uint8_t)SwcIf_NextSchedule_] == SWCIF_SCHEDULE_SWITCH_IMMEDIATE)) {
        SwcIf_ActivateSchedule_(SwcIf_NextSchedule_);
        SwcIf_ScheduleSwitchPending_ = FALSE;
    }
}

Std_ReturnType SwcIf_SetNextSchedule(SwcIf_ScheduleType schedule)
{
    Std_ReturnType ret = E_NOT_OK;

    if (SwcIf_IsValidSchedule_(schedule) == TRUE) {
        SwcIf_NextSchedule_ = schedule;
        SwcIf_ScheduleSwitchPending_ = TRUE;
        ret = E_OK;
    }

    return ret;
}

SwcIf_ScheduleType SwcIf_GetActiveSchedule(void)
{
    return SwcIf_ActiveSchedule_;
}

#if (SWCIF_ENABLE_INPUT_RAW_API == STD_ON)
Std_ReturnType SwcIf_GetInputRaw(const SwcIf_EntrySpanType * span, void * dst)
{
    return SwcIf_GetRaw_(span, dst, SWCIF_AREA_INPUT);
}
#endif

#if (SWCIF_ENABLE_OUTPUT_RAW_API == STD_ON)
Std_ReturnType SwcIf_GetOutputRaw(const SwcIf_EntrySpanType * span, void * dst)
{
    return SwcIf_GetRaw_(span, dst, SWCIF_AREA_OUTPUT);
}
#endif

#if (SWCIF_ENABLE_CONFIG_RAW_API == STD_ON)
Std_ReturnType SwcIf_GetConfigRaw(const SwcIf_EntrySpanType * span, void * dst)
{
    return SwcIf_GetRaw_(span, dst, SWCIF_AREA_CONFIG);
}
#endif

#if (SWCIF_ENABLE_INPUT_OVR_API == STD_ON)
Std_ReturnType SwcIf_SetInputOvr(SwcIf_OvrSlotIdType ovr_slot_id, const SwcIf_EntrySpanType * span, const void * src)
{
    return SwcIf_SetOvr_(ovr_slot_id, SWCIF_AREA_INPUT, span, src);
}
#endif

#if (SWCIF_ENABLE_OUTPUT_OVR_API == STD_ON)
Std_ReturnType SwcIf_SetOutputOvr(SwcIf_OvrSlotIdType ovr_slot_id, const SwcIf_EntrySpanType * span, const void * src)
{
    return SwcIf_SetOvr_(ovr_slot_id, SWCIF_AREA_OUTPUT, span, src);
}
#endif

Std_ReturnType SwcIf_ClearOvr(SwcIf_OvrSlotIdType ovr_slot_id)
{
    Std_ReturnType ret = E_NOT_OK;

    if (SwcIf_IsValidOvrSlot_(ovr_slot_id) == TRUE) {
        if (SwcIf_OvrSlots_[ovr_slot_id].target_ptr != NULL_PTR) {
            SwcIf_ActiveOverrideCount_--;
            SwcIf_OvrSlots_[ovr_slot_id].target_ptr = NULL_PTR;
        }
        SwcIf_OvrSlots_[ovr_slot_id].instance_id = -1;
        SwcIf_OvrSlots_[ovr_slot_id].byte_len = 0u;
        ret = E_OK;
    }

    return ret;
}

boolean SwcIf_IsOvrActive(SwcIf_OvrSlotIdType ovr_slot_id)
{
    boolean ret = FALSE;

    if (SwcIf_IsValidOvrSlot_(ovr_slot_id) == TRUE) {
        if (SwcIf_OvrSlots_[ovr_slot_id].target_ptr != NULL_PTR) {
            ret = TRUE;
        }
    }

    return ret;
}

#if (SWCIF_ENABLE_INSTANCE_INFO_API == STD_ON)
uint16_t SwcIf_GetInstanceCount(void)
{
    return SWCIF_INSTANCE_COUNT;
}

const char * SwcIf_GetInstanceName(SwcIf_InstanceType instance)
{
    const char * ret = NULL_PTR;
    if (SwcIf_IsValidInstance_(instance) == TRUE) {
        ret = SwcIf_InstanceNameTable_[(uint8_t)instance];
    }
    return ret;
}
#endif

#if (SWCIF_ENABLE_INPUT_INFO_API == STD_ON)
uint16_t SwcIf_GetInputEntryCount(SwcIf_InstanceType instance)
{
    return (SwcIf_IsValidInstance_(instance) == TRUE)
        ? SwcIf_InputEntryCountTable_[(uint8_t)instance]
        : 0u;
}

const char * SwcIf_GetInputEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx)
{
    const char * ret = NULL_PTR;
    if ((SwcIf_IsValidInstance_(instance) == TRUE) &&
        (SwcIf_InputEntryNameTable_[(uint8_t)instance] != NULL_PTR) &&
        (entry_idx < SwcIf_InputEntryCountTable_[(uint8_t)instance])) {
        ret = SwcIf_InputEntryNameTable_[(uint8_t)instance][entry_idx];
    }
    return ret;
}

uint16_t SwcIf_GetInputEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx)
{
    uint16_t ret = 0u;
    if ((SwcIf_IsValidInstance_(instance) == TRUE) &&
        (SwcIf_InputEntrySizeTable_[(uint8_t)instance] != NULL_PTR) &&
        (entry_idx < SwcIf_InputEntryCountTable_[(uint8_t)instance])) {
        ret = SwcIf_InputEntrySizeTable_[(uint8_t)instance][entry_idx];
    }
    return ret;
}
#endif

#if (SWCIF_ENABLE_OUTPUT_INFO_API == STD_ON)
uint16_t SwcIf_GetOutputEntryCount(SwcIf_InstanceType instance)
{
    return (SwcIf_IsValidInstance_(instance) == TRUE)
        ? SwcIf_OutputEntryCountTable_[(uint8_t)instance]
        : 0u;
}

const char * SwcIf_GetOutputEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx)
{
    const char * ret = NULL_PTR;
    if ((SwcIf_IsValidInstance_(instance) == TRUE) &&
        (SwcIf_OutputEntryNameTable_[(uint8_t)instance] != NULL_PTR) &&
        (entry_idx < SwcIf_OutputEntryCountTable_[(uint8_t)instance])) {
        ret = SwcIf_OutputEntryNameTable_[(uint8_t)instance][entry_idx];
    }
    return ret;
}

uint16_t SwcIf_GetOutputEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx)
{
    uint16_t ret = 0u;
    if ((SwcIf_IsValidInstance_(instance) == TRUE) &&
        (SwcIf_OutputEntrySizeTable_[(uint8_t)instance] != NULL_PTR) &&
        (entry_idx < SwcIf_OutputEntryCountTable_[(uint8_t)instance])) {
        ret = SwcIf_OutputEntrySizeTable_[(uint8_t)instance][entry_idx];
    }
    return ret;
}
#endif

#if (SWCIF_ENABLE_CONFIG_INFO_API == STD_ON)
uint16_t SwcIf_GetConfigEntryCount(SwcIf_InstanceType instance)
{
    return (SwcIf_IsValidInstance_(instance) == TRUE)
        ? SwcIf_ConfigEntryCountTable_[(uint8_t)instance]
        : 0u;
}

const char * SwcIf_GetConfigEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx)
{
    const char * ret = NULL_PTR;
    if ((SwcIf_IsValidInstance_(instance) == TRUE) &&
        (SwcIf_ConfigEntryNameTable_[(uint8_t)instance] != NULL_PTR) &&
        (entry_idx < SwcIf_ConfigEntryCountTable_[(uint8_t)instance])) {
        ret = SwcIf_ConfigEntryNameTable_[(uint8_t)instance][entry_idx];
    }
    return ret;
}

uint16_t SwcIf_GetConfigEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx)
{
    uint16_t ret = 0u;
    if ((SwcIf_IsValidInstance_(instance) == TRUE) &&
        (SwcIf_ConfigEntrySizeTable_[(uint8_t)instance] != NULL_PTR) &&
        (entry_idx < SwcIf_ConfigEntryCountTable_[(uint8_t)instance])) {
        ret = SwcIf_ConfigEntrySizeTable_[(uint8_t)instance][entry_idx];
    }
    return ret;
}
#endif
