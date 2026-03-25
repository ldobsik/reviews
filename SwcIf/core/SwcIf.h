/* ========================================================================== */
/*  FILE: SwcIf.h                                                             */
/* ========================================================================== */
/**
 * @file SwcIf.h
 * @brief Public SwcIf API.
 */

#ifndef SWCIF_H_
#define SWCIF_H_

#include <stdint.h>
#include "SwcIf_Cfg.h"
#include "SwcIf_Gen.h"
#include "SwcIf_Int.h"

/* ========================================================================== */
/*  Public Types                                                              */
/* ========================================================================== */

/** @brief Runtime instance identifier type. */
typedef int8_t  SwcIf_InstanceType;

/** @brief Runtime schedule identifier type. */
typedef int8_t  SwcIf_ScheduleType;

/** @brief Reflection entry identifier type. */
typedef uint16_t SwcIf_EntryIdxType;

/** @brief Override slot identifier type. */
typedef uint16_t SwcIf_OvrSlotIdType;

/**
 * @brief Logical span descriptor for raw access and override APIs.
 */
typedef struct
{
    SwcIf_InstanceType instance_id;
    SwcIf_EntryIdxType entry_idx;
    uint16_t           byte_offset;
    uint16_t           byte_len;
} SwcIf_EntrySpanType;

/* ========================================================================== */
/*  Public Runtime API                                                        */
/* ========================================================================== */

/** @brief Initialize the SwcIf runtime. */
void SwcIf_Init(void);

/**
 * @brief Enable one runtime instance.
 *
 * @param instance  Runtime instance identifier.
 */
void SwcIf_EnableInstance(SwcIf_InstanceType instance);

/**
 * @brief Disable one runtime instance.
 *
 * @param instance  Runtime instance identifier.
 */
void SwcIf_DisableInstance(SwcIf_InstanceType instance);

/** @brief Execute one SwcIf main-function cycle. */
void SwcIf_MainFunction(void);

/**
 * @brief Request transition to another runtime schedule.
 *
 * @param schedule  Target schedule identifier.
 *
 * @return E_OK on success, E_NOT_OK otherwise.
 */
Std_ReturnType SwcIf_SetNextSchedule(SwcIf_ScheduleType schedule);

/**
 * @brief Get the currently active runtime schedule.
 *
 * @return Active schedule identifier.
 */
SwcIf_ScheduleType SwcIf_GetActiveSchedule(void);

/* ========================================================================== */
/*  Public Raw Access API                                                     */
/* ========================================================================== */

#if (SWCIF_ENABLE_INPUT_RAW_API == STD_ON)
/**
 * @brief Read raw bytes from one logical input entry span.
 *
 * @param span      Span descriptor.
 * @param dst       Destination buffer.
 *
 * @return E_OK on success, E_NOT_OK otherwise.
 */
Std_ReturnType SwcIf_GetInputRaw(const SwcIf_EntrySpanType * span, void * dst);
#endif

#if (SWCIF_ENABLE_OUTPUT_RAW_API == STD_ON)
/**
 * @brief Read raw bytes from one logical output entry span.
 *
 * @param span      Span descriptor.
 * @param dst       Destination buffer.
 *
 * @return E_OK on success, E_NOT_OK otherwise.
 */
Std_ReturnType SwcIf_GetOutputRaw(const SwcIf_EntrySpanType * span, void * dst);
#endif

#if (SWCIF_ENABLE_CONFIG_RAW_API == STD_ON)
/**
 * @brief Read raw bytes from one logical config entry span.
 *
 * @param span      Span descriptor.
 * @param dst       Destination buffer.
 *
 * @return E_OK on success, E_NOT_OK otherwise.
 */
Std_ReturnType SwcIf_GetConfigRaw(const SwcIf_EntrySpanType * span, void * dst);
#endif

/* ========================================================================== */
/*  Public Override API                                                       */
/* ========================================================================== */

#if (SWCIF_ENABLE_INPUT_OVR_API == STD_ON)
/**
 * @brief Install one input override for a logical entry span.
 *
 * @param ovr_slot_id  Override slot identifier.
 * @param span         Span descriptor.
 * @param src          Source buffer.
 *
 * @return E_OK on success, E_NOT_OK otherwise.
 */
Std_ReturnType SwcIf_SetInputOvr(SwcIf_OvrSlotIdType ovr_slot_id, const SwcIf_EntrySpanType * span, const void * src);
#endif

#if (SWCIF_ENABLE_OUTPUT_OVR_API == STD_ON)
/**
 * @brief Install one output override for a logical entry span.
 *
 * @param ovr_slot_id  Override slot identifier.
 * @param span         Span descriptor.
 * @param src          Source buffer.
 *
 * @return E_OK on success, E_NOT_OK otherwise.
 */
Std_ReturnType SwcIf_SetOutputOvr(SwcIf_OvrSlotIdType ovr_slot_id, const SwcIf_EntrySpanType * span, const void * src);
#endif

#if SWCIF_ANY_OVR
/**
 * @brief Clear one override slot.
 *
 * @param ovr_slot_id  Override slot identifier.
 *
 * @return E_OK on success, E_NOT_OK otherwise.
 */
Std_ReturnType SwcIf_ClearOvr(SwcIf_OvrSlotIdType ovr_slot_id);

/**
 * @brief Check whether one override slot is active.
 *
 * @param ovr_slot_id  Override slot identifier.
 *
 * @return TRUE if active, FALSE otherwise.
 */
boolean SwcIf_IsOvrActive(SwcIf_OvrSlotIdType ovr_slot_id);
#endif

/* ========================================================================== */
/*  Public INFO API                                                           */
/* ========================================================================== */

#if (SWCIF_ENABLE_INSTANCE_INFO_API == STD_ON)
/**
 * @brief Get number of configured runtime instances.
 *
 * @return Number of configured runtime instances.
 */
uint16_t SwcIf_GetInstanceCount(void);

/**
 * @brief Get one instance name.
 *
 * @param instance  Runtime instance identifier.
 *
 * @return Pointer to static C-string, or NULL_PTR if invalid.
 */
const char * SwcIf_GetInstanceName(SwcIf_InstanceType instance);
#endif

#if (SWCIF_ENABLE_INPUT_INFO_API == STD_ON)
/** @brief Get input entry count for one instance. */
uint16_t SwcIf_GetInputEntryCount(SwcIf_InstanceType instance);
/** @brief Get input entry name. */
const char * SwcIf_GetInputEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
/** @brief Get input entry size in bytes. */
uint16_t SwcIf_GetInputEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
#endif

#if (SWCIF_ENABLE_OUTPUT_INFO_API == STD_ON)
/** @brief Get output entry count for one instance. */
uint16_t SwcIf_GetOutputEntryCount(SwcIf_InstanceType instance);
/** @brief Get output entry name. */
const char * SwcIf_GetOutputEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
/** @brief Get output entry size in bytes. */
uint16_t SwcIf_GetOutputEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
#endif

#if (SWCIF_ENABLE_CONFIG_INFO_API == STD_ON)
/** @brief Get config entry count for one instance. */
uint16_t SwcIf_GetConfigEntryCount(SwcIf_InstanceType instance);
/** @brief Get config entry name. */
const char * SwcIf_GetConfigEntryName(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
/** @brief Get config entry size in bytes. */
uint16_t SwcIf_GetConfigEntrySize(SwcIf_InstanceType instance, SwcIf_EntryIdxType entry_idx);
#endif

#endif /* SWCIF_H_ */

