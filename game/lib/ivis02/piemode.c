/***************************************************************************/
/*
 * pieMode.h
 *
 * renderer control for pumpkin library functions.
 *
 */
/***************************************************************************/

#include "frame.h"
#include "piedef.h"
#include "pieState.h"
#include "pieMode.h"
#include "pieMatrix.h"
#include "pieFunc.h"
#include "tex.h"
#include "d3dmode.h"
#include "v4101.h"
#include "vSR.h"
#include "3dfxFunc.h"
#include "texd3d.h"
#include "rendmode.h"
#include "Pieclip.h"

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
#ifdef WIN32
	int32		_iVPRIM_DIVTABLE[DIVIDE_TABLE_SIZE];
#endif

static BOOL fogColourSet = FALSE;
static SDWORD d3dActive = 0;
static	BOOL	bDither = FALSE;

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/
//okay just this once
extern void GetDXVersion(LPDWORD pdwDXVersion, LPDWORD pdwDXPlatform);

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/


BOOL	pie_GetDitherStatus( void )
{
	return(bDither);
}

void	pie_SetDitherStatus( BOOL val )
{
	bDither = val;
}

BOOL pie_CheckForDX6(void)
{
	UDWORD	DXVersion, DXPlatform;

	GetDXVersion(&DXVersion, &DXPlatform);

	return (DXVersion >= 0x600);
}

BOOL pie_Initialise(SDWORD mode)
{
	BOOL r;//result
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

	//mode specific initialisation
	if (mode == REND_GLIDE_3DFX)
	{
		pie_SetRenderEngine(ENGINE_GLIDE);
		r = gl_VideoOpen();
#if 1 //FOG ON from Start
		pie_EnableFog(TRUE);
		pie_SetFogColour(0x00B08f5f);//nicks colour
#endif
	}
	else if (mode == REND_D3D_HAL)
	{
		iV_RenderAssign(REND_D3D_HAL,&rendSurface);
		pie_SetRenderEngine(ENGINE_D3D);
		rendSurface.usr = mode;
		r = _mode_D3D_HAL();
	}
	else if (mode == REND_D3D_REF)
	{
		iV_RenderAssign(REND_D3D_REF,&rendSurface);
		pie_SetRenderEngine(ENGINE_D3D);
		rendSurface.usr = mode;
		r = _mode_D3D_REF();
	}
	else if (mode == REND_D3D_RGB)
	{
		iV_RenderAssign(REND_D3D_RGB,&rendSurface);
		pie_SetRenderEngine(ENGINE_D3D);
		rendSurface.usr = mode;
		r = _mode_D3D_RGB();
	}
	else//REND_MODE_SOFTWARE
	{
		pie_SetRenderEngine(ENGINE_4101);
		r = _mode_4101();	// we always want success as jon's stuff does the init
	}

#ifdef WIN32
	if (r)
	{
		pie_SetDefaultStates();
	}
#endif

	if (r)
	{
		iV_RenderAssign(mode,&rendSurface);
		pal_Init();
	}
	else
	{
		
		iV_ShutDown();
		DBERROR(("Initialise videomode failed"));
		return FALSE;
	}
	return TRUE;
}


void pie_ShutDown(void)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
		_close_4101();
		break;
	case ENGINE_SR:
		_close_sr();
		break;
	case ENGINE_GLIDE:
		gl_VideoClose();
		break;
	case ENGINE_D3D:
		_close_D3D();
		break;
	default:
		break;
	}
	pie_SetRenderEngine(ENGINE_UNDEFINED);
}

/***************************************************************************/

void pie_ScreenFlip(CLEAR_MODE clearMode)
{
	UWORD * backDrop;
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
			if (clearMode == CLEAR_OFF OR clearMode == CLEAR_OFF_AND_NO_BUFFER_DOWNLOAD)
			{
				screenFlip(FALSE);//automatically downloads active backdrop and never fogs
			}
			else
			{
				screenFlip(TRUE);//automatically downloads active backdrop and never fogs
			}
		break;
	case ENGINE_D3D:
		pie_D3DRenderForFlip();

		switch (clearMode)
		{
		case CLEAR_OFF:
		case CLEAR_OFF_AND_NO_BUFFER_DOWNLOAD:
			screenFlip(FALSE);
			break;
		case CLEAR_FOG:
			if (pie_GetFogEnabled())
			{
				screen_SetFogColour(pie_GetFogColour());
			}
			else
			{
				screen_SetFogColour(0);
			}
			screenFlip(TRUE);
			break;
		case CLEAR_BLACK:
		default:
			screen_SetFogColour(0);
			screenFlip(TRUE);
			break;
		}
		break;
	case ENGINE_GLIDE:
		if (clearMode == CLEAR_OFF OR clearMode == CLEAR_OFF_AND_NO_BUFFER_DOWNLOAD)
		{
			gl_ScreenFlip(FALSE,TRUE);
		}
		else if (clearMode == CLEAR_FOG)
		{
			backDrop = screen_GetBackDrop();
			if (backDrop != NULL)
			{
				gl_ScreenFlip(TRUE,TRUE);
	   		}
			else
			{
				gl_ScreenFlip(TRUE,FALSE);
			}
		}
		else
		{
			pie_SetFogStatus(FALSE);
			gl_ScreenFlip(TRUE,TRUE);
		}
		
		backDrop = screen_GetBackDrop();

		if (backDrop != NULL AND clearMode!=CLEAR_OFF_AND_NO_BUFFER_DOWNLOAD)
		{
			if (screen_GetBackDropWidth() == 640)
			{
		   		gl_Download640Buffer(backDrop);	// note the change!!! (centered)
			}
			else
			{
		   		gl_DownloadDisplayBuffer(backDrop);	// note the change!!! (centered)
			}
		}
		break;
	case ENGINE_SR:
	default:
		break;
	}
}

/***************************************************************************/

void pie_Clear(UDWORD colour)
{
#ifndef PIEPSX    // Arse	
	switch (pie_GetRenderEngine())
	{
	case ENGINE_SR:
		_clear_sr(colour);
		break;
	case ENGINE_4101:
	case ENGINE_D3D:
	case ENGINE_GLIDE:
	default:
		break;
	}
#endif
}
/***************************************************************************/

void pie_GlobalRenderBegin(void)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_GLIDE:
		if (pie_GetFogEnabled())
		{
			gl_SetFogColour(pie_GetFogColour());
			fogColourSet = TRUE;
		}
		else
		{
//			gl_SetFogColour(pie_GetFogColour(0));
		}
		break;
	case ENGINE_D3D:
		if (d3dActive == 0)
		{
			d3dActive = 1;
			_renderBegin_D3D();
		}
		break;
	default:
		break;
	}
}

void pie_GlobalRenderEnd(BOOL bForceClearToBlack)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_GLIDE:
		if ((fogColourSet) && (bForceClearToBlack))
		{
			gl_SetFogColour(0);
			fogColourSet = FALSE;
		}
		break;
	case ENGINE_D3D:
		if (d3dActive != 0)
		{
			d3dActive = 0;
			_renderEnd_D3D();
		}
		break;
	default:
		break;
	}
}

/***************************************************************************/
UDWORD	pie_GetResScalingFactor( void )
{
UDWORD	resWidth;	//n.b. resolution width implies resolution height...!

	resWidth = pie_GetVideoBufferWidth();
	switch(resWidth)
	{
	case	640:
		return(100);		// game runs in 640, so scale factor is 100 (normal)
		break;
	case	800:
		return(125);
		break;				// as 800 is 125 percent of 640
	case		960:
		return(150);
		break;
	case	    1024:
		return(160);
		break;
	case	    1152:
		return(180);
		break;
	case	    1280:
		return(200);
		break;
	default:
		ASSERT((FALSE,"Unsupported resolution"));
		return(100);		// default to 640
		break;
	}
}
/***************************************************************************/
void pie_LocalRenderBegin(void)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
		_bank_off_4101();
		break;
	case ENGINE_SR:
		_bank_off_sr();
		break;
	default:
		break;
	}
}

void pie_LocalRenderEnd(void)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
		_bank_on_4101();
		break;
	case ENGINE_SR:
		_bank_on_sr();
		break;
	default:
		break;
	}
}


void pie_RenderSetup(void)
{
}
