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
#ifndef _piePalette_
#define _piePalette_

#include "lib/ivis_common/piedef.h"

#define WZCOL_BLACK					psPalette[0]
#define WZCOL_WHITE					psPalette[1]
#define WZCOL_RELOAD_BACKGROUND		psPalette[2]
#define WZCOL_RELOAD_BAR			psPalette[3]
#define WZCOL_HEALTH_HIGH			psPalette[4]
#define WZCOL_HEALTH_MEDIUM			psPalette[5]
#define WZCOL_HEALTH_LOW			psPalette[6]
#define WZCOL_GREEN					psPalette[7]
#define WZCOL_RED					psPalette[8]
#define WZCOL_YELLOW				psPalette[9]
#define WZCOL_MENU_BACKGROUND		psPalette[10]
#define WZCOL_MENU_BORDER			psPalette[11]
#define WZCOL_MENU_LOAD_BORDER		psPalette[12]
#define WZCOL_CURSOR				psPalette[13]
#define WZCOL_MENU_SCORES_INTERIOR	psPalette[14]
#define WZCOL_MENU_SEPARATOR		psPalette[15]
#define WZCOL_TEXT_BRIGHT			psPalette[16]
#define WZCOL_TEXT_MEDIUM			psPalette[17]
#define WZCOL_TEXT_DARK				psPalette[18]
#define WZCOL_SCORE_BOX_BORDER		psPalette[19]
#define WZCOL_SCORE_BOX				psPalette[20]
#define WZCOL_TOOLTIP_TEXT			psPalette[21]
#define WZCOL_UNIT_SELECT_BORDER	psPalette[22]
#define WZCOL_UNIT_SELECT_BOX		psPalette[23]
#define WZCOL_RADAR_BACKGROUND		psPalette[24]
#define WZCOL_MAP_OUTLINE_OK		psPalette[25]
#define WZCOL_MAP_OUTLINE_BAD		psPalette[26]
#define WZCOL_KEYMAP_ACTIVE			psPalette[27]
#define WZCOL_KEYMAP_FIXED			psPalette[28]
#define WZCOL_MENU_SCORE_LOSS		psPalette[29]
#define WZCOL_MENU_SCORE_DESTROYED	psPalette[30]
#define WZCOL_MENU_SCORE_BUILT		psPalette[31]
#define WZCOL_MENU_SCORE_RANK		psPalette[32]
#define WZCOL_FRAME_BORDER_NORMAL	psPalette[33]
#define WZCOL_CONS_TEXT_SYSTEM		psPalette[34]
#define WZCOL_CONS_TEXT_USER		psPalette[35]
#define WZCOL_CONS_TEXT_USER_ALLY	psPalette[36]
#define WZCOL_CONS_TEXT_USER_ENEMY	psPalette[37]
#define WZCOL_CONS_TEXT_DEBUG		psPalette[38]
#define WZCOL_GREY					psPalette[39]
#define WZCOL_MAP_PREVIEW_AIPLAYER  psPalette[40]

#define WZCOL_MAX			41

//*************************************************************************

extern PIELIGHT		psPalette[];

//*************************************************************************

extern void		pal_Init(void);
extern void		pal_ShutDown(void);

static inline PIELIGHT pal_Colour(UBYTE r, UBYTE g, UBYTE b)
{
	PIELIGHT c;

	c.byte.r = r;
	c.byte.g = g;
	c.byte.b = b;
	c.byte.a = UBYTE_MAX;

	return c;
}

static inline PIELIGHT pal_SetBrightness(UBYTE brightness)
{
	PIELIGHT c;

	c.byte.r = brightness;
	c.byte.g = brightness;
	c.byte.b = brightness;
	c.byte.a = UBYTE_MAX;

	return c;
}

#endif
