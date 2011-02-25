/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_SELECTION_H__
#define __INCLUDED_SRC_SELECTION_H__

enum SELECTION_CLASS
{
DS_ALL_UNITS,
DS_BY_TYPE
};

enum SELECTIONTYPE
{
DST_UNUSED,
DST_VTOL,
DST_HOVER,
DST_WHEELED,
DST_TRACKED,
DST_HALF_TRACKED,
DST_ALL_COMBAT,
DST_ALL_DAMAGED,
DST_ALL_SAME
};

// EXTERNALLY REFERENCED FUNCTIONS
extern UDWORD	selDroidSelection( UDWORD	player, SELECTION_CLASS droidClass,
						  SELECTIONTYPE droidType, BOOL bOnScreen );
extern UDWORD	selDroidDeselect		( UDWORD player );
extern UDWORD	selNumSelected			( UDWORD player );
extern void	selNextRepairUnit			( void );
extern void selNextUnassignedUnit		( void );
extern void	selNextSpecifiedBuilding	( UDWORD structType );
extern	void	selNextSpecifiedUnit	(UDWORD unitType);
// select the n'th command droid
extern void selCommander(SDWORD n);

#endif // __INCLUDED_SRC_SELECTION_H__
