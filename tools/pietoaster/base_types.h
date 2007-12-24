/*
 *  PieToaster is an OpenGL application to edit 3D models in
 *  Warzone 2100's (an RTS game) PIE 3D model format, which is heavily
 *  inspired by PieSlicer created by stratadrake.
 *  Copyright (C) 2007  Carl Hee
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _base_types_h
#define _base_types_h

#include <SDL.h>
#include <math.h>

#include "wzglobal.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE

#ifndef MAX_PATH
    #define MAX_PATH          260
#endif

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
#ifndef FORCEINLINE
    #define FORCEINLINE inline
#endif
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

typedef struct _named_matrices_4f {
	float	m11, m12, m13, m14;
	float	m21, m22, m23, m24;
	float	m31, m32, m33, m34;
	float	m41, m42, m43, m44;
} NAMED_MATRICES_4F;

typedef struct _named_matrices_4d {
	double	m11, m12, m13, m14;
	double	m21, m22, m23, m24;
	double	m31, m32, m33, m34;
	double	m41, m42, m43, m44;
} NAMED_MATRICES_4D;

class CPieMatrix4F {
public:
	union {
		float	um[16];
		NAMED_MATRICES_4F	m;
	};

	/*
	CPieMatrix4F	operator+(CPieMatrix4F matrix) = {
		CPieMatrix4F(this->m.m11 + matrix.m.m11,
		this->m.m12 + matrix.m.m12,
		this->m.m13 + matrix.m.m13,
		this->m.m14 + matrix.m.m14,
		this->m.m21 + matrix.m.m21,
		this->m.m22 + matrix.m.m22,
		this->m.m23 + matrix.m.m23,
		this->m.m24 + matrix.m.m24,
		this->m.m31 + matrix.m.m31,
		this->m.m32 + matrix.m.m32,
		this->m.m33 + matrix.m.m33,
		this->m.m34 + matrix.m.m34,
		this->m.m41 + matrix.m.m41,
		this->m.m42 + matrix.m.m42,
		this->m.m43 + matrix.m.m43,
		this->m.m44 + matrix.m.m44)
	};
	*/
};

class CPieMatrix4D {
public:
	union {
		double	um[16];
		NAMED_MATRICES_4D	m;
	};
};

#endif
