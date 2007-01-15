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
#ifdef DISP2D

		// only needed on PC
/*
 * Edit2D.h
 *
 * Interface to the 2d editing routines.
 */
#ifndef _edit2d_h
#define _edit2d_h

/* Whether to show the terrain type on the map */
extern BOOL showTerrain;

/* Initialise the 2D editing module */
extern BOOL ed2dInitialise(void);

/* ShutDown the 2D editing module */
extern void ed2dShutDown(void);

/* Process input for 2D editing */
extern BOOL ed2dProcessInput(void);

/* Display the editing state */
extern void ed2dDisplay(void);

/* Load in a new map */
extern BOOL ed2dLoadMapFile(void);

/* Save the current map */
extern BOOL ed2dSaveMapFile(void);

/* Write the data to the file */
extern BOOL writeMapFile(char *pFileName);

#endif



#endif
