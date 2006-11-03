/***************************************************************************/
/*
 * Ani.c
 *
 * Warzone animation function wrappers
 *
 * Gareth Jones 16/12/97
 */
/***************************************************************************/

#include "lib/gamelib/ani.h"
#include "lib/framework/frameresource.h"


/***************************************************************************/

void *
anim_GetShapeFunc( char * pStr )
{
	return resGetData( "IMD", pStr );
}

/***************************************************************************/
