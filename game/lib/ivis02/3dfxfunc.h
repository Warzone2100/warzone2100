#ifndef _3dfxfunc_h
#define _3dfxfunc_h
#include "piedef.h"

/* 3dfx function headers */

extern BOOL gl_VideoOpen( void );
extern void gl_VideoClose(void);
extern void gl_VSync(void);
extern void gl_Clear(uint32 colour);
extern void gl_RenderEnd(void);
extern void gl_RenderBegin(void);

extern void gl_IntelBitmap(int texPage, int u, int v, int srcWidth, int srcHeight, 
					int x, int y, int destWidth, int destHeight);

extern void gl_Palette(int i, int r, int g, int b);
extern void gl_pPixel(int x, int y, uint32 colour);
extern void gl_pLine(int x0, int y0, int x1, int y1, uint32 colour);
extern void gl_pHLine(int x0, int x1, int y,uint32 colour);
extern void gl_pVLine(int y0, int y1, int x, uint32 colour);
extern void gl_pCircle(int x, int y, int r, uint32 colour);
extern void gl_pCircleFill(int x, int y, int r, uint32 colour);
extern void gl_pBox(int x0, int y0, int x1, int y1, uint32 colour);
extern void gl_pBoxFill(int x0, int y0, int x1, int y1, uint32 colour);
//extern void gl_pBitmap(iBitmap *bmp, int x, int y, int w, int h);
//extern void gl_ppBitmap(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void gl_pBitmapResize(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void gl_pBitmapResizeRot90(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void gl_pBitmapResizeRot180(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void gl_pBitmapResizeRot270(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void gl_pBitmapGet(iBitmap *bmp, int x, int y, int w, int h);
//extern void gl_pBitmapTrans(iBitmap *bmp, int x, int y, int w, int h);
//extern void gl_ppBitmapTrans(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void gl_pBitmapShadow(iBitmap *bmp, int x, int y, int w, int h);
extern void gl_ppBitmapShadow(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void gl_ppBitmapRot90(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void gl_pBitmapRot90(iBitmap *bmp, int x, int y, int w, int h);
extern void gl_ppBitmapRot180(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void gl_pBitmapRot180(iBitmap *bmp, int x, int y, int w, int h);
extern void gl_ppBitmapRot270(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void gl_pBitmapRot270(iBitmap *bmp, int x, int y, int w, int h);
extern void gl_pPolygon(int npoints, iVertex *vrt, iTexture *tex, uint32 type);
extern void gl_pTriangle(iVertex *vrt, iTexture *tex, uint32 type);
extern void gl_pQuad(iVertex *vrt, iTexture *tex, uint32 type);
extern void gl_tTriangle(iVertex *vrt, iTexture *tex);
//extern void gl_tgTriangle(iVertex *vrt, iTexture *tex);
extern void gl_tPolygon(int num, iVertex *vrt, iTexture *tex);
//extern void gl_tgPolygon(int num, iVertex *vrt, iTexture *tex);
extern void gl_pPolygon3D(void);
extern void gl_pLine3D(void);
extern void gl_pPixel3D(void);
extern void gl_pTriangle3D(void);
extern void gl_Pixel(int x, int y, uint32 colour);
extern void gl_Line(int x0, int y0, int x1, int y1, uint32 colour);
extern void gl_HLine(int x0, int x1, int y, uint32 colour);
extern void gl_VLine(int y0, int y1, int x, uint32 colour);
extern void gl_Circle(int x, int y, int r, uint32 colour);
extern void gl_CircleFill(int x, int y, int r, uint32 colour);
extern void gl_Box(int x0,int y0, int x1, int y1, uint32 colour);
extern void gl_BoxFill(int x0, int y0, int x1, int y1, uint32 colour);
extern void gl_Bitmap(iBitmap *bmp, int x, int y, int w, int h);
extern void gl_BitmapResize(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void gl_BitmapResizeRot90(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void gl_BitmapResizeRot180(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void gl_BitmapResizeRot270(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void gl_BitmapGet(iBitmap *bmp, int x, int y, int w, int h);
extern void gl_BitmapTrans(iBitmap *bmp, int x, int y, int w, int h);
extern void gl_BitmapShadow(iBitmap *bmp, int x, int y, int w, int h);
extern void gl_BitmapRot90(iBitmap *bmp, int x, int y, int w, int h);
extern void gl_BitmapRot180(iBitmap *bmp, int x, int y, int w, int h);
extern void gl_BitmapRot270(iBitmap *bmp, int x, int y, int w, int h);
//extern void gl_Polygon(int npoints, iVertex *vrt, iTexture *tex, uint32 type);
//extern void gl_Triangle(iVertex *vrt, iTexture *tex, uint32 type);
extern void gl_Quad(iVertex *vrt, iTexture *tex, uint32 type);
extern void gl_Polygon3D(void);
extern void gl_Line3D(void);
extern void gl_Pixel3D(void);
extern void gl_Triangle3D(void);
extern void gl_ScreenFlip(BOOL bFlip, BOOL bBlack);
extern void gl_DrawMousePointer(int x, int y);
extern void	gl_SetTransFilter(UDWORD rgb,UDWORD tablenumber);
//extern void	gl_TransBoxFill(UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1);
//extern void gl_IntelBitmapDepth(int texPage, int u, int v, int srcWidth, int srcHeight, 
//						   int x, int y, int destWidth, int destHeight, unsigned char brightness, int depth);
extern void gl_DownLoadRadar(unsigned char *buffer, UDWORD texPageID);
extern void	gl_IntelTransBoxFill(SDWORD x0,SDWORD y0, SDWORD x1, SDWORD y1, UDWORD rgb, UDWORD transparency);
//extern void gl_tPolygonLevel(int npoints, iVertex *vrt, iTexture *tex, float lev);
//extern void gl_tTriangleLevel(iVertex *vrt, iTexture *tex, float lev);
extern char *gl_ScreenDump(void);
extern void gl_DrawRadar(UWORD x, UWORD y, UWORD x2, UDWORD y2,UDWORD worldX, UDWORD worldY,UDWORD texPage,SDWORD angle);
//extern void gl_TestTransparency( void );
//extern void gl_IntelTransBitmap(int texPage, int u, int v, int srcWidth, int srcHeight, 
//					int x, int y, int destWidth, int destHeight, 
//					unsigned char brightness, unsigned char transparency);
extern void gl_IntelTransBitmap270(int texPage, int u, int v, int srcWidth, int srcHeight, 
					int x, int y, int destWidth, int destHeight, 
					unsigned char brightness, unsigned char transparency);
//extern void gl_TransPolygon(int num, iVertex *vrt);
//extern void gl_TransTriangle(iVertex *vrt);

extern void gl_DrawImageDef(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y);
//extern void gl_DrawSemiTransImageDef(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,int TransRate);
extern void gl_DrawImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
extern void gl_DrawImageRect(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height);
extern void gl_DrawTransImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
extern void gl_DrawTransImageRect(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height);
extern void gl_DrawColourImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y,UWORD ColourIndex);
extern void gl_DrawTransColourImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y,SWORD ColourIndex);
extern void gl_DrawStretchImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int Width,int Height);
extern void gl_TextRender(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
extern void gl_TextRender270(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
extern void gl_SetGammaValue(float val);
extern void gl_BufferTo3dfx(void *srcData, UDWORD destX, UDWORD 
							destY,UDWORD srcWidth,UDWORD srcHeight,UDWORD srcStride);

extern void gl_UploadDisplayBuffer(UBYTE *DisplayBuffer);
extern void gl_Download640Buffer(UWORD *DisplayBuffer);

extern void gl_DownloadDisplayBuffer(UWORD *DisplayBuffer);
extern void gl_ScaleBitmapRGB(UBYTE *DisplayBuffer,int Width,int Height,int ScaleR,int ScaleG,int ScaleB);
extern void gl_AdditiveTransparency(BOOL val);
extern void gl_SetFogColour(UDWORD colour);
extern void gl_SetFogStatus(BOOL	val);
extern void gl_PIEPolygon(int num, PIEVERTEX *vrt);
extern void gl_PIETriangle(PIEVERTEX *vrt);
extern void	gl_TransBoxFillCorners(SDWORD x0,SDWORD y0, SDWORD x1, SDWORD y1, 
							   UDWORD rgb, UBYTE trans1, UBYTE trans2, UBYTE trans3, UBYTE trans4);
extern void	gl_TransColouredTriangle(PIEVERTEX *vrt, UDWORD rgb, UDWORD transparency);
extern void	gl_TransColouredPolygon(UDWORD num, PIEVERTEX *vrt, UDWORD rgb, UDWORD transparency);
extern void	gl_DrawViewingWindow(iVector *v,UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2,UDWORD colour);
extern BOOL	gl_GlideSurfaceLock( void );
extern BOOL	gl_GlideSurfaceUnlock( void );
extern void	*gl_GetGlideSurfacePointer( void );
extern UDWORD	gl_GetGlideSurfaceStride( void );
//extern BOOL	additive;
#endif
