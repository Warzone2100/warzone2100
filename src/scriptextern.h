/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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
 *  All game variable access functions for the scripts
 */

#ifndef __INCLUDED_SRC_SCRIPTEXTERN_H__
#define __INCLUDED_SRC_SCRIPTEXTERN_H__

// current game level
extern SDWORD	scrGameLevel;
// whether the tutorial is active
extern bool		bInTutorial;
// whether any additional special case victory/failure conditions have been met
extern bool		bExtraVictoryFlag;
extern bool		bExtraFailFlag;
extern bool		bTrackTransporter;

// ID numbers for external variables
enum _externids
{
	EXTID_MAPWIDTH,
	EXTID_MAPHEIGHT,
	EXTID_GAMEINIT,
	EXTID_SELECTEDPLAYER,
	EXTID_GAMELEVEL,
	EXTID_GAMETIME,
	EXTID_TUTORIAL,
	EXTID_MULTIGAMETYPE,
	EXTID_MULTIGAMEHUMANMAX,
	EXTID_MULTIGAMEBASETYPE,
	EXTID_CURSOR,
	EXTID_INTMODE,
	EXTID_TARGETTYPE,				// IHATESCRIPTSANDEVERYTHINGTHEYSTANDFOR(ESPECIALLYONSUNDAYS)
	EXTID_EXTRAVICTORYFLAG,
	EXTID_EXTRAFAILFLAG,
	EXTID_TRACKTRANSPORTER,
	EXTID_MULTIGAMEALLIANCESTYPE,
};

// reset the script externals for a new level
void scrExternReset();

// General function to get some basic game values
bool scrGenExternGet(UDWORD index);

// General function to set some basic game values
bool scrGenExternSet(UDWORD index);

#endif // __INCLUDED_SRC_SCRIPTEXTERN_H__
