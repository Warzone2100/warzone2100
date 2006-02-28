/***************************************************************************/
/*
 * pieMode.h
 *
 * renderer control for pumpkin library functions.
 *
 */
/***************************************************************************/

#include <SDL/SDL.h>
#ifdef _MSC_VER
#include <windows.h>  //needed for gl.h!  --Qamly
#endif
#include <GL/gl.h>

#include "frame.h"
#include "piedef.h"
#include "piestate.h"
#include "piemode.h"
#include "piematrix.h"
#include "piefunc.h"
#include "tex.h"
#include "rendmode.h"
#include "pieclip.h"
#include "screen.h"

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/
#define DIVIDE_TABLE_SIZE		1024
/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

int32		_iVPRIM_DIVTABLE[DIVIDE_TABLE_SIZE];


//static BOOL fogColourSet = FALSE;
//static SDWORD d3dActive = 0;
static BOOL bDither = FALSE;

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/
extern void screenDoDumpToDiskIfRequired();

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

BOOL	pie_GetDitherStatus( void )
{
	return bDither;
}

void	pie_SetDitherStatus( BOOL val )
{
	bDither = val;
}

BOOL pie_Initialise(SDWORD mode)
{
//	BOOL r;//result
	int i;

	pie_InitMaths();
	pie_TexInit();

	pie_SetRenderEngine(ENGINE_UNDEFINED);
	rendSurface.usr = REND_UNDEFINED;
	rendSurface.flags = REND_SURFACE_UNDEFINED;
	rendSurface.buffer = NULL;
	rendSurface.size = 0;

	// divtable: first entry == unity to (ie n/0 == 1 !)
	_iVPRIM_DIVTABLE[0] = iV_DIVMULTP;

	for (i=1; i<DIVIDE_TABLE_SIZE; i++)
	{
		_iVPRIM_DIVTABLE[i-0] = MAKEINT ( FRACTdiv(MAKEFRACT(1),MAKEFRACT(i)) *  iV_DIVMULTP);
	}

	pie_MatInit();
	_TEX_INDEX = 0;

	rendSurface.buffer	= 0;
	rendSurface.flags	= REND_SURFACE_SCREEN;
	rendSurface.width	= pie_GetVideoBufferWidth();
	rendSurface.height	= pie_GetVideoBufferHeight();
	rendSurface.xcentre	= pie_GetVideoBufferWidth()/2;
	rendSurface.ycentre	= pie_GetVideoBufferHeight()/2;
	rendSurface.clip.left	= 0;
	rendSurface.clip.top	= 0;
	rendSurface.clip.right	= pie_GetVideoBufferWidth();
	rendSurface.clip.bottom	= pie_GetVideoBufferHeight();
	rendSurface.xpshift	= 10;
	rendSurface.ypshift	= 10;

	pie_SetRenderEngine(ENGINE_OPENGL); //was ENGINE_D3D -Q

	pie_SetDefaultStates();
	iV_RenderAssign(mode,&rendSurface);
	pal_Init();

	return TRUE;
}


void pie_ShutDown(void) {
	rendSurface.buffer = NULL;
	rendSurface.flags = REND_SURFACE_UNDEFINED;
	rendSurface.usr = REND_UNDEFINED;
	rendSurface.size = 0;

	pie_SetRenderEngine(ENGINE_UNDEFINED);

	pie_CleanUp();
}

/***************************************************************************/

void pie_ScreenFlip(CLEAR_MODE clearMode) {
	PIELIGHT fog_colour;

	screenDoDumpToDiskIfRequired();
	SDL_GL_SwapBuffers();
	switch (clearMode) {
		case CLEAR_OFF_AND_NO_BUFFER_DOWNLOAD:
			break;
		case CLEAR_BLACK:
		default:
			glDepthMask(GL_TRUE);
			fog_colour.argb = pie_GetFogColour();
			glClearColor(fog_colour.byte.r/255.0,
				     fog_colour.byte.g/255.0,
				     fog_colour.byte.b/255.0,
				     fog_colour.byte.a/255.0);
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			break;
	}

	if (screen_GetBackDrop()) {
		screen_Upload(NULL);
	}
}

/***************************************************************************/

void pie_Clear(UDWORD colour) {
}
/***************************************************************************/

void pie_GlobalRenderBegin(void) {
}

void pie_GlobalRenderEnd(BOOL bForceClearToBlack) {
}

/***************************************************************************/
UDWORD	pie_GetResScalingFactor( void ) {
//	UDWORD	resWidth;	//n.b. resolution width implies resolution height...!

	if (pie_GetVideoBufferWidth() * 4 > pie_GetVideoBufferHeight() * 5) {
		return pie_GetVideoBufferHeight()*5/4/6;
	} else {
		return pie_GetVideoBufferWidth()/6;
	}
}

/***************************************************************************/
void pie_LocalRenderBegin(void) {
}

void pie_LocalRenderEnd(void) {
}


void pie_RenderSetup(void) {
}
