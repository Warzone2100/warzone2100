#ifdef INC_GLIDE
/* 3dfx specific functions *
/* 
	Handles initialisation of 3dfx as well as
	texture handling and tri/poly 'bitmap' drawing 
	Alex McLean, 1997

*/

#include "dglide.h"
#include "frame.h"
#include "3dfxtext.h"
#include "3dfxmode.h"
#include "rendmode.h"
#include "piestate.h"
#include "pieclip.h"

GrScreenResolution_t	getGlideResDescriptor( UDWORD resWidth );
#define	MAX_3DFX_WIDTH	800
#define	MAX_3DFX_HEIGHT	600
static long FPUControlWord;

// Set FPU precision to 24bit. (Glide Programming Guide. Pg 150)
// Apparently we need to do this to make the SNAP_BIAS stuff work.
//
static void SetFPUPrecision(void)
{
	long memvar;

	_asm {
		finit;					// Initialise the FPU
		fwait;					// Wait for operation to complete
		fstcw FPUControlWord;	// Store FPU control word
		fwait;					// Wait for operation to complete
		mov eax,FPUControlWord;	// Move control word into a register
		and eax,0xfffffcff;		// Mask of prevision bits to set to 24-bit
		mov memvar,eax;			// Save control word to memory
		fldcw memvar;			// Load control word back to FPU
		fwait;					// Wait for operation to complete
	}
}

// Restore FPU precision.
//
static void RestoreFPUPrecision(void)
{
	_asm {
		finit;					// Initialise the FPU
		fwait;					// Wait for operation to complete
		fldcw FPUControlWord;	// Load FPU control word
		fwait;					// Wait for operation to complete
	}
}


GrScreenResolution_t	getGlideResDescriptor( UDWORD resWidth )
{
	switch(resWidth)
	{
	case	640:
		return(GR_RESOLUTION_640x480);
		break;
		
	case	800:
		return(GR_RESOLUTION_800x600);
		break;

	default:
		ASSERT((FALSE,"Request made for unsupported Glide resolution."));
		return(GR_RESOLUTION_640x480);
		break;
	}
}

/* Fires up our 3dfx card */
BOOL	init3dfx(void)
{
GrHwConfiguration		hwconfig;
GrScreenResolution_t	resolution;
FxBool					success;					
//GrFog_t fogTable[64];

	/* Kill present 3dfx app */
	grGlideShutdown();

   	
	// See how many 3dfx cards we can get a hold of 
	grSstQueryBoards(&hwconfig);
	if(!(hwconfig.num_sst))
	{
		DBERROR(("grSstQueryBoards: returned 0"));
		return(FALSE);
	}

	/* Initialize Glide */
    grGlideInit();

    /* See if we can get a hold of a 3dfx card? */
	if( grSstQueryHardware( &hwconfig ) )
	{
		/* Select TMU 0	*/
		grSstSelect( 0 );

		resolution = getGlideResDescriptor(pie_GetVideoBufferWidth());

		/* See if we can open the window? resolution*/
		success = grSstWinOpen( (FxU32) frameGetWinHandle(), resolution, GR_REFRESH_60Hz, GR_COLORFORMAT_ARGB, GR_ORIGIN_UPPER_LEFT, 2, 1 );
		if(!success)
		{
			DBERROR(("Glide grSstWinOpen failed, can't open window on 3dfx."));
			return(FALSE);
		}

	}
	else
	{
		/* Looks like there's no 3dfx card on board or it's incorrectly installed */
		DBERROR(("Cannot find a 3dfx card?!"));
		return(FALSE);
	}

	reset3dfx();

	/* New stuff for w - buffer */
	grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
//	grDepthMask(TRUE);//write enabled//pie_SetDefaultStates sets this
//	grDepthBufferFunction(GR_CMP_LEQUAL);//pie_SetDefaultStates sets this
	grDepthBiasLevel(-1);
//	pie_SetDefaultStates();
	SetFPUPrecision();

	return(TRUE);
}


void reset3dfx(void)
{
	int		i;
	long	entry;
	long	cardPal[256];
	iColour* psPalette;

	//limit 3DFX to 800 600
	if (pie_GetVideoBufferWidth() > MAX_3DFX_WIDTH)
	{
		pie_SetVideoBufferWidth(MAX_3DFX_WIDTH);
		pie_SetVideoBufferWidth(MAX_3DFX_HEIGHT);
	}
		

	pie_SetRenderEngine(ENGINE_GLIDE);
	init3dfxTexMemory();
	gl_SetupTexturing();

	rendSurface.buffer		= 0;
	rendSurface.flags		= REND_SURFACE_SCREEN;
	rendSurface.usr			= REND_GLIDE_3DFX;
	rendSurface.size		= 0;
	rendSurface.width		= pie_GetVideoBufferWidth();
	rendSurface.height		= pie_GetVideoBufferHeight();
	rendSurface.xcentre		= pie_GetVideoBufferWidth()/2;
	rendSurface.ycentre		= pie_GetVideoBufferHeight()/2;
	rendSurface.clip.left	= 0;
	rendSurface.clip.top	= 0;
	rendSurface.clip.right	= pie_GetVideoBufferWidth();
	rendSurface.clip.bottom	= pie_GetVideoBufferHeight();
	rendSurface.xpshift		= 10;
	rendSurface.ypshift		= 10;

	for (i=0; i<iV_SCANTABLE_MAX; i++)
		rendSurface.scantable[i] = i * pie_GetVideoBufferWidth();
	/* If we're adding a palette and running on a 3dfx, then bang it down to the card */
	psPalette = pie_GetGamePal();
	for(i=0; i<256; i++)
	{
		entry = 0;
		entry = psPalette[i].r;
		entry = entry<<8;
		entry = entry | (long)psPalette[i].g;
		entry = entry<<8;
		entry = entry | (long)psPalette[i].b;
		cardPal[i] = (long)entry;
	}
	/* Make sure we send our palette to the 3dfx card via a glide call */
	grTexDownloadTable(GR_TMU0, GR_TEXTABLE_PALETTE, &cardPal);
}


/* Kills the 3dfx display */
void	close3dfx( void )
{
	free3dfxTexMemory();
	grSstWinClose();
	grGlideShutdown();

	RestoreFPUPrecision();
}


// Deativate 3DFX when the game looses focus
BOOL gl_Deactivate(void)
{

	grSstWinClose();
//	InvalidateRect( frameGetWinHandle(), 0,0 );
//	grSstControl(GR_CONTROL_DEACTIVATE);

	return TRUE;
}

// Reactive the 3DFX when the game gets focus
BOOL gl_Reactivate(void)
{
	BOOL	success;
	int		i;
	long	entry;
	long	cardPal[256];
	iColour* psPalette;

	/* See if we can open the window? resolution*/
	success = grSstWinOpen( (FxU32) frameGetWinHandle(),
							getGlideResDescriptor(pie_GetVideoBufferWidth()),
							GR_REFRESH_60Hz,
							GR_COLORFORMAT_ARGB,
							GR_ORIGIN_UPPER_LEFT,
							2, 1 );
	if(!success)
	{
		DBERROR(("Glide grSstWinOpen failed, can't open window on 3dfx."));
		return FALSE;
	}
//	gl_SetupTexturing();

	/* Make sure we send our palette to the 3dfx card via a glide call */
	psPalette = pie_GetGamePal();
	for(i=0; i<256; i++)
	{
		entry = 0;
		entry = psPalette[i].r;
		entry = entry<<8;
		entry = entry | (long)psPalette[i].g;
		entry = entry<<8;
		entry = entry | (long)psPalette[i].b;
		cardPal[i] = (long)entry;
	}
	grTexDownloadTable(GR_TMU0, GR_TEXTABLE_PALETTE, &cardPal);

	/* New stuff for w - buffer */
	grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
//	grDepthMask(TRUE);//write enabled//pie_SetDefaultStates sets this
//	grDepthBufferFunction(GR_CMP_LEQUAL);//pie_SetDefaultStates sets this
	grDepthBiasLevel(-1);
//	pie_SetDefaultStates();
	
//	pie_SetDefaultStates();
	// reset the fog colour so it gets set back to the correct value
	gl_SetFogColour(0);
	pie_ResetStates();

	if (!gl_RestoreTextures())
	{
		DBERROR(("Failed to reload textures."));
		return FALSE;
	}

	return TRUE;
}

#endif
