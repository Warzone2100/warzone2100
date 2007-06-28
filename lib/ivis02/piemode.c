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
#include "piestate.h"
#include "piemode.h"
#include "piematrix.h"
#include "piefunc.h"
#include "tex.h"

#include "v4101.h"
#include "vsr.h"

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

static	BOOL	bDither = FALSE;

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

	pie_SetRenderEngine(ENGINE_4101);
	r = _mode_4101();	// we always want success as jon's stuff does the init

	if (r)
	{
		pie_SetDefaultStates();
	}


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
	default:
		break;
	}
	pie_SetRenderEngine(ENGINE_UNDEFINED);
}

/***************************************************************************/

void pie_ScreenFlip(CLEAR_MODE clearMode)
{
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
	default:
		break;
	}
}

/***************************************************************************/

void pie_Clear(UDWORD colour)
{

	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	default:
		break;
	}

}
/***************************************************************************/

void pie_GlobalRenderBegin(void)
{
	switch (pie_GetRenderEngine())
	{
	default:
		break;
	}
}

void pie_GlobalRenderEnd(BOOL bForceClearToBlack)
{
	switch (pie_GetRenderEngine())
	{
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
	default:
		break;
	}
}


void pie_RenderSetup(void)
{
}
