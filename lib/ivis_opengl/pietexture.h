/***************************************************************************/
/*
 * pieState.h
 *
 * render State controlr all pumpkin image library functions.
 *
 */
/***************************************************************************/

#ifndef _pieTexture_h
#define _pieTexture_h


/***************************************************************************/

#include "lib/framework/frame.h"
#include "lib/ivis_common/piedef.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/


/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/


/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern UDWORD pie_GetLastPageDownloaded(void);

extern int pie_AddBMPtoTexPages( 	iSprite* s, char* filename, int type,
					iBool bColourKeyed, iBool bResource);

#endif // _pieTexture_h
