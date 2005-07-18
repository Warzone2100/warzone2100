/***************************************************************************/

#include <stdio.h>

#include "frame.h"
#include "pcx.h"
#include "tex.h"
#include "rendmode.h"

#include "d3drender.h"
#include "texd3d.h"

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/


/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/
/***************************************************************************/
/*
 * d3dCreateSurface
 * Create a DirectDraw Surface of the given description.  Using this function
 * ensures that all surfaces end up in system memory if that option was set.
 * Returns the result of the CreateSurface call.
 */
/***************************************************************************/

HRESULT
d3dCreateSurface( LPDDSURFACEDESC2 lpDDSurfDesc,
					LPDIRECTDRAWSURFACE4 FAR *lpDDSurface, BOOL bHardware )
{
	LPDIRECTDRAW4	psDD;
	HRESULT			hRes;

	/* get local copy of DirectDraw object pointer from framework */
	psDD = screenGetDDObject();

    if ( bHardware )
	{
        lpDDSurfDesc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
	}

	hRes = psDD->lpVtbl->CreateSurface( psDD, lpDDSurfDesc,
											 lpDDSurface, NULL);

    return hRes;
}

/***************************************************************************/
/*
 * d3dGetSurfDesc
 * Get the description of the given surface.  Returns the result of the
 * GetSurfaceDesc call.
 */
/***************************************************************************/

HRESULT
d3dGetSurfDesc(LPDDSURFACEDESC2 lpDDSurfDesc,LPDIRECTDRAWSURFACE4 lpDDSurf)
{
    HRESULT result;

	memset(lpDDSurfDesc, 0, sizeof(DDSURFACEDESC2));
    lpDDSurfDesc->dwSize = sizeof(DDSURFACEDESC2);
    result = lpDDSurf->lpVtbl->GetSurfaceDesc(lpDDSurf, lpDDSurfDesc);
	
	return result;
}

/***************************************************************************/
/*
 * NearestPowerOf2
 *
 * Calculates next power of 2 up from current value
 * (used because D3D textures need to be power of 2 wide and high).
 */
/***************************************************************************/

UWORD
NearestPowerOf2( UDWORD i)
{
	SWORD lShift = 0;

	while(i < (UDWORD)(1 << lShift))
	{
		lShift ++;
	}
	ASSERT (((lShift < 11),"NearestPowerOf2: value %i out of bounds\n", i));
	return (1 << lShift);
}

UWORD
NearestPowerOf2withShift( UDWORD i, SWORD *shift)
{
	SWORD lShift = 0;

	while(i > (UDWORD)(1 << lShift))
	{
		lShift ++;
	}
	*shift = lShift;
	ASSERT (((lShift < 11),"NearestPowerOf2: value %i out of bounds\n", i));
	return (1 << lShift);
}

/***************************************************************************/

BOOL
D3DTexCreateFromIvisTex( TEXPAGE_D3D			*psTexPage,
						 iTexture				*psIvisTex,
						 iColour				*psIvisPal,
						 LPDDPIXELFORMAT		pDDSurfDescTexture,
						 PALETTEENTRY			*psPal,
						 BOOL					bHardware )
{
	LPDIRECTDRAWSURFACE4		psSurfaceSrc = NULL;
	LPDIRECT3DTEXTURE2		psTextureSrc = NULL;
	DWORD					dwCaps;
	HRESULT					hResult;
	DDSURFACEDESC2			ddsd;
	LPDIRECTDRAW4			psDD;
	DWORD					pcaps;
	UWORD					i;
	HDC						hDC;
	DWORD					dwColourKey;

	psDD = screenGetDDObject();

	ASSERT((psDD != NULL,
		"surfCreateFromPCX: NULL DD object - framework not initialised?"));

	/* get next power of 2 for width and height of surface */
	psTexPage->iWidth  = NearestPowerOf2withShift( psIvisTex->width , &(psTexPage->widthShift));
	psTexPage->iHeight = NearestPowerOf2withShift( psIvisTex->height, &(psTexPage->heightShift));

	if ( surfCreate( &psSurfaceSrc, psTexPage->iWidth, psTexPage->iHeight,
					  DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY,
					  pDDSurfDescTexture,
					  psTexPage->bColourKeyed, TRUE ) == FALSE )
	{
		DBERROR( ("surfCreateFromPCX: couldn't create sys mem surface.\n") );
		return FALSE;
	}

	hDC = GetDC(NULL);

	/* Get the current palette */
	GetSystemPaletteEntries( hDC, 0, (1 << 8), psPal );
	ReleaseDC( NULL, hDC );

	/*
	 * Change the flags on the palette entries to allow D3D to change
	 * some of them.  In the window case, we must not change the top and
	 * bottom ten (system colors), but in a fullscreen mode we can have
	 * all but the first and last.
	 */
	for ( i=0; i<256; i++ )
	{
		psPal[i].peRed   = psIvisPal[i].r;
		psPal[i].peGreen = psIvisPal[i].g;
		psPal[i].peBlue  = psIvisPal[i].b;
		psPal[i].peFlags = D3DPAL_FREE | PC_RESERVED;
	}

	if (!surfLoadFrom8Bit(psSurfaceSrc, psIvisTex->width,
		psIvisTex->height, psIvisTex->bmp, psPal))
	{
		return FALSE;
	}

    /* Query source surface for a texture interface */
	hResult = psSurfaceSrc->lpVtbl->QueryInterface( psSurfaceSrc,
							&IID_IDirect3DTexture2,
							(LPVOID*) &psTextureSrc );
	if ( hResult != DD_OK )
	{
		DBERROR( ("Failed to obtain D3D texture interface for src texture.\n%s",
			      DDErrorToString(hResult)) );
		goto exit_with_error;
	}

	/* set destination caps */
	dwCaps = DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD;

	if ( !bHardware )
	{
		dwCaps |= DDSCAPS_SYSTEMMEMORY;
	}

	/*
	 * Create an empty texture surface to load the source texture into.
	 * The DDSCAPS_ALLOCONLOAD flag allows the DD driver to wait until the
	 * load call to allocate the texture in memory because at this point,
	 * we may not know how much memory the texture will take up (e.g. it
	 * could be compressed to an unknown size in video memory).
	 * Make sure SW renderers get textures in system memory
	 */
	if ( surfCreate( &psTexPage->psSurface, psTexPage->iWidth,
					 psTexPage->iHeight, dwCaps,
					 pDDSurfDescTexture,
					 psTexPage->bColourKeyed, FALSE ) == FALSE )
	{
        DBERROR( ("surfCreateFromPCX: couldn't create surface.\n") );
		goto exit_with_error;
	}

    if ( pDDSurfDescTexture->dwFlags & DDPF_PALETTEINDEXED8)
	{
		pcaps = DDPCAPS_8BIT | DDPCAPS_ALLOW256;
    }
	else if (pDDSurfDescTexture->dwFlags & DDPF_PALETTEINDEXED4)
	{
		pcaps = DDPCAPS_4BIT;
    }
	else
	{
		pcaps = 0;
    }

	/* copy palette from into dest if necessary */
    if ( pcaps )
	{	
		hResult = psDD->lpVtbl->CreatePalette(psDD, pcaps,
								psPal, &psTexPage->psPalette, NULL);
		if (hResult != DD_OK)
		{
			DBERROR( ("Failed to create a palette for the destination texture.\n%s",
					DDErrorToString(hResult)) );
			goto exit_with_error;
		}

		/* copy palette entries from src to dest */
		psTexPage->psPalette->lpVtbl->SetEntries( psTexPage->psPalette, 0, 0, 256, psPal );

		/* set palette to dest surface */
		hResult = psTexPage->psSurface->lpVtbl->SetPalette(
						psTexPage->psSurface, psTexPage->psPalette);
		if (hResult != DD_OK)
		{
			DBERROR( ("Failed to set the destination texture's palette.\n%s",
					  DDErrorToString(hResult)) );
			goto exit_with_error;
		}
    }	
	
	/* Query destination surface for a texture interface */
	hResult = psTexPage->psSurface->lpVtbl->QueryInterface(
							psTexPage->psSurface,
							&IID_IDirect3DTexture2,
							(LPVOID*) &psTexPage->psTexture );
	if ( hResult != DD_OK )
	{
		DBERROR( ("Failed to obtain D3D texture interface for a destination texture.\n%s",
			      DDErrorToString(hResult)) );
		goto exit_with_error;
	}

    /*
     * Load the source texture into the destination.  During this call, a
     * driver could compress or reformat the texture surface and put it in
     * video memory.
     */
    hResult = psTexPage->psTexture->lpVtbl->Load( psTexPage->psTexture, psTextureSrc );
	if ( hResult != DD_OK )
	{
		DBERROR( ("Could not load a source texture into a destination texture.\n%s",
			      DDErrorToString( hResult )) );
		goto exit_with_error;
    }

	psTexPage->bColourKeyed = psIvisTex->bColourKeyed;

	/* set dest surface colour key */
	if ( psTexPage->bColourKeyed )
	{
		/* set colour key to first entry in palette */
		dwColourKey = 0;// RGB( psPal[0].peRed, psPal[0].peGreen, psPal[0].peBlue );
		hResult = DDSetColorKey( psTexPage->psSurface, dwColourKey );
		if ( hResult != DD_OK )
		{
			DBERROR( ("D3DTexCreateFromIvisTex: couldn't set dest colour key.\n%s",
						DDErrorToString(hResult)) );
			goto exit_with_error;
		}
	}

	/* Did the texture end up in video memory? */
	hResult = d3dGetSurfDesc(&ddsd, psTexPage->psSurface );
	if ( hResult != DD_OK )
	{
		DBERROR( ("D3DTexCreateFromIvisTex: couldn't get dest surface desc.\n%s",
					DDErrorToString(hResult)) );
		goto exit_with_error;
    }

	if ( ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY )
	{
		psTexPage->bInVideo = TRUE;
	}
	else
	{
		psTexPage->bInVideo = FALSE;
	}

	if ( D3DGetAlphaKey() == TRUE )
	{
		/* For textures with real alpha (not palettized), set transparent bits */
		if( ddsd.ddpfPixelFormat.dwRGBAlphaBitMask )
		{
			DWORD	dwAlphaMask = ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
			DWORD	dwRGBMask   = ( ddsd.ddpfPixelFormat.dwRBitMask |
									ddsd.ddpfPixelFormat.dwGBitMask |
									ddsd.ddpfPixelFormat.dwBBitMask );
			DWORD	dwColorkey  = 0x00000000; // Colorkey on black
			DWORD	*p32, x, y;
			WORD	*p16;

			// Lock the texture surface
			while( psTexPage->psSurface->lpVtbl->Lock( psTexPage->psSurface,
					NULL, &ddsd, 0, NULL ) == DDERR_WASSTILLDRAWING );
			 
			// Add an opaque alpha value to each non-colorkeyed pixel
			for( y=0; y<ddsd.dwHeight; y++ )
			{
				p16 =  (WORD*)((BYTE*)ddsd.lpSurface + y*ddsd.lPitch);
				p32 = (DWORD*)((BYTE*)ddsd.lpSurface + y*ddsd.lPitch);

				for( x=0; x<ddsd.dwWidth; x++ )
				{
					if( ddsd.ddpfPixelFormat.dwRGBBitCount == 16 )
					{
						if( ( *p16 &= dwRGBMask ) != dwColorkey )
						{
							*p16 |= dwAlphaMask;
						}
						p16++;
					}
					if( ddsd.ddpfPixelFormat.dwRGBBitCount == 32 )
					{
						if( ( *p32 &= dwRGBMask ) != dwColorkey )
						{
							*p32 |= dwAlphaMask;
						}
						p32++;
					}
				}
			}

			psTexPage->psSurface->lpVtbl->Unlock( psTexPage->psSurface, NULL );
		}
	}

	/* done with src */
	RELEASE( psTextureSrc );
	surfRelease( psSurfaceSrc );

	return TRUE;

exit_with_error:

	surfRelease( psSurfaceSrc );

	return FALSE;
}

