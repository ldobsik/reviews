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

STATIC_ASSERT(SWCIF_OVR_SLOT_COUNT <= 32u);

static uint16_t SwcIf_ScheduleTick_;
static const SwcIf_ScheduleExpiryPointType * SwcIf_CurrentExpiryPoint_;
static SwcIf_ScheduleType SwcIf_ActiveSchedule_;
static SwcIf_ScheduleType SwcIf_NextSchedule_;
static boolean SwcIf_ScheduleSwitchPending_;
boolean SwcIf_InstanceEnabled_[SWCIF_INSTANCE_COUNT];

#if SWCIF_ANY_OVR
static uint16_t SwcIf_ActiveOverrideCount_;
static SwcIf_OvrSlotType SwcIf_OvrSlots_[SWCIF_OVR_SLOT_COUNT];
static boolean SwcIf_IsValidOvrSlot_(SwcIf_OvrSlotIdType ovr_slot_id)
{
    return (boolean)(ovr_slot_id < SWCIF_OVR_SLOT_COUNT);
}
#endif

static boolean SwcIf_IsValidInstance_(SwcIf_InstanceType instance)
{
    return (boolean)((instance >= 0) && (instance < SWCIF_INSTANCE_COUNT));
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

#if (SWCIF_ANY_RAW || SWCIF_ANY_OVR)
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
#endif

#if SWCIF_ANY_RAW
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
#endif

#if SWCIF_ANY_OVR
static Std_ReturnType SwcIf_SetOvr_(SwcIf_OvrSlotIdType slot, SwcIf_AreaType area, const SwcIf_EntrySpanType * span, const void * src)
{
    Std_ReturnType ret = E_NOT_OK;
    if ((src != NULL_PTR) && (SwcIf_IsValidOvrSlot_(slot) == TRUE) && (area != SWCIF_AREA_CONFIG)) {
        uint8_t * target_ptr = SwcIf_ResolveRawPtr_(span, area);
        if ((target_ptr != NULL_PTR) && (span->byte_len <= SWCIF_OVR_BUFFER_LEN)) {
            uint8_t m_idx = (uint8_t)span->instance_id;
            if (area == SWCIF_AREA_OUTPUT) {
                m_idx += SWCIF_INSTANCE_COUNT;
            }

            if (SwcIf_OvrSlots_[slot].target_ptr == NULL_PTR) {
                SwcIf_ActiveOverrideCount_++;
            } else {
                uint8_t old_idx = SwcIf_OvrSlots_[slot].mask_idx;
                SwcIf_InstanceMasks_[old_idx] &= ~(((SwcIf_OvrMask_type)1u) << slot);
            }

            SwcIf_OvrSlots_[slot].target_ptr = target_ptr;
            SwcIf_OvrSlots_[slot].mask_idx = m_idx;
            SwcIf_OvrSlots_[slot].byte_len = span->byte_len;
            (void)memcpy(SwcIf_OvrSlots_[slot].data, src, span->byte_len);
            SwcIf_InstanceMasks_[m_idx] |= (((SwcIf_OvrMask_type)1u) << slot);
            ret = E_OK;
        }
    }
    return ret;
}

void SwcIf_ApplyOverrides_(SwcIf_OvrMask_type active_mask)
{
    uint8_t slot_idx = 0u;
    while (active_mask != 0u) {
        if ((active_mask & 1u) != 0u) {
            SWCIF_ENTER_CRITICAL();
            (void)memcpy(SwcIf_OvrSlots_[slot_idx].target_ptr, SwcIf_OvrSlots_[slot_idx].data, SwcIf_OvrSlots_[slot_idx].byte_len);
            SWCIF_EXIT_CRITICAL();
        }
        active_mask >>= 1u;
        slot_idx++;
    }
}
#endif /* SWCIF_ANY_OVR */

void SwcIf_Init(void)
{
    SwcIf_ActivateSchedule_(SwcIf_DefaultSchedule_);
    SwcIf_ScheduleSwitchPending_ = FALSE;

    for (SwcIf_InstanceType instance = 0; instance < SWCIF_INSTANCE_COUNT; instance++) {
        SwcIf_InstanceEnabled_[(uint8_t)instance] = TRUE;
        SwcIf_BindFnTable_[(uint8_t)instance](SwcIf_InstanceObjectTable_[(uint8_t)instance]);
        SwcIf_InitFnTable_[(uint8_t)instance](SwcIf_InstanceObjectTable_[(uint8_t)instance]);
    }

#if SWCIF_ANY_OVR
    SwcIf_ActiveOverrideCount_ = 0u;
    for (uint8_t i = 0u; i < (SWCIF_INSTANCE_COUNT * 2u); i++) {
        SwcIf_InstanceMasks_[i] = 0u;
    }
    for (SwcIf_OvrSlotIdType slot = 0u; slot < SWCIF_OVR_SLOT_COUNT; slot++) {
        SwcIf_OvrSlots_[slot].target_ptr = NULL_PTR;
        SwcIf_OvrSlots_[slot].mask_idx = 0u;
        SwcIf_OvrSlots_[slot].byte_len = 0u;
    }
#endif
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
    if (((SwcIf_ScheduleSwitchPending_ == TRUE) &&
         (SwcIf_ScheduleSwitchMode_[(uint8_t)SwcIf_NextSchedule_] == SWCIF_SCHEDULE_SWITCH_IMMEDIATE)) ||
        ((SwcIf_CurrentExpiryPoint_->offset_tick == SwcIf_ScheduleTick_) &&
         (SwcIf_CurrentExpiryPoint_->run_count == 0u))) {
        SwcIf_ActivateSchedule_(SwcIf_NextSchedule_);
        SwcIf_ScheduleSwitchPending_ = FALSE;
    }

    if (SwcIf_CurrentExpiryPoint_->offset_tick == SwcIf_ScheduleTick_) {
        SwcIf_ExecScheduleTick_(SwcIf_ActiveSchedule_, (uint8_t)SwcIf_ScheduleTick_);
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
Std_ReturnType SwcIf_GetInputRaw(const SwcIf_EntrySpanType * span, void * dst) { return SwcIf_GetRaw_(span, dst, SWCIF_AREA_INPUT); }
#endif
#if (SWCIF_ENABLE_OUTPUT_RAW_API == STD_ON)
Std_ReturnType SwcIf_GetOutputRaw(const SwcIf_EntrySpanType * span, void * dst) { return SwcIf_GetRaw_(span, dst, SWCIF_AREA_OUTPUT); }
#endif
#if (SWCIF_ENABLE_CONFIG_RAW_API == STD_ON)
Std_ReturnType SwcIf_GetConfigRaw(const SwcIf_EntrySpanType * span, void * dst) { return SwcIf_GetRaw_(span, dst, SWCIF_AREA_CONFIG); }
#endif
#if (SWCIF_ENABLE_INPUT_OVR_API == STD_ON)
Std_ReturnType SwcIf_SetInputOvr(SwcIf_OvrSlotIdType ovr_slot_id, const SwcIf_EntrySpanType * span, const void * src) { return SwcIf_SetOvr_(ovr_slot_id, SWCIF_AREA_INPUT, span, src); }
#endif
#if (SWCIF_ENABLE_OUTPUT_OVR_API == STD_ON)
Std_ReturnType SwcIf_SetOutputOvr(SwcIf_OvrSlotIdType ovr_slot_id, const SwcIf_EntrySpanType * span, const void * src) { return SwcIf_SetOvr_(ovr_slot_id, SWCIF_AREA_OUTPUT, span, src); }
#endif

#if SWCIF_ANY_OVR
Std_ReturnType SwcIf_ClearOvr(SwcIf_OvrSlotIdType ovr_slot_id)
{
    Std_ReturnType ret = E_NOT_OK;
    if (SwcIf_IsValidOvrSlot_(ovr_slot_id) == TRUE) {
        if (SwcIf_OvrSlots_[ovr_slot_id].target_ptr != NULL_PTR) {
            SwcIf_ActiveOverrideCount_--;
            uint8_t m_idx = SwcIf_OvrSlots_[ovr_slot_id].mask_idx;
            SwcIf_InstanceMasks_[m_idx] &= ~(((SwcIf_OvrMask_type)1u) << ovr_slot_id);
            SwcIf_OvrSlots_[ovr_slot_id].target_ptr = NULL_PTR;
        }
        SwcIf_OvrSlots_[ovr_slot_id].mask_idx = 0u;
        SwcIf_OvrSlots_[ovr_slot_id].byte_len = 0u;
        ret = E_OK;
    }
    return ret;
}

boolean SwcIf_IsOvrActive(SwcIf_OvrSlotIdType ovr_slot_id)
{
    return (boolean)((SwcIf_IsValidOvrSlot_(ovr_slot_id) == TRUE) && (SwcIf_OvrSlots_[ovr_slot_id].target_ptr != NULL_PTR));
}

boolean SwcIf_IsInputOvrActive(SwcIf_InstanceType instance)
{
    return (boolean)((SwcIf_IsValidInstance_(instance) == TRUE) && (SwcIf_InstanceMasks_[(uint8_t)instance] != 0u));
}

boolean SwcIf_IsOutputOvrActive(SwcIf_InstanceType instance)
{
    return (boolean)((SwcIf_IsValidInstance_(instance) == TRUE) && (SwcIf_InstanceMasks_[(uint8_t)instance + SWCIF_INSTANCE_COUNT] != 0u));
}
#endif

#if (SWCIF_ENABLE_INSTANCE_INFO_API == STD_ON)
uint16_t SwcIf_GetInstanceCount(void) { return SWCIF_INSTANCE_COUNT; }
const char * SwcIf_GetInstanceName(SwcIf_InstanceType instance) { return (SwcIf_IsValidInstance_(instance) == TRUE) ? SwcIf_InstanceNameTable_[(uint8_t)instance] : NULL_PTR; }
#endif
#if (SWCIF_ENABLE_INPUT_INFO_API == STD_ON)
uint16_t SwcIf_GetInputEntryCount(SwcIf_InstanceType instance) { return (SwcIf_IsValidInstance_(instance) == TRUE) ? SwcIf_InputEntryCountTable_[(uint8_t)instance] : 0u; }
const char * SwcIf_GetInputEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx) { return ((SwcIf_IsValidInstance_(instance) == TRUE) && (SwcIf_InputEntryNameTable_[(uint8_t)instance] != NULL_PTR) && (entry_idx < SwcIf_InputEntryCountTable_[(uint8_t)instance])) ? SwcIf_InputEntryNameTable_[(uint8_t)instance][entry_idx] : NULL_PTR; }
uint16_t SwcIf_GetInputEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx) { return ((SwcIf_IsValidInstance_(instance) == TRUE) && (SwcIf_InputEntrySizeTable_[(uint8_t)instance] != NULL_PTR) && (entry_idx < SwcIf_InputEntryCountTable_[(uint8_t)instance])) ? SwcIf_InputEntrySizeTable_[(uint8_t)instance][entry_idx] : 0u; }
#endif
#if (SWCIF_ENABLE_OUTPUT_INFO_API == STD_ON)
uint16_t SwcIf_GetOutputEntryCount(SwcIf_InstanceType instance) { return (SwcIf_IsValidInstance_(instance) == TRUE) ? SwcIf_OutputEntryCountTable_[(uint8_t)instance] : 0u; }
const char * SwcIf_GetOutputEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx) { return ((SwcIf_IsValidInstance_(instance) == TRUE) && (SwcIf_OutputEntryNameTable_[(uint8_t)instance] != NULL_PTR) && (entry_idx < SwcIf_OutputEntryCountTable_[(uint8_t)instance])) ? SwcIf_OutputEntryNameTable_[(uint8_t)instance][entry_idx] : NULL_PTR; }
uint16_t SwcIf_GetOutputEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx) { return ((SwcIf_IsValidInstance_(instance) == TRUE) && (SwcIf_OutputEntrySizeTable_[(uint8_t)instance] != NULL_PTR) && (entry_idx < SwcIf_OutputEntryCountTable_[(uint8_t)instance])) ? SwcIf_OutputEntrySizeTable_[(uint8_t)instance][entry_idx] : 0u; }
#endif
#if (SWCIF_ENABLE_CONFIG_INFO_API == STD_ON)
uint16_t SwcIf_GetConfigEntryCount(SwcIf_InstanceType instance) { return (SwcIf_IsValidInstance_(instance) == TRUE) ? SwcIf_ConfigEntryCountTable_[(uint8_t)instance] : 0u; }
const char * SwcIf_GetConfigEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx) { return ((SwcIf_IsValidInstance_(instance) == TRUE) && (SwcIf_ConfigEntryNameTable_[(uint8_t)instance] != NULL_PTR) && (entry_idx < SwcIf_ConfigEntryCountTable_[(uint8_t)instance])) ? SwcIf_ConfigEntryNameTable_[(uint8_t)instance][entry_idx] : NULL_PTR; }
uint16_t SwcIf_GetConfigEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx) { return ((SwcIf_IsValidInstance_(instance) == TRUE) && (SwcIf_ConfigEntrySizeTable_[(uint8_t)instance] != NULL_PTR) && (entry_idx < SwcIf_ConfigEntryCountTable_[(uint8_t)instance])) ? SwcIf_ConfigEntrySizeTable_[(uint8_t)instance][entry_idx] : 0u; }
#endif
