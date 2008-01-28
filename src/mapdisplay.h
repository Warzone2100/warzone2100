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
#ifndef _mapdisplay_h
#define _mapdisplay_h

#include "lib/ivis_common/pietypes.h"

/* MapDisplay.h */

#define BUFFER_GRIDX	22
#define BUFFER_GRIDY	22


/*Flag to switch code for bucket sorting in renderFeatures etc
  for the renderMapToBuffer code */
  /*This is no longer used but may be useful for testing so I've left it in - maybe
  get rid of it eventually? - AB 1/4/98*/
extern BOOL	doBucket;
extern BOOL	godMode;

// The surface to render Message information into for the Intelligence Screen
extern iSurface	*mapSurface;

/* renders up to two IMDs into the surface - used by message display in Intelligence Map */
extern void renderIMDToBuffer(iSurface *pSurface, iIMDShape *pIMD,
							  iIMDShape *pIMD2, UDWORD WindowX,UDWORD WindowY,
							  UDWORD OriginX,UDWORD OriginY);
extern void renderResearchToBuffer(RESEARCH *psResearch,
                            UDWORD OriginX, UDWORD OriginY);

#endif // _mapdisplay_h
