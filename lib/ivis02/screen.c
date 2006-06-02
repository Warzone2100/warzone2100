/*
 * Screen.c
 *
 * Basic double buffered display using direct draw.
 *
 */

#include <stdio.h>
#include <SDL/SDL.h>

#include "lib/framework/frame.h"
#include "lib/framework/frameint.h"
#include "screen.h"

/* Control Whether the back buffer is in system memory for full screen */
#define FULL_SCREEN_SYSTEM	TRUE

/* The Current screen size and bit depth */
UDWORD		screenWidth = 0;
UDWORD		screenHeight = 0;
UDWORD		screenDepth = 0;

SDL_Surface     *screen;

/* The current screen mode (full screen/windowed) */
SCREEN_MODE		screenMode = SCREEN_WINDOWED;

/* Which mode (of operation) the library is running in */
DISPLAY_MODES	displayMode;

/* The handle for the main application window */
HANDLE		hWndMain = NULL;

/* The Front and back buffers */
LPDIRECTDRAWSURFACE4	psFront = NULL;
/* The back buffer is not static to give a back door to display routines so
 * they can get at the back buffer directly.
 * Mainly did this to link in Sam's 3D engine.
 */
LPDIRECTDRAWSURFACE4	psBack = NULL;

/* The actual palette entries for the display palette */
#define PAL_MAX				256
static SDL_Color			asPalEntries[PAL_MAX];

/* The bit depth at which it is assumed the mode is palettised */
#define PALETTISED_BITDEPTH   8

/* The number of bits in one colour gun of the windows PALETTEENTRY struct */
#define PALETTEENTRY_BITS 8

/* The Pixel format of the front buffer */
DDPIXELFORMAT		sFrontBufferPixelFormat;

/* The Pixel format of the back buffer */
DDPIXELFORMAT		sBackBufferPixelFormat;

/* Window's Pixel format */
DDPIXELFORMAT		sWinPixelFormat;

/* The size of the windows display mode */
//static UDWORD		winDispWidth, winDispHeight;

// The current flip state
FLIP_STATE	screenFlipState;

//backDrop
#define BACKDROP_WIDTH	640
#define BACKDROP_HEIGHT	480
UWORD*  pBackDropData = NULL;
BOOL    bBackDrop = FALSE;
BOOL    bUpload = FALSE;

//fog
DWORD	fogColour = 0;


/* flag forcing buffers into video memory */
static BOOL					g_bVidMem;

static UDWORD	backDropWidth = BACKDROP_WIDTH;
static UDWORD	backDropHeight = BACKDROP_HEIGHT;


SDL_Surface *screenGetSDL()
{
        return screen;
}




/* Initialise the double buffered display */
BOOL screenInitialise(UDWORD		width,			// Display width
					  UDWORD		height,			// Display height
					  UDWORD		bitDepth,		// Display bit depth
					  BOOL			fullScreen,		// Whether to start windowed
													// or full screen.
					  BOOL			bVidMem,		// Whether to put surfaces in
													// video memory
					  BOOL			bDDraw,			// Whether to create ddraw surfaces												// video memory
					  HANDLE		hWindow)		// The main windows handle
{
	UDWORD				i,j,k, index;
	int video_flags;

	/* Store the screen information */
	screenWidth = width;
	screenHeight = height;
	screenDepth = bitDepth;

	/* store vidmem flag */
	g_bVidMem = bVidMem;

	video_flags = SDL_HWPALETTE | SDL_HWSURFACE | SDL_DOUBLEBUF;
	screen = SDL_SetVideoMode(screenWidth, screenHeight, screenDepth, video_flags);
	if (!screen) {
		printf("Error: SDL_SetVideoMode failed (%s).\n", SDL_GetError());
		return FALSE;
	}

	/* If we're going to run in a palettised mode initialise the palette */
	if (bitDepth == PALETTISED_BITDEPTH)
	{
		memset(asPalEntries, 0, sizeof(*asPalEntries) * PAL_MAX);

		/* Bash in a range of colours */
		for(i=0; i<=4; i++)
		{
			for(j=0; j<=4; j++)
			{
				for(k=0; k<=4; k++)
				{
					index = i*25 + j*5 + k;
					asPalEntries[index].r = (UBYTE)(i*63);
					asPalEntries[index].g = (UBYTE)(j*63);
					asPalEntries[index].b = (UBYTE)(k*63);
				}
			}
		}

		/* Fill in a grey scale */
		for(i=0; i<64; i++)
		{
			asPalEntries[i+125].r = (UBYTE)(i<<2);
			asPalEntries[i+125].g = (UBYTE)(i<<2);
			asPalEntries[i+125].b = (UBYTE)(i<<2);
		}

		/* Colour 0 is always black */
		asPalEntries[0].r = 0;
		asPalEntries[0].g = 0;
		asPalEntries[0].b = 0;

		/* Colour 255 is always white */
		asPalEntries[255].r = 0xff;
		asPalEntries[255].g = 0xff;
		asPalEntries[255].b = 0xff;

		SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, asPalEntries, 0, 256);
	}

	screenMode = SCREEN_WINDOWED;
	if (fullScreen)
	{
		screenToggleMode();
	}

	return TRUE;
}


/* Release the DD objects */
void screenShutDown(void)
{
	if (screen != NULL)
	{
		SDL_FreeSurface(screen);
	}
}

BOOL screenReInit( void )
{
	BOOL	bFullScreen;

	if ( screenMode == SCREEN_FULLSCREEN )
	{
		bFullScreen = TRUE;
	}
	else
	{
		bFullScreen = FALSE;
	}

	screenShutDown();

	return screenInitialise( screenWidth, screenHeight, screenDepth, bFullScreen,
								g_bVidMem, TRUE, hWndMain );
}

/* Return a pointer to the Direct Draw 2 object */
LPDIRECTDRAW4 screenGetDDObject(void)
{
	return NULL;
}


/* Return a pointer to the Direct Draw back buffer surface */
LPDIRECTDRAWSURFACE4 screenGetSurface(void)
{
	return psBack;
}

/* Return a pointer to the Front buffer pixel format */
DDPIXELFORMAT *screenGetFrontBufferPixelFormat(void)
{
	return NULL;
}

/* Return a bit depth of the Front buffer */
UDWORD screenGetFrontBufferBitDepth(void)
{
	return screen->format->BitsPerPixel;
}

/* Return a pixel masks of the Front buffer */
BOOL screenGetFrontBufferPixelFormatMasks(ULONG *amask, ULONG *rmask, ULONG *gmask, ULONG *bmask)
{
	*amask = screen->format->Amask;
	*rmask = screen->format->Rmask;
	*gmask = screen->format->Gmask;
	*bmask = screen->format->Bmask;
	return TRUE;
}

/* Return a pointer to the back buffer pixel format */
DDPIXELFORMAT *screenGetBackBufferPixelFormat(void)
{
	return NULL;
}

/* Return a bit depth of the Back buffer */
UDWORD screenGetBackBufferBitDepth(void)
{
	return screen->format->BitsPerPixel;
}

/* Return a pixel masks of the Back buffer */
BOOL screenGetBackBufferPixelFormatMasks(ULONG *amask, ULONG *rmask, ULONG *gmask, ULONG *bmask)
{
	*amask = screen->format->Amask;
	*rmask = screen->format->Rmask;
	*gmask = screen->format->Gmask;
	*bmask = screen->format->Bmask;
	return TRUE;
}

/*
 * screenRestoreSurfaces
 *
 * Restore the direct draw surfaces if they have been lost.
 *
 * This is only used internally within the library.
 */
void screenRestoreSurfaces(void)
{
	/* nothing to do */
}

void screen_SetBackDrop(UWORD *newBackDropBmp, UDWORD width, UDWORD height)
{
	bBackDrop = TRUE;
	pBackDropData = newBackDropBmp;
	backDropWidth = width;
	backDropHeight = height;
}

void screen_StopBackDrop(void)
{
	bBackDrop = FALSE;
}

void screen_RestartBackDrop(void)
{
	bBackDrop = TRUE;
}

UWORD* screen_GetBackDrop(void)
{
	if (bBackDrop == TRUE)
	{
		return pBackDropData;
	}
	return NULL;
}

UDWORD screen_GetBackDropWidth(void)
{
	if (bBackDrop == TRUE)
	{
		return backDropWidth;
	}
	return 0;
}

void screen_Upload(UWORD *newBackDropBmp)
{
	UDWORD		y;
	UBYTE		*Src;
	UBYTE		*Dest;
	UWORD		backdropPitch = BACKDROP_WIDTH*(screen->format->BitsPerPixel >> 3);

        if (SDL_LockSurface(screen) != 0)
	{
		DBERROR(("Back buffer lock failed:\n"));
		return;
	}

        switch (screen->format->BitsPerPixel)
        {
	case 8: //assume src bmp is palettised to same palette
	case 16:
	case 24:
	case 32:
		Src  = (UBYTE *)newBackDropBmp;
		Dest = (UBYTE *)screen->pixels;
		Dest += screen->pitch*(screen->h-BACKDROP_HEIGHT)>>1;
		Dest += (screen->pitch-backdropPitch)>>1;
		for(y=0; (y<screenHeight); y++)
		{
			memcpy(Dest, Src, backdropPitch);
			Src += backdropPitch;
			Dest += screen->pitch;
		}
		break;
	default:
		DBPRINTF(("Upload not implemented for this bit depth"));
		break;
        }

        SDL_UnlockSurface(screen);
}

void screen_SetFogColour(UDWORD newFogColour)
{
	UDWORD red, green, blue;
	static UDWORD currentFogColour = 0;

	if (newFogColour != currentFogColour)
	{

		if (screen->format->BitsPerPixel == 16)//only set in 16 bit modes
		{
			if (screen->format->Gmask == 0x07e0)//565
			{
				red = newFogColour >> 8;
				red &= screen->format->Rmask;
				green = newFogColour >> 5;
				green &= screen->format->Gmask;
				blue = newFogColour >> 3;
				blue &= screen->format->Bmask;
				fogColour = red + green + blue;
			}
			else if (screen->format->Gmask == 0x03e0)//555
			{
				red = newFogColour >> 9;
				red &= screen->format->Rmask;
				green = newFogColour >> 6;
				green &= screen->format->Gmask;
				blue = newFogColour >> 3;
				blue &= screen->format->Bmask;
				fogColour = red + green + blue;
			}
		}
		currentFogColour = newFogColour;
	}
	return;
}

/* Flip back and front buffers */
//always clears or renders backdrop
void screenFlip(BOOL clearBackBuffer)
{
	SDL_Flip(screen);
	if ((bBackDrop) && (pBackDropData != NULL) AND clearBackBuffer)
	{
		// copy back drop
                screen_Upload(pBackDropData);
	}
	else if (clearBackBuffer)
	{
		SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
	}
}

/* Swap between windowed and full screen mode */
void screenToggleMode(void)
{

	if ((displayMode == MODE_WINDOWED) || (displayMode == MODE_FULLSCREEN))
	{
		/* The framework can only run in the current screen mode */
		return;
	}

	if (SDL_WM_ToggleFullScreen(screen))
	{
		if (screenMode == SCREEN_WINDOWED)
		{
			screenMode = SCREEN_FULLSCREEN;
		}
		else if (screenMode == SCREEN_FULLSCREEN)
		{
			screenMode = SCREEN_WINDOWED;
		}
	}
}

/* Swap between windowed and full screen mode */
BOOL screenToggleVideoPlaybackMode(void)
{
#if 0
	HRESULT				ddrval;
	DDSURFACEDESC2		ddsd;		// Direct Draw surface description
#if !FULL_SCREEN_SYSTEM
	DDSCAPS				ddscaps;
#endif

	if ((displayMode == MODE_WINDOWED) || (displayMode == MODE_FULLSCREEN))
	{
		/* The framework can only run in the current screen mode */
		return TRUE;
	}

	if (screenMode == SCREEN_WINDOWED)
	{
		return TRUE;
	}

	if (screenMode == SCREEN_FULLSCREEN)
	{
		screenDepth = 16;
		screenMode = SCREEN_FULLSCREEN_VIDEO;
		RELEASE(psPalette);
	}
	else if (screenMode == SCREEN_FULLSCREEN_VIDEO)
	{
		screenDepth = 8;
		screenMode = SCREEN_FULLSCREEN;
	}

	RELEASE(psBack);
	RELEASE(psFront);

	/* Set the display mode */
	ddrval = psDD->lpVtbl->SetDisplayMode(
				psDD,
				screenWidth, screenHeight, screenDepth,
				0,0);		// Set these so the DD1 version is used
	if (ddrval != DD_OK)
	{
		DBERROR(("Set display mode failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}

#if FULL_SCREEN_SYSTEM
	/* Create the Primary Surface. */
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	ddrval = psDD->lpVtbl->CreateSurface(
					psDD,
					&ddsd,
					&psFront,
					NULL);
	if (ddrval != DD_OK)
	{
		DBERROR(("Create Primary Surface failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}

	//copy the palette
	memset(&sFrontBufferPixelFormat, 0, sizeof(DDPIXELFORMAT));
	sFrontBufferPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddrval = psFront->lpVtbl->GetPixelFormat(
				psFront,
				&sFrontBufferPixelFormat);
	if (ddrval != DD_OK)
	{
		DBERROR(("Couldn't get pixel format:\n%s",
					DDErrorToString(ddrval)));
		return FALSE;
	}

	/* Create the back buffer */
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwWidth = screenWidth;
	ddsd.dwHeight = screenHeight;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
	/* Force the back buffer into system memory unless flag set */
	if ( !g_bVidMem )
	{
		ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
	}
	ddrval = psDD->lpVtbl->CreateSurface(
				psDD,
				&ddsd,
				&psBack,
				NULL);
	if (ddrval != DD_OK)
	{
		DBERROR(("Create Back surface failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}
#else
	/* Create the Primary Surface. */
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

	ddsd.dwBackBufferCount = 1;	// Use 2 for triple buffering, 1 for double

	ddrval = psDD->lpVtbl->CreateSurface(
					psDD,
					&ddsd,
					&psFront,
					NULL);
	if (ddrval != DD_OK)
	{
		DBERROR(("Create Primary Surface failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}

	//copy the palette
	memset(&sFrontBufferPixelFormat, 0, sizeof(DDPIXELFORMAT));
	sFrontBufferPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddrval = psFront->lpVtbl->GetPixelFormat(
				psFront,
				&sFrontBufferPixelFormat);
	if (ddrval != DD_OK)
	{
		DBERROR(("Couldn't get pixel format:\n%s",
					DDErrorToString(ddrval)));
		return FALSE;
	}

	/* Get the back buffer */
	memset(&ddscaps, 0, sizeof(DDSCAPS));
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	ddrval = psFront->lpVtbl->GetAttachedSurface(
				psFront,
				&ddscaps,
				&psBack);
	if (ddrval != DD_OK)
	{
		DBERROR(("Get Back surface failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}
#endif
	/* If we are in a palettised mode, create a palette */
	if (screenDepth == PALETTISED_BITDEPTH)
	{
		/* Create the palette from the stored palette entries */
		ddrval = psDD->lpVtbl->CreatePalette(
						psDD,
						DDPCAPS_8BIT,
						asPalEntries,
						&psPalette,
						NULL);
		if (ddrval != DD_OK)
		{
			DBERROR(("Failed to create palette:\n%s", DDErrorToString(ddrval)));
			return FALSE;
		}

		/* Assign the palette to the front buffer */
		ddrval = psFront->lpVtbl->SetPalette(psFront, psPalette);
		if (ddrval != DD_OK)
		{
			DBERROR(("Couldn't set palette for front buffer:\n%s",
						DDErrorToString(ddrval)));
		}

		/* Assign the palette to the back buffer */
		ddrval = psFront->lpVtbl->SetPalette(psBack, psPalette);
		if (ddrval != DD_OK)
		{
			DBERROR(("Couldn't set palette for back buffer:\n%s",
						DDErrorToString(ddrval)));
		}
	}

#endif
	return TRUE;
}

SCREEN_MODE screenGetMode( void )
{
	return screenMode;
}

/* Set screen mode */
void screenSetMode(SCREEN_MODE mode)
{
	/* If the mode is the same as the current one, don't have to do
	 * anything.  Otherwise toggle the mode.
	 */
	if (mode != screenMode)
	{
		screenToggleMode();
	}
}

/* In full screen mode flip to the GDI buffer.
 * Use this if you want the user to see any GDI output.
 * This is mainly used so that ASSERTs and message boxes appear
 * even in full screen mode.
 */
void screenFlipToGDI(void)
{
	/* do nothing */
}

/* Set palette entries for the display buffer
 * first specifies the first palette entry. count the number of entries
 * The psPalette should have at least first + count entries in it.
 */
void screenSetPalette(UDWORD first, UDWORD count, PALETTEENTRY *psEntries)
{
	UDWORD	i;

	ASSERT(((first+count-1 < PAL_MAX),
		"screenSetPalette: invalid entry range"));

	if (count == 0)
	{
		return;
	}

	/* ensure that colour 0 is black and 255 is white */
	if ((first == 0 || first == 255) && count == 1)
	{
		return;
	}

	if (first == 0)
	{
		first = 1;
		count -= 1;
	}
	if (first + count - 1 == PAL_MAX)
	{
		count -= 1;
	}

        for (i = 0; i < count; i++)
        {
                asPalEntries[first + i].r = psEntries[first + i].peRed;
                asPalEntries[first + i].g = psEntries[first + i].peGreen;
                asPalEntries[first + i].b = psEntries[first + i].peBlue;
        }

        SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, asPalEntries + first, first, count);
}

/* Return the best colour match when in a palettised mode */
UBYTE screenGetPalEntry(UBYTE red, UBYTE green, UBYTE blue)
{
	UDWORD	i, minDist, dist;
	UDWORD	redDiff,greenDiff,blueDiff;
	UBYTE	colour;

	ASSERT((screen->format->BitsPerPixel == 8,
		"screenSetPalette: not in a palettised mode"));

	minDist = 0xff*0xff*0xff;
	colour = 0;
	for(i = 0; i < PAL_MAX; i++)
	{
		redDiff = asPalEntries[i].r - red;
		greenDiff = asPalEntries[i].g - green;
		blueDiff = asPalEntries[i].b - blue;
		dist = redDiff*redDiff + greenDiff*greenDiff + blueDiff*blueDiff;
		if (dist < minDist)
		{
			minDist = dist;
			colour = (UBYTE)i;
		}
		if (minDist == 0)
		{
			break;
		}
	}

	return colour;
}

char* screenDumpToDisk(char* path) {
	return NULL;
}

/* Output text to the display screen at location x,y.
 * The remaining arguments are as printf.
 */
void screenTextOut(UDWORD x, UDWORD y, STRING *pFormat, ...)
{
}

