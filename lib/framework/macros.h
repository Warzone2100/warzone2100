/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/*! \file macros.h
 *  \brief Various macro definitions
 */
#ifndef MACROS_H
#define MACROS_H

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ABS(a) (((a) < 0) ? (-(a)) : (a))

#define ABSDIF(a,b) ((a)>(b) ? (a)-(b) : (b)-(a))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]) + WZ_ASSERT_ARRAY_EXPR(x))

#define CLIP(val, min, max) do                                                \
{                                                                             \
    if ((val) < (min)) (val) = (min);                                         \
    else if ((val) > (max)) (val) = (max);                                    \
} while(0)

/**
 * Returns the index of the lowest value in the array.
 */
static inline int arrayMin(const int *array, const size_t n, size_t *index)
{
	size_t i, minIdx;
	
	// Find the index of the minimum value
	for (i = minIdx = 0; i < n; i++)
	{
		if (array[i] < array[minIdx])
		{
			minIdx = i;
		}
	}
	
	// If requested, store the index of the minimum value
	if (index != NULL)
	{
		*index = minIdx;
	}
	
	// Return the value
	return array[minIdx];
}

static inline float arrayMinF(const float *array, const size_t n, size_t *index)
{
	size_t i, minIdx;
	
	// Find the index of the minimum value
	for (i = minIdx = 0; i < n; i++)
	{
		if (array[i] < array[minIdx])
		{
			minIdx = i;
		}
	}
	
	// If requested, store the index of the minimum value
	if (index != NULL)
	{
		*index = minIdx;
	}
	
	// Return the value
	return array[minIdx];
}

static inline int arrayMax(const int *array, const size_t n, size_t *index)
{
	size_t i, maxIdx;
	
	// Find the index of the maximum value
	for (i = maxIdx = 0; i < n; i++)
	{
		if (array[i] > array[maxIdx])
		{
			maxIdx = i;
		}
	}
	
	// If requested, store the index of the maximum value
	if (index != NULL)
	{
		*index = maxIdx;
	}
	
	// Return the value
	return array[maxIdx];
}

static inline float arrayMaxF(const float *array, const size_t n, size_t *index)
{
	size_t i, maxIdx;
	
	// Find the index of the maximum value
	for (i = maxIdx = 0; i < n; i++)
	{
		if (array[i] > array[maxIdx])
		{
			maxIdx = i;
		}
	}
	
	// If requested, store the index of the maximum value
	if (index != NULL)
	{
		*index = maxIdx;
	}
	
	// Return the value
	return array[maxIdx];
}

/*
   defines for ONEINX
   Use: if (ONEINX) { code... }
*/
#define	ONEINTWO				(rand()%2==0)
#define ONEINTHREE				(rand()%3==0)
#define ONEINFOUR				(rand()%4==0)
#define ONEINFIVE				(rand()%5==0)
#define ONEINEIGHT				(rand()%8==0)
#define ONEINTEN				(rand()%10==0)

#define MACROS_H_STRINGIFY(x) #x
#define TOSTRING(x) MACROS_H_STRINGIFY(x)

#define AT_MACRO __FILE__ ":" TOSTRING(__LINE__)

#define MKID(a) MKID_(a, __LINE__)
#define MKID_(a, b) a ## b

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // MACROS_H
