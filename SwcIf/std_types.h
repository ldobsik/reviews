#ifndef STD_TYPES_H_
#define STD_TYPES_H_

#include <stdint.h>
#include <stddef.h>

typedef uint8_t boolean;
typedef uint8_t Std_ReturnType;

#ifndef TRUE
#define TRUE ((boolean)1u)
#endif
#ifndef FALSE
#define FALSE ((boolean)0u)
#endif

#ifndef STD_ON
#define STD_ON 1u
#endif
#ifndef STD_OFF
#define STD_OFF 0u
#endif

#ifndef E_OK
#define E_OK ((Std_ReturnType)0u)
#endif
#ifndef E_NOT_OK
#define E_NOT_OK ((Std_ReturnType)1u)
#endif

#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(COND) typedef char static_assertion_##__LINE__[(COND) ? 1 : -1]
#endif

#endif /* STD_TYPES_H_ */
