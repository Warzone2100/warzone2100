/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
/* Simple short file - only because there was nowhere else for it logically to go */
/**
 * @file difficulty.c
 * Handes the difficulty level effects on gameplay.
 */


/*
	Changed to allow seperate modifiers for enemy and player damage.
*/

#include "lib/framework/frame.h"
#include "difficulty.h"
#include "src/multiplay.h"


// ------------------------------------------------------------------------------------

static DIFFICULTY_LEVEL	presDifLevel = DL_NORMAL;
static int              fDifPlayerModifier;
static int              fDifEnemyModifier;


// ------------------------------------------------------------------------------------
/* Sets the game difficulty level */
void	setDifficultyLevel(DIFFICULTY_LEVEL lev)
{

	switch (lev)
	{
	case	DL_EASY:
		fDifPlayerModifier = 120;
		fDifEnemyModifier = 100;
		break;
	case	DL_NORMAL:
		fDifPlayerModifier = 100;
		fDifEnemyModifier = 100;
		break;
	case	DL_HARD:
	case	DL_INSANE:
		fDifPlayerModifier = 80;
		fDifEnemyModifier = 100;
		break;
	case	DL_TOUGH:
		fDifPlayerModifier = 100;
		fDifEnemyModifier = 50;    // they do less damage!
		break;
	case	DL_KILLER:
		fDifPlayerModifier = 999;  // 10 times
		fDifEnemyModifier = 1;     // almost nothing
		break;
	}
	presDifLevel = lev;
}

// ------------------------------------------------------------------------------------
/* Returns the difficulty level */
DIFFICULTY_LEVEL	getDifficultyLevel(void)
{
	return (presDifLevel);
}

// ------------------------------------------------------------------------------------
int modifyForDifficultyLevel(int basicVal, bool IsPlayer)
{
	if (bMultiPlayer && presDifLevel != DL_KILLER && presDifLevel != DL_TOUGH)  // ignore multiplayer or skirmish (unless using biffer baker) games
	{
		return basicVal;
	}
	if (IsPlayer)
	{
		return basicVal * fDifPlayerModifier / 100;
	}
	else
	{
		return basicVal * fDifEnemyModifier / 100;
	}
}
// ------------------------------------------------------------------------------------
