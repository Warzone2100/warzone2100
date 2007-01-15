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
 * AIDef.h
 *
 * Structure definitions for the AI system
 *
 */
#ifndef _aidef_h
#define _aidef_h

typedef enum _ai_state
{
	AI_PAUSE,			// do no ai
	AI_ATTACK,			// attacking a target
	AI_MOVETOTARGET,	// moving to a target
	AI_MOVETOLOC,		// moving to a location
	AI_MOVETOSTRUCT,	// moving to a structure to build
	AI_BUILD,			// build a Structure
} AI_STATE;

typedef struct _ai_data
{
	AI_STATE				state;
	struct _base_object		*psTarget;
/*	removed 27.2.98		john.
	struct _weapon			*psSelectedWeapon;
	struct _structure_stats	*psStructToBuild;
	UDWORD					timeStarted;				//time a function was started -eg build, produce
	//UDWORD					pointsToAdd;		NOT USED AT PRESENT 13/08/97
	UDWORD					structX, structY;			// location of StructToBuild
	UDWORD					moveX,moveY;				// location target for movement	
	struct _droid			*psGroup;					// The list pointer for the droids group
	*/
} AI_DATA;


#endif

