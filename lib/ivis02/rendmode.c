#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dos.h>
#include "rendmode.h"
#ifdef WIN32
#include "pieClip.h"
#endif
#ifdef WIN32
#include "d3dmode.h"
#include "v4101.h"
#endif
#include "rendFunc.h"
#include "vsr.h"
//#include "3dfxfunc.h"
//#include "rendFunc.h"
#include "textDraw.h"

#include "bug.h"
#include "piePalette.h"
#include "pieState.h"
#include "ivispatch.h"
#include "fractions.h"

#ifdef WIN32
#ifdef INC_GLIDE
#include "dGlide.h"
#include "3dfxfunc.h"
#endif
#else

#include "vpsx.h"
#include "psxvram.h"
#endif

//*************************************************************************
//*************************************************************************

//void (*iV_VideoClose)(void);
void (*iV_VSync)(void);
//*************************************************************************

iSurface	rendSurface;
iSurface	*psRendSurface;
static int	g_mode = REND_UNDEFINED;

//*************************************************************************

#ifdef WIN32
static uint8	*_VIDEO_MEM;
static int32	_VIDEO_SIZE;
static iBool	_VIDEO_LOCK;
#endif

//*************************************************************************
//temporary definition
//void (*iV_pPolygon)(int npoints, iVertex *vrt, iTexture *tex, uint32 type); 
//void (*iV_pTriangle)(iVertex *vrt, iTexture *tex, uint32 type);
void (*iV_ppBitmapColourTrans)(iBitmap *bmp, int x, int y, int w, int h, int ow,int ColourIndex);
//void (*iV_DrawStretchImage)(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int Width,int Height);
//*************************************************************************



//*************************************************************************
//*** return mode size in bytes
//*
//*
//******

#ifdef WIN32
static BOOL	bHas3DNow;
BOOL	weHave3DNow( void )
{
	return(bHas3DNow);
}

#define	CPUID	__asm _emit 0x0f __asm _emit 0xa2

BOOL	cpuHas3DNow( void )
{
BOOL	b3DNow;
	/* As of yet - we haven't found an AMD 3DNow! equipped processor */
	b3DNow = FALSE;

	_asm
	{
		push	ebx
		push	edx
		push	ecx
		pushfd
		pushfd
		pop		eax
		mov		edx,eax
		xor		eax,0200000h
		push	eax
		popfd
		pushfd
		pop		eax
		cmp		eax,edx
		// quit if the processor has no cpu id query instructions
		jz		has_no_3dnow			
		// Otherwise, establish what kind of CPU we have found
		xor		eax,eax
		// issue the cpuid instruction
		CPUID
		// Now we need to check for an AMD processor - the id is authenticAMD
		cmp		ebx,068747541h		// htuA
		jnz		has_no_3dnow
		cmp		edx,069746e65h		// itne
		jnz		has_no_3dnow
		cmp		ecx,0444d4163h		// DMAc
		jnz		has_no_3dnow
		// At this point we could check for other vendors that support AMD technology and 3DNow, but....
		mov		eax,080000000h
		CPUID
		test	eax,eax
		jz		has_no_3dnow
		mov		eax,080000001h
		CPUID
		test	edx,080000000h	// we have 3DNow!
		jz		has_no_3dnow
		// Need to send back that we have 3D now support
		mov		eax,1	// we have it
		jmp		has_3d_now
has_no_3dnow:
		mov		eax,0
has_3d_now:
		mov		b3DNow,eax
		popfd
		pop		ecx
		pop		edx
		pop		ebx
	}
	return(b3DNow);
}
#endif

#ifdef WIN32
int32 iV_VideoMemorySize(int mode)

{
	int32 size;


	switch (mode) {
		case iV_MODE_4101:
		case REND_D3D_RGB:
		case REND_D3D_HAL:
		case REND_D3D_REF:
		case REND_GLIDE_3DFX:
								size = pie_GetVideoBufferWidth() * pie_GetVideoBufferHeight(); break;
		default:
			size = 0;
	}

	return size;
}

//*************************************************************************
//*** allocate and lock video memory (call only once!)
//*
//*
//******

iBool iV_VideoMemoryLock(int mode)

{
	int32 size;

	if ((size = iV_VideoMemorySize(mode)) == 0)
		return FALSE;

	if ((_VIDEO_MEM = (uint8 *) iV_HeapAlloc(size)) == NULL)
		return(0);

	_VIDEO_SIZE = size;
	_VIDEO_LOCK = TRUE;

	iV_DEBUG1("vid[VideoMemoryLock] = locked %dK of video memory\n",size/1024);

	return TRUE;
}

//*************************************************************************
//***
//*
//*
//******

void iV_VideoMemoryFree(void)

{
	if (_VIDEO_LOCK) {
		iV_DEBUG0("vid[VideoMemoryFree] = video memory not freed (locked)\n");
		return;
	}


	if (_VIDEO_MEM) {
		iV_HeapFree(_VIDEO_MEM,_VIDEO_SIZE);
		_VIDEO_MEM = NULL;
		_VIDEO_SIZE = 0;
		iV_DEBUG0("vid[VideoMemoryFree] = video memory freed\n");
	}
}

//*************************************************************************
//***
//*
//*
//******

void iV_VideoMemoryUnlock(void)

{

	if (_VIDEO_LOCK)
		_VIDEO_LOCK = FALSE;

	iV_DEBUG0("vid[VideoMemoryUnlock] = video memory unlocked\n");

	iV_VideoMemoryFree();
}

//*************************************************************************
//***
//*
//*
//******

uint8 *iV_VideoMemoryAlloc(int mode)

{

	int32 size;

	size = iV_VideoMemorySize(mode);

	if (size == 0)
		return NULL;


	if (_VIDEO_LOCK) {
		if (size <= _VIDEO_SIZE)
			return _VIDEO_MEM;

		iV_DEBUG0("vid[VideoMemoryAlloc] = memory locked with smaller size than required!\n");
		return NULL;
	}


	if ((_VIDEO_MEM = (uint8 *) iV_HeapAlloc(size)) == NULL)
		return NULL;

	_VIDEO_SIZE = size;

	iV_DEBUG1("vid[VideoMemoryAlloc] = allocated %dK video memory\n",size/1024);

	return _VIDEO_MEM;
}
#endif

//*************************************************************************
//***
//*
//*
//******
#ifdef PSX
void _iv_vid_setup(void)
{
	int i;
#ifdef WIN32
	pie_SetRenderEngine(ENGINE_UNDEFINED);
#endif
	rendSurface.usr = REND_UNDEFINED;
	rendSurface.flags = REND_SURFACE_UNDEFINED;
#ifndef PIEPSX		// was #ifdef WIN32
	rendSurface.buffer = NULL;
#endif
	rendSurface.size = 0;
}
#endif

iSurface *iV_SurfaceCreate(uint32 flags, int width, int height, int xp, int yp, uint8 *buffer)
{
	iSurface *s;
	int i;
#ifdef PSX
	AREA *SurfaceVram;
#endif

#ifndef PIEPSX		// was #ifdef WIN32
	assert(buffer!=NULL);	// on playstation this MUST be null
#else
//	assert(buffer==NULL);	// Commented out by Paul as a temp measure to make it work.
#endif

	if ((s = (iSurface *) iV_HeapAlloc(sizeof(iSurface))) == NULL)
		return NULL;

#ifdef PSX
	SurfaceVram=AllocTexture(width,height,2,0);	// allocate some 32k colour texture ram
	if (SurfaceVram==NULL) return NULL;

	s->VRAMLocation.x=SurfaceVram->area_x0;
	s->VRAMLocation.y=SurfaceVram->area_y0;
	s->VRAMLocation.w=width;
	s->VRAMLocation.h=height;

	ClearImage(&s->VRAMLocation,0,0,0);	// clear the area to black.
#endif
	s->flags = flags;
	s->xcentre = width>>1;
	s->ycentre = height>>1;
	s->xpshift = xp;
	s->ypshift = yp;
	s->width = width;
	s->height = height;
	s->size = width * height;
#ifndef PIEPSX		// was #ifdef WIN32
	s->buffer = buffer;
	for (i=0; i<iV_SCANTABLE_MAX; i++)
		s->scantable[i] = i * width;
#endif
	s->clip.left = 0;
	s->clip.right = width-1;
	s->clip.top = 0;
	s->clip.bottom = height-1;

	iV_DEBUG2("vid[SurfaceCreate] = created surface width %d, height %d\n",width,height);

	return s;
}


// user must free s->buffer before calling
void iV_SurfaceDestroy(iSurface *s)

{

	// if renderer assigned to surface
	if (psRendSurface == s)
		psRendSurface = NULL;

	if (s)
		iV_HeapFree(s,sizeof(iSurface));
		
}

//*************************************************************************
//*** assign renderer
//*
//* params	mode	= render mode (screen/user) see iV_MODE_...
//*
//******

void rend_Assign(int mode, iSurface *s)
{
	iV_RenderAssign(mode, s);

	/* Need to look into this - won't the unwanted called still set render surface? */
	psRendSurface = s;
}


// pre VideoOpen
void rend_AssignScreen(void)

{
	iV_RenderAssign(rendSurface.usr,&rendSurface);
}


#ifdef PSX
iBool iV_VideoOpen(int mode)
{
	iBool r;

	switch (mode) {
		case REND_PSX:
			r = _mode_psx();
			break;
		default:
			r = FALSE;

	}


	if (r) {
		iV_RenderAssign(mode,&rendSurface);
		pal_Init();
	}

	return r;
}
#endif


int iV_GetDisplayWidth(void)
{
	return rendSurface.width;
}

int iV_GetDisplayHeight(void)
{
	return rendSurface.height;
}

//*************************************************************************
//
// function pointers for render assign
//
//*************************************************************************

//void (*pie_VideoShutDown)(void);
void (*iV_VSync)(void);
//void (*iV_Clear)(uint32 colour);
//void (*iV_RenderEnd)(void);
//void (*iV_RenderBegin)(void);
//void (*iV_Palette)(int i, int r, int g, int b);

//void (*pie_Draw3DShape)(iIMDShape *shape, int frame, int team, UDWORD colour, UDWORD specular, int pieFlag, int pieData);
void (*iV_pLine)(int x0, int y0, int x1, int y1, uint32 colour);
//void (*iV_Line)(int x0, int y0, int x1, int y1, uint32 colour);
//void (*iV_Polygon)(int npoints, iVertex *vrt, iTexture *tex, uint32 type);
//void (*iV_Triangle)(iVertex *vrt, iTexture *tex, uint32 type);
//void (*iV_TransPolygon)(int num, iVertex *vrt);
void (*iV_TransTriangle)(iVertex *vrt);
//void (*iV_Box)(int x0,int y0, int x1, int y1, uint32 colour);
//void (*iV_BoxFill)(int x0, int y0, int x1, int y1, uint32 colour);

char* (*iV_ScreenDumpToDisk)(void);

//void (*iV_DownloadDisplayBuffer)(UBYTE *DisplayBuffer);
//void (*pie_DownLoadRadar)(unsigned char *buffer);

//void (*iV_TransBoxFill)(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);
//void (*iV_UniTransBoxFill)(SDWORD x0,SDWORD y0, SDWORD x1, SDWORD y1, UDWORD rgb, UDWORD transparency);

//void (*iV_DrawImage)(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
//void (*iV_DrawImageRect)(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height);
//void (*iV_DrawTransImage)(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
//void (*iV_DrawTransImageRect)(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height);
//void (*iV_DrawSemiTransImageDef)(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,int TransRate);

//void (*iV_DrawStretchImage)(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int Width,int Height);

void (*iV_ppBitmap)(iBitmap *bmp, int x, int y, int w, int h, int ow);
void (*iV_ppBitmapTrans)(iBitmap *bmp, int x, int y, int w, int h, int ow);

void (*iV_SetTransFilter)(UDWORD rgb,UDWORD tablenumber);
//void (*iV_DrawColourTransImage)(IMAGEFILE *ImageFile,UWORD ID,int x,int y,UWORD ColourIndex);

void (*iV_UniBitmapDepth)(int texPage, int u, int v, int srcWidth, int srcHeight, 
						int x, int y, int destWidth, int destHeight, unsigned char brightness, int depth);

//void (*iV_SetGammaValue)(float val);
//void (*iV_SetFogStatus)(BOOL val);
//void (*iV_SetFogTable)(UDWORD color, UDWORD zMin, UDWORD zMax);
void (*iV_SetTransImds)(BOOL trans);

//mapdisplay

void (*iV_tgTriangle)(iVertex *vrt, iTexture *tex);
void (*iV_tgPolygon)(int num, iVertex *vrt, iTexture *tex);

//void (*iV_DrawImageDef)(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y);


//design
//void (*iV_UploadDisplayBuffer)(UBYTE *DisplayBuffer);
//void (*iV_ScaleBitmapRGB)(UBYTE *DisplayBuffer,int Width,int Height,int ScaleR,int ScaleG,int ScaleB);

//text
//void (*iV_BeginTextRender)(SWORD ColourIndex);
//void (*iV_TextRender)(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
//void (*iV_TextRender270)(IMAGEFILE *ImageFile,UWORD ID,int x,int y);

//*************************************************************************
//
// function pointers for render assign
//
//*************************************************************************



#ifndef PIETOOL
void iV_RenderAssign(int mode, iSurface *s)
{
	/* Need to look into this - won't the unwanted called still set render surface? */
	psRendSurface = s;

#ifdef WIN32
	bHas3DNow = cpuHas3DNow();	// do some funky stuff to see if we have an AMD
#endif
	/* Force to 3dfx if glide included */
/*#ifdef INC_GLIDE
	mode = REND_GLIDE_3DFX;
//	if(iV_Line == gl_Line)
//	{
//		// Don't reassign if it's already done
//		return;
//	}
//
#endif
*/
#ifdef PSX
// If psx version then always force mode to iV_MODE_PSX.
	mode = REND_PSX;

	rendSurface.width = 640;
	rendSurface.height = 480;
	pie_Set2DClip(0,0,rendSurface.width,rendSurface.height);

#endif

	g_mode = mode;

	switch (mode) {

#ifndef PIEPSX		// was #ifdef WIN32
		case iV_MODE_SURFACE:
			_clear_sr(0);
//			pie_Draw3DShape				= pie_Draw3DIntelShape;
//setup
//			pie_VideoShutDown 				= _close_sr;
			iV_VSync 					= _vsync_sr;
//			iV_Clear 					= _clear_sr;
//			iV_RenderEnd 				= _bank_on_sr;
//			iV_RenderBegin 			= _bank_off_sr;
//			iV_Palette 					= _palette_sr;

			iV_SetTransFilter  			= SetTransFilter;
//rend	
			iV_pLine 					= _line_sr;
//			iV_Line 					= line;
//			iV_Polygon					= iV_pPolygon;
//			iV_Triangle					= iV_pTriangle;

//box
//			iV_Box 						= box;
//			iV_BoxFill 					= boxf;
//			iV_TransBoxFill	   			= TransBoxFill;
//blit	
			iV_ppBitmap 				= _bitmap_sr;
			iV_ppBitmapTrans			= _tbitmap_sr;
//			iV_DrawImageDef			= DrawImageDef;
//			iV_DrawSemiTransImageDef = DrawSemiTransImageDef;
//			iV_DrawImage			= DrawImage;
//			iV_DrawImageRect		= DrawImageRect;
//			iV_DrawTransImage		= DrawTransImage;
//			iV_DrawTransImageRect	= DrawTransImageRect;
//			iV_DrawColourTransImage	= DrawTransColourImage;
//			iV_DrawStretchImage		= NULL;
//			iV_ScaleBitmapRGB		= ScaleBitmapRGB;
//text
//			iV_BeginTextRender		= BeginTextRender;
//			iV_TextRender			= TextRender;
//			iV_TextRender270		= TextRender270;
//			iV_EndTextRender		= EndTextRender;
//buffer
//			pie_DownLoadRadar		= DownLoadRadar;
//			iV_UploadDisplayBuffer	= UploadDisplayBuffer;
//			iV_DownloadDisplayBuffer = DownloadDisplayBuffer;


// indirect
			
			iV_pBox 						= _box_sr;
//			iV_pPolygon 	  			= _polygon_sr;
//			iV_pTriangle 				= _triangle_sr;

//			iV_pPixel 					= _pixel_sr;
//			iV_pHLine 					= _hline_sr;
//			iV_pVLine 					= _vline_sr;
//			iV_pCircle 					= _circle_sr;
//			iV_pCircleFill 			= _circlef_sr;
//			iV_pQuad						= _quad_sr;
//			iV_pBoxFill 				= _boxf_sr;
//			iV_ppBitmapColour			= _bitmapcolour_sr;
//			iV_ppBitmapColourTrans		= _tbitmapcolour_sr;
//			iV_pBitmap					= pbitmap;
//			iV_pBitmapResize 			= _rbitmap_sr;
//			iV_pBitmapResizeRot90	= _rbitmapr90_sr;
//			iV_pBitmapResizeRot180	= _rbitmapr180_sr;
//			iV_pBitmapResizeRot270	= _rbitmapr270_sr;
//			iV_pBitmapGet 				= _gbitmap_sr;
//			iV_pBitmapTrans			= ptbitmap;
//			iV_ppBitmapShadow			= _sbitmap_sr;
//			iV_pBitmapShadow			= psbitmap;
//			iV_ppBitmapRot90			= _bitmapr90_sr;
//			iV_pBitmapRot90			= pbitmapr90;
//   		iV_ppBitmapRot180			= _bitmapr180_sr;
//			iV_pBitmapRot180			= pbitmapr180;
//   		iV_ppBitmapRot270			= _bitmapr270_sr;
//			iV_pBitmapRot270			= pbitmapr270;
//			iV_HLine 					= hline;
//			iV_VLine 		  			= vline;
//			iV_Circle 		  			= circle;
//			iV_CircleFill 				= circlef;
//			iV_Quad						= ivquad;

//			iV_Bitmap 					= bitmap;
//			iV_BitmapResize 			= rbitmap;
//			iV_BitmapResizeRot90		= rbitmapr90;
//			iV_BitmapResizeRot180	= rbitmapr180;
//			iV_BitmapResizeRot270 	= rbitmapr270;
//			iV_BitmapGet 				= gbitmap;
//			iV_BitmapTrans				= tbitmap;
//			iV_BitmapShadow			= sbitmap;
//			iV_BitmapRot90				= bitmapr90;
//			iV_BitmapRot180			= bitmapr180;
//			iV_BitmapRot270			= bitmapr270;
//			iV_DrawColourImage		= DrawColourImage;
/*
			iV_pPolygon3D 		= _p_polygon3d;
			iV_pLine3D 			= _p_line3d;
			iV_pPixel3D 		= _p_pixel3d;
			iV_pTriangle3D 	= _p_triangle3d;

			iV_Polygon3D 		= _polygon3d;
			iV_Line3D 			= _line3d;
			iV_Pixel3D 			= _pixel3d;
			iV_Triangle3D 		= _triangle3d;
*/
			break;


		case iV_MODE_4101:
//			pie_Draw3DShape				= pie_Draw3DIntelShape;
//			pie_VideoShutDown 		 		= _close_4101;
			iV_VSync 			 		= _vsync_4101;
//			iV_Clear 			 		= _clear_4101;
//			iV_RenderEnd 				= _bank_on_4101;
//			iV_RenderBegin 			= _bank_off_4101;
//			iV_Palette 			 		= _palette_4101;
//			iV_Pixel 			 		= _pixel_4101;
//			iV_pPixel 			 		= _pixel_4101;
			iV_pLine 			 		= _line_4101;
//			iV_pHLine 			 		= _hline_4101;
//			iV_pVLine 			 		= _vline_4101;
//			iV_pCircle 			 		= _circle_4101;
//			iV_pCircleFill 	 		= _circlef_4101;
//			iV_pPolygon 		 		= _polygon_4101;
//			iV_pQuad				 		= _quad_4101;
//			iV_pTriangle 		 		= _triangle_4101;
//			iV_tTriangle				=  _ttriangle_4101;
			iV_tgTriangle				= _tgtriangle_4101;
//			iV_tPolygon					= _tpolygon_4101;
			iV_tgPolygon				= _tgpolygon_4101;
			iV_pBox 				 		= _box_4101;
			iV_pBoxFill 		 		= _boxf_4101;
			iV_ppBitmap 		 		= _bitmap_4101;
//			iV_ppBitmapColour			= _bitmapcolour_4101;
			iV_ppBitmapColourTrans		= _tbitmapcolour_4101;
//			iV_pBitmap			 		= pbitmap;
//			iV_pBitmapResize 	 		= _rbitmap_4101;
//			iV_pBitmapResizeRot90 	= _rbitmapr90_4101;
//			iV_pBitmapResizeRot180	= _rbitmapr180_4101;
//			iV_pBitmapResizeRot270	= _rbitmapr270_4101;
//			iV_pBitmapGet 				= _gbitmap_4101;
			iV_ppBitmapTrans			= _tbitmap_4101;
//			iV_pBitmapTrans			= ptbitmap;
//			iV_ppBitmapShadow			= _sbitmap_4101;
//			iV_pBitmapShadow			= psbitmap;
//			iV_ppBitmapRot90			= _bitmapr90_4101;
//			iV_pBitmapRot90			= pbitmapr90;
//			iV_ppBitmapRot180			= _bitmapr180_4101;
//			iV_pBitmapRot180			= pbitmapr180;
//			iV_ppBitmapRot270			= _bitmapr270_4101;
//			iV_pBitmapRot270			= pbitmapr270;

//			iV_Line 					= line;
//			iV_HLine 					= hline;
//			iV_VLine 					= vline;
//			iV_Circle 					= circle;
//			iV_CircleFill 				= circlef;
//			iV_Polygon 					= iV_pPolygon;
//			iV_Quad						= ivquad;
//			iV_Triangle 				= iV_pTriangle;
//			iV_Box 						= box;
//			iV_BoxFill 					= boxf;
//			iV_Bitmap 					= bitmap;
//			iV_BitmapResize 			= rbitmap;
//			iV_BitmapResizeRot90		= rbitmapr90;
//			iV_BitmapResizeRot180		= rbitmapr180;
//			iV_BitmapResizeRot270 		= rbitmapr270;
//			iV_BitmapGet 				= gbitmap;
//			iV_BitmapTrans				= tbitmap;
//			iV_BitmapShadow				= sbitmap;
//			iV_BitmapRot90				= bitmapr90;
//			iV_BitmapRot180				= bitmapr180;
//			iV_BitmapRot270				= bitmapr270;
			iV_SetTransFilter  			= SetTransFilter;
//			iV_TransBoxFill	   			= TransBoxFill;

//			iV_DrawImageDef			= DrawImageDef;
//			iV_DrawSemiTransImageDef = DrawSemiTransImageDef;
//			iV_DrawImage			= DrawImage;
//			iV_DrawImageRect		= DrawImageRect;
//			iV_DrawTransImage		= DrawTransImage;
//			iV_DrawTransImageRect	= DrawTransImageRect;
//			iV_DrawColourImage		= DrawColourImage;
//			iV_DrawColourTransImage	= DrawTransColourImage;
//			iV_DrawStretchImage		= NULL;

//			iV_BeginTextRender		= BeginTextRender;
//			iV_TextRender			= TextRender;
//			iV_TextRender270		= TextRender270;
//			iV_EndTextRender		= EndTextRender;

//			pie_DownLoadRadar		= DownLoadRadar;

//			iV_UploadDisplayBuffer	= UploadDisplayBuffer;
//			iV_DownloadDisplayBuffer = DownloadDisplayBuffer;
//			iV_ScaleBitmapRGB		= ScaleBitmapRGB;

/*
			iV_pPolygon3D 		= _p_polygon3d;
			iV_pLine3D 			= _p_line3d;
			iV_pPixel3D 		= _p_pixel3d;
			iV_pTriangle3D 	= _p_triangle3d;

			iV_Polygon3D 		= _polygon3d;
			iV_Line3D 			= _line3d;
			iV_Pixel3D 			= _pixel3d;
			iV_Triangle3D 		= _triangle3d;
*/
			break;

		case REND_GLIDE_3DFX:
 //			pie_Draw3DShape					= pie_Draw3DIntelShape;
//			pie_VideoShutDown 		 		= gl_VideoClose;
			iV_VSync 			 		= gl_VSync;
//			iV_Clear 			 		= gl_Clear;
//			iV_RenderEnd 				= gl_RenderEnd;
//			iV_RenderBegin 				= gl_RenderBegin;
//			iV_Palette 			 		= gl_Palette;
//			iV_Pixel 			 		= gl_Pixel;
//			iV_pPixel 			 		= gl_pPixel;
			iV_pLine 			 		= gl_pLine;
//			iV_pHLine 			 		= gl_pHLine;
//			iV_pVLine 			 		= gl_pVLine;
//			iV_pCircle 			 		= gl_pCircle;
//			iV_pCircleFill 	 			= gl_pCircleFill;
//			iV_pPolygon 		 		= gl_pPolygon;
//			iV_pQuad				 	= gl_Quad;  
//			iV_pTriangle 		 		= gl_pTriangle;
//			iV_tTriangle				= gl_tTriangle;
//			iV_tgTriangle				= gl_tgTriangle;
//			iV_tPolygon					= gl_tPolygon;
//			iV_tgPolygon				= gl_tgPolygon;
			iV_pBox 				 	= gl_pBox; 
			iV_pBoxFill 		 		= gl_pBoxFill;
//			iV_ppBitmap 		 		= gl_ppBitmap;//not called on 3dfx
//			iV_pBitmap			 		= gl_pBitmap;
//			iV_pBitmapResize 	 		= gl_pBitmapResize;
//			iV_pBitmapResizeRot90 		= gl_BitmapResizeRot90;
//			iV_pBitmapResizeRot180		= gl_BitmapResizeRot180;
//			iV_pBitmapResizeRot270		= gl_BitmapResizeRot270;
//			iV_pBitmapGet 				= gl_pBitmapGet;
//			iV_ppBitmapTrans			= gl_ppBitmapTrans;
//			iV_pBitmapTrans				= gl_pBitmapTrans;
//			iV_ppBitmapShadow			= gl_ppBitmapShadow;
//			iV_pBitmapShadow			= gl_pBitmapShadow;
//			iV_ppBitmapRot90			= gl_ppBitmapRot90;
//			iV_pBitmapRot90				= gl_pBitmapRot90;
//			iV_ppBitmapRot180			= gl_ppBitmapRot180;
//			iV_pBitmapRot180			= gl_pBitmapRot180;
//			iV_ppBitmapRot270			= gl_ppBitmapRot270;
//			iV_pBitmapRot270			= gl_pBitmapRot270;
//			iV_MousePointer				= gl_DrawMousePointer;
//			iV_Line 					= gl_Line;
//			iV_HLine 					= gl_HLine;
//			iV_VLine 					= gl_VLine;
//			iV_Circle 					= gl_Circle;
//			iV_CircleFill 				= gl_CircleFill;
//			iV_Polygon 					= gl_Polygon;
//			iV_Quad						= gl_Quad;
//			iV_Triangle 				= gl_Triangle;
//			iV_Box 						= gl_Box;
//			iV_BoxFill 					= gl_BoxFill;
//			iV_Bitmap 					= gl_Bitmap;
//			iV_BitmapResize 			= gl_BitmapResize;
//			iV_BitmapResizeRot90		= gl_BitmapResizeRot90;
//			iV_BitmapResizeRot180		= gl_BitmapResizeRot180;
//			iV_BitmapResizeRot270 		= gl_BitmapResizeRot270;
//			iV_BitmapGet 				= gl_BitmapGet;
//			iV_BitmapTrans				= gl_BitmapTrans;
//			iV_BitmapShadow				= gl_BitmapShadow;
//			iV_BitmapRot90				= gl_BitmapRot90;
//			iV_BitmapRot180				= gl_BitmapRot180;
//			iV_BitmapRot270				= gl_BitmapRot270;
			iV_SetTransFilter  			= gl_SetTransFilter;
//			iV_TransBoxFill	   			= gl_TransBoxFill;
//			iV_UniTransBoxFill			= gl_IntelTransBoxFill;
//			iV_UniBitmapDepth			= gl_IntelBitmapDepth;
//			pie_DownLoadRadar			= gl_DownLoadRadar;
//			iV_DownLoadTexture			= gl_DownLoadTexture;
//			iV_DownLoadTextureAbs		= gl_DownLoadTextureAbs;
//			iV_TexPolygonLevel				= gl_tPolygonLevel;
//			iV_TexTriangleLevel			= gl_tTriangleLevel;
			iV_ScreenDumpToDisk			= gl_ScreenDump;
//			iV_DrawRadar				= gl_DrawRadar;
//			iV_DrawImageDef			= gl_DrawImageDef;
//			iV_DrawSemiTransImageDef = gl_DrawSemiTransImageDef;
//			iV_DrawImage			= gl_DrawImage;
//			iV_DrawImageRect		= gl_DrawImageRect;
//			iV_DrawTransImage		= gl_DrawTransImage;
//			iV_DrawTransImageRect	= gl_DrawTransImageRect;
//			iV_DrawColourImage		= gl_DrawColourImage;
//			iV_DrawColourTransImage	= gl_DrawTransColourImage;
//			iV_DrawStretchImage		= gl_DrawStretchImage;

//			iV_BeginTextRender		= gl_BeginTextRender;
//			iV_TextRender270		= gl_TextRender270;
//			iV_TextRender			= gl_TextRender;
//			iV_EndTextRender		= gl_EndTextRender;

//			iV_TestTransparency			= gl_TestTransparency;
//			iV_UniTransBitmap			= gl_IntelTransBitmap;
//			iV_TransTriangle			= gl_TransTriangle;
//			iV_TransPolygon				= gl_TransPolygon;
//			iV_SetGammaValue			= gl_SetGammaValue;

//			iV_UploadDisplayBuffer	= gl_UploadDisplayBuffer;
//			iV_DownloadDisplayBuffer = gl_DownloadDisplayBuffer;
//			iV_ScaleBitmapRGB		= gl_ScaleBitmapRGB;
//			iV_AdditiveTransparency = gl_AdditiveTransparency;
//			iV_SetFogTable			= gl_SetFogTable;
//			iV_SetFogStatus			= gl_SetFogStatus;

			break;
 
		case REND_D3D_RGB:
		case REND_D3D_HAL:
		case REND_D3D_REF:
//			pie_Draw3DShape				= pie_Draw3DIntelShape;
//			pie_VideoShutDown 		 	= _close_D3D;
//			iV_RenderBegin				= _renderBegin_D3D;
//			iV_RenderEnd 				= _renderEnd_D3D;
//			iV_pPolygon 		 		= _polygon_D3D;
//			iV_pQuad			 		= _quad_D3D;
//			iV_pTriangle 		 		= _triangle_D3D;
			iV_VSync 			 		= _vsync_4101;
//			iV_Clear 			 		= _clear_4101;
//			iV_Palette 			 		= _palette_D3D;
//			iV_Pixel 			 		= _dummyFunc1_D3D;
//			iV_pPixel 			 		= _dummyFunc1_D3D;
			iV_pLine 			 		= _dummyFunc2_D3D;
//			iV_pHLine 			 		= _dummyFunc3_D3D;
//			iV_pVLine 			 		= _dummyFunc3_D3D;
//			iV_pCircle 			 		= _dummyFunc3_D3D;
//			iV_pCircleFill 		 		= _dummyFunc3_D3D;
			iV_pBox 			 		= _dummyFunc2_D3D;
			iV_pBoxFill 		 		= _dummyFunc2_D3D;
			iV_ppBitmap 		 		= _dummyFunc5_D3D;
//			iV_ppBitmapColour			= _dummyFunc6_D3D;
			iV_ppBitmapColourTrans		= _dummyFunc6_D3D;
//			iV_pBitmap			 		= _dummyFunc4_D3D;
//			iV_pBitmapResize 	 		= _dummyFunc6_D3D;
//			iV_pBitmapResizeRot90		= _dummyFunc6_D3D;
//			iV_pBitmapResizeRot180		= _dummyFunc6_D3D;
//			iV_pBitmapResizeRot270		= _dummyFunc6_D3D;
//			iV_pBitmapGet 				= _dummyFunc4_D3D;
			iV_ppBitmapTrans			= _dummyFunc5_D3D;
//			iV_pBitmapTrans				= _dummyFunc4_D3D;
//			iV_ppBitmapShadow			= _dummyFunc5_D3D;
//			iV_pBitmapShadow			= _dummyFunc4_D3D;
//			iV_ppBitmapRot90			= _dummyFunc5_D3D;
//			iV_pBitmapRot90				= _dummyFunc4_D3D;
//			iV_ppBitmapRot180			= _dummyFunc5_D3D;
//			iV_pBitmapRot180			= _dummyFunc4_D3D;
//			iV_ppBitmapRot270			= _dummyFunc5_D3D;
//			iV_pBitmapRot270			= _dummyFunc4_D3D;

//			iV_Line 					= _dummyFunc2_D3D;
//			iV_HLine 					= _dummyFunc3_D3D;
//			iV_VLine 					= _dummyFunc3_D3D;
//			iV_Circle 					= _dummyFunc3_D3D;
//			iV_CircleFill 				= _dummyFunc3_D3D;
//			iV_Polygon 					= iV_pPolygon;
//			iV_Quad						= _dummyFunc8_D3D;
//			iV_Triangle 				= iV_pTriangle;
//			iV_Box 						= _dummyFunc2_D3D;
//			iV_BoxFill 					= _dummyFunc2_D3D;
//			iV_Bitmap 					= _dummyFunc4_D3D;
//			iV_BitmapResize 			= _dummyFunc6_D3D;
//			iV_BitmapResizeRot90		= _dummyFunc6_D3D;
//			iV_BitmapResizeRot180		= _dummyFunc6_D3D;
//			iV_BitmapResizeRot270 		= _dummyFunc6_D3D;
//			iV_BitmapGet 				= _dummyFunc4_D3D;
//			iV_BitmapTrans				= _dummyFunc4_D3D;
//			iV_BitmapShadow				= _dummyFunc4_D3D;
//			iV_BitmapRot90				= _dummyFunc4_D3D;
//			iV_BitmapRot180				= _dummyFunc4_D3D;
//			iV_BitmapRot270				= _dummyFunc4_D3D;
			iV_SetTransFilter  			= SetTransFilter_D3D;
//			iV_TransBoxFill	   			= TransBoxFill_D3D;

//			iV_DrawImageDef			= _dummyFunc4_D3D;//DrawImageDef;
//			iV_DrawSemiTransImageDef = _dummyFunc4_D3D;//DrawSemiTransImageDef;
//			iV_DrawImage			= _dummyFunc4_D3D;//DrawImage;
//			iV_DrawImageRect		= _dummyFunc4_D3D;//DrawImageRect;
//			iV_DrawTransImage		= _dummyFunc4_D3D;//DrawTransImage;
//			iV_DrawTransImageRect	= _dummyFunc4_D3D;//DrawTransImageRect;
//			iV_DrawStretchImage		= NULL;

//			iV_BeginTextRender		= _dummyFunc4_D3D;//BeginTextRender;
//			iV_TextRender270		= _dummyFunc4_D3D;//TextRender270;
//			iV_TextRender			= _dummyFunc4_D3D;//TextRender;
//			iV_EndTextRender		= _dummyFunc4_D3D;//EndTextRender;

//			pie_DownLoadRadar		= _dummyFunc4_D3D;//DownLoadRadar;

//			iV_UploadDisplayBuffer	= _dummyFunc1_D3D;
//			iV_DownloadDisplayBuffer = _dummyFunc1_D3D;
//			iV_ScaleBitmapRGB		= _dummyFunc4_D3D;

			break;

#else
			case REND_PSX:
//			pie_VideoShutDown 		 		= _close_psx;
			iV_VSync 			 		= _vsync_psx;
//			iV_Clear 			 		= _clear_psx;
//			iV_RenderEnd 				= _bank_on_psx;
//			iV_RenderBegin 			= _bank_off_psx;
//			iV_Palette 			 		= _palette_psx;
//			iV_Pixel 			 		= _pixel_psx;
//			iV_pPixel 			 		= _pixel_psx;
			iV_pLine 			 		= _line_psx;
//			iV_pHLine 			 		= _hline_psx;
//			iV_pVLine 			 		= _vline_psx;
//			iV_pCircle 			 		= _circle_psx;
//			iV_pCircleFill 	 		= _circlef_psx;
//			iV_pPolygon 		 		= _polygon_psx;
//			iV_pQuad				 		= _quad_psx;
//			iV_pTriangle 		 		= _triangle_psx;
			iV_pBox 				 		= _box_psx;
			iV_pBoxFill 		 		= _boxf_psx;
			iV_ppBitmap 		 		= _bitmap_psx;
//			iV_ppBitmapColour			= _bitmapcolour_psx;
			iV_ppBitmapColourTrans		= _tbitmapcolour_psx;
//			iV_pBitmap			 		= pbitmap;
//			iV_pBitmapResize 	 		= _rbitmap_psx;
//			iV_pBitmapResizeRot90 	= _rbitmapr90_psx;
//			iV_pBitmapResizeRot180	= _rbitmapr180_psx;
//			iV_pBitmapResizeRot270	= _rbitmapr270_psx;
//			iV_pBitmapGet 				= _gbitmap_psx;
			iV_ppBitmapTrans			= _tbitmap_psx;
//			iV_pBitmapTrans			= ptbitmap;
//			iV_ppBitmapShadow			= _sbitmap_psx;
//			iV_pBitmapShadow			= psbitmap;
//			iV_ppBitmapRot90			= _bitmapr90_psx;
//			iV_pBitmapRot90			= pbitmapr90;
//			iV_ppBitmapRot180			= _bitmapr180_psx;
//			iV_pBitmapRot180			= pbitmapr180;
//			iV_ppBitmapRot270			= _bitmapr270_psx;
//			iV_pBitmapRot270			= pbitmapr270;

//			iV_Line 					= line;
//			iV_HLine 					= hline;
//			iV_VLine 					= vline;
//			iV_Circle 					= circle;
//			iV_CircleFill 				= circlef;
//			iV_Polygon 					= iV_pPolygon;
//			iV_Quad						= ivquad;
//			iV_Triangle 				= iV_pTriangle;
//			iV_Box 						= box;
//			iV_BoxFill 					= boxf;
//			iV_Bitmap 					= bitmap;
//			iV_BitmapResize 			= rbitmap;
//			iV_BitmapResizeRot90		= rbitmapr90;
//			iV_BitmapResizeRot180	= rbitmapr180;
//			iV_BitmapResizeRot270 	= rbitmapr270;
//			iV_BitmapGet 				= gbitmap;
//			iV_BitmapTrans				= tbitmap;
//			iV_BitmapShadow			= sbitmap;
//			iV_BitmapRot90				= bitmapr90;
//			iV_BitmapRot180			= bitmapr180;
//			iV_BitmapRot270			= bitmapr270;
			iV_SetTransFilter			= SetTransFilter_psx;
//			iV_TransBoxFill				= TransBoxFill_psx;

//			iV_DrawImageDef			= DrawImageDef_PSX;
//			iV_DrawSemiTransImageDef = DrawSemiTransImageDef_PSX;
//			iV_DrawImage			= DrawImage_PSX;
//			iV_DrawImageRect		= DrawImageRect_PSX;
//			iV_DrawTransImage		= DrawTransImage_PSX;
//			iV_DrawTransImageRect	= DrawTransImageRect_PSX;
//			iV_DrawColourImage		= DrawColourImage_PSX;
//			iV_DrawColourTransImage	= DrawTransColourImage_PSX;
//			iV_DrawStretchImage		= DrawStretchImage_PSX;

//			iV_BeginTextRender		= BeginTextRender;
//			iV_TextRender270		= TextRender270;
//			iV_TextRender			= TextRender;
//			iV_EndTextRender		= EndTextRender;

//			pie_DownLoadRadar		= DownLoadRadar;

//			iV_UploadDisplayBuffer	= UploadDisplayBuffer_PSX;
//			iV_DownloadDisplayBuffer = DownloadDisplayBuffer_PSX;
//			iV_ScaleBitmapRGB		= ScaleBitmapRGB_PSX;

			break;
#endif
	}


	iV_DEBUG0("vid[RenderAssign] = assigned renderer :\n");
#ifndef PIEPSX		// was #ifdef WIN32
	iV_DEBUG5("usr %d\nflags %x\nxcentre, ycentre %d\nbuffer %p\n",
			s->usr,s->flags,s->xcentre,s->ycentre,s->buffer);
#endif

}
#endif	// don't want this function at all if we have PIETOOL defined