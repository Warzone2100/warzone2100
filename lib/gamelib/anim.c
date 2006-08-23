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
#include "lib/ivis_common/geo.h"

#include "anim.h"
#include "parser.h"

/***************************************************************************/
/* structs */


/***************************************************************************/
/* global variables */

ANIMGLOBALS	g_animGlobals;

/***************************************************************************/
/* local functions */

//static UINT	anim_HashFunction( int iKey1, int iKey2 );

/***************************************************************************/
/*
 * Anim functions
 */
/***************************************************************************/

BOOL
anim_Init( GETSHAPEFUNC pGetShapeFunc )
{
	int		iSizeAnim2D = sizeof(ANIM2D),
			iSizeAnim3D = sizeof(ANIM3D);

	/* ensure ANIM2D and ANIM3D structs same size */
	if ( iSizeAnim2D != iSizeAnim3D )
	{
		debug( LOG_ERROR, "anim_Init: ANIM2D and ANIM3D structs not same size in anim.h!" );
		abort();
	}

	/* init globals */
	g_animGlobals.psAnimList    = NULL;
	g_animGlobals.uwCurObj      = 0;
	g_animGlobals.uwCurState    = 0;
	g_animGlobals.pGetShapeFunc = pGetShapeFunc;

	return TRUE;
}

/***************************************************************************/

void
anim_ReleaseAnim( BASEANIM *psAnim )
{
	ANIM3D		*psAnim3D;

	// remove the anim from the list
	LIST_REMOVE(g_animGlobals.psAnimList, psAnim, BASEANIM);

	/* free anim scripts */
	FREE( psAnim->psStates );

	/* free anim shape */
	if ( psAnim->animType == ANIM_3D_FRAMES ||
		 psAnim->animType == ANIM_3D_TRANS     )
	{
		psAnim3D = (ANIM3D *) psAnim;
		FREE( psAnim3D->apFrame );
	}

	FREE(psAnim);
}

/***************************************************************************/

BOOL
anim_Shutdown( void )
{
	BASEANIM	*psAnim, *psAnimTmp;

	if (g_animGlobals.psAnimList != NULL)
	{
		debug( LOG_NEVER, "anim_Shutdown: warning: anims still allocated" );
	}

	/* empty anim list */
	psAnim = g_animGlobals.psAnimList;
	while ( psAnim != NULL )
	{
		psAnimTmp = psAnim->psNext;
		anim_ReleaseAnim( psAnim );
		psAnim = psAnimTmp;
	}

	return TRUE;
}

/***************************************************************************/

static void
anim_InitBaseMembers( BASEANIM * psAnim, UWORD uwStates, UWORD uwFrameRate,
						UWORD uwObj, UBYTE ubType, UWORD uwID )
{
	psAnim->uwStates    = uwStates;
	psAnim->uwFrameRate = uwFrameRate;
	psAnim->uwObj       = uwObj;
	psAnim->ubType      = ubType;
	psAnim->uwID        = uwID;
	psAnim->uwAnimTime  = (UWORD) (uwStates*1000 / psAnim->uwFrameRate);

	/* allocate frames */
	psAnim->psStates = MALLOC( uwObj*psAnim->uwStates*sizeof(ANIM_STATE) );
}

/***************************************************************************/

BOOL
anim_Create3D( char szPieFileName[], UWORD uwStates,
				UWORD uwFrameRate, UWORD uwObj, UBYTE ubType, UWORD uwID )
{
	ANIM3D		*psAnim3D;
	iIMDShape	*psFrames;
	UWORD		uwFrames, i;

	/* allocate anim */
	if ( (psAnim3D = MALLOC(sizeof(ANIM3D))) == NULL )
	{
		return FALSE;
	}

	/* get local pointer to shape */
	psAnim3D->psFrames = (g_animGlobals.pGetShapeFunc) (szPieFileName);

	/* count frames in imd */
	psFrames = psAnim3D->psFrames;
	uwFrames = 0;
	while ( psFrames != NULL )
	{
#ifdef DEBUG
		/* Where does this constant come from, though? - Per */
		if ((UDWORD)psFrames==0xcdcdcdcd)
		{
			printf("bad pointer in Create 3D !!!!  -[%s]\n", szPieFileName);
		}
#endif
		uwFrames++;
		psFrames = psFrames->next;
	}

	/* check frame count matches script */
	if ( ubType == ANIM_3D_TRANS && uwObj != uwFrames )
	{
		debug( LOG_ERROR, "anim_Create3D: frames in pie %s != script objects %i\n",
					szPieFileName, uwObj );
		abort();
		return FALSE;
	}

	/* get pointers to individual frames */
	psAnim3D->apFrame = MALLOC( uwFrames*sizeof(iIMDShape *) );
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

	return TRUE;
}

/***************************************************************************/

void
anim_BeginScript( void )
{
	/* update globals */
	g_animGlobals.uwCurState = 0;
}

/***************************************************************************/

BOOL
anim_EndScript( void )
{
	BASEANIM	*psAnim;

	/* get pointer to current anim */
	psAnim = g_animGlobals.psAnimList;

	if ( g_animGlobals.uwCurState != psAnim->uwStates )
	{
		debug( LOG_ERROR, "anim_End3D: states in current anim not consistent with header\n" );
		abort();
		return FALSE;
	}

	/* update globals */
	g_animGlobals.uwCurObj++;

	return TRUE;
}

/***************************************************************************/

BOOL
anim_AddFrameToAnim( int iFrame, VECTOR3D vecPos, VECTOR3D vecRot,
						VECTOR3D vecScale )
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

	return TRUE;
}

/***************************************************************************/

BASEANIM *
anim_GetAnim( UWORD uwAnimID )
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

void
anim_SetVals( char szFileName[], UWORD uwAnimID )
{
	/* get track pointer from resource */
	BASEANIM	*psAnim = resGetData( "ANI", szFileName );

	if ( psAnim == NULL )
	{
		debug( LOG_ERROR, "anim_SetVals: can't find anim %s\n", szFileName );
		abort();
		return ;
	}

	/* set anim vals */
	psAnim->uwID = uwAnimID;
	strcpy( psAnim->szFileName, szFileName );
}

/***************************************************************************/

// the playstation version uses sscanf's ... see animload.c
BASEANIM *anim_LoadFromBuffer(char *pBuffer, UDWORD size)
{
	if ( ParseResourceFile( pBuffer, size ) == FALSE )
	{
		debug( LOG_ERROR, "anim_LoadFromBuffer: couldn't parse file\n" );
		abort();
		return NULL;
	}

	/* loaded anim is at head of list */
	return g_animGlobals.psAnimList;
}

/***************************************************************************/

UWORD
anim_GetAnimID( char *szName )
{
	BASEANIM	*psAnim;
	char		*cPos = strstr( szName, ".ani" );

	if ( cPos == NULL )
	{
		debug( LOG_ERROR, "anim_GetAnimID: %s isn't .ani file\n", szName );
		abort();
		return NO_ANIM;
	}

	/* find matching anim string in list */
	psAnim = g_animGlobals.psAnimList;
	while( psAnim != NULL &&
			stricmp( psAnim->szFileName, szName ) != 0 )
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

iIMDShape *
anim_GetShapeFromID( UWORD uwID )
{
	BASEANIM	*psAnim;
	ANIM3D		*psAnim3D;

	/* find matching anim id in list */
	psAnim = g_animGlobals.psAnimList;
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
		psAnim3D = (ANIM3D *) psAnim;

		ASSERT( PTRVALID( psAnim3D, sizeof(ANIM3D)),
				"anim_GetShapeFromID: invalid anim pointer\n" );

		return psAnim3D->psFrames;
	}
}

/***************************************************************************/

UWORD
anim_GetFrame3D( ANIM3D *psAnim, UWORD uwObj, UDWORD udwGameTime,
			UDWORD udwStartTime, UDWORD udwStartDelay,
			VECTOR3D *psVecPos, VECTOR3D *psVecRot, VECTOR3D *psVecScale )
{
	DWORD		dwTime;
	UWORD		uwState, uwFrame;
	ANIM_STATE	*psState;

	/* calculate current anim frame */
	dwTime = udwGameTime - udwStartTime - udwStartDelay;

	/* return NULL if animation still delayed */
	if ( dwTime < 0 )
	{
		return ANIM_DELAYED;
	}

	uwFrame = (UWORD) ((dwTime % psAnim->uwAnimTime) *
							psAnim->uwFrameRate / 1000);

	/* check in range */
	ASSERT( uwFrame<psAnim->uwStates,
			"anim_GetObjectFrame3D: error in animation calculation\n" );

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
