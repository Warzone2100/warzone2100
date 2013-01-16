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

#ifndef __INCLUDED_SRC_SCORES_H__
#define __INCLUDED_SRC_SCORES_H__

enum DATA_INDEX
{
WD_UNITS_BUILT,
WD_UNITS_KILLED,
WD_UNITS_LOST,
WD_STR_BUILT,
WD_STR_KILLED,
WD_STR_LOST,
WD_ARTEFACTS_FOUND,
WD_MISSION_STARTED,
WD_SHOTS_ON_TARGET,
WD_SHOTS_OFF_TARGET,
WD_BARBARIANS_MOWED_DOWN
};

// --------------------------------------------------------------------
/* The mission results data */
struct MISSION_DATA
{
	uint32_t    unitsBuilt;		// How many units were built
	uint32_t    unitsKilled;	// How many enemy units you blew up
	uint32_t    unitsLost;		// How many units were lost
	uint32_t    strBuilt;		// How many structures we built
	uint32_t    strKilled;		// How many enemy structures you blew up
	uint32_t    strLost;		// How many structures were lost
	uint32_t    artefactsFound;	// How many artefacts were found
	uint32_t    missionStarted;	// When was the mission started
	uint32_t    shotsOnTarget;	// How many hits
	uint32_t    shotsOffTarget;	// How many misses
	uint32_t    babasMowedDown; // How many barbarians did we mow down?
};
extern MISSION_DATA	missionData;
// Could use widgets, but hey.....
struct STAT_BAR
{
UDWORD	topX,topY;		// Obvious
UDWORD	width,height;	// Height down screen and width _unfilled_
UDWORD	percent;		// What percentage full is it?
UDWORD	stringID;		// String resource name to stick next to it.
UDWORD	queTime;		// How many game ticks before it's active?
bool	bQueued;		// Already fired off?
bool	bActive;		// Is this one active?
UDWORD	number;			// %d string for the associated text string.
};

enum
{
STAT_UNIT_LOST,
STAT_UNIT_KILLED,
STAT_STR_LOST,
STAT_STR_BLOWN_UP,
STAT_UNITS_BUILT,
STAT_UNITS_NOW,
STAT_STR_BUILT,
STAT_STR_NOW,
STAT_ROOKIE,
STAT_GREEN,
STAT_TRAINED,
STAT_REGULAR,
STAT_VETERAN,
STAT_CRACK,
STAT_ELITE,
STAT_SPECIAL,
STAT_ACE
};



extern bool	scoreInitSystem			( void );
extern void	scoreUpdateVar			( DATA_INDEX var );
extern void	scoreDataToConsole		( void );
extern void	scoreDataToScreen		( void );
void getAsciiTime(char *psText, unsigned time);
extern bool readScoreData			( const char* fileName );
extern bool writeScoreData			( const char* fileName );

#endif // __INCLUDED_SRC_SCORES_H__
