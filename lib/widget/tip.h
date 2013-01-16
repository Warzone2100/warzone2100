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
/** @file
 *  Interface to the tool tip display module
 */

#ifndef __INCLUDED_LIB_WIDGET_TIP_H__
#define __INCLUDED_LIB_WIDGET_TIP_H__

#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/widgbase.h"

/* Initialise the tool tip module */
extern void tipInitialise(void);

/*
 * Setup a tool tip.
 * The tip module will then wait until the correct points to
 * display and then remove the tool tip.
 * i.e. The tip will not be displayed immediately.
 * Calling this while another tip is being displayed will restart
 * the tip system.
 * psSource is the widget that started the tip.
 * x,y,width,height - specify the position of the button to place the
 * tip by.
 */
extern void tipStart(WIDGET *psSource, const char *pTip, enum iV_fonts NewFontID,
                     PIELIGHT *pColours, SDWORD x, SDWORD y, UDWORD width, UDWORD height);

/* Stop a tool tip (e.g. if the hilite is lost on a button).
 * psSource should be the same as the widget that started the tip.
 */
extern void tipStop(WIDGET *psSource);

/* Update and possibly display the tip */
extern void tipDisplay(void);

#endif // __INCLUDED_LIB_WIDGET_TIP_H__
