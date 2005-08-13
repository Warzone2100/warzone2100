// vid.c 0.1 10-01-96.22-11-96
#ifndef _rendmode_h_
#define _rendmode_h_
#include "ivisdef.h"
#include "pieblitfunc.h"
#include "bitimage.h"
#include "textdraw.h"

//*************************************************************************
//patch

#define iV_RenderBegin			pie_LocalRenderBegin
#define iV_RenderEnd			pie_LocalRenderEnd
#define	iV_Line				pie_Line	
#define	iV_Box				pie_Box	
#define	iV_BoxFill			pie_BoxFillIndex
#define	iV_TransBoxFill			pie_TransBoxFill		
#define	iV_UniTransBoxFill		pie_UniTransBoxFill		
#define	iV_DrawImage			pie_ImageFileID
#define	iV_DrawImageRect		pie_ImageFileIDTile
#define	iV_DrawTransImage		pie_ImageFileID
#define	iV_DrawTransImageRect		pie_ImageFileIDTile
#define	iV_DrawStretchImage		pie_ImageFileIDStretch
//#define	iV_DrawColourTransImage	pie_ImageFileIDColour
#define	iV_DrawImageDef			pie_ImageDef
#define	iV_DrawSemiTransImageDef	pie_ImageDefTrans
#define iV_UploadDisplayBuffer		pie_UploadDisplayBuffer
#define iV_DownloadDisplayBuffer	pie_DownloadDisplayBuffer
#define iV_ScaleBitmapRGB		pie_ScaleBitmapRGB

//*************************************************************************

#define iV_MODE_4101		0x4101			// DDX 640x480x256
#define REND_D3D_RGB		0x133			// Direct3D 640x480x16bit RGB renderer (mmx)
#define REND_D3D_HAL		0x143			// Direct3D 640x480x16bit hardware
#define REND_D3D_REF		0x153			// Direct3D 640x480x16bit hardware
#define REND_GLIDE_3DFX		0x200			// 3dfx Glide API
#define REND_16BIT		0x400			// 16bit software mode for video
#define iV_MODE_SURFACE		0x10000			// off-screen surface
#define REND_PSX		0x20000			// PlayStation - added by tjc
#define REND_UNDEFINED		-1			// undefined mode

//*************************************************************************
// polygon flags	b0..b7: col, b24..b31: anim index

//#define PIE_FLAT			0x00000100
#define PIE_TEXTURED			0x00000200
//#define PIE_WIRE			0x00000400
#define PIE_COLOURKEYED			0x00000800
//#define PIE_GOURAUD			0x00001000
#define PIE_NO_CULL			0x00002000
//#define PIE_TEXANIM			0x00004000	// PIE_TEX must be set also
#define PIE_PSXTEX			0x00008000	// - use playstation texture allocation method
#define PIE_BSPFRESH			0x00010000	// Freshly created by the BSP 
#define PIE_NOHALFPSXTEX		0x00020000
#define PIE_ALPHA			0x00040000

//*************************************************************************

#define REND_SURFACE_UNDEFINED		0
#define REND_SURFACE_SCREEN		1
#define REND_SURFACE_USR		2

#define REND_MAX_X			pie_GetVideoBufferWidth()
#define iV_SCREEN_Y_MAX			pie_GetVideoBufferHeight()
#define iV_SCREEN_SIZE_MAX		(iV_SCREEN_X_MAX * iV_SCREEN_Y_MAX)
#define iV_SCREEN_WIDTH			(rendSurface.width)
#define iV_SCREEN_HEIGHT		(rendSurface.height)
#define iV_SCREEN_BUFFER		(rendSurface.buffer)

//*************************************************************************

extern iSurface	rendSurface;
extern iSurface	*psRendSurface;

//*************************************************************************

extern int32 iV_VideoMemorySize(int mode);
extern iBool iV_VideoMemoryLock(int mode);
extern void iV_VideoMemoryFree(void);
extern void iV_VideoMemoryUnlock(void);
extern uint8 *iV_VideoMemoryAlloc(int mode);
extern void rend_AssignScreen(void);
extern void rend_Assign(int mode, iSurface *s);
extern void iV_RenderAssign(int mode, iSurface *s);
extern void iV_SurfaceDestroy(iSurface *s);
extern iSurface *iV_SurfaceCreate(uint32 flags, int width, int height, int xp, int yp, uint8 *buffer);

//*************************************************************************

extern int iV_GetDisplayWidth(void);
extern int iV_GetDisplayHeight(void);
extern BOOL weHave3DNow( void );	// called whenever - returns a boolean

//*************************************************************************
// vid stuff still to be cut down
//*************************************************************************

//extern void (*pie_VideoShutDown)(void);
//extern void (*pie_Draw3DShape)(iIMDShape *shape, int frame, int team, UDWORD colour, UDWORD specular, int pieFlag, int pieData);
extern void (*iV_VSync)(void);
//extern void (*iV_Clear)(uint32 colour);
//extern void (*iV_RenderEnd)(void);
//extern void (*iV_RenderBegin)(void);
//extern void (*iV_Palette)(int i, int r, int g, int b);

extern void (*iV_pLine)(int x0, int y0, int x1, int y1, uint32 colour);
//extern void (*iV_Line)(int x0, int y0, int x1, int y1, uint32 colour);
//extern void (*iV_Polygon)(int npoints, iVertex *vrt, iTexture *tex, uint32 type);
//extern void (*iV_Triangle)(iVertex *vrt, iTexture *tex, uint32 type);
//extern void (*iV_TransPolygon)(int num, iVertex *vrt);
extern void (*iV_TransTriangle)(iVertex *vrt);
//extern void (*iV_Box)(int x0,int y0, int x1, int y1, uint32 colour);
//extern void (*iV_BoxFill)(int x0, int y0, int x1, int y1, uint32 colour);

extern char* (*iV_ScreenDumpToDisk)(void);

//extern void (*iV_DownloadDisplayBuffer)(UBYTE *DisplayBuffer);
//extern void (*pie_DownLoadRadar)(unsigned char *buffer);

//extern void (*iV_TransBoxFill)(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);
//extern void (*iV_UniTransBoxFill)(SDWORD x0,SDWORD y0, SDWORD x1, SDWORD y1, UDWORD rgb, UDWORD transparency);

//extern void (*iV_DrawImage)(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
//extern void (*iV_DrawImageRect)(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height);
//extern void (*iV_DrawTransImage)(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
//extern void (*iV_DrawTransImageRect)(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height);
//extern void (*iV_DrawSemiTransImageDef)(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,int TransRate);

//extern void (*iV_DrawStretchImage)(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int Width,int Height);

extern void (*iV_ppBitmap)(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void (*iV_ppBitmapTrans)(iBitmap *bmp, int x, int y, int w, int h, int ow);


extern void (*iV_SetTransFilter)(UDWORD rgb,UDWORD tablenumber);
//extern void (*iV_DrawColourTransImage)(IMAGEFILE *ImageFile,UWORD ID,int x,int y,UWORD ColourIndex);

extern void (*iV_UniBitmapDepth)(int texPage, int u, int v, int srcWidth, int srcHeight, 
							int x, int y, int destWidth, int destHeight, unsigned char brightness, int depth);

//extern void (*iV_SetGammaValue)(float val);
//extern void (*iV_SetFogStatus)(BOOL val);
//extern void (*iV_SetFogTable)(UDWORD color, UDWORD zMin, UDWORD zMax);
extern void (*iV_SetTransImds)(BOOL trans);

//mapdisplay

extern void (*iV_tgTriangle)(iVertex *vrt, iTexture *tex);
extern void (*iV_tgPolygon)(int num, iVertex *vrt, iTexture *tex);

//extern void (*iV_DrawImageDef)(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y);


//design
//extern void (*iV_UploadDisplayBuffer)(UBYTE *DisplayBuffer);
//extern void (*iV_ScaleBitmapRGB)(UBYTE *DisplayBuffer,int Width,int Height,int ScaleR,int ScaleG,int ScaleB);


/* Blit a transparent rectangle to the back buffer */
extern void	iVBlitTransRect(UDWORD x0, UDWORD x1, UDWORD y0, UDWORD y1);

/* Optimised DWORD read/write to memory */
extern void	iVFBlitTransRect(UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1);

/* Possible filter colours for the transparency rectangle blit */
#define TINT_BLUE	0
#define TRANS_GREY	1
#define TRANS_BLUE	2
#define TRANS_BRITE	3
#define TINT_DEEPBLUE	4

//extern void (*iV_BeginTextRender)(SWORD ColourIndex);
//extern void (*iV_TextRender)(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
//extern void (*iV_TextRender270)(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
//extern void (*iV_EndTextRender)(void);

extern void iV_DrawMousePointer(int x,int y);
extern void iV_SetMousePointer(IMAGEFILE *ImageFile,UWORD ImageID);
//*************************************************************************
extern void (*iV_ppBitmapColourTrans)(iBitmap *bmp, int x, int y, int w, int h, int ow,int ColourIndex);

#endif
