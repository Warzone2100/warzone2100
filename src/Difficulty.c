// ------------------------------------------------------------------------------------
/* Simple short file - only because there was nowhere else for it logically to go */
/* Handes the difficulty level effects on gameplay */


/*
	Changed to allow seperate modifiers for enemy and player damage.
*/


// ------------------------------------------------------------------------------------
#include "Frame.h"
#include "Difficulty.h"
// ------------------------------------------------------------------------------------
DIFFICULTY_LEVEL	presDifLevel = DL_NORMAL;
FRACT				fDifPlayerModifier;
FRACT				fDifEnemyModifier;


void getModifiers(FRACT *Player,FRACT *Enemy)
{
	*Player = fDifPlayerModifier;
	*Enemy = fDifEnemyModifier;
}



void setModifiers(FRACT Player,FRACT Enemy)
{
	fDifPlayerModifier = Player;
	fDifEnemyModifier = Enemy;
}


// ------------------------------------------------------------------------------------
/* Sets the game difficulty level */
void	setDifficultyLevel(DIFFICULTY_LEVEL lev)
{

#ifdef WIN32
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
		DBERROR(("Invalid difficulty level selected - forcing NORMAL"));
		break;
	}
#else
	switch(lev)
	{
	case	DL_EASY:
		fDifPlayerModifier = FRACTCONST(190,100);
		fDifEnemyModifier = FRACTCONST(100,115);
		break;
	case	DL_NORMAL:
		fDifPlayerModifier = FRACTCONST(130,100);
		fDifEnemyModifier = FRACTCONST(100,115);
		break;
	case	DL_HARD:
		fDifPlayerModifier = FRACTCONST(110,100);	// was (125,100)
		fDifEnemyModifier = FRACTCONST(100,115);
		break;
//	case	DL_EASY:
//		fDifPlayerModifier = FRACTCONST(150,100);
//		fDifEnemyModifier = FRACTCONST(100,115);
//		break;
//	case	DL_NORMAL:
//		fDifPlayerModifier = FRACTCONST(110,100);	// was (125,100)
//		fDifEnemyModifier = FRACTCONST(100,115);
//		break;
//	case	DL_HARD:
//		fDifPlayerModifier = FRACTCONST(100,100);
//		fDifEnemyModifier = FRACTCONST(100,100);
//		break;
	default:
		DBERROR(("Invalid difficulty level selected - forcing NORMAL"));
		break;
	}
#endif

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
