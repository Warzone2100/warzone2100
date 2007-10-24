#ifndef _base_types_h
#define _base_types_h

#include "SDL.h"
#include "math.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE

#define MAX_PATH          260

typedef signed __int8		int8_t;
typedef unsigned __int8		uint8_t;
typedef signed __int16		int16_t;
typedef unsigned __int16	uint16_t;
typedef signed __int32		int32_t;
typedef unsigned __int32	uint32_t;
typedef signed __int64		int64_t;
typedef unsigned __int64	uint64_t;
	#ifdef FORCEINLINE
		#undef FORCEINLINE
	#endif
	#if (_MSC_VER >= 1200)
		#define FORCEINLINE __forceinline
	#else
		#define FORCEINLINE __inline
	#endif
#elif defined(__GNUC__)
#  include <limits.h>
#  define MAX_PATH PATH_MAX
#define HAVE_STDINT_H	1
#define FORCEINLINE inline
#else	/* exotic OSs */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#ifndef _SIZE_T_DEFINED_
#define _SIZE_T_DEFINED_
typedef unsigned int size_t;
#endif
typedef unsigned int uintptr_t;
#endif /* __GNUC__ || _MSC_VER */

/* Basic numeric types */
typedef uint8_t  UBYTE;
typedef int8_t   SBYTE;
typedef uint16_t UWORD;
typedef int16_t  SWORD;
typedef uint32_t UDWORD;
typedef int32_t  SDWORD;

#define MAX(a,b) a > b ? a : b
#define MIN(a,b) a < b ? a : b

#endif
