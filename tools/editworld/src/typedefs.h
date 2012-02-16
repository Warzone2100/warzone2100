/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

	$Revision$
	$Id$
	$HeadURL$
*/

#ifndef __INCLUDED_TYPEDEFS_H__
#define __INCLUDED_TYPEDEFS_H__

typedef signed long SLONG;
typedef signed short SWORD;
typedef signed char SBYTE;
typedef unsigned long ULONG;
typedef unsigned short UWORD;
typedef unsigned char UBYTE;

#define UBYTE_MAX 0xff
#define UWORD_MAX 0xffff
#define UDWORD_MAX 0xffffffff

typedef signed		char	STRING;
typedef	unsigned	int		UDWORD;
typedef	signed		int		SDWORD;

typedef int BOOL;

#ifndef NULL
#define NULL (0)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#define	setRect(rct,x0,y0,x1,y1) (rct)->top=y0; (rct)->left=x0; \
											(rct)->bottom=y1; (rct)->right=x1
#define	setRectWH(rct,x,y,width,height) (rct)->top=y; (rct)->left=x; \
											(rct)->bottom=y+height; (rct)->right=x+width

#endif // __INCLUDED_TYPEDEFS_H__
