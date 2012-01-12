/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
/***************************************************************************/
/*
 * Anim.c
 *
 * Anim functions
 *
 * Gareth Jones 11/7/97
 *
 */
/***************************************************************************/

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/frameresource.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/framework/fixedpoint.h"

#include "anim.h"
#include "parser.h"
#include "lib/framework/listmacs.h"

#define INT_SCALE       1000

/***************************************************************************/
/* structs */


/***************************************************************************/
/* global variables */

static struct
{
        BASEANIM                *psAnimList;
        UWORD                   uwCurObj;
	UWORD			uwCurState;
}
g_animGlobals;

/***************************************************************************/
/*
 * Anim functions
 */
/***************************************************************************/

/**
 *	Initialise animation subsystem.
 */
bool anim_Init()
{
	/* init globals */
	g_animGlobals.psAnimList    = NULL;
	g_animGlobals.uwCurObj      = 0;
	g_animGlobals.uwCurState    = 0;

	return true;
}

/***************************************************************************/

/**
 *	Stop and release given animation.
 */
void anim_ReleaseAnim(BASEANIM *psAnim)
{
	// remove the anim from the list
	if (g_animGlobals.psAnimList)
	{
		LIST_REMOVE(g_animGlobals.psAnimList, psAnim, BASEANIM);
	}
	/* free anim scripts */
	free( psAnim->psStates );

	/* free anim shape */
	if (psAnim->animType == ANIM_3D_FRAMES || psAnim->animType == ANIM_3D_TRANS)
	{
		ANIM3D	*psAnim3D = (ANIM3D *)psAnim;

		free( psAnim3D->apFrame );
	}

	free(psAnim);
}

/**
 *	Shut down animation subsystem.
 */
bool anim_Shutdown()
{
	BASEANIM	*psAnim, *psAnimTmp;

	if (g_animGlobals.psAnimList != NULL)
	{
		debug(LOG_ERROR, "anim_Shutdown: warning: anims still allocated");
	}

	/* empty anim list */
	psAnim = g_animGlobals.psAnimList;
	while ( psAnim != NULL )
	{
		psAnimTmp = psAnim->psNext;
		anim_ReleaseAnim( psAnim );
		psAnim = psAnimTmp;
	}

	return true;
}

static void anim_InitBaseMembers(BASEANIM * psAnim, UWORD uwStates, UWORD uwFrameRate,
                                 UWORD uwObj, ANIM_MODE ubType, UWORD uwID)
{
	psAnim->uwStates    = uwStates;
	psAnim->uwFrameRate = uwFrameRate;
	psAnim->uwObj       = uwObj;
	psAnim->ubType      = ubType;
	psAnim->uwID        = uwID;
	psAnim->uwAnimTime  = (UWORD) (uwStates*1000 / psAnim->uwFrameRate);

	/* allocate frames */
	psAnim->psStates = (ANIM_STATE*)malloc( uwObj*psAnim->uwStates*sizeof(ANIM_STATE) );
}

/**
 *	Create animation for a model. Called from animation script.
 */
bool anim_Create3D(char szPieFileName[], UWORD uwStates, UWORD uwFrameRate, UWORD uwObj, 
                   ANIM_MODE ubType, UWORD uwID)
{
	ANIM3D		*psAnim3D;
	iIMDShape	*psFrames;
	UWORD		uwFrames, i;

	/* allocate anim */
	if ( (psAnim3D = (ANIM3D*)malloc(sizeof(ANIM3D))) == NULL )
	{
		return false;
	}

	/* get local pointer to shape */
	psAnim3D->psFrames = (iIMDShape*)resGetData("IMD", szPieFileName);

	/* count frames in imd */
	psFrames = psAnim3D->psFrames;
	uwFrames = 0;
	while ( psFrames != NULL )
	{
		uwFrames++;
		psFrames = psFrames->next;
	}

	/* check frame count matches script */
	if ( ubType == ANIM_3D_TRANS && uwObj != uwFrames )
	{
		ASSERT(false, "Frames in pie %s != script objects %i", szPieFileName, uwObj);
		free(psAnim3D);
		return false;
	}

	/* get pointers to individual frames */
	psAnim3D->apFrame = (iIMDShape**)malloc( uwFrames*sizeof(iIMDShape *) );
	psFrames = psAnim3D->psFrames;
	for ( i=0; i<uwFrames; i++ )
	{
		psAnim3D->apFrame[i] = psFrames;
		psFrames = psFrames->next;
	}

	/* init members */
	psAnim3D->animType = ubType;
	anim_InitBaseMembers( (BASEANIM * ) psAnim3D, uwStates,
							uwFrameRate, uwObj, ubType, uwID );

	/* add to head of list */
	psAnim3D->psNext = g_animGlobals.psAnimList;
	g_animGlobals.psAnimList = (BASEANIM * ) psAnim3D;

	/* update globals */
	g_animGlobals.uwCurObj = 0;

	return true;
}

/***************************************************************************/

void anim_BeginScript()
{
	/* update globals */
	g_animGlobals.uwCurState = 0;
}

/***************************************************************************/

bool anim_EndScript()
{
	BASEANIM	*psAnim;

	/* get pointer to current anim */
	psAnim = g_animGlobals.psAnimList;

	if ( g_animGlobals.uwCurState != psAnim->uwStates )
	{
		ASSERT(false, "States in current anim not consistent with header");
		return false;
	}

	/* update globals */
	g_animGlobals.uwCurObj++;

	return true;
}

/***************************************************************************/

bool anim_AddFrameToAnim(int iFrame, Vector3i vecPos, Vector3i vecRot, Vector3i vecScale)
{
	ANIM_STATE	*psState;
	BASEANIM	*psAnim;
	UWORD		uwState;

	/* get pointer to current anim */
	psAnim = g_animGlobals.psAnimList;

	/* check current anim valid */
	ASSERT( psAnim != NULL, "anim_AddFrameToAnim: NULL current anim\n" );

	/* check frame number in range */
	ASSERT( iFrame<psAnim->uwStates,
			"anim_AddFrameToAnim: frame number %i > %i frames in imd\n",
			iFrame, psAnim->uwObj );

	/* get state */
	uwState = (g_animGlobals.uwCurObj * psAnim->uwStates) +
				g_animGlobals.uwCurState;
	psState = &psAnim->psStates[uwState];

	/* set state pointer */
	psState->uwFrame = (UWORD) iFrame;

	psState->vecPos.x = vecPos.x;
	psState->vecPos.y = vecPos.y;
	psState->vecPos.z = vecPos.z;

	/* max anims right-handed; Necromancer anims left */
	psState->vecAngle.x  = vecRot.x;
	psState->vecAngle.y  = vecRot.y;
	psState->vecAngle.z  = vecRot.z;

	psState->vecScale.x  = vecScale.x;
	psState->vecScale.y  = vecScale.y;
	psState->vecScale.z  = vecScale.z;

	/* update globals */
	g_animGlobals.uwCurState++;

	return true;
}

/***************************************************************************/

BASEANIM *anim_GetAnim(UWORD uwAnimID)
{
	BASEANIM	*psAnim;

	/* find matching anim id in list */
	psAnim = g_animGlobals.psAnimList;
	while( psAnim != NULL && psAnim->uwID != uwAnimID )
	{
		psAnim = psAnim->psNext;
	}

	return psAnim;
}

/***************************************************************************/

void anim_SetVals(char szFileName[], UWORD uwAnimID)
{
	/* get track pointer from resource */
	BASEANIM	*psAnim = (BASEANIM*)resGetData( "ANI", szFileName );

	if ( psAnim == NULL )
	{
		ASSERT(false, "Can't find anim %s", szFileName);
		return ;
	}

	/* set anim vals */
	psAnim->uwID = uwAnimID;
	sstrcpy(psAnim->szFileName, szFileName);
}

/***************************************************************************/

BASEANIM *anim_LoadFromFile(PHYSFS_file* fileHandle)
{
	if ( ParseResourceFile( fileHandle ) == false )
	{
		ASSERT(false, "Couldn't parse file");
		return NULL;
	}

	/* loaded anim is at head of list */
	return g_animGlobals.psAnimList;
}

/***************************************************************************/

UWORD anim_GetAnimID(char *szName)
{
	BASEANIM	*psAnim;
	char		*cPos = strstr( szName, ".ani" );

	if ( cPos == NULL )
	{
		ASSERT(false, "%s isn't .ani file", szName);
		return NO_ANIM;
	}

	/* find matching anim string in list */
	psAnim = g_animGlobals.psAnimList;
	while( psAnim != NULL && strcasecmp( psAnim->szFileName, szName ) != 0 )
	{
		psAnim = psAnim->psNext;
	}

	if ( psAnim != NULL )
	{
		return psAnim->uwID;
	}
	else
	{
		return NO_ANIM;
	}
}

/***************************************************************************/

iIMDShape *anim_GetShapeFromID(UWORD uwID)
{
	BASEANIM	*psAnim = g_animGlobals.psAnimList;

	/* find matching anim id in list */
	while( psAnim != NULL && psAnim->uwID != uwID )
	{
		psAnim = psAnim->psNext;
	}

	if ( psAnim == NULL )
	{
		return NULL;
	}
	else
	{
		ANIM3D	*psAnim3D = (ANIM3D *)psAnim;

		ASSERT(psAnim3D != NULL, "anim_GetShapeFromID: invalid anim pointer");

		return psAnim3D->psFrames;
	}
}

/***************************************************************************/

UWORD anim_GetFrame3D(ANIM3D *psAnim, UWORD uwObj, UDWORD udwGraphicsTime, UDWORD udwStartTime,
                      UDWORD udwStartDelay, Vector3i *psVecPos, Vector3i *psVecRot, Vector3i *psVecScale)
{
	SDWORD		dwTime;
	UWORD		uwState, uwFrame;
	ANIM_STATE	*psState;

	/* calculate current anim frame */
	dwTime = udwGraphicsTime - udwStartTime - udwStartDelay;

	/* return NULL if animation still delayed */
	if ( dwTime < 0 )
	{
		return ANIM_DELAYED;
	}

	uwFrame = (dwTime % psAnim->uwAnimTime) * psAnim->uwFrameRate / 1000;

	/* check in range */
	ASSERT(uwFrame<psAnim->uwStates, "anim_GetObjectFrame3D: error in animation calculation");

	/* find current state */
	uwState = (uwObj * psAnim->uwStates) + uwFrame;
	psState = &psAnim->psStates[uwState];

	psVecPos->x   = psState->vecPos.x / INT_SCALE;
	psVecPos->y   = psState->vecPos.y / INT_SCALE;
	psVecPos->z   = psState->vecPos.z / INT_SCALE;

	psVecRot->x   = psState->vecAngle.x * DEG_1 / INT_SCALE;
	psVecRot->y   = psState->vecAngle.y * DEG_1 / INT_SCALE;
	psVecRot->z   = psState->vecAngle.z * DEG_1 / INT_SCALE;

	psVecScale->x = psState->vecScale.x;
	psVecScale->y = psState->vecScale.y;
	psVecScale->z = psState->vecScale.z;

	if ( psAnim->ubType == ANIM_3D_TRANS )
	{
		return uwFrame;
	}
	else
	{
		return psAnim->psStates[uwState].uwFrame;
	}
}

/***************************************************************************/
