/***************************************************************************/
/*
 * d3dTexMan.c
 *
 * directx 6 texture management system pumpkin library.
 *
 */
/***************************************************************************/

#include "frame.h"
#include "piedef.h"
#include "piestate.h"
#include "piepalette.h"
#include "d3drender.h"
#include "dx6texman.h"
#include "tex.h"


/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/
	

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

#define MAX_TEX_PAGES 32
#define HIGH_RES_PAGES 9
#define BIG_TEXTURE_SIZE 256
#define SMALL_TEXTURE_SIZE 128
#define RADAR_TEXTURE_SIZE 256
#define RADAR_SIZE 128
#define MIN_TEX_MEMORY 1048576
#define REQ_8BIT_TEX_MEMORY (2*1048576)
#define REQ_TEX_MEMORY (4*1048576)

typedef struct _tex_container
{
	LPDIRECTDRAWSURFACE4 psSurface4;
	LPDIRECT3DTEXTURE2	 psTexture2;	
} TEXTURECONTAINER;

typedef struct _tex_search_info
{
	SDWORD	requestedPaletteMode;
	UDWORD	requestedBPP;
	BOOL	b8BPPAvailable;
	BOOL	bFoundGoodFormat;
	DDPIXELFORMAT ddpf; // Result of texture format search
} TEXSEARCHINFO;

typedef	enum	TEX_MAN_MODE
				{
					FULL_16BIT,
					MED_16BIT,
					LOW_16BIT,
					FULL_8BIT
				}
				TEX_MAN_MODE;

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

TEXTURECONTAINER aTextures[MAX_TEX_PAGES];
TEXTURECONTAINER radarTexture;

TEXSEARCHINFO	texInfo;

TEX_MAN_MODE	texSize = FULL_16BIT;
LPDIRECTDRAWSURFACE4	psImage256Surface4 = NULL;
LPDIRECTDRAWSURFACE4	psImage128Surface4 = NULL;
LPDIRECTDRAWSURFACE4	psRadarSurf4       = NULL;
LPDIRECTDRAWPALETTE		pDDPal;
LPDIRECT3DMATERIAL3		psMtrl3  = NULL;
D3DMATERIAL		mtrl;
D3DMATERIALHANDLE		hMtrl;

UWORD	texPal16Bit[PALETTE_SIZE];
/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/

static HRESULT CALLBACK TextureSearchCallback( DDPIXELFORMAT* pddpf, VOID* param );
static BOOL dtm_CreateTextureSurface(LPDIRECTDRAWSURFACE4 *ppsSurface4, SDWORD size, DWORD dwCaps);
static BOOL dtm_surfLoadFrom8Bit(LPDIRECTDRAWSURFACE4 psSurf4, UDWORD width, UDWORD height, UBYTE *pImageData, BOOL b8Bit);
static BOOL dtm_Build16BitTexturePalette(DDPIXELFORMAT* DDPixelFormat);
static void SetTextureStageStates(void);
static void SetMaterial(void);

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

/*
 *	BOOL dtm_Initialise(void)
 *
 *	called after screen and z buffers alloacted and after d3dDevices initialised
 *	Allocates 32 256 * 256 16bit texture pages 4Mb if possible
 *	else tries to Allocate 8 bit pages else chooses a compression ratio
 */

BOOL dtm_Initialise(void)
{
	HRESULT		hResult;
	BOOL		bSpace = TRUE;
	SDWORD		i, j;
	SDWORD		videoTextureSize = BIG_TEXTURE_SIZE;
	LPDIRECT3DDEVICE3 pD3D3;
	LPDIRECTDRAW4	 pDD4;
//	DDSURFACEDESC2	 ddSurfDesc2;
	DDSCAPS2	ddsCaps2;
	UDWORD      totalVideoMemory;
	UDWORD      freeVideoMemory;

	texInfo.requestedPaletteMode = DDPF_RGB;
	texInfo.requestedBPP	= 16;
	texInfo.b8BPPAvailable	= FALSE;
	texInfo.bFoundGoodFormat = FALSE;


	pD3D3 = d3d_GetpsD3DDevice3();
	pDD4 = d3d_GetpsDD4();

	// Enumerate the texture formats, and find the closest device-supported
	// texture pixel format
    hResult = pD3D3->lpVtbl->EnumTextureFormats(pD3D3, TextureSearchCallback, &texInfo );

	if ( hResult != D3D_OK )
	{
		ASSERT((FALSE,"dtm_Initialise: EnumTextureFormats failed\n%s",DDErrorToString(hResult)));
		return FALSE;
	}

	if (!texInfo.bFoundGoodFormat)
	{
		ASSERT((FALSE,"dtm_Initialise: RGB texture mode not found\n%s",DDErrorToString(hResult)));
		return FALSE;
	}

	memset(&ddsCaps2, 0, sizeof(DDSCAPS2));
	ddsCaps2.dwCaps = DDSCAPS_TEXTURE; 


	hResult = pDD4->lpVtbl->GetAvailableVidMem(pDD4, &ddsCaps2, &totalVideoMemory, &freeVideoMemory);
	if ( hResult != D3D_OK )
	{
		ASSERT((FALSE,"dtm_Initialise: GetAvailableVidMem failed\n%s",DDErrorToString(hResult)));
		return FALSE;
	}

	switch (pie_GetTexCap())
	{
		default:
		case TEX_CAP_UNDEFINED:
			if (freeVideoMemory < MIN_TEX_MEMORY)
			{
				texSize = LOW_16BIT;
				DBERROR(("DX6 has reported insufficient texture space %d.", freeVideoMemory));
				return FALSE;
			}
			else if (((freeVideoMemory >= REQ_8BIT_TEX_MEMORY) && (freeVideoMemory < REQ_TEX_MEMORY)) && texInfo.b8BPPAvailable)
			{
		#ifdef JEREMY
				DBPRINTF(("DX6 has reported limited texture space %d.\nSwitching to 8 bit texture mode.", freeVideoMemory));
		#endif
				texSize = FULL_8BIT;
				texInfo.requestedPaletteMode = DDPF_PALETTEINDEXED8;
				texInfo.requestedBPP	= 8;
				texInfo.bFoundGoodFormat = FALSE;
				// Enumerate the texture formats, and find the closest device-supported
				// texture pixel format
				hResult = pD3D3->lpVtbl->EnumTextureFormats(pD3D3, TextureSearchCallback, &texInfo );

				if ( hResult != D3D_OK )
				{
					DBERROR(("DX6 Initialise: EnumTextureFormats failed\n%s",DDErrorToString(hResult)));
					return FALSE;
				}

				if (!texInfo.bFoundGoodFormat)
				{
					DBERROR(("DX6 Initialise: RGB texture mode not found\n%s",DDErrorToString(hResult)));
					return FALSE;
				}

				hResult = pDD4->lpVtbl->CreatePalette(pDD4,	DDPCAPS_8BIT | DDPCAPS_ALLOW256, pie_GetWinPal(), &pDDPal, NULL);
				if (hResult != DD_OK)
				{
					DBERROR(("DX6 Initialise: CreatePalette failed for palettised mode.\n%s",DDErrorToString(hResult)));
					return FALSE;
				}
			}
			else if (freeVideoMemory < REQ_TEX_MEMORY)
			{
				texSize = MED_16BIT;
				DBPRINTF(("DX6 has reported limited texture space %d.\nSwitching to low texture mode.", freeVideoMemory));
			}
			break;
		case TEX_CAP_2M:
			texSize = MED_16BIT;
			break;
		case TEX_CAP_8BIT:
			if (texInfo.b8BPPAvailable)
			{
		#ifdef JEREMY
				DBPRINTF(("DX6 has reported limited texture space %d.\nSwitching to 8 bit texture mode.", freeVideoMemory));
		#endif
				texSize = FULL_8BIT;
				texInfo.requestedPaletteMode = DDPF_PALETTEINDEXED8;
				texInfo.requestedBPP	= 8;
				texInfo.bFoundGoodFormat = FALSE;
				// Enumerate the texture formats, and find the closest device-supported
				// texture pixel format
				hResult = pD3D3->lpVtbl->EnumTextureFormats(pD3D3, TextureSearchCallback, &texInfo );

				if ( hResult != D3D_OK )
				{
					DBERROR(("DX6 Initialise: EnumTextureFormats failed\n%s",DDErrorToString(hResult)));
					return FALSE;
				}

				if (!texInfo.bFoundGoodFormat)
				{
					DBERROR(("DX6 Initialise: texture mode not found\n%s",DDErrorToString(hResult)));
					return FALSE;
				}

				hResult = pDD4->lpVtbl->CreatePalette(pDD4,	DDPCAPS_8BIT | DDPCAPS_ALLOW256, pie_GetWinPal(), &pDDPal, NULL);
				if (hResult != DD_OK)
				{
					DBERROR(("DX6 Initialise: CreatePalette failed for palettised mode.\n%s",DDErrorToString(hResult)));
					return FALSE;
				}
			}
			else
			{
				texSize = FULL_16BIT;
			}
			break;
		case TEX_CAP_FULL:
			texSize = FULL_16BIT;
			break;
	}
	
	//clear surface pointers
	for (i= 0; (i < MAX_TEX_PAGES); i++)
	{
		aTextures[i].psSurface4 = NULL;
	}

	
	
	if ((texSize == FULL_16BIT) || (texSize == FULL_8BIT))
	{
		bSpace = TRUE;
		for (i= 0; ((i < MAX_TEX_PAGES) & bSpace); i++)
		{
			bSpace = dtm_CreateTextureSurface(&(aTextures[i].psSurface4), videoTextureSize, DDSCAPS_TEXTURE | DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY);

			if (bSpace)
			{
					// Create the texture
				hResult = aTextures[i].psSurface4->lpVtbl->QueryInterface(aTextures[i].psSurface4, &IID_IDirect3DTexture2,
													 (LPVOID*) &aTextures[i].psTexture2);
				if ( hResult != D3D_OK )
				{
					DBERROR(("DX6 Initialise: Query Interfacefailed for Texture Surface\n%s",DDErrorToString(hResult)));
					return FALSE;
				}
			}
			else
			{
				//that failed so free them and try for MED textures
				for (j= 0; (j < i); j++)
				{
					aTextures[j].psSurface4->lpVtbl->Release(aTextures[j].psSurface4);
				}
				if (texSize == FULL_8BIT)
				{
					//something went wrong with 8bit mode
					//so get a 16bit mode
					texInfo.requestedPaletteMode = DDPF_RGB;
					texInfo.requestedBPP	= 16;
					texInfo.b8BPPAvailable	= FALSE;
					texInfo.bFoundGoodFormat = FALSE;
					// Enumerate the texture formats, and find the closest device-supported
					// texture pixel format
					hResult = pD3D3->lpVtbl->EnumTextureFormats(pD3D3, TextureSearchCallback, &texInfo );

					if ( hResult != D3D_OK )
					{
						DBERROR(("dtm_Initialise: EnumTextureFormats failed on second pass\n%s",DDErrorToString(hResult)));
						return FALSE;
					}

					if (!texInfo.bFoundGoodFormat)
					{
						DBERROR(("dtm_Initialise: RGB texture mode failed after 8bit fail\n%s",DDErrorToString(hResult)));
						return FALSE;
					}
				}
				texSize = MED_16BIT;
			}
		}
	}

	if (texSize == MED_16BIT)
	{
		bSpace = TRUE;
		for (i= 0; ((i < HIGH_RES_PAGES) & bSpace); i++)
		{
			bSpace = dtm_CreateTextureSurface(&(aTextures[i].psSurface4), videoTextureSize, DDSCAPS_TEXTURE | DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY);

			if (bSpace)
			{
					// Create the texture
				hResult = aTextures[i].psSurface4->lpVtbl->QueryInterface(aTextures[i].psSurface4, &IID_IDirect3DTexture2,
													 (LPVOID*) &aTextures[i].psTexture2);
				if ( hResult != D3D_OK )
				{
					DBERROR(("DX6 Initialise: Query Interface failed for Texture Surface (2)\n%s",DDErrorToString(hResult)));
					return FALSE;
				}
			}
			else
			{
				//that failed so free them and try for MED textures
				for (j= 0; (j < i); j++)
				{
					aTextures[j].psSurface4->lpVtbl->Release(aTextures[j].psSurface4);
				}
				texSize = LOW_16BIT;
			}
		}

		for (i= HIGH_RES_PAGES; ((i < MAX_TEX_PAGES) & bSpace); i++)
		{
			bSpace = dtm_CreateTextureSurface(&(aTextures[i].psSurface4), SMALL_TEXTURE_SIZE, DDSCAPS_TEXTURE | DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY);

			if (bSpace)
			{
					// Create the texture
				hResult = aTextures[i].psSurface4->lpVtbl->QueryInterface(aTextures[i].psSurface4, &IID_IDirect3DTexture2,
													 (LPVOID*) &aTextures[i].psTexture2);
				if ( hResult != D3D_OK )
				{
					DBERROR(("DX6 Initialise: Query Interface failed for Texture Surface (3)\n%s",DDErrorToString(hResult)));
					return FALSE;
				}
			}
			else
			{
				//that failed so free them and try for MED textures
				for (j= 0; (j < i); j++)
				{
					aTextures[j].psSurface4->lpVtbl->Release(aTextures[j].psSurface4);
				}
				texSize = LOW_16BIT;
			}
		}
	}
		
	if (texSize == LOW_16BIT)
	{
		DBERROR(("DX6 has reported insufficient texture space %d.", freeVideoMemory));//mar10!!jps
		return FALSE;//mar10!!jps
		bSpace = TRUE;
		for (i= 0; ((i < MAX_TEX_PAGES) & bSpace); i++)
		{
			bSpace = dtm_CreateTextureSurface(&(aTextures[i].psSurface4), videoTextureSize, DDSCAPS_TEXTURE | DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY);

			if (bSpace)
			{
					// Create the texture
				hResult = aTextures[i].psSurface4->lpVtbl->QueryInterface(aTextures[i].psSurface4, &IID_IDirect3DTexture2,
													 (LPVOID*) &aTextures[i].psTexture2);
				if ( hResult != D3D_OK )
				{
					DBERROR(("DX6 Initialise: Query Interface failed for Texture Surface (4)\n%s",DDErrorToString(hResult)));
					return FALSE;
				}
			}
		}
	}

	dtm_CreateTextureSurface(&psImage256Surface4, BIG_TEXTURE_SIZE, DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD | DDSCAPS_SYSTEMMEMORY);

	if ((texSize == MED_16BIT) || (texSize == LOW_16BIT))
	{
		dtm_CreateTextureSurface(&psImage128Surface4, SMALL_TEXTURE_SIZE, DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD | DDSCAPS_SYSTEMMEMORY);
		dtm_CreateTextureSurface(&psRadarSurf4, SMALL_TEXTURE_SIZE, DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD | DDSCAPS_SYSTEMMEMORY);
	}
	else
	{
		dtm_CreateTextureSurface(&psRadarSurf4, BIG_TEXTURE_SIZE, DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD | DDSCAPS_SYSTEMMEMORY);
	}

	if (texSize != FULL_8BIT)
	{	
		dtm_Build16BitTexturePalette(&texInfo.ddpf);
	}

	SetMaterial();

	dtm_SetTexturePage(0);

	SetTextureStageStates();

	return TRUE;
}

BOOL dtm_ReleaseTextures(void)
{
	SDWORD		i;
	//free them and try for MED textures
	for (i= 0; (i < MAX_TEX_PAGES); i++)
	{
		if (aTextures[i].psSurface4 != NULL)
		{
			aTextures[i].psSurface4->lpVtbl->Release(aTextures[i].psSurface4);
			aTextures[i].psSurface4 = NULL;
		}
	}

	/* release upload surfaces */
	if ( psImage256Surface4 != NULL )
	{
		psImage256Surface4->lpVtbl->Release( psImage256Surface4 );
		psImage256Surface4 = NULL;
	}
	if ( psImage128Surface4 != NULL )
	{
		psImage128Surface4->lpVtbl->Release( psImage128Surface4 );
		psImage128Surface4 = NULL;
	}

	return TRUE;
}

BOOL dtm_ReLoadTexture(SDWORD i)
{
	if (i<0 || i>= RADAR_TEXPAGE_D3D)
	{
		return FALSE;
	}
	/* set pie texture pointer */
	if (_TEX_PAGE[i].tex.bmp == NULL)
	{
		return FALSE;
	}
	return 	dtm_LoadTexSurface( &_TEX_PAGE[i].tex, i);
}

BOOL dtm_ReloadAllTextures(void)
{
	SDWORD		i;
	//free them and try for MED textures
	for (i= 0; (i < MAX_TEX_PAGES); i++)
	{
		/* set pie texture pointer */
		if ((_TEX_PAGE[i].tex.bmp != NULL) && (i != RADAR_TEXPAGE_D3D))
		{
			dtm_LoadTexSurface( &_TEX_PAGE[i].tex, i);
		}
	}
	return TRUE;
}

BOOL dtm_RestoreTextures(void)
{
	SDWORD		i;
	//free them and try for MED textures
	for (i= 0; (i < MAX_TEX_PAGES); i++)
	{
		if (aTextures[i].psSurface4->lpVtbl->IsLost(aTextures[i].psSurface4) != DD_OK)

		{
			aTextures[i].psSurface4->lpVtbl->Restore(aTextures[i].psSurface4);
			/* set pie texture pointer */
			if ((_TEX_PAGE[i].tex.bmp != NULL) && (i != RADAR_TEXPAGE_D3D))
			{
				dtm_LoadTexSurface( &_TEX_PAGE[i].tex, i);
			}
		}
	}
	return TRUE;
}

BOOL dtm_CreateTextureSurface(LPDIRECTDRAWSURFACE4 *ppsSurface4, SDWORD size, DWORD dwCaps)
{
	LPDIRECT3DDEVICE3 pD3D3;
	LPDIRECTDRAW4	 pDD4;
	DDSURFACEDESC2	 ddSurfDesc2;
	HRESULT		hResult;
	DDCOLORKEY	blackKey;

	pD3D3 = d3d_GetpsD3DDevice3();
	pDD4 = d3d_GetpsDD4();

	blackKey.dwColorSpaceLowValue = 0;
	blackKey.dwColorSpaceHighValue = 0;

	// set texture caps */
	memset(&ddSurfDesc2, 0, sizeof(DDSURFACEDESC2));
	ddSurfDesc2.dwSize = sizeof(DDSURFACEDESC2);
	ddSurfDesc2.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS ;
	ddSurfDesc2.dwWidth = size;
	ddSurfDesc2.dwHeight = size;
	ddSurfDesc2.ddsCaps.dwCaps = dwCaps;
	memcpy(&ddSurfDesc2.ddpfPixelFormat, (void *)&texInfo.ddpf, sizeof(DDPIXELFORMAT));

	//create the surface
	hResult = pDD4->lpVtbl->CreateSurface(pDD4, &ddSurfDesc2, ppsSurface4, NULL);
	if (hResult != DD_OK)
	{
		DBPRINTF(("dtm_CreateTextureSurface: Create texture surface failed:\n%s", DDErrorToString(hResult)));
		DBPRINTF(("Unable to allocate sufficient texture space.\nSwitching to low texture mode."));
		return FALSE;
	}

	if (texInfo.requestedPaletteMode == DDPF_PALETTEINDEXED8)
	{
		// Set the palette on the texture surface
		hResult = (*ppsSurface4)->lpVtbl->SetPalette(*ppsSurface4, pDDPal);
		if (hResult != DD_OK)
		{
			DBPRINTF(("dtm_CreateTextureSurface: SetPalette failed for texture surface:\n%s",DDErrorToString(hResult)));
			return FALSE;
		}
	}

	if ( D3DGetAlphaKey() == FALSE )
	{
		//enable colourkeying for this surface
		hResult = (*ppsSurface4)->lpVtbl->SetColorKey(*ppsSurface4, DDCKEY_SRCBLT , &blackKey);
		if (hResult != DD_OK)
		{
			DBPRINTF(("dtm_CreateTextureSurface: SetColorKey failed for texture surface:\n%s",
					DDErrorToString(hResult)));
			return FALSE;
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Name: TextureSearchCallback()
// Desc: Enumeration callback routine to find a best-matching texture format. 
//       The param data is the DDPIXELFORMAT of the best-so-far matching
//       texture. Note: the desired BPP is passed in the dwSize field, and the
//       default BPP is passed in the dwFlags field.
//-----------------------------------------------------------------------------
static HRESULT CALLBACK TextureSearchCallback( DDPIXELFORMAT* pddpf, VOID* param )
{
	TEXSEARCHINFO*	psTexInfo = (TEXSEARCHINFO*)param;

    if( NULL==pddpf || NULL==param )
        return DDENUMRET_OK;


    // Skip any funky modes
    if (pddpf->dwFlags & (DDPF_LUMINANCE|DDPF_BUMPLUMINANCE|DDPF_BUMPDUDV))
    {
		return DDENUMRET_OK;
	}

	if ((pddpf->dwFlags & DDPF_PALETTEINDEXED8))//8 bits available
	{
		psTexInfo->b8BPPAvailable = TRUE;

		if (psTexInfo->requestedPaletteMode == DDPF_PALETTEINDEXED8)
		{
			// Accept the first 8-bit palettized format we get
		    memcpy( &psTexInfo->ddpf, pddpf, sizeof(DDPIXELFORMAT) );
			psTexInfo->bFoundGoodFormat = TRUE;
			return DDENUMRET_CANCEL;
		}
		else//ignor if not requested
		{
			return DDENUMRET_OK;
		}
	}
			
	if (psTexInfo->bFoundGoodFormat)//must be still looking for 8bit availability 
	{
		return DDENUMRET_OK;
	}

	if (!(pddpf->dwFlags & psTexInfo->requestedPaletteMode))//not an RGB mode so ignor
	{
		return DDENUMRET_OK;
	}

	// Else, skip any paletized formats (all modes under 16bpp)
	if( pddpf->dwRGBBitCount < psTexInfo->requestedBPP )
	{
		return DDENUMRET_OK;
	}

	// Else, skip any FourCC formats
	if( pddpf->dwFourCC != 0 )
		return DDENUMRET_OK;

	// Make sure current alpha format agrees with requested format type
	if ( D3DGetAlphaKey() == TRUE )
	{
		if ( !(pddpf->dwFlags & DDPF_ALPHAPIXELS) )
		{
			return DDENUMRET_OK;
		}
	}
	else
	{
		if (pddpf->dwFlags & DDPF_ALPHAPIXELS)
		{
			return DDENUMRET_OK;
		}
	}

    // Check if we found a good match
    if (pddpf->dwRGBBitCount == psTexInfo->requestedBPP)
    {
        memcpy(&psTexInfo->ddpf, pddpf, sizeof(DDPIXELFORMAT) );
		psTexInfo->bFoundGoodFormat = TRUE;
        return DDENUMRET_OK;
    }

    return DDENUMRET_OK;
}

void dtm_SetTexturePage(SDWORD i)
{
	LPDIRECT3DDEVICE3	pd3dDevice3 = d3d_GetpsD3DDevice3();

	if ((i >= 0) && (i < MAX_TEX_PAGES))
	{
		pd3dDevice3->lpVtbl->SetTexture(pd3dDevice3, 0, aTextures[i].psTexture2 );
	}
	else
	{
		pd3dDevice3->lpVtbl->SetTexture(pd3dDevice3, 0, NULL);
	}
//	SetTextureStageStates();
}

void SetTextureStageStates(void)
{
	LPDIRECT3DDEVICE3	pd3dDevice3 = d3d_GetpsD3DDevice3();

    pd3dDevice3->lpVtbl->SetTextureStageState(pd3dDevice3, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    pd3dDevice3->lpVtbl->SetTextureStageState(pd3dDevice3, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    pd3dDevice3->lpVtbl->SetTextureStageState(pd3dDevice3, 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	pd3dDevice3->lpVtbl->SetTextureStageState(pd3dDevice3, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR );
	pd3dDevice3->lpVtbl->SetTextureStageState(pd3dDevice3, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR );
	pd3dDevice3->lpVtbl->SetTextureStageState(pd3dDevice3, 0, D3DTSS_MIPFILTER, D3DTFP_NONE );
	pd3dDevice3->lpVtbl->SetRenderState(pd3dDevice3, D3DRENDERSTATE_DITHERENABLE, TRUE );
	pd3dDevice3->lpVtbl->SetRenderState(pd3dDevice3, D3DRENDERSTATE_SPECULARENABLE, TRUE );
}

void dx6_SetBilinear( BOOL bBilinearOn )
{
	HRESULT				hResult;
	LPDIRECT3DDEVICE3	pd3dDevice3 = d3d_GetpsD3DDevice3();

	if (bBilinearOn)
	{
		hResult = pd3dDevice3->lpVtbl->SetTextureStageState(pd3dDevice3, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR );
		ASSERT(( hResult == D3D_OK,"dx6_SetBilinear: bilinear SetRenderState failed\n%s",DDErrorToString(hResult) ));
		hResult = pd3dDevice3->lpVtbl->SetTextureStageState(pd3dDevice3, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR );
		ASSERT(( hResult == D3D_OK,"dx6_SetBilinear: bilinear SetRenderState failed\n%s",DDErrorToString(hResult) ));
	}
	else
	{
		hResult = pd3dDevice3->lpVtbl->SetTextureStageState(pd3dDevice3, 0, D3DTSS_MINFILTER, D3DTFN_POINT );
		ASSERT(( hResult == D3D_OK,"dx6_SetBilinear: bilinear SetRenderState failed\n%s",DDErrorToString(hResult) ));
		hResult = pd3dDevice3->lpVtbl->SetTextureStageState(pd3dDevice3, 0, D3DTSS_MAGFILTER, D3DTFG_POINT );
		ASSERT(( hResult == D3D_OK,"dx6_SetBilinear: bilinear SetRenderState failed\n%s",DDErrorToString(hResult) ));
	}
}

void SetMaterial(void)
{ 
	HRESULT				hResult;
	LPDIRECT3D3			pd3d3 = d3d_GetpsD3D();
	LPDIRECT3DDEVICE3	pd3dDevice3 = d3d_GetpsD3DDevice3();

    hResult = pd3d3->lpVtbl->CreateMaterial(pd3d3, &psMtrl3, NULL);
    if( hResult != DD_OK )
	{
		ASSERT((FALSE,"dtm SetMaterial: CreateMaterial failed\n%s",DDErrorToString(hResult)));
		return;
	}
	ZeroMemory( &mtrl, sizeof(D3DMATERIAL) );
    mtrl.dwSize       = sizeof(D3DMATERIAL);
    mtrl.dcvDiffuse.r = mtrl.dcvAmbient.r = 1.0f;
    mtrl.dcvDiffuse.g = mtrl.dcvAmbient.g = 1.0f;
    mtrl.dcvDiffuse.b = mtrl.dcvAmbient.b = 1.0f;
    mtrl.dwRampSize   = 16L; // A default ramp size
    mtrl.power = 40.0f;

    psMtrl3->lpVtbl->SetMaterial(psMtrl3, &mtrl );
    psMtrl3->lpVtbl->GetHandle(psMtrl3, pd3dDevice3, &hMtrl );
    pd3dDevice3->lpVtbl->SetLightState(pd3dDevice3, D3DLIGHTSTATE_MATERIAL, hMtrl );
	pd3dDevice3->lpVtbl->SetLightState(pd3dDevice3, D3DLIGHTSTATE_AMBIENT,  0x40404040 );
}

void
dtm_SetSurfaceAlpha( LPDIRECTDRAWSURFACE4 psSurf4 )
{
	HRESULT			hResult;
	DDSURFACEDESC2	ddsd;
	DWORD			dwAlphaMask, dwRGBMask,
					dwColorkey = 0x00000000; // Colorkey on black
	WORD			*p16;
	DWORD			*p32;
	DWORD			x, y;

	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);

	hResult = psSurf4->lpVtbl->GetSurfaceDesc( psSurf4, &ddsd );
	if( hResult != DD_OK )
	{
		ASSERT( (FALSE,"dtm_SetSurfaceAlpha: GetSurfaceDesc failed\n%s",
					DDErrorToString(hResult)) );
		return;
	}

	if( ddsd.ddpfPixelFormat.dwRGBAlphaBitMask )
	{
		// Lock the texture surface
		memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		while( psSurf4->lpVtbl->Lock( psSurf4, NULL, &ddsd, 0, NULL ) ==
				DDERR_WASSTILLDRAWING );
			 
		dwAlphaMask = ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
		dwRGBMask   = ( ddsd.ddpfPixelFormat.dwRBitMask |
						ddsd.ddpfPixelFormat.dwGBitMask |
						ddsd.ddpfPixelFormat.dwBBitMask );

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
						*p16 |= dwAlphaMask;
					p16++;
				}
				if( ddsd.ddpfPixelFormat.dwRGBBitCount == 32 )
				{
					if( ( *p32 &= dwRGBMask ) != dwColorkey )
						*p32 |= dwAlphaMask;
					p32++;
				}
			}
			
			psSurf4->lpVtbl->Unlock( psSurf4, NULL );
		}
	}
}

BOOL dtm_LoadTexSurface( iTexture *psIvisTex, SDWORD index )
{
	LPDIRECTDRAWSURFACE4	psSurf4;
	DDCOLORKEY				blackKey;
	
	blackKey.dwColorSpaceLowValue = 0;
	blackKey.dwColorSpaceHighValue = 0;

	if ((texSize == LOW_16BIT) || ((texSize == MED_16BIT) && (index >= HIGH_RES_PAGES)))
	{
		psSurf4 = psImage128Surface4;
	}
	else
	{
		psSurf4 = psImage256Surface4;
	}
//mar7	if (!dtm_surfLoadFrom8Bit(psSurf4, psIvisTex->width, psIvisTex->height, psIvisTex->bmp, (texSize == FULL_8BIT)))
	if (!surfLoadFrom8Bit(psSurf4, psIvisTex->width, psIvisTex->height, psIvisTex->bmp,  pie_GetWinPal()))
	{
		ASSERT((FALSE,"dtm SetMaterial: CreateMaterial failed."));
		return FALSE;
	}

#if 1 //alpaha is set for palette used in dtm_surfLoadFrom8Bit
	if ( D3DGetAlphaKey() == TRUE )
	{
		/* set transparent bits for textures with real alpha (not palettized) */
		dtm_SetSurfaceAlpha( psSurf4 );
	}
#endif
	
	/* copy texture to on-card surface */
    aTextures[index].psSurface4->lpVtbl->Blt(aTextures[index].psSurface4, NULL, psSurf4, NULL, DDBLT_WAIT, NULL );

	return TRUE;
}

 
BOOL dtm_BLTRadarToTex(void)
{
#if 0 //alpaha is set for palette used in dtm_surfLoadFrom8Bit
	if ( D3DGetAlphaKey() == TRUE )
	{
		/* set transparent bits for textures with real alpha (not palettized) */
		dtm_SetSurfaceAlpha( psRadarSurf4 );
	}
#endif

   aTextures[RADAR_TEXPAGE_D3D].psSurface4->lpVtbl->Blt(aTextures[RADAR_TEXPAGE_D3D].psSurface4, NULL, psRadarSurf4, NULL, DDBLT_WAIT, NULL );
   return TRUE;
}


BOOL dtm_LoadRadarSurface(BYTE* radarBuffer)
{
	if ((texSize == MED_16BIT) || (texSize == LOW_16BIT))
	{
		psRadarSurf4 = psImage128Surface4;
	}
	else
	{
		psRadarSurf4 = psImage256Surface4;
	}

	if (!dtm_surfLoadFrom8Bit(psRadarSurf4, 128, 128, radarBuffer, (texSize == FULL_8BIT)))
	{
		ASSERT((FALSE,"dtm SetMaterial: CreateMaterial failed."));
		return FALSE;
	}
	return dtm_BLTRadarToTex();
}

SDWORD dtm_GetRadarTexImageSize(void)
{
	if ((texSize == MED_16BIT) || (texSize == LOW_16BIT))
	{
		return 256;//the radar fills the texture
	}
	return 128;//the radar is in the top left of the image
}


BOOL dtm_surfLoadFrom8Bit(
			LPDIRECTDRAWSURFACE4		psSurf4,		// The surface to load to
			UDWORD width, UDWORD height,			// The size of the image data
			UBYTE				*pImageData,		// The image data
			BOOL	b8Bit)							// palette size
{
	UBYTE	*s8;
	UBYTE	*d8;
	UWORD	*d16;
	UDWORD	x, y;
	DDSURFACEDESC2	ddsd;

	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	while( psSurf4->lpVtbl->Lock( psSurf4, NULL, &ddsd, 0, NULL ) ==
				DDERR_WASSTILLDRAWING );


	s8 = pImageData;
	if (b8Bit)
	{
		d8 = ddsd.lpSurface;
		for( y=0; y<height; y++ )
		{
        
			for( x=0; x<width; x++ )
			{
				d8[x] = s8[x];
			}
			s8 += width;
			d8 += ddsd.lPitch;
		}
	}
	else
	{
		d16 = (UWORD*)ddsd.lpSurface;
		for( y=0; y<height; y++ )
		{
        
			for( x=0; x<width; x++ )
			{
				d16[x] = texPal16Bit[s8[x]];
			}
			s8 += width;
			d16 = (UWORD *)((UBYTE *)d16 + ddsd.lPitch);
		}
	}
	

	psSurf4->lpVtbl->Unlock( psSurf4, NULL );
	
	return TRUE;
}

BOOL dtm_Build16BitTexturePalette(DDPIXELFORMAT* DDPixelFormat)
{
iColour* psPal;
UDWORD	i;
UWORD	alpha, red,green,blue;
BYTE				ap = 0,	ac = 0, rp = 0,	rc = 0, gp = 0,	gc = 0, bp = 0, bc = 0;
ULONG				mask;

	/*
	// Cannot convert iof not 16bit mode 
	*/
	
	ASSERT((DDPixelFormat->dwRGBBitCount == 16, "dtm_Build16BitTexturePalette RGB bit count not 16"));

	psPal =	pie_GetGamePal();

	/*
	// Cannot playback if not 16bit mode 
	*/
	if( DDPixelFormat->dwRGBBitCount == 16 )
	{
		/*
		// Find out the RGB type of the surface and tell the codec...
		*/
		mask = DDPixelFormat->dwRGBAlphaBitMask;

		if(mask!=0)
		{
			while(!(mask & 1))
			{
				mask>>=1;
				ap++;
			}
		}

		while((mask & 1))
		{
			mask>>=1;
			ac++;
		}

		mask = DDPixelFormat->dwRBitMask;

		if(mask!=0)
		{
			while(!(mask & 1))
			{
				mask>>=1;
				rp++;
			}
		}

		while((mask & 1))
		{
			mask>>=1;
			rc++;
		}

		mask = DDPixelFormat->dwGBitMask;

		if(mask!=0)
		{
			while(!(mask & 1))
			{
				mask>>=1;
				gp++;
			}
		}

		while((mask & 1))
		{
			mask>>=1;
			gc++;
		}

		mask = DDPixelFormat->dwBBitMask;

		if(mask!=0)
		{
			while(!(mask & 1))
			{
				mask>>=1;
				bp++;
			}
		}

		while((mask & 1))
		{
			mask>>=1;
			bc++;
		}
	}

	alpha = 0;

	for(i=0; i<PALETTE_SIZE; i++)
	{
		//alpha = 0 when i = 0
		red = (UWORD) psPal[i].r;
		green = (UWORD) psPal[i].g;
		blue = (UWORD) psPal[i].b;

		alpha >>= (8-ac);  
		red >>= (8-rc);  
		blue >>= (8-bc);  
		green >>= (8-gc);  

		alpha <<= ap;  
		red <<= rp;  
		blue <<= bp;  
		green <<= gp;
		
		texPal16Bit[i] = alpha + red + green + blue;		
		alpha = 0xff;//alpha = 0xff when i > 0
	}
	return(TRUE);

}
