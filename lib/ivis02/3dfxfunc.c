#ifdef INC_GLIDE
#include "ivi.h"
#include "piePalette.h"
#include "pieState.h"
#include "pieTexture.h"
#include "Frame.h"
#include "BitImage.h"
#include "3dfxfunc.h"
#include "3dfxmode.h"
#include "3dfxText.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"
#include "dGlide.h"
#include "imd.h"
#include "rendmode.h"
#include "PieClip.h"
#include "pieMatrix.h"

#define SNAP_BIAS ((float)0.0)
//#define SNAP_BIAS ((float)(3<<18))

//OOW_SCALE controls the depth range ivis uses, alower scale stretchs our depth range across more of the available range
#define OOW_SCALE 1.0f 

static char	*filename[20];
static UBYTE	frameNumber = 0;
static GrState	previousState;
static UDWORD	previousPage;
static	screenClearColour = 0x00000000;
#define IMAGEINTENSITY 255
#define COLOURINTENSITY 0xffffffff

#define	LINE_DEPTH		100

/* Not externally referenceable */
void gl_Line(int x0, int y0, int x1, int y1, uint32 colour);
void gl_LineClipped(int x0, int y0, int x1, int y1, uint32 colour);

//-- New functions for locking the Glide buffer!

// Lets us know whether the Glide surface is presently locked or not
BOOL	bGlideSurfaceLocked = FALSE;
// Holds info abou the Glide surface
GrLfbInfo_t	glideSurfaceInfo;




// Lock the Glide surface - returns TRUE is it works
BOOL	gl_GlideSurfaceLock( void )
{
BOOL	success;

	success = 	grLfbLock(	GR_LFB_WRITE_ONLY,
				GR_BUFFER_BACKBUFFER,
				GR_LFBWRITEMODE_565,
				GR_ORIGIN_UPPER_LEFT,
				FXFALSE,
				&glideSurfaceInfo);
	if(success) bGlideSurfaceLocked = TRUE;
	else bGlideSurfaceLocked = FALSE;

	return(success);
}

// Unlock the Glide surface
BOOL	gl_GlideSurfaceUnlock( void )
{
BOOL	success;

	success = grLfbUnlock(GR_LFB_WRITE_ONLY,GR_BUFFER_BACKBUFFER);
	if(success) bGlideSurfaceLocked = FALSE;
	return(success);
}

// Returns a void* pointer to the top left corner of the Glide surface 
// if we have an active lock and NULL otherwise
void	*gl_GetGlideSurfacePointer( void )
{
	if(bGlideSurfaceLocked)
	{
		return(glideSurfaceInfo.lfbPtr);
	}
	else
	{
		return(NULL);
	}
}

// Returns how many bytes wide the Glide surface is...
UDWORD	gl_GetGlideSurfaceStride( void )
{
	if(bGlideSurfaceLocked)
	{
		return(glideSurfaceInfo.strideInBytes);
	}
	else
	{
		return(0);
	}
}

/* 3dfx function calls - replicating those needed in vid.c */
/* Alex McLean, Pumpkin, 1997 */
/*	
	NOTE - IT MIGHT BE WORTH IN THE CASE OF FUNCTIONS THAT GET AND SET STATE PRIOR TO AND
	AFTER RENDERING, REMOVING THESE STATE CALLS. SAY FOR EXAMPLE, YOU'VE GOT TO DO 100 BOX FILLS 
	(OR WHATEVER) IN ONE PASS - IT'S HARDLY WORTH A STATE SAVE/SET BEFORE AND AFTER EACH ONE.
	PERHAPS HAVE IT SUCH THAT A FUNCTION CALL INCLUDES A PARAMETER THAT SPECIFIES WHETHER
	STATE MAINTENANCE IS NECESSARY (COLOUR/TEXTURE/ALPHA - UNITS).
*/

/* ---------------------------------------------------------------------------------- */
/*	Initialises the 3dfx card and opens the window */
/* ---------------------------------------------------------------------------------- */
BOOL gl_VideoOpen( void )
{
 	return init3dfx();
}

/* ---------------------------------------------------------------------------------- */
/* Closes down the 3dfx card */
/* ---------------------------------------------------------------------------------- */
/* Shuts down the 3dfx card and returns the window focus */
void gl_VideoClose(void)
{
	close3dfx();
	return;
}

/* ---------------------------------------------------------------------------------- */
/* Flips the screen and clears the buffer soon to be drawn */
/* ---------------------------------------------------------------------------------- */
void gl_ScreenFlip(BOOL bFlip, BOOL bBlack)
{


	while(grBufferNumPending()>=2);

	/* Wait on a vblank */
	grBufferSwap(1);

	/* And wait until idle - this might not be necessary...?*/
	if(bFlip)
	{
		if (bBlack)
		{
			grBufferClear(0,0,GR_WDEPTHVALUE_FARTHEST);
		}
		else
		{
			grBufferClear(screenClearColour,0,GR_WDEPTHVALUE_FARTHEST);
		}
	}
	grSstIdle();
}

/* ---------------------------------------------------------------------------------- */
/* Look at intel bitmap - not yet being called to see how to bitmap blit see below */
/* ---------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------------- */
/*	This is how a bitmap blit should be done */
/*	Should be all you need - will take any bitmap in texture memory and blit
	it to the screen at any position and stretch it or shrink it as necessary.
/*	MIGHT NEED TO ALTER PARAMETERS IN ACCORDANCE WITH PLAYASTATION VERSION
	THIS FUNCTION WILL ALWAYS TRY TO TRANSPARENTLY BLIT THE BITMAP USING COLOUR
	RGBA = 0X00000000 AS TRANSPARENT. THERE'S NO POINT IN HAVING SEPARATE
	FUNCTIONS FOR TRANSPARENT AND NON-TRANSPARENT AS IT TAKES NO LONGER ON A 3DFX.
/* ---------------------------------------------------------------------------------- */
/* 
Intelligent 3dfx specific bitmap printer:-
texPage - which texture page do you want to print from?
u - x coord into tex page of thing to print
v - y coord into tex page of thing to print
srcWidth - how much (width) of tex page to print
srcHeight -	how much (hieght) of tex page to print
destWidth - width of screen rect to blit into
destHeight - height of screen rect to blit into
x,y  - where to put it on screen
*/
// ColourKeying set externally to this routine
// Texturing set externally to this routine
// Current colour set externally to this routine
// Bilinear set externally to this routine
//called form gl_DrawImageDef as gl_IntelBitmap
//called form gl_DrawTransImage as gl_IntelBitmap
//called form gl_DrawImageRect as gl_IntelBitmap
//called form gl_TextRender as gl_IntelBitmap //new pie states implemented
//3dfx blit func
void gl_IntelBitmap(int texPage, int u, int v, int srcWidth, int srcHeight, 
					int x, int y, int destWidth, int destHeight)
{
	GrVertex	vtxs[4];
	UDWORD	colour;
	// Set texture page if necessary
	pie_SetTexturePage(texPage);

	if(texPage==31)
	{
		srcWidth*=2;
		srcHeight*=2;
	}
	/* Top left */
	vtxs[0].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[0].x = (float)x;
	vtxs[0].y = (float)y;
	vtxs[0].tmuvtx[0].sow = (float)u*INV_INTERFACE_DEPTH_3DFX;
	vtxs[0].tmuvtx[0].tow = (float)v*INV_INTERFACE_DEPTH_3DFX;

	/* Top Right */
	vtxs[1].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[1].x = (float)(x+destWidth);
	vtxs[1].y = (float)y;
	vtxs[1].tmuvtx[0].sow = (float)(u+srcWidth)*INV_INTERFACE_DEPTH_3DFX;
	vtxs[1].tmuvtx[0].tow = (float)v*INV_INTERFACE_DEPTH_3DFX;


	/* Bottom right */
	vtxs[2].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[2].x = (float)(x+destWidth);
	vtxs[2].y = (float)(y+destHeight);
	vtxs[2].tmuvtx[0].sow = (float)(u+srcWidth)*INV_INTERFACE_DEPTH_3DFX;
	vtxs[2].tmuvtx[0].tow = (float)(v+srcHeight)*INV_INTERFACE_DEPTH_3DFX;

	/* Bottom left */
	vtxs[3].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[3].x = (float)x;
	vtxs[3].y = (float)(y+destHeight);
	vtxs[3].tmuvtx[0].sow = (float)u*INV_INTERFACE_DEPTH_3DFX;
	vtxs[3].tmuvtx[0].tow = (float)(v+srcHeight)*INV_INTERFACE_DEPTH_3DFX;

	/*	Specify their brightness here - make sure they're all the same 
		or the bitmap will get gouraud shaded */
	colour = pie_GetColour();

	vtxs[0].b = vtxs[1].b = vtxs[2].b = vtxs[3].b = (float)(colour & 0xff);
	colour  >>= 8;
	vtxs[0].g = vtxs[1].g = vtxs[2].g = vtxs[3].g = (float)(colour & 0xff);
	colour  >>= 8;
	vtxs[0].r = vtxs[1].r = vtxs[2].r = vtxs[3].r = (float)(colour & 0xff);
	colour  >>= 8;
	vtxs[0].a = vtxs[1].a = vtxs[2].a = vtxs[3].a = (float)(colour & 0xff);

	/* Draw two triangles - the polygon function is a bit odd! */
	grDrawTriangle(&vtxs[0],&vtxs[1],&vtxs[2]);
	grDrawTriangle(&vtxs[0],&vtxs[2],&vtxs[3]);

	return;
}

//called form gl_TextRender270 as gl_IntelBitmap270
//3dfx blit func
void gl_IntelBitmap270(int texPage, int u, int v, int srcWidth, int srcHeight, 
					int x, int y, int destWidth, int destHeight)
{
	GrVertex	vtxs[4];
	UDWORD	colour;

	/* Check this is valid? */
	pie_SetTexturePage(texPage);

	/* Sent up texture ratios to 1 to 1 */
	vtxs[0].oow = 1.0f;
	vtxs[1] = vtxs[2] = vtxs[3] = vtxs[0];

	/* Top left */
	vtxs[0].x = (float)x;
	vtxs[0].y = (float)y;
	vtxs[0].tmuvtx[0].sow = (float)(u+srcWidth);
	vtxs[0].tmuvtx[0].tow = (float)v;

	/* Top Right */
	vtxs[1].x = (float)(x+destWidth);
	vtxs[1].y = (float)y;
	vtxs[1].tmuvtx[0].sow = (float)(u+srcWidth);
	vtxs[1].tmuvtx[0].tow = (float)(v+srcHeight);


	/* Bottom right */
	vtxs[2].x = (float)(x+destWidth);
	vtxs[2].y = (float)(y+destHeight);
	vtxs[2].tmuvtx[0].sow = (float)u;
	vtxs[2].tmuvtx[0].tow = (float)(v+srcHeight);

	/* Bottom left */
	vtxs[3].x = (float)x;
	vtxs[3].y = (float)(y+destHeight);
	vtxs[3].tmuvtx[0].sow = (float)u;
	vtxs[3].tmuvtx[0].tow = (float)v;


	/*	Specify their brightness here - make sure they're all the same 
		or the bitmap will get gouraud shaded */
	colour = pie_GetColour();
	vtxs[0].b = vtxs[1].b = vtxs[2].b = vtxs[3].b = (float)(colour & 0xff);
	colour  >>= 8;
	vtxs[0].g = vtxs[1].g = vtxs[2].g = vtxs[3].g = (float)(colour & 0xff);
	colour  >>= 8;
	vtxs[0].r = vtxs[1].r = vtxs[2].r = vtxs[3].r = (float)(colour & 0xff);
	colour  >>= 8;
	vtxs[0].a = vtxs[1].a = vtxs[2].a = vtxs[3].a = (float)(colour & 0xff);


	/* Draw two triangles - the polygon function is a bit odd! */
	grDrawTriangle(&vtxs[0],&vtxs[1],&vtxs[2]);
	grDrawTriangle(&vtxs[0],&vtxs[2],&vtxs[3]);

	return;
}


/* --------------------------------------------------------------------------- */
/* Box fill - just draws a polygon of specified colour */
/* --------------------------------------------------------------------------- */
//called from drawdroidselections as iV_boxFill
//called from tipdisplay as iV_boxFill
//3dfx blit func
void gl_BoxFill(int x0, int y0, int x1, int y1, uint32 colour)
{
unsigned int	red,green,blue,argb;
iColour* psPalette = pie_GetGamePal();

		/* Set alpha to 0 */
		argb = 0;
		/* get the red,green and blue colours */
		red = psPalette[colour].r;
		green = psPalette[colour].g;
		blue = psPalette[colour].b;

		argb = ((red<<16) | (green<<8) | (blue));

		gl_IntelTransBoxFill(x0,y0,x1,y1,argb, MAX_UB_LIGHT); 
}

/* ----------------------------------------------------------------------------------- */
/* Standard RGB polygon draw - used in imd drawing */
/* ----------------------------------------------------------------------------------- */
void gl_PIEPolygon(int num, PIEVERTEX *vrt)
{
GrVertex	vtxs[3];
int		count;
float invZ;
	vtxs[0].x = (float)vrt[0].sx;
	vtxs[0].y = (float)vrt[0].sy;

	invZ = 1.0f/vrt[0].sz;
	vtxs[0].oow = OOW_SCALE*invZ;
	vtxs[0].tmuvtx[0].sow = (float)vrt[0].tu*invZ;
	vtxs[0].tmuvtx[0].tow = (float)vrt[0].tv*invZ;
	vtxs[0].r = (float)vrt[0].light.byte.r;
	vtxs[0].g = (float)vrt[0].light.byte.g;
	vtxs[0].b = (float)vrt[0].light.byte.b;
	vtxs[0].a = (float)vrt[0].light.byte.a;
	
	/* Set up vertex one */
	/*	Thereafter we'll only need to set up one of the vertices
		and switch the paramater order to the tri. draw */
	invZ = 1.0f/vrt[1].sz;
	vtxs[1].oow = OOW_SCALE*invZ;
	vtxs[1].x = (float)vrt[1].sx;
	vtxs[1].y = (float)vrt[1].sy;
	vtxs[1].tmuvtx[0].sow = (float)vrt[1].tu*invZ;
	vtxs[1].tmuvtx[0].tow = (float)vrt[1].tv*invZ;
	vtxs[1].r = (float)vrt[1].light.byte.r;
	vtxs[1].g = (float)vrt[1].light.byte.g;
	vtxs[1].b = (float)vrt[1].light.byte.b;
	vtxs[1].a = (float)vrt[1].light.byte.a;
	
	

	count = 2;
	while(count<num)
	{
		/* Is it even? */
		if( (count&0x1) == 0)
		{
			invZ = 1.0f/vrt[count].sz;
			vtxs[2].oow = OOW_SCALE*invZ;
			vtxs[2].x = (float)vrt[count].sx;
			vtxs[2].y = (float)vrt[count].sy;
			vtxs[2].tmuvtx[0].sow = (float)vrt[count].tu*invZ;
			vtxs[2].tmuvtx[0].tow = (float)vrt[count].tv*invZ;
			vtxs[2].r = (float)vrt[count].light.byte.r;
			vtxs[2].g = (float)vrt[count].light.byte.g;
			vtxs[2].b = (float)vrt[count].light.byte.b;
			vtxs[2].a = (float)vrt[count].light.byte.a;
			grDrawTriangle( &vtxs[0], &vtxs[1], &vtxs[2] );
		}
 		else
		{
			invZ = 1.0f/vrt[count].sz;
			vtxs[1].oow = OOW_SCALE*invZ;
			vtxs[1].x = (float)vrt[count].sx;
			vtxs[1].y = (float)vrt[count].sy;
			vtxs[1].tmuvtx[0].sow = (float)vrt[count].tu*invZ;
			vtxs[1].tmuvtx[0].tow = (float)vrt[count].tv*invZ;
			vtxs[1].r = (float)vrt[count].light.byte.r;
			vtxs[1].g = (float)vrt[count].light.byte.g;
			vtxs[1].b = (float)vrt[count].light.byte.b;
			vtxs[1].a = (float)vrt[count].light.byte.a;
			grDrawTriangle( &vtxs[0], &vtxs[2], &vtxs[1] );
		}
		count++;
	}
}
/* ---------------------------------------------------------------------------------- */
/* Textured light level polygon */
/* ---------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------------- */
/*	Line draw Clipper - then it calls gl_lineClipped
/* ---------------------------------------------------------------------------------- */

void gl_Line(int x0, int y0, int x1, int y1, uint32 colour)
{
	register code1, code2, code;
	int x, y;

	code1 = code2 = 0;

	if (y0>psRendSurface->clip.bottom)   	code1 = 1;
	else if (y0<psRendSurface->clip.top) 	code1 = 2;
	if (x0>psRendSurface->clip.right)	  	code1 |= 4;
	else if (x0<psRendSurface->clip.left)	code1 |= 8;
	if (y1>psRendSurface->clip.bottom)		code2 = 1;
	else if (y1<psRendSurface->clip.top) 	code2 = 2;
	if (x1>psRendSurface->clip.right)	  	code2 |= 4;
	else if (x1<psRendSurface->clip.left)	code2 |= 8;

	// line totally outside screen: send it packing

	if ((code1 & code2) != 0)
		return;

	// line totally inside screen: just draw it now

	if ((code1 | code2) == 0) {
		gl_LineClipped(x0,y0,x1,y1,colour);
		return;
	}

	// at least one point needs clipping

	code = (code1 != 0) ? code1 : code2;

	if (code & 1) {
		x = x0 + (x1-x0) * (psRendSurface->clip.bottom-y0)/(y1-y0);
		y = psRendSurface->clip.bottom;
	} else if (code & 2) {
		x = x0 + (x1-x0) * (psRendSurface->clip.top-y0)/(y1-y0);
		y = psRendSurface->clip.top;
	} else if (code & 4) {
		y = y0 + (y1-y0) * (psRendSurface->clip.right-x0)/(x1-x0);
		x = psRendSurface->clip.right;
	} else if (code & 8) {
		y = y0 + (y1-y0) * (psRendSurface->clip.left-x0)/(x1-x0);
		x = psRendSurface->clip.left;
	}


	if (code == code1) {
		x0 = x;
		y0 = y;
	} else {
		x1 = x;
		y1 = y;
	}

	code1 = code2 = 0;


	if (y0>psRendSurface->clip.bottom)		code1 = 1;
	else if (y0<psRendSurface->clip.top)	code1 = 2;
	if (x0>psRendSurface->clip.right)  	code1 |= 4;
	else if (x0<psRendSurface->clip.left)	code1 |= 8;
	if (y1>psRendSurface->clip.bottom)		code2 = 1;
	else if (y1<psRendSurface->clip.top)	code2 = 2;
	if (x1>psRendSurface->clip.right)		code2 |= 4;
	else if (x1<psRendSurface->clip.left)	code2 |= 8;

	if ((code1 & code2) != 0)
		return;

	if ((code1 | code2) == 0) {
		gl_LineClipped(x0,y0,x1,y1,colour);
		return;
	}

	code = (code1 != 0) ? code1 : code2;

	if (code & 1) {
		x = x0 + (x1-x0) * (psRendSurface->clip.bottom-y0)/(y1-y0);
		y = psRendSurface->clip.bottom;
	} else if (code & 2) {
		x = x0 + (x1-x0) * (psRendSurface->clip.top-y0)/(y1-y0);
		y = psRendSurface->clip.top;
	} else if (code & 4) {
		y = y0 + (y1-y0) * (psRendSurface->clip.right-x0)/(x1-x0);
		x = psRendSurface->clip.right;
	} else if (code & 8) {
		y = y0 + (y1-y0) * (psRendSurface->clip.left-x0)/(x1-x0);
		x = psRendSurface->clip.left;
	}

	if (code == code1)
		gl_LineClipped(x,y,x1,y1,colour);
	else
		gl_LineClipped(x0,y0,x,y,colour);
}

/* ---------------------------------------------------------------------------------- */
/*	Line draw - 
	Colour selection is a bit wierd - remember window was opened with ABGR format! */
/* ---------------------------------------------------------------------------------- */

void gl_LineClipped(int x0, int y0, int x1, int y1, uint32 colour)
{
GrVertex	vert1,vert2;
unsigned int	col;
unsigned int	red,green,blue;
iColour* psPalette = pie_GetGamePal();

	/* get the red,green and blue colours */
	red = psPalette[colour].r;
	green = psPalette[colour].g;
	blue = psPalette[colour].b;

	/* Flush out the alpha value */
	col = 0;
	/* Get colour into abgr long word */
	col = ((MAX_UB_LIGHT << 24) | (red<<16) | (green<<8) | blue);

	/* Set constant colour */
	pie_SetColour(col);
	pie_SetRendMode(REND_FLAT);

	// Adding in the SNAP_BIAS doesn't do the job but what the hell.
	vert1.x = ((float)x0) + SNAP_BIAS + 0.5f;
	vert1.y = ((float)y0) + SNAP_BIAS + 0.5f;
	vert2.x = ((float)x1) + SNAP_BIAS + 0.5f;
	vert2.y = ((float)y1) + SNAP_BIAS + 0.5f;

	vert1.r = vert2.r = (float)red;
	vert1.g = vert2.g = (float)green;
	vert1.b = vert2.b = (float)blue;
	vert1.a = vert2.a = 0.0f;

	vert1.ooz = 65535.0F;
	vert1.oow = OOW_SCALE/LINE_DEPTH;
	vert2.ooz = 65535.0F;
	vert2.oow = OOW_SCALE/LINE_DEPTH;

	grDrawLine(&vert1,&vert2);	// A steaming big pile of poo!

	return;
}

#define LINE_HACK 1
#define TESTX 2
#define TESTY 2

/* --------------------------------------------------------------------------- */
/* Uses the line draw to draw a box */
/* --------------------------------------------------------------------------- */
void gl_Box(int x0,int y0, int x1, int y1, uint32 colour)
{
	// Adds LINE_HACK to left and top lines cause grDrawLine sucks big time.
	gl_Line(x0,y1,x0,y0,colour);
	gl_Line(x1,y0,x1,y1,colour);
	gl_Line(x1,y1,x0,y1,colour);
	gl_Line(x0,y0,x1,y0,colour);
}


/* ----------------------------------------------------------------------------------- */
/* Standard RGB triangle draw - used in imd draw */
/* ----------------------------------------------------------------------------------- */
void gl_PIETriangle(PIEVERTEX *vrt)
{
SDWORD maxX, maxY, minX, minY;
GrVertex	vtxA,vtxB,vtxC;
float	invZ;	

#ifdef _DEBUG
		maxX = pie_MAX(vrt[0].sx,vrt[1].sx);
		maxX = pie_MAX(vrt[2].sx,maxX);
		maxY = pie_MAX(vrt[0].sy,vrt[1].sy);
		maxY = pie_MAX(vrt[2].sy,maxY);

		minX = pie_MIN(vrt[0].sx,vrt[1].sx);
		minX = pie_MIN(vrt[2].sx,minX);
		minY = pie_MIN(vrt[0].sy,vrt[1].sy);
		minY = pie_MIN(vrt[2].sy,minY);

		ASSERT(((maxX-minX) < 1280,"Big Triangle dtected"));
		ASSERT(((maxY-minY) < 960,"Big Triangle dtected"));
#endif


		vtxA.x = (float) vrt[0].sx;
		vtxA.y = (float) vrt[0].sy;
		vtxB.x = (float) vrt[1].sx;
		vtxB.y = (float) vrt[1].sy;
		vtxC.x = (float) vrt[2].sx;
		vtxC.y = (float) vrt[2].sy;

		invZ = 1.0f/vrt[0].sz;
		vtxA.oow = OOW_SCALE*invZ;
		vtxA.tmuvtx[0].sow = (float)vrt[0].tu*invZ;
		vtxA.tmuvtx[0].tow = (float)vrt[0].tv*invZ;

		invZ = 1.0f/vrt[1].sz;
  		vtxB.oow = OOW_SCALE*invZ;
		vtxB.tmuvtx[0].sow = (float)vrt[1].tu*invZ;
		vtxB.tmuvtx[0].tow = (float)vrt[1].tv*invZ;
  		
		invZ = 1.0f/vrt[2].sz;
		vtxC.oow = OOW_SCALE*invZ;
		vtxC.tmuvtx[0].sow = (float)vrt[2].tu*invZ;
		vtxC.tmuvtx[0].tow = (float)vrt[2].tv*invZ;

        vtxA.r = (float)vrt[0].light.byte.r;
		vtxA.g = (float)vrt[0].light.byte.g;
		vtxA.b = (float)vrt[0].light.byte.b;
		vtxA.a = (float)vrt[0].light.byte.a;

        vtxB.r = (float)vrt[1].light.byte.r;
		vtxB.g = (float)vrt[1].light.byte.g;
		vtxB.b = (float)vrt[1].light.byte.b; 
		vtxB.a = (float)vrt[1].light.byte.a;

        vtxC.r = (float)vrt[2].light.byte.r;
		vtxC.g = (float)vrt[2].light.byte.g;
		vtxC.b = (float)vrt[2].light.byte.b;
		vtxC.a = (float)vrt[2].light.byte.a;

        grDrawTriangle( &vtxA, &vtxB, &vtxC );

	return;
}
/* ----------------------------------------------------------------------------------- */
/* Temporary hack function only - needs to specify a cursor in final version though*/
/* ----------------------------------------------------------------------------------- */
void gl_DrawMousePointer(int x, int y)
{
GrVertex	vtxA,vtxB,vtxC;

		vtxA.x = (float) x;
		vtxA.y = (float) y;

		vtxB.x = (float) x+32;
		vtxB.y = (float) y+16;
		
		vtxC.x = (float) x;
		vtxC.y = (float) y+32;
		
#ifdef STATES
		pie_SetColour(255);
		pie_SetTextured(FALSE);
		pie_SetColourCombine(FALSE);
#endif
        grDrawTriangle( &vtxA, &vtxB, &vtxC );
}

/* ----------------------------------------------------------------------------------- */
/* 
	Draws a transparent box of specified dimensions - used in the interface 
	This one's a bit different in that you can specify the colour to fill in and 
	also it's opacity (0 - completely transparent, 255 - solid) 
*/
/* ----------------------------------------------------------------------------------- */
//called from drawRadar
//called from RenderWindow as iV_UniTransBoxFill 
//3dfx blit func

#define	D_WIDTH		pie_GetVideoBufferWidth()
#define	D_HEIGHT	pie_GetVideoBufferHeight()

void	gl_IntelTransBoxFill(SDWORD x0,SDWORD y0, SDWORD x1, SDWORD y1, UDWORD rgb, UDWORD transparency)
{
GrVertex	vtxs[4];

	/* Naive clipper - but easy because it's only a box - excuse the lack of bracketing! */
	/* WARNING - ASSSUMES 640,480 */
	if(x0<0) x0 = 0;
	else if(x0>D_WIDTH-1) x0 = D_WIDTH-1;
	if(x1<0) x1 = 0;
	else if(x1>D_WIDTH-1) x1 = D_WIDTH-1;

	if(y0<0) y0 = 0;
	else if(y0>D_HEIGHT) y0 = D_HEIGHT-1;
	if(y1<0) y1 = 0;
	else if(y1>D_HEIGHT) y1 = D_HEIGHT-1;

 	transparency &= 0xff;
	rgb &= 0x00ffffff;
	rgb += transparency << 24;
//either set constant colour or vertex colours
	pie_SetColour(rgb);
	if (transparency == 0 )
	{
		rgb += (32 << 24);
		pie_SetColour(rgb);
		pie_SetBilinear(FALSE);
		pie_SetRendMode(REND_FILTER_FLAT);
	}
	else if (transparency < 255)
	{
		pie_SetRendMode(REND_ALPHA_FLAT);
	}
	else
	{
		pie_SetRendMode(REND_FLAT);
	}
	pie_SetFogStatus(FALSE);

	vtxs[0].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[0].x = (float)x0 + SNAP_BIAS + 0.5f;
	vtxs[0].y = (float)y0 + SNAP_BIAS + 0.5f;

	vtxs[1].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[1].x = (float)x1 + SNAP_BIAS + 0.5f;
	vtxs[1].y = (float)y0 + SNAP_BIAS + 0.5f;

	vtxs[2].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[2].x = (float)x1 + SNAP_BIAS + 0.5f;
	vtxs[2].y = (float)y1 + SNAP_BIAS + 0.5f;

	vtxs[3].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[3].x = (float)x0 + SNAP_BIAS + 0.5f;
	vtxs[3].y = (float)y1 + SNAP_BIAS + 0.5f;

 	vtxs[0].a = vtxs[1].a = vtxs[2].a = vtxs[3].a = 0.0f;
	vtxs[0].r = vtxs[1].r = vtxs[2].r = vtxs[3].r = 0.0f;
	vtxs[0].g = vtxs[1].g = vtxs[2].g = vtxs[3].g = 0.0f;
	vtxs[0].b = vtxs[1].b = vtxs[2].b = vtxs[3].b = 0.0f;

	grDrawTriangle(&vtxs[0],&vtxs[1],&vtxs[2]);
	grDrawTriangle(&vtxs[0],&vtxs[2],&vtxs[3]);
}

/* ---------------------------------------------------------------------------------- */
void	gl_DrawViewingWindow(iVector *v,UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2,UDWORD colour)
{
GrVertex	vtx[4];

	pie_SetRendMode(REND_FILTER_ITERATED);
	pie_SetColour(colour);

   	vtx[0].x = (float) v[0].x;
	vtx[0].y = (float) v[0].y;
	vtx[1].x = (float) v[1].x;
	vtx[1].y = (float) v[1].y;
	vtx[2].x = (float) v[2].x;
	vtx[2].y = (float) v[2].y;
	vtx[3].x = (float) v[3].x;
	vtx[3].y = (float) v[3].y;


	vtx[0].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtx[1].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtx[2].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtx[3].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;


	vtx[0].a = vtx[1].a = 65.0f;
	vtx[2].a = vtx[3].a = 128.0f;
	vtx[0].r = vtx[1].r = vtx[2].r = vtx[3].r = 0.0f;
	vtx[0].g = vtx[1].g = vtx[2].g = vtx[3].g = 0.0f;
	vtx[0].b = vtx[1].b = vtx[2].b = vtx[3].b = 0.0f;
   
	grClipWindow((FxU32)x1,(FxU32)y1,(FxU32)x2,(FxU32)y2);

 	
	grDrawTriangle( &vtx[0], &vtx[1], &vtx[2] );
	grDrawTriangle( &vtx[1], &vtx[3], &vtx[2] );

	grClipWindow((FxU32)0,(FxU32)0,(FxU32)pie_GetVideoBufferWidth(),(FxU32)pie_GetVideoBufferHeight);

}
/* ---------------------------------------------------------------------------------- */
void	gl_TransColouredTriangle(PIEVERTEX *vrt, UDWORD rgb, UDWORD transparency)
{
GrVertex	vtx[3];
float		invZ;

/*
	if(vrt[0].sx<0 OR vrt[0].sx >= D_WIDTH-1 OR
	vrt[1].sx<0 OR vrt[1].sx >= D_WIDTH-1 OR
	vrt[2].sx<0 OR vrt[2].sx >= D_WIDTH-1 OR
	vrt[0].sy<0 OR vrt[0].sy >= D_HEIGHT-1 OR
	vrt[1].sy<0 OR vrt[1].sy >= D_HEIGHT-1 OR
	vrt[2].sy<0 OR vrt[1].sy >= D_HEIGHT-1)
	{
		return;
	}
*/

	transparency &= 0xff;
	rgb &= 0x00ffffff;
	rgb += transparency << 24;
//either set constant colour or vertex colours
	pie_SetColour(rgb);
	if (transparency == 0 )
	{
		return;
	}
	else if (transparency < 255)
	{
		pie_SetRendMode(REND_ALPHA_FLAT);
		pie_SetColour(rgb);
	}
	else
	{
		pie_SetRendMode(REND_FLAT);
	}

	vtx[0].x = (float) vrt[0].sx;
	vtx[0].y = (float) vrt[0].sy;
	vtx[1].x = (float) vrt[1].sx;
	vtx[1].y = (float) vrt[1].sy;
	vtx[2].x = (float) vrt[2].sx;
	vtx[2].y = (float) vrt[2].sy;

	invZ = 1.0f/vrt[0].sz;
	vtx[0].oow = OOW_SCALE*invZ;

	invZ = 1.0f/vrt[1].sz;
	vtx[1].oow = OOW_SCALE*invZ;
  
	invZ = 1.0f/vrt[2].sz;
	vtx[2].oow = OOW_SCALE*invZ;

	vtx[0].a = vtx[1].a = vtx[2].a = 0.0f;
	vtx[0].r = vtx[1].r = vtx[2].r = 0.0f;
	vtx[0].g = vtx[1].g = vtx[2].g = 0.0f;
	vtx[0].b = vtx[1].b = vtx[2].b = 0.0f;

	grDrawTriangle( &vtx[0], &vtx[1], &vtx[2] );
}
/* ---------------------------------------------------------------------------------- */
void	gl_TransColouredPolygon(UDWORD num, PIEVERTEX *vrt, UDWORD rgb, UDWORD transparency)
{
GrVertex	vtx[3];
float		invZ;
UDWORD		count;

/*
	if(vrt[0].sx<0 OR vrt[0].sx >= D_WIDTH-1 OR
	vrt[1].sx<0 OR vrt[1].sx >= D_WIDTH-1 OR
	vrt[2].sx<0 OR vrt[2].sx >= D_WIDTH-1 OR
	vrt[0].sy<0 OR vrt[0].sy >= D_HEIGHT-1 OR
	vrt[1].sy<0 OR vrt[1].sy >= D_HEIGHT-1 OR
	vrt[2].sy<0 OR vrt[1].sy >= D_HEIGHT-1)
	{
		return;
	}
*/

	transparency &= 0xff;
	rgb &= 0x00ffffff;
	rgb += transparency << 24;
//either set constant colour or vertex colours
	pie_SetColour(rgb);
	if (transparency == 0 )
	{
		rgb += (32 << 24);
		pie_SetColour(rgb);
		pie_SetBilinear(FALSE);
		pie_SetRendMode(REND_FILTER_FLAT);
	}
	else if (transparency < 255)
	{
		pie_SetRendMode(REND_ALPHA_FLAT);
		pie_SetColour(rgb);
	}
	else
	{
		pie_SetRendMode(REND_FLAT);
	}

	vtx[0].x = (float) vrt[0].sx;
	vtx[0].y = (float) vrt[0].sy;
	invZ = 1.0f/vrt[0].sz;
	vtx[0].oow = OOW_SCALE*invZ;

	vtx[1].x = (float) vrt[1].sx;
	vtx[1].y = (float) vrt[1].sy;
	invZ = 1.0f/vrt[1].sz;
	vtx[1].oow = OOW_SCALE*invZ;

	vtx[0].a = vtx[1].a = vtx[2].a = 0.0f;
	vtx[0].r = vtx[1].r = vtx[2].r = 0.0f;
	vtx[0].g = vtx[1].g = vtx[2].g = 0.0f;
	vtx[0].b = vtx[1].b = vtx[2].b = 0.0f;

	count = 2;
	while(count<num)
	{
			/* Is it even? */
		if( (count&0x1) == 0)
		{
			vtx[2].x = (float) vrt[count].sx;
			vtx[2].y = (float) vrt[count].sy;
		   	invZ = 1.0f/vrt[count].sz;
			vtx[2].oow = OOW_SCALE*invZ;
			grDrawTriangle( &vtx[0], &vtx[1], &vtx[2] );

		}
		else
		{
			vtx[1].x = (float) vrt[count].sx;
			vtx[1].y = (float) vrt[count].sy;
			invZ = 1.0f/vrt[count].sz;
			vtx[1].oow = OOW_SCALE*invZ;
			grDrawTriangle( &vtx[0], &vtx[1], &vtx[2] );
		}
	}


	  
   

	grDrawTriangle( &vtx[0], &vtx[1], &vtx[2] );
}


/* ---------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------------- */
void	gl_TransBoxFillCorners(SDWORD x0,SDWORD y0, SDWORD x1, SDWORD y1, 
							   UDWORD rgb, UBYTE trans1, UBYTE trans2, UBYTE trans3, UBYTE trans4)
{
GrVertex	vtxs[4];

	if(x0<0) x0 = 0;
	else if(x0>D_WIDTH-1) x0 = D_WIDTH-1;
	if(x1<0) x1 = 0;
	else if(x1>D_WIDTH-1) x1 = D_WIDTH-1;

	if(y0<0) y0 = 0;
	else if(y0>D_HEIGHT) y0 = D_HEIGHT-1;
	if(y1<0) y1 = 0;
	else if(y1>D_HEIGHT) y1 = D_HEIGHT-1;

	pie_SetFogStatus(FALSE);

	pie_SetRendMode(REND_FILTER_ITERATED);
	pie_SetColour(rgb);

	vtxs[0].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[0].x = (float)x0 + SNAP_BIAS + 0.5f;
	vtxs[0].y = (float)y0 + SNAP_BIAS + 0.5f;

	vtxs[1].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[1].x = (float)x1 + SNAP_BIAS + 0.5f;
	vtxs[1].y = (float)y0 + SNAP_BIAS + 0.5f;

	vtxs[2].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[2].x = (float)x1 + SNAP_BIAS + 0.5f;
	vtxs[2].y = (float)y1 + SNAP_BIAS + 0.5f;

	vtxs[3].oow = 1.0f*INV_INTERFACE_DEPTH_3DFX;
	vtxs[3].x = (float)x0 + SNAP_BIAS + 0.5f;
	vtxs[3].y = (float)y1 + SNAP_BIAS + 0.5f;

	vtxs[0].a = trans1;
	vtxs[1].a = trans2;
	vtxs[2].a = trans3;
	vtxs[3].a = trans4;
	vtxs[0].r = vtxs[1].r = vtxs[2].r = vtxs[3].r = 0.0f;
	vtxs[0].g = vtxs[1].g = vtxs[2].g = vtxs[3].g = 0.0f;
	vtxs[0].b = vtxs[1].b = vtxs[2].b = vtxs[3].b = 0.0f;

	grDrawTriangle(&vtxs[0],&vtxs[1],&vtxs[2]);
	grDrawTriangle(&vtxs[0],&vtxs[2],&vtxs[3]);
}

/* ---------------------------------------------------------------------------------- */

void gl_BufferTo3dfx(void *srcData, UDWORD destX, UDWORD destY,UDWORD srcWidth,UDWORD srcHeight,UDWORD srcStride)
{
	grLfbWriteRegion(GR_BUFFER_BACKBUFFER, destX, destY, GR_LFB_SRC_FMT_565, srcWidth, srcHeight, srcStride, srcData);
}

/* ---------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------------- */

void gl_SetFogStatus(BOOL	val)
{
	if(val)
	{
		grFogMode(GR_FOG_WITH_ITERATED_ALPHA);
	}
	else
	{
		grFogMode(GR_FOG_DISABLE);
	}
}

void gl_SetFogColour(UDWORD color)
{
	static currentFogColour = 0;
	if (color != currentFogColour)
	{
		screenClearColour = color;
		grFogColorValue(color);
		currentFogColour = color;
	}
}

/* ---------------------------------------------------------------------------------- */
/* 
	Down loads the radar buffer to 3dfx Texture memory 
	For the time being - dump it into page 31 until we decide where it's going
*/
/* ---------------------------------------------------------------------------------- */
void gl_DownLoadRadar(unsigned char *buffer, UDWORD texPageID)
{
GrTexInfo	info;

	info.smallLod = GR_LOD_128;
	info.largeLod = GR_LOD_128;
	info.aspectRatio = GR_ASPECT_1x1;
	info.format = GR_TEXFMT_P_8;
	info.data = buffer;

	/* Download texture data to TMU */
	grTexDownloadMipMap( GR_TMU0,
                        texPageID*(256*256),
                        GR_MIPMAPLEVELMASK_BOTH,
                        &info );
}

/* ---------------------------------------------------------------------------------- */
/* 
	Draws the radar to the screen as two tri plots. Could also be done
	by using intelBitmap call above
*/
/* ---------------------------------------------------------------------------------- */
#ifdef USE_FOG_TABLE //unused code

void gl_SetFogTable(UDWORD color, UDWORD zMin, UDWORD zMax)
{
GrFog_t fogTable[GR_FOG_TABLE_SIZE];
SDWORD depth;

	screenClearColour = color;
	grFogMode(GR_FOG_WITH_TABLE);
	grFogColorValue(color);
#define CUSTOM_FOG TRUE
#if CUSTOM_FOG
	for(depth = 0;depth <= 52;depth++)
	{
		fogTable[depth] = 0;
	}
//
//	fogTable[53] = 20;// original fog table to much for ground fog
//	fogTable[54] = 40;
//	fogTable[55] = 70;
//	fogTable[56] = 105;
//	fogTable[57] = 145;
//	fogTable[58] = 195;
//
	fogTable[53] = 0;
	fogTable[54] = 0;
	fogTable[55] = 0;
	fogTable[56] = 3;//fog set to minimum increment of 63 per buffer
	fogTable[57] = 66;
	fogTable[58] = 129;
	fogTable[59] = 192;
	for(depth = 60;depth < GR_FOG_TABLE_SIZE;depth++)
	{
		fogTable[depth] = 255;
	}
#else
//	guFogGenerateLinear(fogTable,(float)zMin,(float)zMax);
#endif

	grFogTable(fogTable);
}
#endif

//unused


/* Will rotate the vectors for you */
void rotateVector2D(iVector *Vector,iVector *TVector,iVector *Pos,int Angle,int Count)
{
	int Cos = COS(Angle);
	int Sin = SIN(Angle);
	int ox = 0;
	int oy = 0;
	int i;
	iVector *Vec = Vector;
	iVector *TVec = TVector;

	if(Pos) {
		ox = Pos->x;
		oy = Pos->y;
	}

	for(i=0; i<Count; i++) {
		TVec->x = ( (Vec->x*Cos + Vec->y*Sin) >> FP12_SHIFT ) + ox;
		TVec->y = ( (Vec->y*Cos - Vec->x*Sin) >> FP12_SHIFT ) + oy;
		Vec++;
		TVec++;
	}
}


/* ---------------------------------------------------------------------------------- */
/* 
	Screen Dump Function 
	Just dumps a 3dfx screen shot out to disk. (Using CURRENT PATH as destination) 
	cycles thru' the names dump0.raw to dump19.raw.
	The files are just 24 bit raw (640*480*3) images - extracted from
	the 3dfx frame buffer and 5-6-5 colour. They're a bit dark - need to scale up a
	bit or look into gamma correction?
*/
/* ---------------------------------------------------------------------------------- */

char *gl_ScreenDump(void)
{
FILE	*fp;
UWORD	bufPixel;
UDWORD	i;
UWORD	*buffer;
UBYTE	*rgbBuffer;
char	*filenum[20];
BLOCK_HEAP	*psHeap;
UDWORD	width,height;

	width = pie_GetVideoBufferWidth();
	height = pie_GetVideoBufferHeight();

	/* 16 bit 640*480 screen surface */
	psHeap = memGetBlockHeap();
	memSetBlockHeap(NULL);
	buffer = (UWORD *)MALLOC(width*height*2);
	rgbBuffer = (UBYTE *)MALLOC(width*height*3);

	if(!buffer OR !rgbBuffer)
	{
		DBERROR(("Can't get memory for the screen dump buffers!"));		
	}

	if(!grLfbReadRegion(GR_BUFFER_FRONTBUFFER,0,0,width,height,width*2,buffer)) 
	{
		DBERROR(("Can't do the screen dump!!"));
		return(NULL);
	}
	else
	{
		for(i=0; i<width*height; i++)
		{
			bufPixel = (UWORD)buffer[i];
			/* five bits */
			rgbBuffer[i*3+0] = (UBYTE)(((bufPixel&0xf800)>>11)<<3);
			/* six bits for green */
			rgbBuffer[i*3+1] = (UBYTE)(((bufPixel&0x7e0)>>5)<<2);
			/* five bits */
			rgbBuffer[i*3+2] = (UBYTE)((bufPixel&0x1f)<<3);
		}
		
		
		strcpy((char*)filename,"dump");
		sprintf((char*)filenum,"%d",frameNumber);
		strcat((char*)filename,(char*)filenum);
		strcat((char*)filename,".raw");
		if(frameNumber++ >= 20)
		{
			frameNumber = 0;
		}

		if((fp = fopen((char*)filename,"wb"))==NULL)
		{
			DBERROR(("Can't open the 3dfx screen dump file for writing"));
			return(NULL);
		}
		else
		{
			if(fwrite((void*)rgbBuffer,width*height*3,1,fp)!=1)
			{
				DBERROR(("Can't write out the 3dfx screen dump file"));
				return(NULL);
			}
		}
	}
	fclose(fp);
	FREE(buffer);
	FREE(rgbBuffer);
	memSetBlockHeap(psHeap);

	return((char*)filename);
}





/* Sets the 3dfx gamma value */
void gl_SetGammaValue(float val)
{
	grGammaCorrectionValue(val);
}

/* ---------------------------------------------------------------------------------- */
/* Textured polygon - not shaded - no point in implementing for 3dfx? */
/* ---------------------------------------------------------------------------------- */
void gl_tPolygon(int num, iVertex *vrt, iTexture *tex)
{return;}

/* --------------------------------------------------------------------------- */
/* Standard bitmap function - not implemented - not called ? */
/* --------------------------------------------------------------------------- */
void gl_Bitmap(iBitmap *bmp, int x, int y, int w, int h)
{return;}

/* ----------------------------------------------------------------------------------- */
/* This isn't necessary for the 3dfx so leave empty */
/* ----------------------------------------------------------------------------------- */
void gl_SetTransFilter(UDWORD rgb,UDWORD tablenumber)
{return;}

/* ---------------------------------------------------------------------------------- */
/* Are these ones even used? NO jps*/
/* ----------------------------------------------------------------------------------- */
void gl_Quad(iVertex *vrt, iTexture *tex, uint32 type)
{return;}
void gl_pBitmapResize(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)
{return;}
void gl_pBitmapResizeRot90(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)
{return;}
void gl_pBitmapResizeRot180(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)
{return;}
void gl_pBitmapResizeRot270(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)
{return;}
void gl_pBitmapGet(iBitmap *bmp, int x, int y, int w, int h)
{return;}
void gl_pBitmapShadow(iBitmap *bmp, int x, int y, int w, int h)
{return;}
void gl_ppBitmapShadow(iBitmap *bmp, int x, int y, int w, int h, int ow)
{return;}
void gl_ppBitmapRot90(iBitmap *bmp, int x, int y, int w, int h, int ow)
{return;}
void gl_pBitmapRot90(iBitmap *bmp, int x, int y, int w, int h)
{return;}
void gl_ppBitmapRot180(iBitmap *bmp, int x, int y, int w, int h, int ow)
{return;}
void gl_pBitmapRot180(iBitmap *bmp, int x, int y, int w, int h)
{return;}
void gl_ppBitmapRot270(iBitmap *bmp, int x, int y, int w, int h, int ow)
{return;}
void gl_pBitmapRot270(iBitmap *bmp, int x, int y, int w, int h)
{return;}
void gl_pPolygon(int npoints, iVertex *vrt, iTexture *tex, uint32 type)
{return;}
void gl_pTriangle(iVertex *vrt, iTexture *tex, uint32 type)
{return;}
void gl_pQuad(iVertex *vrt, iTexture *tex, uint32 type)
{return;}
void gl_tTriangle(iVertex *vrt, iTexture *tex)
{return;}
void gl_pPolygon3D(void)
{return;}
void gl_pLine3D(void)
{return;}
void gl_pPixel3D(void)
{return;}
void gl_pTriangle3D(void)
{return;}
void gl_Pixel(int x, int y, uint32 colour)
{return;}
void gl_HLine(int x0, int x1, int y, uint32 colour)
{return;}
void gl_VLine(int y0, int y1, int x, uint32 colour)
{return;}
void gl_Circle(int x, int y, int r, uint32 colour)
{return;}
void gl_CircleFill(int x, int y, int r, uint32 colour)
{return;}
void gl_VSync(void)
{return;}
void gl_Palette(int i, int r, int g, int b)
{return;}
void gl_pPixel(int x, int y, uint32 colour)
{return;}
void gl_pLine(int x0, int y0, int x1, int y1, uint32 colour)
{return;}
void gl_pHLine(int x0, int x1, int y,uint32 colour)
{return;}
void gl_pVLine(int y0, int y1, int x, uint32 colour)
{return;}
void gl_pCircle(int x, int y, int r, uint32 colour)
{return;}
void gl_pCircleFill(int x, int y, int r, uint32 colour)
{return;}
void gl_pBox(int x0, int y0, int x1, int y1, uint32 colour)
{return;}
void gl_pBoxFill(int x0, int y0, int x1, int y1, uint32 colour)
{return;}
void gl_BitmapResize(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)
{return;}
void gl_BitmapResizeRot90(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)
{return;}
void gl_BitmapResizeRot180(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)
{return;}
void gl_BitmapResizeRot270(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)
{return;}
void gl_BitmapGet(iBitmap *bmp, int x, int y, int w, int h)
{return;}
void gl_BitmapTrans(iBitmap *bmp, int x, int y, int w, int h)
{return;}
void gl_BitmapShadow(iBitmap *bmp, int x, int y, int w, int h)
{return;}
void gl_BitmapRot90(iBitmap *bmp, int x, int y, int w, int h)
{return;}
void gl_BitmapRot180(iBitmap *bmp, int x, int y, int w, int h)
{return;}
void gl_BitmapRot270(iBitmap *bmp, int x, int y, int w, int h)
{return;}
void gl_RenderEnd(void)
{return;}
void gl_RenderBegin(void)
{return;}
void gl_Clear(uint32 colour)
{return;}


//called from draw radar as iV_DrawImageDef
void gl_DrawImageDef(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y)
{
	pie_SetBilinear(TRUE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);

	gl_IntelBitmap(Image->TPageID, Image->Tu, Image->Tv, Image->Width, Image->Height, 
					x+Image->XOffset, y+Image->YOffset, Image->Width, Image->Height);
	return;
}


void gl_DrawImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y)
{
	IMAGEDEF *Image;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	pie_SetBilinear(TRUE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);

	gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID], Image->Tu, Image->Tv, Image->Width, Image->Height, 
					x+Image->XOffset, y+Image->YOffset, Image->Width, Image->Height);
}

//called form RenderWindow as gl_IntelBitmap as iV_DrawTransImage
//3dfx blit func
void gl_DrawTransImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y)
{
	IMAGEDEF *Image;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	pie_SetBilinear(TRUE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);

	gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID], Image->Tu, Image->Tv, Image->Width, Image->Height, 
					x+Image->XOffset, y+Image->YOffset, Image->Width, Image->Height);
}

//called form RenderWindow as gl_IntelBitmap
//3dfx blit func
void gl_DrawImageRect(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height)
{
	UDWORD RepWidth = 1;
	UDWORD RepWidthRemain = 0;
	UDWORD RepHeight = 1;
	UDWORD RepHeightRemain = 0;
	BOOL Tiled = FALSE;
	UDWORD w,h;
	UDWORD tx,ty;

	IMAGEDEF *Image;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	if(Width + x0 > Image->Width) {
		RepWidth = Width / Image->Width;
		RepWidthRemain = Width - RepWidth * Image->Width;
		Width = Image->Width;
		Tiled = TRUE;
	}

	if(Height+ y0 > Image->Height) {
		RepHeight = Height / Image->Height;
		RepHeightRemain = Height - RepHeight * Image->Height;
		Height = Image->Height;
		Tiled = TRUE;
	}


	pie_SetBilinear(TRUE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);

	if(Tiled) {
		ty = y;
		for(h=0; h<RepHeight; h++) {
			tx = x;
			for(w=0; w<RepWidth; w++) {
				gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID],
								Image->Tu, Image->Tv,
								Width, Height, 
								tx+Image->XOffset,ty+Image->YOffset,
								Width, Height);
				tx += Width;
			}
			if(RepWidthRemain) {
				gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID],
								Image->Tu, Image->Tv,
								RepWidthRemain,Height,
								tx+Image->XOffset,ty+Image->YOffset,
								RepWidthRemain,Height);
			}
			ty += Height;
		}

		if(RepHeightRemain) {
			tx = x;
			for(w=0; w<RepWidth; w++) {
				gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID],
								Image->Tu, Image->Tv,
								Width,RepHeightRemain,
								tx+Image->XOffset,ty+Image->YOffset,
								Width,RepHeightRemain);
				tx += Width;
			}
			if(RepWidthRemain) {
				gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID],
								Image->Tu, Image->Tv,
								RepWidthRemain,RepHeightRemain,
								tx+Image->XOffset,ty+Image->YOffset,
								RepWidthRemain,RepHeightRemain);
			}
		}
	} else {
		gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID],
						Image->Tu, Image->Tv,
						Width,Height,
						x+Image->XOffset,y+Image->YOffset,
						Width,Height);
	}
}


void gl_DrawTransImageRect(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height)
{
	gl_DrawImageRect(ImageFile,ID,x,y,x0,y0,Width,Height);
}


void gl_DrawTransColourImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y,SWORD ColourIndex)
{
	IMAGEDEF *Image;
	UDWORD Red;
	UDWORD Green;
	UDWORD Blue;
	UDWORD Alpha = MAX_UB_LIGHT;
	iColour* psPalette = pie_GetGamePal();

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	pie_SetBilinear(FALSE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	if (ColourIndex == PIE_TEXT_WHITE)
	{
		pie_SetColour((GrColor_t)PIE_TEXT_WHITE_COLOUR);
	}
	else if (ColourIndex == PIE_TEXT_LIGHTBLUE)
	{
		pie_SetColour((GrColor_t)PIE_TEXT_LIGHTBLUE_COLOUR);
	}
	else if (ColourIndex == PIE_TEXT_DARKBLUE)
	{
		pie_SetColour((GrColor_t)PIE_TEXT_DARKBLUE_COLOUR);
	}
	else 
	{
		Red  = psPalette[ColourIndex].r;
		Green= psPalette[ColourIndex].g;
		Blue = psPalette[ColourIndex].b;
		pie_SetColour((GrColor_t)((Alpha<<24) | (Red<<16) | (Green<<8) | Blue));
	}

	gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID], Image->Tu, Image->Tv, Image->Width, Image->Height, 
					x+Image->XOffset, y+Image->YOffset, Image->Width, Image->Height);
}

void gl_DrawStretchImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int Width,int Height)
{
	IMAGEDEF *Image;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	pie_SetBilinear(TRUE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);
	gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID], Image->Tu, Image->Tv, Image->Width, Image->Height, 
					x+Image->XOffset, y+Image->YOffset, Width, Height);
}



static GrState TextState;

//called form iV_drawtest as iV_textrender
//3dfx blit func
void gl_TextRender(IMAGEFILE *ImageFile,UWORD ID,int x,int y)
{
	IMAGEDEF *Image;
	assert(ID < ImageFile->Header.NumImages);

	Image = &ImageFile->ImageDefs[ID];
	x += Image->XOffset;
	y += Image->YOffset;
	gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID], //  int texPage, 		   
			   Image->Tu,			//u	
			   Image->Tv,			//v
			   Image->Width,		//w
			   Image->Height,		//h
			   x,					//destx
			   y,					//desty
			   Image->Width,		//destw
			   Image->Height);		//desth
}

void gl_TextRender270(IMAGEFILE *ImageFile,UWORD ID,int x,int y)
{
	IMAGEDEF *Image;
	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	y -= iV_GetImageWidth(ImageFile,ID);

	x += Image->YOffset;
	y += Image->XOffset;

	//set current transparency to half(128) for all 270 text
	pie_SetColour(0x80ffffff);
	pie_SetRendMode(REND_ALPHA_TEXT);

	gl_IntelBitmap270(ImageFile->TPageIDs[Image->TPageID], //  int texPage, 		   
			   Image->Tu,			//u	
			   Image->Tv,			//v
			   Image->Width,		//w
			   Image->Height,		//h
			   x,					//destx
			   y,					//desty
			   Image->Height,		//destw
			   Image->Width);		//desth
}


// Upload the current display back buffer into system memory.
//
void gl_UploadDisplayBuffer(UBYTE *DisplayBuffer)
{
UDWORD	width,height,stride;

	width = pie_GetVideoBufferWidth();
	stride = width*2;
	height = pie_GetVideoBufferHeight();

	if(!grLfbReadRegion(GR_BUFFER_BACKBUFFER,0,0,width,height,stride,DisplayBuffer)) {
		DBERROR(("Unable to upload display buffer!"));
	}
}


// Download buffer in system memory to the display back buffer.
//
void gl_DownloadDisplayBuffer(UWORD *DisplayBuffer)
{
//UDWORD	xOffset,yOffset;
UDWORD	width,height,stride;

	width = pie_GetVideoBufferWidth();
	stride = width*2;
	height = pie_GetVideoBufferHeight();

//	xOffset = (pie_GetVideoBufferWidth() - BACKDROP_WIDTH)/2;
//	yOffset = (pie_GetVideoBufferHeight() - BACKDROP_HEIGHT)/2;

	if(!grLfbWriteRegion(GR_BUFFER_BACKBUFFER,0,0,GR_LFB_SRC_FMT_565,width,height,stride,DisplayBuffer)) {
		DBERROR(("Unable to download display buffer!"));
	}
}

// additional function - similiar one needed for D3D and software
void gl_Download640Buffer(UWORD *DisplayBuffer)
{
UDWORD	xOffset,yOffset;

	xOffset = (pie_GetVideoBufferWidth() - BACKDROP_WIDTH)/2;
	yOffset = (pie_GetVideoBufferHeight() - BACKDROP_HEIGHT)/2;

	if(!grLfbWriteRegion(GR_BUFFER_BACKBUFFER,xOffset,yOffset,GR_LFB_SRC_FMT_565,640,480,1280,DisplayBuffer)) {
		DBERROR(("Unable to download display buffer!"));
	}
}


// Scale a bitmaps colour. Assumes 3dfx display pixel format.
//
void gl_ScaleBitmapRGB(UBYTE *DisplayBuffer,int Width,int Height,int ScaleR,int ScaleG,int ScaleB)
{
#if(1)
// Optimized version, well fast but only divides by 2.
//	UDWORD *Buffer = (UDWORD*)DisplayBuffer;
//	int i;
//	int Size = (Width/2)*Height;
//
//   	for(i=0; i<Size; i++) {
//		*Buffer = (*Buffer>>1) & 0x7bef7bef;
//		Buffer++;
//   	}
#else
// Slow version, works fine but takes around a second for a 640x480 bitmap.
	UWORD *Buffer = (UWORD*)DisplayBuffer;
	UWORD Pixel;
	float r,g,b;
	int i;

   	for(i=0; i<Width*Height; i++) {
   		Pixel = *Buffer;

   		r = (float)((Pixel & 0xf800) >> 11);
   		g = (float)((Pixel & 0x7e0) >> 5);
   		b = (float)(Pixel & 0x1f);

		r = r / (float)ScaleR;
		g = g / (float)ScaleG;
		b = b / (float)ScaleB;

		Pixel = (((UWORD)r) & 0x1f) << 11;
		Pixel |= (((UWORD)g) & 0x3f) << 5;
		Pixel |= (((UWORD)b) & 0x1f);

		*Buffer = Pixel;
		Buffer++;
   	}
#endif
}

#endif

