/***************************************************************************/

#include "lib/ivis_common/pietypes.h"
#include "lib/ivis_common/piestate.h"
#include "arrow.h"

/***************************************************************************/

// sizes for the group heap
#define ARROW_HEAP_INIT	15
#define ARROW_HEAP_EXT	15

/***************************************************************************/

typedef struct ARROW
{
	iVector			vecBase;
	iVector			vecHead;
	UBYTE			iColour;
	struct ARROW	*psNext;
}
ARROW;

/***************************************************************************/


/***************************************************************************/

OBJ_HEAP	*g_psArrowHeap;
ARROW		*g_psArrowList = NULL;

/***************************************************************************/

BOOL
arrowInit( void )
{
	if (!HEAP_CREATE(&g_psArrowHeap, sizeof(ARROW),
			ARROW_HEAP_INIT, ARROW_HEAP_EXT))
	{
		return FALSE;
	}

	return TRUE;
}

/***************************************************************************/

void
arrowShutDown( void )
{
	HEAP_DESTROY(g_psArrowHeap);
}

/***************************************************************************/

BOOL
arrowAdd( SDWORD iBaseX, SDWORD iBaseY, SDWORD iBaseZ,
			SDWORD iHeadX, SDWORD iHeadY, SDWORD iHeadZ, UBYTE iColour )
{
	ARROW	*psArrow;

	if ( !HEAP_ALLOC( g_psArrowHeap, (void*) &psArrow) )
	{
		return FALSE;
	}

	/* ivis y,z swapped */
	psArrow->vecBase.x = iBaseX;
	psArrow->vecBase.y = iBaseZ;
	psArrow->vecBase.z = iBaseY;

	psArrow->vecHead.x = iHeadX;
	psArrow->vecHead.y = iHeadZ;
	psArrow->vecHead.z = iHeadY;

	psArrow->iColour   = iColour;

	/* add to list */
	psArrow->psNext = g_psArrowList;
	g_psArrowList   = psArrow;
	return TRUE;
}

/***************************************************************************/

void
arrowDrawAll( void )
{
	ARROW	*psArrow, *psArrowTemp;

	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(FALSE);

	/* draw and clear list */
	psArrow = g_psArrowList;

	while ( psArrow != NULL )
	{
		draw3dLine( &psArrow->vecHead, &psArrow->vecBase, psArrow->iColour );
		psArrowTemp = psArrow->psNext;
		HEAP_FREE( g_psArrowHeap, psArrow );
		psArrow = psArrowTemp;
	}

	/* reset list */
	g_psArrowList = NULL;

	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
}

/***************************************************************************/
