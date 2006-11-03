/***************************************************************************/
/*
 * Aud.c
 *
 * Warzone audio wrapper functions
 *
 * Gareth Jones 16/12/97
 */
/***************************************************************************/

#include "lib/framework/frame.h"
#include "base.h"
#include "map.h"
#include "disp2d.h"
#include "display3d.h"
#include "lib/ivis_common/piedef.h"
#include "lib/gamelib/gtime.h"

#include "cluster.h"
#include "lib/sound/aud.h"
#include "audio_id.h"

/***************************************************************************/

extern BOOL		display3D;
extern UDWORD	mapX;
extern UDWORD	mapY;
extern iView	player;
extern UDWORD	distance;

/* Map Position of top right hand corner of the screen */
extern UDWORD	viewX;
extern UDWORD	viewY;

/***************************************************************************/

BOOL
audio_ObjectDead( void * psObj )
{
	SIMPLE_OBJECT	*psSimpleObj = (SIMPLE_OBJECT *) psObj;
	BASE_OBJECT		*psBaseObj;
	PROJ_OBJECT		*psProj;

	/* check is valid simple object pointer */
	if ( !PTRVALID(psSimpleObj, sizeof(SIMPLE_OBJECT)) )
	{
		debug( LOG_NEVER, "audio_ObjectDead: simple object pointer invalid\n" );
		return TRUE;
	}

	/* check projectiles */
	if ( psSimpleObj->type == OBJ_BULLET )
	{
		psProj = (PROJ_OBJECT *) psSimpleObj;
		if ( !PTRVALID(psProj, sizeof(PROJ_OBJECT)) )
		{
			debug( LOG_NEVER, "audio_ObjectDead: projectile object pointer invalid\n" );
			return TRUE;
		}
		else
		{
			if ( psProj->state == PROJ_POSTIMPACT )
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}
	else
	{
		/* check base object */
		psBaseObj = (BASE_OBJECT *) psObj;

		/* check is valid pointer */
		if ( !PTRVALID(psBaseObj, sizeof(BASE_OBJECT)) )
		{
			debug( LOG_NEVER, "audio_ObjectDead: base object pointer invalid\n" );
			return TRUE;
		}
		else
		{
			return psBaseObj->died;
		}
	}
}

/***************************************************************************/

void
audio_Get2DPlayerPos( SDWORD *piX, SDWORD *piY, SDWORD *piZ )
{
	*piX = mapX << TILE_SHIFT;
	*piY = mapY << TILE_SHIFT;
	*piZ = 0;
}

/***************************************************************************/

void
audio_Get3DPlayerPos( SDWORD *piX, SDWORD *piY, SDWORD *piZ )
{
	/* player's y and z interchanged */
	*piX = player.p.x + ((visibleXTiles/2) << TILE_SHIFT);
	*piY = player.p.z + ((visibleYTiles/2) << TILE_SHIFT);
	*piZ = player.p.y;

	/* invert y to match QSOUND axes */
	*piY = (GetHeightOfMap() << TILE_SHIFT) - *piY;
}

/***************************************************************************/
/*
 * get player direction vector - always 0 in 2D
 */
/***************************************************************************/

void
audio_Get2DPlayerRotAboutVerticalAxis( SDWORD *piA )
{
	*piA = (SWORD) 0;
}

/***************************************************************************/
/*
 * get player direction vector - angle about vertical (y) ivis axis
 */
/***************************************************************************/

void
audio_Get3DPlayerRotAboutVerticalAxis( SDWORD *piA )
{
	*piA = player.r.y / DEG_1;
}

/***************************************************************************/

BOOL
audio_Display3D( void )
{
	return display3D;
}

/***************************************************************************/
/*
 * audio_GetStaticPos
 *
 * Get QSound axial position from world (x,y)
 */
/***************************************************************************/

void
audio_GetStaticPos( SDWORD iWorldX, SDWORD iWorldY,
					SDWORD *piX, SDWORD *piY, SDWORD *piZ )
{
	*piX = iWorldX;
	*piZ = map_TileHeight( iWorldX >> TILE_SHIFT,
						   iWorldY >> TILE_SHIFT );
	/* invert y to match QSOUND axes */
	*piY = (GetHeightOfMap() << TILE_SHIFT) - iWorldY;
}

/***************************************************************************/

void
audio_GetObjectPos( void *psObj, SDWORD *piX, SDWORD *piY, SDWORD *piZ )
{
	BASE_OBJECT	*psBaseObj = (BASE_OBJECT *) psObj;

	/* check is valid pointer */
	ASSERT( PTRVALID(psBaseObj, sizeof(BASE_OBJECT)),
			"audio_GetObjectPos: game object pointer invalid\n" );

	*piX = psBaseObj->x;
	*piZ = map_TileHeight( psBaseObj->x >> TILE_SHIFT,
						   psBaseObj->y >> TILE_SHIFT );

	/* invert y to match QSOUND axes */
	*piY = (GetHeightOfMap() << TILE_SHIFT) - psBaseObj->y;
}

/***************************************************************************/

UWORD
audio_GetScreenWidth( void )
{
	return (UWORD) DISP_WIDTH;
}

/***************************************************************************/
/*
 * audio_GetClusterCentre
 *
 * returns FALSE if no droids moving
 */
/***************************************************************************/

BOOL
audio_GetClusterCentre( void *psClusterObj, SDWORD *piX, SDWORD *piY, SDWORD *piZ )
{
	SDWORD	iClusterID, iNumObj;
	DROID	*psDroid = (DROID *) psClusterObj;
	BOOL	bDroidInClusterMoving = FALSE;

	/* check valid pointer */
	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
			"audio_GetClusterCentre: game object pointer invalid\n" );

	iNumObj = *piX = *piY = *piZ = 0;

	/* clustGetClusterID returns 0 if cluster is empty or no droids moving */
	iClusterID = clustGetClusterID( psClusterObj );
	if ( iClusterID == 0 )
	{
		debug( LOG_NEVER, "audio_GetClusterCentre: empty cluster!\n" );
		return FALSE;
	}
	else
	{
		clustInitIterate( iClusterID );
		do
		{
			psDroid = (DROID *) clustIterate();
			if ( psDroid != NULL && psDroid->sMove.Status != MOVEINACTIVE )
			{
				iNumObj++;
				*piX += psDroid->x;
				*piY += psDroid->y;
				*piZ += psDroid->z;
				bDroidInClusterMoving = TRUE;
			}
		}
		while ( psDroid != NULL );

		/* get average */
		if ( bDroidInClusterMoving == TRUE )
		{
			*piX /= iNumObj;
			*piY /= iNumObj;
			*piZ /= iNumObj;

			/* invert y to match QSOUND axes */
			*piY = (GetHeightOfMap() << TILE_SHIFT) - *piY;
		}
	}

	return bDroidInClusterMoving;
}

/***************************************************************************/
/*
 * audio_GetNewClusterObject
 *
 * get next droid in cluster if current object dead
 */
/***************************************************************************/

BOOL
audio_GetNewClusterObject( void **psClusterObj, SDWORD iClusterID )
{
	DROID	*psDroid = (DROID *) *psClusterObj;

	/* check valid pointer */
	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
			"audio_GetNewClusterObject: game object pointer invalid\n" );

	/* return if droid not dead */
	if ( !psDroid->died )
	{
		return FALSE;
	}

	if ( iClusterID == 0 )
	{
		debug( LOG_NEVER, "audio_GetNewClusterObject: empty cluster!\n" );
		return FALSE;;
	}
	else
	{
		/* find next undying droid in cluster */
		clustInitIterate( iClusterID );
		do
		{
			psDroid = (DROID *) clustIterate();
			if ( psDroid != NULL && !psDroid->died )
			{
				*psClusterObj = psDroid;
				return TRUE;
			}
		}
		while ( psDroid != NULL );
	}

	return FALSE;
}

/***************************************************************************/

BOOL
audio_ClusterEmpty( void * psClusterObj )
{
	/* clustGetClusterID returns 0 if cluster is empty */
	if ( clustGetClusterID( psClusterObj ) == 0 )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/***************************************************************************/

SDWORD
audio_GetClusterIDFromObj( void *psClusterObj )
{
	return clustGetClusterID( psClusterObj );
}

/***************************************************************************/

BOOL
audio_GetIDFromStr( char *pWavStr, SDWORD *piID )
{
	return audioID_GetIDFromStr( pWavStr, piID );
}

/***************************************************************************/

UDWORD
sound_GetGameTime( void )
{
	return gameTime;
}

/***************************************************************************/
