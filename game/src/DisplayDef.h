/*
 * DisplayDef.h
 *
 * Definitions of the display structures
 *
 */
#ifndef _displaydef_h
#define _displaydef_h

#include "imd.h"
#ifdef WIN32			// ffs am
#include "pieClip.h"
#endif
#ifdef WIN32
#define DISP_WIDTH		(pie_GetVideoBufferWidth()) 
#define DISP_HEIGHT		(pie_GetVideoBufferHeight())
#else
#define DISP_WIDTH		(640)
#define DISP_HEIGHT		(480)
#endif
#define DISP_HARDBITDEPTH	(16)
#define DISP_BITDEPTH	(8)
#ifdef WIN32
#define	BOUNDARY_X			(16)
#define BOUNDARY_Y			(16)
//#define BOUNDARY_X		(DISP_WIDTH/20)	   // proportional to resolution - Alex M
//#define	BOUNDARY_Y		(DISP_WIDTH/16)
//#define	BOUNDARY_X		(24)
//#define	BOUNDARY_Y		(24)
#else	// PSX version.
#define	BOUNDARY_X		(32)
#define	BOUNDARY_Y		(32)
#endif

#ifdef WIN32
typedef struct _screen_disp_data
{
	//UDWORD		dummy;
	iIMDShape	*imd;
//	BOOL		drawnThisFrame;		// for sorting - have we drawn the imd already?
	UDWORD		frameNumber;		// last frame it was drawn
//	UDWORD		animFrame;			// anim Frame
//	SDWORD		bucketDepth;
//	BOOL		onScreen;
//	UDWORD		numTiles;
	UDWORD		screenX,screenY;
	UDWORD		screenR; 
} SCREEN_DISP_DATA;

#else		// playstation uses the cutdown version
typedef struct _screen_disp_data
{
	iIMDShape	*imd;
	UDWORD		frameNumber;
	UWORD		screenX,screenY;
	UWORD		screenR; 
} SCREEN_DISP_DATA;
#endif


#endif

