/***************************************************************************/
/*
 * pieclip.h
 *
 * clipping for all pumpkin image library functions.
 *
 */
/***************************************************************************/

#ifndef _pieclip_h
#define _pieclip_h

/***************************************************************************/

#include "lib/framework/frame.h"
#include "piedef.h"


/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

#define CLIP_BORDER	0

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
void pie_Set2DClip(int x0, int y0, int x1, int y1);
int	pie_ClipTextured(int npoints, PIEVERTEX *points, PIEVERTEX *clip, BOOL bSpecular);
int	pie_ClipTexturedTriangleFast(PIEVERTEX *v1, PIEVERTEX *v2, PIEVERTEX *v3, PIEVERTEX *clipped, BOOL bSpecular);
BOOL	pie_SetVideoBufferDepth(UDWORD depth);
BOOL	pie_SetVideoBufferWidth(UDWORD width);
BOOL	pie_SetVideoBufferHeight(UDWORD height);
UDWORD	pie_GetVideoBufferDepth( void );
UDWORD	pie_GetVideoBufferWidth( void );
UDWORD	pie_GetVideoBufferHeight( void );

#endif // _pieclip_h
