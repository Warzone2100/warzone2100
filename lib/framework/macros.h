/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define ABSDIF(a,b) ((a)>(b) ? (a)-(b) : (b)-(a))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]) + WZ_ASSERT_ARRAY_EXPR(x))

#define CLIP(val, min, max) do                                                \
{                                                                             \
    if ((val) < (min)) (val) = (min);                                         \
    else if ((val) > (max)) (val) = (max);                                    \
} while(0)

/*
   defines for ONEINX
   Use: if (ONEINX) { code... }
*/
#define	ONEINTWO				(rand()%2==0)
#define ONEINTHREE				(rand()%3==0)
#define ONEINFOUR				(rand()%4==0)
#define ONEINFIVE				(rand()%5==0)
#define ONEINTEN				(rand()%10==0)

#define MACROS_H_STRINGIFY(x) #x
#define TOSTRING(x) MACROS_H_STRINGIFY(x)

#define AT_MACRO __FILE__ ":" TOSTRING(__LINE__)

#endif // MACROS_H
