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

#define SWCIF_INT_CAT_(A, B) SWCIF_INT_CAT__(A,B)
#define SWCIF_INT_CAT__(A, B) A ## B

#define SWCIF_STATIC_BIND_FN(INSTANCE)      SWCIF_INT_CAT_(SwcIf_StaticBind_, INSTANCE)
#define SWCIF_IN_FN(INSTANCE)               SWCIF_INT_CAT_(SwcIf_InputRoute_, INSTANCE)
#define SWCIF_OUT_FN(INSTANCE)              SWCIF_INT_CAT_(SwcIf_OutputRoute_,INSTANCE)
#define SWCIF_OUT_CPTR_NAME(INSTANCE)       SWCIF_INT_CAT_(SwcIf_OutputCPtr_, INSTANCE)
#define SWCIF_OUT_CPTR(INSTANCE)            SWCIF_OUT_CPTR_NAME(INSTANCE)()
#define SWCIF_IN_IDX(INSTANCE, NAME)        SWCIF_INT_CAT_(SWCIF_INT_CAT_(SWCIF_, INSTANCE), SWCIF_INT_CAT_(_IN_IDX_, NAME))
#define SWCIF_OUT_IDX(INSTANCE, NAME)       SWCIF_INT_CAT_(SWCIF_INT_CAT_(SWCIF_, INSTANCE), SWCIF_INT_CAT_(_OUT_IDX_, NAME))
#define SWCIF_CONFIG_IDX(INSTANCE, NAME)    SWCIF_INT_CAT_(SWCIF_INT_CAT_(SWCIF_, INSTANCE), SWCIF_INT_CAT_(_CONFIG_IDX_, NAME))
#define SWCIF_SERVER_NAME(INSTANCE, SERVER) SWCIF_INT_CAT_(SWCIF_INT_CAT_(SwcIf_ServerClosure_, INSTANCE), SWCIF_INT_CAT_(_, SERVER))
#define SWCIF_SERVER(INSTANCE, SERVER)      SWCIF_SERVER_NAME(INSTANCE, SERVER)

#endif /* SWCIF_INT_H_ */

