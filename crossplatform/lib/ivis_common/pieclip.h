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

#include "frame.h"
#include "piedef.h"


/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

#define CLIP_BORDER	0

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
extern void pie_Set2DClip(int x0, int y0, int x1, int y1);
extern int	pie_PolyClipTex2D(int npoints, iVertex *points, iVertex *clip);
extern int	pie_PolyClip2D(int npoints, iVertex *points, iVertex *clip);
extern int	pie_ClipTextured(int npoints, PIEVERTEX *points, PIEVERTEX *clip, BOOL bSpecular);
extern int	pie_ClipTexturedTriangleFast(PIEVERTEX *v1, PIEVERTEX *v2, PIEVERTEX *v3, PIEVERTEX *clipped, BOOL bSpecular);
extern int	pie_ClipFlat2dLine(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);
extern BOOL	pie_SetVideoBuffer(UDWORD width, UDWORD height);
extern UDWORD	pie_GetVideoBufferWidth( void );
extern UDWORD	pie_GetVideoBufferHeight( void );

#endif // _pieclip_h
