#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/pieclip.h"
#include "lib/ivis_common/rendfunc.h"
#include "lib/ivis_common/textdraw.h"
#include "lib/ivis_common/bug.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/ivispatch.h"
#include "lib/framework/fractions.h"

//*************************************************************************

void (*iV_VSync)(void);

//*************************************************************************

iSurface	rendSurface;
iSurface	*psRendSurface;

//*************************************************************************


static uint8	*_VIDEO_MEM;
static int32	_VIDEO_SIZE;
static iBool	_VIDEO_LOCK;


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

int32 iV_VideoMemorySize(int mode)

{
	UDWORD size;

	size = pie_GetVideoBufferWidth() * pie_GetVideoBufferHeight();

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

	debug( LOG_3D, "vid[VideoMemoryLock] = locked %dK of video memory\n", size/1024 );

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


//*************************************************************************
//***
//*
//*
//******

iSurface *iV_SurfaceCreate(uint32 flags, int width, int height, int xp, int yp, uint8 *buffer)
{
	iSurface *s;
	int i;



	assert(buffer!=NULL);	// on playstation this MUST be null


	if ((s = (iSurface *) iV_HeapAlloc(sizeof(iSurface))) == NULL)
		return NULL;


	s->flags = flags;
	s->xcentre = width>>1;
	s->ycentre = height>>1;
	s->xpshift = xp;
	s->ypshift = yp;
	s->width = width;
	s->height = height;
	s->size = width * height;

	s->buffer = buffer;
	for (i=0; i<iV_SCANTABLE_MAX; i++)
		s->scantable[i] = i * width;

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
//******

void rend_Assign(iSurface *s)
{
	iV_RenderAssign(s);

	/* Need to look into this - won't the unwanted called still set render surface? */
	psRendSurface = s;
}


// pre VideoOpen
void rend_AssignScreen(void)
{
	iV_RenderAssign(&rendSurface);
}

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

//void (*iV_DownloadDisplayBuffer)(char *DisplayBuffer);
//void (*pie_DownLoadRadar)(char *buffer);

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
void iV_RenderAssign(iSurface *s)
{
	/* Need to look into this - won't the unwanted called still set render surface? */
	psRendSurface = s;

	iV_SetTransFilter = SetTransFilter;

	iV_DEBUG0("vid[RenderAssign] = assigned renderer :\n");
	iV_DEBUG5("usr %d\nflags %x\nxcentre, ycentre %d\nbuffer %p\n",
			s->usr,s->flags,s->xcentre,s->ycentre,s->buffer);
}
#endif	// don't want this function at all if we have PIETOOL defined
