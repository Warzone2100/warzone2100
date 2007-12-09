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
//*************************************************************************

#define PALETTE_MAX	8

#define PALETTE_SIZE	256
#define PALETTE_SHADE_LEVEL 16

#define COL_TRANS			0
#define COL_BLACK			colours[0]
#define COL_BLUE			colours[1]
#define COL_GREEN			colours[2]
#define COL_CYAN			colours[3]
#define COL_RED				colours[4]
#define COL_MAGENTA  		colours[5]
#define COL_BROWN			colours[6]
#define COL_GREY			colours[7]
#define COL_DARKGREY		colours[8]
#define COL_LIGHTBLUE		colours[9]
#define COL_LIGHTGREEN		colours[10]
#define COL_LIGHTCYAN		colours[11]
#define COL_LIGHTRED		colours[12]
#define COL_LIGHTMAGENTA	colours[13]
#define COL_YELLOW			colours[14]
#define COL_WHITE			colours[15]

#define WZCOL_BLACK			psPalette[0]
#define WZCOL_WHITE			psPalette[1]
#define WZCOL_RELOAD_BACKGROUND		psPalette[2]
#define WZCOL_RELOAD_BAR		psPalette[3]
#define WZCOL_HEALTH_HIGH		psPalette[4]
#define WZCOL_HEALTH_MEDIUM		psPalette[5]
#define WZCOL_HEALTH_LOW		psPalette[6]
#define WZCOL_GREEN			psPalette[7]
#define WZCOL_RED			psPalette[8]
#define WZCOL_YELLOW			psPalette[9]
#define WZCOL_MENU_BACKGROUND		psPalette[10]
#define WZCOL_MENU_BORDER		psPalette[11]
#define WZCOL_MENU_LOAD_BORDER		psPalette[12]
#define WZCOL_CURSOR			psPalette[13]
#define WZCOL_MENU_SCORES_INTERIOR	psPalette[14]
#define WZCOL_MENU_SEPARATOR		psPalette[15]
#define WZCOL_MAX			16

//*************************************************************************

extern Uint8		colours[];
extern Uint8		palShades[PALETTE_SIZE * PALETTE_SHADE_LEVEL];
extern PIELIGHT		psPalette[];

//*************************************************************************
extern void		pal_Init(void);
extern void		pal_ShutDown(void);
extern void		pal_BuildAdjustedShadeTable( void );
extern Uint8	pal_GetNearestColour(Uint8 r, Uint8 g, Uint8 b);
extern int		pal_AddNewPalette(PIELIGHT *pal);
extern void		pal_PaletteSet(void);
extern PIELIGHT		*pie_GetGamePal(void);
extern PIELIGHT		pal_SetBrightness(UBYTE brightness);

#endif
