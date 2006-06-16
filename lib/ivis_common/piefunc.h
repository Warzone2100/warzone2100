/***************************************************************************/
/*
 * piefunc.h
 *
 * type defines for extended image library functions.
 *
 */
/***************************************************************************/

#ifndef _piefunc_h
#define _piefunc_h

/***************************************************************************/

#include "lib/framework/frame.h"




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
extern void pie_DownLoadBufferToScreen(void *srcData, UDWORD destX, UDWORD
							destY,UDWORD srcWidth,UDWORD srcHeight,UDWORD srcStride);
extern void pie_RectFilter(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour);
extern void pie_DrawBoundingDisc(iIMDShape *shape, int pieFlag);
extern void pie_Blit(SDWORD texPage, SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);
extern void pie_Sky(SDWORD texPage, PIEVERTEX* aSky );
extern void pie_Water(SDWORD texPage, SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, SDWORD height, SDWORD translucency);
extern void pie_InitMaths(void);
extern UBYTE pie_ByteScale(UBYTE a, UBYTE b);
extern void	pie_CornerBox(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour,
					  UBYTE a, UBYTE b, UBYTE c, UBYTE d);
//extern void	pie_doWeirdBoxFX(UDWORD x, UDWORD y, UDWORD x2, UDWORD y2);
extern void	pie_doWeirdBoxFX(UDWORD x, UDWORD y, UDWORD x2, UDWORD y2, UDWORD	trans);
extern void	pie_TransColouredTriangle(PIEVERTEX *vrt, UDWORD rgb, UDWORD trans);
extern void pie_RenderImageToSurface(LPDIRECTDRAWSURFACE4 lpDDS4, SDWORD surfaceOffsetX, SDWORD surfaceOffsetY, UWORD* pSrcData, SDWORD srcWidth, SDWORD srcHeight, SDWORD srcStride);
extern void	pie_DrawViewingWindow( iVector *v, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2,UDWORD colour);

#endif // _piedef_h
