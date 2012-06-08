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
*/
#ifndef _piePalette_
#define _piePalette_

#include "lib/ivis_opengl/piedef.h"

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
#define WZCOL_MENU_SHADOW			psPalette[41]
#define WZCOL_DBLUE					psPalette[42]
#define WZCOL_LBLUE					psPalette[43]
#define WZCOL_BLUEPRINT_VALID		psPalette[44]
#define WZCOL_BLUEPRINT_INVALID		psPalette[45]
#define WZCOL_BLUEPRINT_PLANNED		psPalette[46]
#define WZCOL_HEALTH_HIGH_SHADOW	psPalette[47]
#define WZCOL_HEALTH_MEDIUM_SHADOW	psPalette[48]
#define WZCOL_HEALTH_LOW_SHADOW		psPalette[49]
#define WZCOL_HEALTH_RESISTANCE		psPalette[50]
#define WZCOL_TEAM1					psPalette[51]
#define WZCOL_TEAM2					psPalette[52]
#define WZCOL_TEAM3					psPalette[53]
#define WZCOL_TEAM4					psPalette[54]
#define WZCOL_TEAM5					psPalette[55]
#define WZCOL_TEAM6					psPalette[56]
#define WZCOL_TEAM7					psPalette[57]
#define WZCOL_TEAM8					psPalette[58]
#define WZCOL_FORM_BACKGROUND				psPalette[59]
#define WZCOL_FORM_TEXT					psPalette[60]
#define WZCOL_FORM_LIGHT				psPalette[61]
#define WZCOL_FORM_DARK					psPalette[62]
#define WZCOL_FORM_HILITE				psPalette[63]
#define WZCOL_FORM_CURSOR				psPalette[64]
#define WZCOL_FORM_TIP_BACKGROUND			psPalette[65]
#define WZCOL_FORM_DISABLE				psPalette[66]
#define WZCOL_DESIGN_POWER_FORM_BACKGROUND		psPalette[67]
#define WZCOL_POWER_BAR					psPalette[68]
#define WZCOL_ACTION_PROGRESS_BAR_MAJOR			psPalette[69]
#define WZCOL_ACTION_PROGRESS_BAR_MINOR			psPalette[70]
#define WZCOL_ACTION_PRODUCTION_RUN_TEXT		psPalette[71]
#define WZCOL_ACTION_PRODUCTION_RUN_BACKGROUND		psPalette[72]
#define WZCOL_LOADING_BAR_BACKGROUND			psPalette[73]
#define WZCOL_MAP_PREVIEW_HQ					psPalette[74]
#define WZCOL_MAP_PREVIEW_OIL					psPalette[75]
#define WZCOL_FOG					psPalette[76]
#define WZCOL_TEAM9                                     psPalette[77]
#define WZCOL_TEAM10                                    psPalette[78]
#define WZCOL_TEAM11                                    psPalette[79]
#define WZCOL_TEAM12                                    psPalette[80]
#define WZCOL_TEAM13                                    psPalette[81]
#define WZCOL_TEAM14                                    psPalette[82]
#define WZCOL_TEAM15                                    psPalette[83]
#define WZCOL_TEAM16                                    psPalette[84]
#define WZCOL_BLUEPRINT_PLANNED_BY_ALLY                 psPalette[85]
#define WZCOL_CONSTRUCTION_BARTEXT                      psPalette[86]
#define WZCOL_POWERQUEUE_BARTEXT                        psPalette[87]
#define WZCOL_MAP_PREVIEW_BARREL                        psPalette[88]

#define WZCOL_MAX                                       89

//*************************************************************************

extern PIELIGHT		psPalette[];

//*************************************************************************

extern void		pal_Init(void);
extern void		pal_ShutDown(void);
extern PIELIGHT		pal_GetTeamColour(int team);

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

static inline PIELIGHT pal_RGBA(UBYTE r, UBYTE g, UBYTE b, UBYTE a)
{
	PIELIGHT c;

	c.byte.r = r;
	c.byte.g = g;
	c.byte.b = b;
	c.byte.a = a;

	return c;
}

static inline void pal_PIELIGHTtoRGBA4f(float *rgba4f, PIELIGHT rgba)
{
	rgba4f[0] = rgba.byte.r / 255.0;
	rgba4f[1] = rgba.byte.g / 255.0;
	rgba4f[2] = rgba.byte.b / 255.0;
	rgba4f[3] = rgba.byte.a / 255.0;
}

#endif
