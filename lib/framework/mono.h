/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/*
 *	File:	mono.h
 *
 */

#ifndef _mono_h
#define _mono_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

#include "types.h"

/*
*****************************************************************************************
*	monochrome text attributes
*****************************************************************************************
*/
#define	MONO_NORMAL					(0x07)
#define	MONO_BRIGHT					(0x0F)
#define	MONO_BLINK					(0x87)
#define	MONO_UNDERLINE				(0x01)
#define	MONO_BRIGHT_UNDERLINE		(0x09)
#define	MONO_REVERSED				(0x70)
#define	MONO_BLINK_REVERSED			(0xF0)


/*
*****************************************************************************************
*	monochrome screen dimensions
*****************************************************************************************
*/
#define	MONO_SCREEN_WIDTH			(80)
#define	MONO_SCREEN_HEIGHT			(25)


/*
*****************************************************************************************
*	library function prototypes
*****************************************************************************************
*/
extern void		 dbg_MONO_ClearRectangle(SDWORD, SDWORD, SDWORD, SDWORD);
extern void		 dbg_MONO_ClearScreen(void);
extern void		 dbg_MONO_PrintString(SDWORD, SDWORD, SBYTE *, ...);

/* Tell lint that PrintString takes printf like arguments */
/*lint -printf(3,dbg_MONO_PrintString) */

#endif
