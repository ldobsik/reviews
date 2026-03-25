/* ========================================================================== */
/*  FILE: SwcIf_Int.h                                                         */
/* ========================================================================== */
/**
 * @file SwcIf_Int.h
 * @brief Static token-pasting helpers for SwcIf symbols.
 */

#ifndef SWCIF_INT_H_
#define SWCIF_INT_H_

/* ========================================================================== */
/*  Static Token-Pasting Helpers                                              */
/* ========================================================================== */

#define SWCIF_STATIC_BIND_FN(INSTANCE)      SwcIf_StaticBind_ ## INSTANCE
#define SWCIF_IN_FN(INSTANCE)               SwcIf_InputRoute_ ## INSTANCE
#define SWCIF_OUT_FN(INSTANCE)              SwcIf_OutputRoute_ ## INSTANCE
#define SWCIF_OUT_CPTR_NAME(INSTANCE)       SwcIf_OutputCPtr_ ## INSTANCE
#define SWCIF_OUT_CPTR(INSTANCE)            SWCIF_OUT_CPTR_NAME(INSTANCE)()
#define SWCIF_IN_IDX(INSTANCE, NAME)        SWCIF_ ## INSTANCE ## _IN_IDX_ ## NAME
#define SWCIF_OUT_IDX(INSTANCE, NAME)       SWCIF_ ## INSTANCE ## _OUT_IDX_ ## NAME
#define SWCIF_CONFIG_IDX(INSTANCE, NAME)    SWCIF_ ## INSTANCE ## _CONFIG_IDX_ ## NAME
#define SWCIF_SERVER_NAME(INSTANCE, SERVER) SwcIf_ServerClosure_ ## INSTANCE ## _ ## SERVER
#define SWCIF_SERVER(INSTANCE, SERVER)      SWCIF_SERVER_NAME(INSTANCE, SERVER)

#endif /* SWCIF_INT_H_ */

