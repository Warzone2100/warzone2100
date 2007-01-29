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
// ------------------------------------------------------------------------------------
/* Simple short file - only because there was nowhere else for it logically to go */
/* Handes the difficulty level effects on gameplay */


/*
	Changed to allow seperate modifiers for enemy and player damage.
*/


// ------------------------------------------------------------------------------------
#include "lib/framework/frame.h"
#include "difficulty.h"
// ------------------------------------------------------------------------------------
DIFFICULTY_LEVEL	presDifLevel = DL_NORMAL;
FRACT				fDifPlayerModifier;
FRACT				fDifEnemyModifier;


void setModifiers(FRACT Player,FRACT Enemy)
{
	fDifPlayerModifier = Player;
	fDifEnemyModifier = Enemy;
}


// ------------------------------------------------------------------------------------
/* Sets the game difficulty level */
void	setDifficultyLevel(DIFFICULTY_LEVEL lev)
{

	switch(lev)
	{
	case	DL_EASY:
		fDifPlayerModifier = FRACTCONST(120,100);
		fDifEnemyModifier = FRACTCONST(100,100);
		break;
	case	DL_NORMAL:
		fDifPlayerModifier = FRACTCONST(100,100);
		fDifEnemyModifier = FRACTCONST(100,100);
		break;
	case	DL_HARD:
		fDifPlayerModifier = FRACTCONST(80,100);
		fDifEnemyModifier = FRACTCONST(100,100);
		break;
	case	DL_KILLER:
		fDifPlayerModifier = FRACTCONST(999,100);	// 10 times
		fDifEnemyModifier = FRACTCONST(1,100);		// almost nothing
		break;
	case	DL_TOUGH:
		fDifPlayerModifier = FRACTCONST(100,100);
		fDifEnemyModifier = FRACTCONST(50,100);	// they do less damage!
		break;
	default:
		debug( LOG_ERROR, "Invalid difficulty level selected - forcing NORMAL" );
		abort();
		break;
	}

	presDifLevel = lev;
}

// ------------------------------------------------------------------------------------
/* Returns the difficulty level */
DIFFICULTY_LEVEL	getDifficultyLevel( void )
{
	return(presDifLevel);
}

// ------------------------------------------------------------------------------------
SDWORD	modifyForDifficultyLevel(SDWORD basicVal,BOOL IsPlayer)
{
	SDWORD	retVal;

// You can't garantee that we don't want damage modifiers in normal difficulty.
//	/* Unmodified! */
//	if(getDifficultyLevel() == DL_NORMAL)
//	{
//		return(basicVal);
//	}

//	retVal = basicVal*fDifModifier;
	if(IsPlayer) {
		retVal = (SDWORD)ROUND(basicVal*fDifPlayerModifier);
	} else {
		retVal = (SDWORD)ROUND(basicVal*fDifEnemyModifier);
	}

//	DBPRINTF(("%d : %d %d\n",IsPlayer,basicVal,retVal));

	return retVal;
}
// ------------------------------------------------------------------------------------
