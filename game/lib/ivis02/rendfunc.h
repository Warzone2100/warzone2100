/***************************************************************************/
/*
 * rendfunc.h
 *
 * render functions for base render library.
 *
 */
/***************************************************************************/

#ifndef _rendFunc_h
#define _rendFunc_h


/***************************************************************************/

#include "frame.h"
#include "piedef.h"

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
//*************************************************************************
// functions accessed dirtectly from rendmode
//*************************************************************************
extern void line(int x0, int y0, int x1, int y1, uint32 colour);
extern void box(int x0, int y0, int x1, int y1, uint32 colour);
extern void	SetTransFilter(UDWORD rgb,UDWORD tablenumber);
extern void	TransBoxFill(UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1);
extern void DrawImageDef(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y);
extern void DrawSemiTransImageDef(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,int TransRate);
extern void DrawImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
extern void DrawImageRect(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height);
extern void DrawTransImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
extern void DrawTransImageRect(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height);
extern void DrawTransColourImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y,SWORD ColourIndex);
extern void iV_SetMousePointer(IMAGEFILE *ImageFile,UWORD ImageID);
extern void iV_DrawMousePointer(int x,int y);
extern void DownLoadRadar(unsigned char *buffer);
extern void UploadDisplayBuffer(UBYTE *DisplayBuffer);
extern void DownloadDisplayBuffer(UBYTE *DisplayBuffer);
extern void ScaleBitmapRGB(UBYTE *DisplayBuffer,int Width,int Height,int ScaleR,int ScaleG,int ScaleB);


extern UDWORD iV_GetMouseFrame(void);

//*************************************************************************
// functions accessed indirectly from rendmode
//*************************************************************************
extern void (*iV_pBox)(int x0, int y0, int x1, int y1, uint32 colour);
extern void (*iV_pBoxFill)(int x0, int y0, int x1, int y1, uint32 colour);


//*************************************************************************
#endif // _rendFunc_h
