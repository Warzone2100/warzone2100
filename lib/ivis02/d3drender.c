/***************************************************************************/

#include <stdio.h>

#include "ivi.h"
#include "rendmode.h"
#include "tex.h"
#include "piePalette.h"
#include "pieState.h"
#include "pieClip.h"
#include "frameint.h"

#include "d3drender.h"
#include "texd3d.h"
#include "dx6TexMan.h"

#ifndef PIEPSX		// was #ifdef WIN32


/***************************************************************************/
/* Defines */
//set hardware clipping control
#ifdef D3D_CLIP_IN_SOFTWARE
	#define CLIP_STATUS				D3DDP_DONOTCLIP
#else
	#define CLIP_STATUS				0
#endif

#define	MAX_STR					128

#define MAXD3DDRIVERS			10
#define	MAXDISPLAYMODES			50
#define	MAXTEXTUREFORMATS		50

#define	TEXEL_OFFSET_256		(1.0f/512.0f)

/***************************************************************************/

enum
{
  UNKNOWN_ID,
  RIVA_128,
  RIVA_TNT,
  PERMEDIA2
};

/***************************************************************************/
/* Macros */

#define ATTEMPT(x) if (!(x)) goto exit_with_error
#define ATTEMPTD3D(x) if (x != D3D_OK) goto exit_with_error

/***************************************************************************/
/* local typedefs */

typedef struct D3DDRIVER
{
	char				szName[30];		/* short name of the driver        */
	char				szAbout[50];	/* short string about the driver   */
	D3DDEVICEDESC		Desc;			/* D3DDEVICEDESC for complete info */
	GUID				Guid;			/* device ID                       */
	BOOL				bIsHardware;	/* hardware device?                */
	BOOL				bDoesZBuffer;	/* can do zBuffering?		       */
	BOOL				bDoesTextures;	/* can do texture mapping?         */
	BOOL				bCanDoWindow;	/* can render to display depth?    */
}
D3DDRIVER;

/* Describe a display mode */
typedef struct D3DAPPMODE
{
	int		w;					/* width */
	int		h;					/* height */
	int		bpp;				/* bits per pixel */
	BOOL	bThisDriverCanDo;	/* can current driver render in this mode? */
}
D3DAPPMODE;

/* Describe a texture format */
typedef struct TEXTUREFORMAT
{
	DDPIXELFORMAT	ddsd;			/* complete texture information        */
	BOOL			bPalettized;	/* format palettized                   */
	BOOL			bAlphaKeyed;	/* colour keying on alpha texture      */
	UWORD			uwRedBPP;		/* red bits per pixel                  */
	UWORD			uwBlueBPP;		/* blue bits per pixel                 */
	UWORD			uwGreenBPP;		/* green bits per pixel                */
	UWORD			uwIndexBPP;		/* number of bits in palette index     */
}
TEXTUREFORMAT;

typedef struct DISP3DGLOBALS
{
	LPDIRECT3D3				psD3D3;			/* D3D object                  */
	LPDIRECT3DDEVICE3		psD3DDevice3;	/* D3D device                  */
	LPDIRECT3DVIEWPORT3		psD3DViewport3;	/* D3D viewport                */
	LPDIRECTDRAW4			psDD4;			/* DD object                   */
	LPDIRECTDRAWSURFACE4	psBack4;			/* back buffer pointer         */
	UWORD					uwNumDrivers;	/* number of D3D drivers       */
    UWORD					uwCurrDriver;	/* current D3D driver          */
	D3DDRIVER				Driver[MAXD3DDRIVERS];	/* available drivers   */
	D3DAPPMODE				WindowsDisplay;	/* current Windows disply mode */
	LPDIRECTDRAWSURFACE4	psZBuffer;		/* z-buffer surface            */
	LPDIRECTDRAWPALETTE		psPalette;		/* Front buffer's palette      */
	LPDIRECT3DLIGHT			psD3DLight;		/* pointer to light            */

//	SIZE					sizeClient;		/* dimensions of client window */
	POINT					posClientOnPrimary;	/* position of client win  */

	UWORD					uwNumTextureFormats;/* num. texture formats    */
	UWORD					uwCurrTextureFormat;/* current texture format  */

	/* description of all available texture formats */
	TEXTUREFORMAT			TextureFormat[MAXTEXTUREFORMATS];

	BOOL					bHaveTextHandles;
	DWORD					dwNumTextures;

	BOOL					bPrimaryPalettized;	/* front buff palettized?  */
	BOOL					bPaletteActivate;	/* front buff's pal valid? */
	BOOL					bBackBufferInVideo;	/* back buf in video mem?  */
	BOOL					bZBufferInVideo;	/* Z-buf in video mem?     */
	BOOL					bIsPrimary;			/* primary DD dev?         */

//	BOOL					bZBufferOn;			/* Z buffer is on          */
//	BOOL					bZBufferCompareOn;	/* Z buffer compare is on  */
	BOOL					bPerspCorrect;		/* persp correction is on  */
	D3DSHADEMODE			ShadeMode;			/* flat, gouraud, phong?   */
//	D3DTEXTUREFILTER		TextureFilter;		/* bi/linear text filter   */
	D3DTEXTUREBLEND			TextureBlend;		/* use shade/copy mode?    */
	BOOL					bSubPixel;			/* sub pixel correction    */
	BOOL					bLastPixel;			/* last pixel rendering    */
	D3DFILLMODE				FillMode;			/* solid, lines or points? */
	BOOL					bDithering;			/* dithering is on         */
	BOOL					bSpecular;			/* specular highlights?    */
	BOOL					bAntialiasing;		/* anti-aliasing?          */
	BOOL					bFogEnabled;		/* fog is on               */
	D3DCOLOR				FogColor;			/* fog color               */
	D3DFOGMODE				FogMode;			/* linear, exp. etc.       */
	D3DVALUE				FogStart;			/* begining depth          */
	D3DVALUE				FogEnd;				/* ending depth            */
}
DISP3DGLOBALS;

/***************************************************************************/

extern iColour	_PALETTES[PALETTE_MAX][256];

/* local funcs */

static BOOL		rend_InitD3D( void );
static void		D3DFreeTexturePage( TEXPAGE_D3D *psTexPage );
static BOOL		CreateDevice( int iDriver );
static BOOL		CreateViewport( DWORD dwWidth, DWORD dwHeight );
static BOOL		SetRenderState( void );
static BOOL		ChooseDriver( void );
static void		D3DSetCulling( BOOL bCullingOn );

/***************************************************************************/
/* global variables */

static D3DINFO			g_sD3Dinfo;
static DISP3DGLOBALS	g_sD3DGlob;
static void				*g_pCurTexture = NULL;
static D3DTLVERTEX		d3dVrts[pie_MAX_POLY_VERTS];
static float			g_fTextureOffset = 0.0f;
static SDWORD			g_iCardID = UNKNOWN_ID;
static BOOL				g_bTexelOffsetOn;

/***************************************************************************/

BOOL
InitD3D( D3DINFO *psD3Dinfo )
{
	/* copy input struct */
	memcpy( &g_sD3Dinfo, psD3Dinfo, sizeof(D3DINFO) );

	return rend_InitD3D();
}

/***************************************************************************/
#if 0
void
D3DFreeTexturePages( void )
{
	DWORD	i;

	/* free D3D texture pages */
	for ( i=0; i<iV_TEX_MAX; i++ )
	{
		if ( _TEX_PAGE[i].tex.pTex != NULL )
		{
			D3DFreeTexturePage( _TEX_PAGE[i].tex.pTex );
			FREE( _TEX_PAGE[i].tex.pTex );
			_TEX_PAGE[i].tex.pTex = NULL;
		}
	}
}
#endif
/***************************************************************************/

void
ShutDownD3D( void )
{
	dtm_ReleaseTextures();

	RELEASE( g_sD3DGlob.psD3DDevice3 );
	RELEASE( g_sD3DGlob.psPalette );
	RELEASE( g_sD3DGlob.psD3DViewport3 );

	if ( g_sD3Dinfo.bHardware == TRUE )//inc reference renderer
	{
		RELEASE( g_sD3DGlob.psZBuffer );
	}

	RELEASE( g_sD3DGlob.psD3D3 );
}

/***************************************************************************/

void
BeginSceneD3D( void )
{
	HRESULT				hResult;
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;
	D3DRECT				rClear;
	static BOOL bFirstError = FALSE;

	if ( g_sD3Dinfo.bHardware == TRUE )//inc reference renderer
	{
		rClear.x1 = 0;
		rClear.y1 = 0;
		rClear.x2 = pie_GetVideoBufferWidth();
		rClear.y2 = pie_GetVideoBufferHeight();

		hResult = g_sD3DGlob.psD3DViewport3->lpVtbl->Clear(
					g_sD3DGlob.psD3DViewport3, 1, &rClear, D3DCLEAR_ZBUFFER );
		if ( hResult != D3D_OK )
		{
			DBERROR( ("BeginSceneD3D: z buffer clear failed:\n%s",
						DDErrorToString(hResult) ) );
		}
	}
	
	hResult = psDev->lpVtbl->BeginScene( psDev );
	if ( hResult != D3D_OK )
	{
		if (!bFirstError)
		{
			ASSERT((bFirstError,"BeginSceneD3D: BeginScene failed\n%s",
					DDErrorToString(hResult) ) );
			bFirstError = TRUE;
		}
	}
}

/***************************************************************************/

void
EndSceneD3D( void )
{
	HRESULT	hResult;
	static BOOL bFirstError = FALSE;

	hResult = g_sD3DGlob.psD3DDevice3->lpVtbl->EndScene(
				g_sD3DGlob.psD3DDevice3 );
	if ( hResult != D3D_OK )
	{
		ASSERT((bFirstError,"EndSceneD3D: EndScene failed\n%s",DDErrorToString(hResult)));
		bFirstError = TRUE;;
	}
}

/***************************************************************************/


/***************************************************************************/
/*
 * lowest level poly draw
 */
/***************************************************************************/

void D3DDrawPoly( int nVerts, D3DTLVERTEX * psVert)
{
	HRESULT			hResult;
	static BOOL bFirstError = FALSE;
	if (nVerts == 3)//triangle
	{
		hResult = g_sD3DGlob.psD3DDevice3->lpVtbl->DrawPrimitive(
					g_sD3DGlob.psD3DDevice3,
					D3DPT_TRIANGLEFAN,
					D3DFVF_TLVERTEX,
					(LPVOID)psVert,
					nVerts,
					CLIP_STATUS );
	}
	else if (nVerts > 3)//poly
	{
		hResult = g_sD3DGlob.psD3DDevice3->lpVtbl->DrawPrimitive(
					g_sD3DGlob.psD3DDevice3,
					D3DPT_TRIANGLEFAN,
					D3DFVF_TLVERTEX,
					(LPVOID)psVert,
					nVerts,
					CLIP_STATUS );
	}
	else if (nVerts == 2)//line
	{
		hResult = g_sD3DGlob.psD3DDevice3->lpVtbl->DrawPrimitive(
					g_sD3DGlob.psD3DDevice3,
					D3DPT_LINELIST,
					D3DFVF_TLVERTEX,
					(LPVOID)psVert,
					nVerts,
					CLIP_STATUS );
	}
	else//point
	{
		hResult = g_sD3DGlob.psD3DDevice3->lpVtbl->DrawPrimitive(
					g_sD3DGlob.psD3DDevice3,
					D3DPT_POINTLIST,
					D3DFVF_TLVERTEX,
					(LPVOID)psVert,
					nVerts,
					CLIP_STATUS );
	}


	if ( hResult != D3D_OK ) 
	{
		ASSERT((bFirstError,"DrawTriD3D: DrawPrimitive failed\n%s",DDErrorToString(hResult)));
		bFirstError = TRUE;
	}
}

/***************************************************************************/
/*
 * PIEVERTEX poly draw
 */
/***************************************************************************/

void
D3D_PIEPolygon( SDWORD numVerts, PIEVERTEX* pVrts)
{
     SDWORD    i;
     SDWORD    iFlags = 0;

     for(i = 0; i < numVerts; i++)
     {
          d3dVrts[i].sx = (float)pVrts[i].sx;
          d3dVrts[i].sy = (float)pVrts[i].sy;
          //cull triangles with off screen points
          if (d3dVrts[i].sy > (float)LONG_TEST)
          {
               return;
          }
          d3dVrts[i].sz = (float)pVrts[i].sz * (float)INV_MAX_Z;
          d3dVrts[i].rhw = (float)1.0 / pVrts[i].sz;
          d3dVrts[i].tu = (float)pVrts[i].tu * (float)INV_TEX_SIZE + g_fTextureOffset;
          d3dVrts[i].tv = (float)pVrts[i].tv * (float)INV_TEX_SIZE + g_fTextureOffset;
          d3dVrts[i].color = pVrts[i].light.argb;
          d3dVrts[i].specular = pVrts[i].specular.argb;
     }

     D3DDrawPoly( numVerts, d3dVrts);
}

/***************************************************************************/

void
D3DInitDesc( LPD3DDEVICEDESC psD3DDevDesc )
{
    memset( psD3DDevDesc, 0, sizeof(D3DDEVICEDESC) );
    psD3DDevDesc->dwSize                  = sizeof(D3DDEVICEDESC);
    psD3DDevDesc->dtcTransformCaps.dwSize = sizeof(D3DTRANSFORMCAPS);
    psD3DDevDesc->dlcLightingCaps.dwSize  = sizeof(D3DLIGHTINGCAPS);
    psD3DDevDesc->dpcLineCaps.dwSize      = sizeof(D3DPRIMCAPS);
    psD3DDevDesc->dpcTriCaps.dwSize       = sizeof(D3DPRIMCAPS);
}

/***************************************************************************/

void
D3DGetCaps( void )
{
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;
	HRESULT				hRes;
	D3DDEVICEDESC		sD3DHWDevDesc,  
						sD3DHELDevDesc;

	D3DInitDesc( &sD3DHWDevDesc );
	D3DInitDesc( &sD3DHELDevDesc );
	ATTEMPTD3D( (hRes = psDev->lpVtbl->GetCaps( psDev, &sD3DHWDevDesc, &sD3DHELDevDesc )) );

	if ( !(sD3DHWDevDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGVERTEX) )
	{
		DBPRINTF( ("D3DGetCaps: device can't do vertex fog\n") );
	}

	if ( !(sD3DHWDevDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_ALPHAFLATBLEND) )
	{
		DBPRINTF( ("D3DGetCaps: device can't do alpha\n") );
		/* set stippled alpha */
		if ( sD3DHWDevDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_ALPHAFLATSTIPPLED )
		{
			DBPRINTF( ("Enabling stippled alpha") );
			ATTEMPTD3D( (psDev->lpVtbl->SetRenderState( psDev,
							D3DRENDERSTATE_STIPPLEDALPHA, TRUE )) );
		}
	}

	return;

exit_with_error:

	DBERROR( ("D3DGetCaps:\n%s\n", DDErrorToString(hRes)) );
}

/***************************************************************************/

void
D3DEnableFog( BOOL bEnable )
{
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;
	HRESULT				hRes;
	static BOOL			bEnableLast, bFirst = TRUE;
 
	if ( bFirst || (bEnableLast != bEnable) )
	{
	    ATTEMPTD3D( (hRes = psDev->lpVtbl->SetRenderState( psDev,
						D3DRENDERSTATE_FOGENABLE, bEnable)) );
	}

	if ( bFirst )
	{
		bFirst = FALSE;
	}

	bEnableLast = bEnable;

	return;

exit_with_error:

	DBERROR( ("D3DEnableFog:\n%s\n", DDErrorToString(hRes)) );
}

/***************************************************************************/

void
D3DSetFogColour( D3DCOLOR dwColor )
{
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;
	HRESULT				hRes;
 
    ATTEMPTD3D( (hRes = psDev->lpVtbl->SetRenderState(psDev,
					D3DRENDERSTATE_FOGCOLOR, dwColor)) );

	return;

exit_with_error:

	DBERROR( ("D3DSetFogColour:\n%s\n", DDErrorToString(hRes)) );
}

/***************************************************************************/

void
D3DSetTexelOffsetState( BOOL bOffsetOn )
{
	g_bTexelOffsetOn = bOffsetOn;
}

/***************************************************************************/

SDWORD
D3DCheckCard( void )
{
	LPDIRECTDRAW4		psDD = screenGetDDObject();
	HRESULT				hRes;
	DDDEVICEIDENTIFIER	did;

	hRes = psDD->lpVtbl->GetDeviceIdentifier( psDD, &did, 0 );
	if ( hRes != DD_OK )
	{
		DBPRINTF( ("D3DCheckCard: couldn't get card ID\n") );
		return UNKNOWN_ID;
	}

	switch ( did.dwVendorId )
	{
		case 0x12d2:
			if ( did.dwDeviceId == 0x18 || did.dwDeviceId == 0x19 )
			{
				return RIVA_128;
			}
			break;
	
		case 0x10DE:
			if ( did.dwDeviceId == 0x20 || did.dwDeviceId == 0x28 ||
				 did.dwDeviceId == 0x2C                              )
			{
				return RIVA_TNT;
			}
			break;

		case 0x3D3D:
			if ( did.dwDeviceId == 0x0007 || did.dwDeviceId == 0x0009 )
			{
				return PERMEDIA2;
			}
			break;

		case 0x104C:
			if ( did.dwDeviceId == 0x3D07 )
			{
				return PERMEDIA2;
			}
			break;
	}

	return UNKNOWN_ID;
}

/***************************************************************************/

void
D3DSetCardSpecificParams( void )
{
	g_iCardID = D3DCheckCard();

	switch ( g_iCardID )
	{
		case RIVA_128:
			DBPRINTF( ("Nvidia Riva 128 detected\n") );
			g_sD3Dinfo.bAlphaKey = TRUE;
			if ( g_bTexelOffsetOn )
			{
				g_fTextureOffset = TEXEL_OFFSET_256;
			}
			break;

		case RIVA_TNT:
			DBPRINTF( ("Nvidia Riva TNT detected\n") );
			g_sD3Dinfo.bAlphaKey = TRUE;
			if ( g_bTexelOffsetOn )
			{
				g_fTextureOffset = TEXEL_OFFSET_256;
			}
			break;

		case PERMEDIA2:
			DBPRINTF( ("3DLabs Permedia 2 detected\n") );
			pie_SetFogCap(FOG_CAP_GREY);
			break;

		case UNKNOWN_ID:
			break;
	}
}

/***************************************************************************/

void	
D3DValidateDevice( void )
{
	DWORD			dwPasses;
	HRESULT			hRes;

	D3DSetTranslucencyMode( TRANS_ALPHA );
	hRes = g_sD3DGlob.psD3DDevice3->lpVtbl->ValidateDevice(
			g_sD3DGlob.psD3DDevice3, &dwPasses );
	if ( hRes != D3D_OK )
	{
		DBPRINTF( ("D3DValidateDevice: can't do trans_alpha: D3D reported %s\n",
					DDErrorToString(hRes) ) );
	}

	D3DSetTranslucencyMode( TRANS_ADDITIVE );
	hRes = g_sD3DGlob.psD3DDevice3->lpVtbl->ValidateDevice(
			g_sD3DGlob.psD3DDevice3, &dwPasses );
	if ( hRes != D3D_OK )
	{
		DBPRINTF( ("D3DValidateDevice: can't do trans_additive: D3D reported %s\n",
					DDErrorToString(hRes) ) );
	}
}

/***************************************************************************/

void
D3DSetTranslucencyMode( TRANSLUCENCY_MODE transMode )
{
	HRESULT				hResult;
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;
	static BOOL			bFirst = TRUE,
						bBlendEnableLast;
	BOOL				bBlendEnable;

	static D3DBLEND		srcBlendLast  = D3DBLEND_ZERO,
						destBlendLast = D3DBLEND_ZERO;
	D3DBLEND			srcBlend, destBlend;

	static DWORD		dwAlphaOpLast   = D3DTOP_DISABLE, 
						dwAlphaArg1Last = -1,
						dwAlphaArg2Last = -1;
	DWORD				dwAlphaOp       = D3DTOP_DISABLE,
						dwAlphaArg1     = -1,
						dwAlphaArg2     = -1;

	//dont write to z buffer if alpha on
	//controlled by piestates
	switch (transMode)
	{
		case TRANS_ALPHA:
			//GR_BLEND_SRC_ALPHA,
			//GR_BLEND_ONE_MINUS_SRC_ALPHA,
			srcBlend     = D3DBLEND_SRCALPHA;
			destBlend    = D3DBLEND_INVSRCALPHA;
			bBlendEnable = TRUE;

			dwAlphaOp   = D3DTOP_MODULATE;
			dwAlphaArg1 = D3DTA_TEXTURE;
			dwAlphaArg2 = D3DTA_DIFFUSE;
			break;

		case TRANS_ADDITIVE:
			//GR_BLEND_SRC_ALPHA,
			//GR_BLEND_ONE,
			srcBlend     = D3DBLEND_ONE;
			destBlend    = D3DBLEND_ONE;
			bBlendEnable = TRUE;

			dwAlphaOp    = D3DTOP_SELECTARG1;
			dwAlphaArg1  = D3DTA_DIFFUSE;
			break;

		case TRANS_FILTER:						
			//GR_BLEND_SRC_ALPHA,
			//GR_BLEND_SRC_COLOR,
			srcBlend     = D3DBLEND_SRCALPHA;
			destBlend    = D3DBLEND_SRCCOLOR;
			bBlendEnable = TRUE;

			dwAlphaOp    = D3DTOP_SELECTARG1; 
			dwAlphaArg1  = D3DTA_DIFFUSE;
			break;

		default:
		case TRANS_DECAL:
			srcBlend     = D3DBLEND_ONE;
			destBlend    = D3DBLEND_ZERO;
			bBlendEnable = FALSE;

			dwAlphaOp    = D3DTOP_SELECTARG1;
			dwAlphaArg1  = D3DTA_TEXTURE;
			break;
	}

	if ( bFirst || (srcBlend != srcBlendLast) )
	{
		ATTEMPTD3D( (hResult = psDev->lpVtbl->SetRenderState( psDev,
								D3DRENDERSTATE_SRCBLEND, srcBlend)) );
	}

	if ( bFirst || (destBlend != destBlendLast) )
	{
		ATTEMPTD3D( (hResult = psDev->lpVtbl->SetRenderState( psDev,
								D3DRENDERSTATE_DESTBLEND, destBlend)) );
	}

	if ( bFirst || (bBlendEnable != bBlendEnableLast) )
	{
		ATTEMPTD3D( (hResult = psDev->lpVtbl->SetRenderState( psDev,
								D3DRENDERSTATE_ALPHABLENDENABLE, bBlendEnable )) );
	}

	if ( bFirst || (dwAlphaOp != dwAlphaOpLast) )
	{
		ATTEMPTD3D( (hResult = psDev->lpVtbl->SetTextureStageState( psDev, 0,
					D3DTSS_ALPHAOP, dwAlphaOp )) );
	}

	if ( (bFirst || (dwAlphaArg1 != dwAlphaArg1Last)) && (dwAlphaArg1 != -1) )
	{
		ATTEMPTD3D( (hResult = psDev->lpVtbl->SetTextureStageState( psDev, 0,
					D3DTSS_ALPHAARG1, dwAlphaArg1 )) );
	}

	if ( (bFirst || (dwAlphaArg2 != dwAlphaArg2Last)) && (dwAlphaArg2 != -1) )
	{
		ATTEMPTD3D( (hResult = psDev->lpVtbl->SetTextureStageState( psDev, 0,
					D3DTSS_ALPHAARG2, dwAlphaArg2 )) );
	}

	/* update statics */
	if ( bFirst == TRUE )
	{
		bFirst = FALSE;
	}
	srcBlendLast    = srcBlend;
	destBlendLast   = destBlend;
	dwAlphaOpLast   = dwAlphaOp;
	dwAlphaArg1Last = dwAlphaArg1;
	dwAlphaArg2Last = dwAlphaArg2;

	return;

exit_with_error:

	DBERROR( ("D3DSetTranslucencyMode:\n%s\n", DDErrorToString(hResult)) );
}

/***************************************************************************/

void
D3DSetAlphaKey( BOOL bAlphaOn )
{
	g_sD3Dinfo.bAlphaKey = bAlphaOn;
}

/***************************************************************************/

BOOL
D3DGetAlphaKey( void )
{
	return g_sD3Dinfo.bAlphaKey;
}

/***************************************************************************/

void
D3DSetColourKeying( BOOL bKeyingOn )
{
	HRESULT				hResult;
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;

	if ( D3DGetAlphaKey() == TRUE )
	{
		hResult = psDev->lpVtbl->SetRenderState( psDev,
						D3DRENDERSTATE_ALPHATESTENABLE, bKeyingOn );
		if ( hResult != D3D_OK )
		{
			DBERROR( ("D3DSetColourKeying: alpha test SetRenderState failed\n%s",
						DDErrorToString(hResult) ) );
		}
	}
	else
	{
		hResult = psDev->lpVtbl->SetRenderState( psDev,
						D3DRENDERSTATE_BLENDENABLE, bKeyingOn );
		if ( hResult != D3D_OK )
		{
			DBERROR( ("D3DSetColourKeying: blend enable SetRenderState failed\n%s",
						DDErrorToString(hResult) ) );
		}
		hResult = psDev->lpVtbl->SetRenderState( psDev,
						D3DRENDERSTATE_COLORKEYENABLE, bKeyingOn );
		if ( hResult != D3D_OK )
		{
			DBERROR( ("D3DSetColourKeying: colour key SetRenderState failed\n%s",
						DDErrorToString(hResult) ) );
		}
	}
}

/***************************************************************************/

void D3DSetDepthBuffer( BOOL bDepthBufferOn )
{
	HRESULT				hResult;
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;

	hResult =  psDev->lpVtbl->SetRenderState( psDev,
				D3DRENDERSTATE_ZENABLE, bDepthBufferOn &&
				g_sD3DGlob.Driver[g_sD3DGlob.uwCurrDriver].bDoesZBuffer );
	if ( hResult != D3D_OK )
	{
		DBERROR( ("D3DSetDepthBuffer: bZBufferOn SetRenderState failed\n%s",
					DDErrorToString(hResult) ) );
	}
}

/***************************************************************************/

void D3DSetDepthWrite( BOOL bWriteEnable )
{
	HRESULT				hResult;
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;

	hResult =  psDev->lpVtbl->SetRenderState( psDev,
				D3DRENDERSTATE_ZWRITEENABLE, bWriteEnable &&
				g_sD3DGlob.Driver[g_sD3DGlob.uwCurrDriver].bDoesZBuffer);
	if ( hResult != D3D_OK )
	{
		DBERROR( ("D3DSetDepthWrite: bWriteEnable SetRenderState failed\n%s",
					DDErrorToString(hResult) ) );
	}
}

/***************************************************************************/

void D3DSetDepthCompare( D3DCMPFUNC depthCompare )
{
	HRESULT				hResult;
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;

	hResult =  psDev->lpVtbl->SetRenderState( psDev,
					D3DRENDERSTATE_ZFUNC, depthCompare );
	if ( hResult != D3D_OK )
	{
		DBERROR( ("D3DSetDepthCompare: depthCompare SetRenderState failed\n%s",
					DDErrorToString(hResult) ) );
	}
}

/***************************************************************************/

void
D3DSetCulling( BOOL bCullingOn )
{
	HRESULT				hResult;
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;
	D3DCULL				cullMode;

	if ( bCullingOn == TRUE )
	{
		cullMode = D3DCULL_CCW;
	}
	else
	{
		cullMode = D3DCULL_NONE;
	}

	hResult = psDev->lpVtbl->SetRenderState( psDev,
					D3DRENDERSTATE_CULLMODE, cullMode );
	if ( hResult != D3D_OK )
	{
		DBERROR( ("D3DSetCulling: cull mode SetRenderState failed\n%s",
					DDErrorToString(hResult) ) );
	}
}

/***************************************************************************/

void
D3DSetClipWindow(SDWORD xMin, SDWORD yMin, SDWORD xMax, SDWORD yMax)
{
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;
	D3DCLIPSTATUS		status;
	HRESULT				hResult;

	status.dwFlags = D3DCLIPSTATUS_EXTENTS2;
	status.dwStatus = 0;
	status.maxx = (float)xMax;
	status.minx = (float)xMin;
	status.maxy = (float)yMax;
	status.miny = (float)yMin;
	status.maxz = 0.0f;
	status.minz = 0.0f;

	/* set clip state */
	hResult = psDev->lpVtbl->SetClipStatus( psDev,  &status);
	if ( hResult != D3D_OK )
	{
		DBERROR( ("D3DSetClipWindow: failed\n%s",
					DDErrorToString(hResult) ) );
	}
}
/***************************************************************************/
/*
 * SetRenderState
 *
 * Sets the render state and light state for the current viewport.
 */
/***************************************************************************/

BOOL
SetRenderState( void )
{
	HRESULT				hResult;
	LPDIRECT3DDEVICE3	psDev = g_sD3DGlob.psD3DDevice3;
	D3DCLIPSTATUS		status={ D3DCLIPSTATUS_EXTENTS2, 0, 640.0f, 0.0f,
									480.0f, 0.0f, 0.0f, 0.0f };

//	/* set rendering defaults */
//	if ( g_sD3Dinfo.bHardware == TRUE )
//	{
//		g_sD3DGlob.TextureFilter     = D3DFILTER_NEAREST;//D3DFILTER_LINEAR
//	}
//	else
//	{
//		g_sD3DGlob.TextureFilter     = D3DFILTER_NEAREST;
//	}

	/* set rendering defaults */
//	if ( g_sD3Dinfo.bZBufferOn == TRUE )
//	{
//		g_sD3DGlob.bZBufferOn        = TRUE;
//		g_sD3DGlob.bZBufferCompareOn = TRUE;
//	}
//	else
//	{
//		g_sD3DGlob.bZBufferOn        = FALSE;
//		g_sD3DGlob.bZBufferCompareOn = FALSE;
//	}

	g_sD3DGlob.ShadeMode         = D3DSHADE_GOURAUD;
	g_sD3DGlob.bPerspCorrect     = FALSE;
	g_sD3DGlob.TextureBlend      = D3DTBLEND_MODULATEALPHA;
	g_sD3DGlob.bSubPixel		 = TRUE;
	g_sD3DGlob.bLastPixel		 = TRUE;
	g_sD3DGlob.FillMode          = D3DFILL_SOLID;
	g_sD3DGlob.bDithering        = FALSE;
	g_sD3DGlob.bSpecular         = TRUE;
	g_sD3DGlob.bAntialiasing     = FALSE;
	g_sD3DGlob.bFogEnabled       = FALSE;
	g_sD3DGlob.FogColor          = 0;

	/* If no D3D Viewport, return true because it is not required */
	if ( !g_sD3DGlob.psD3DViewport3 )
	{
		return TRUE;
	}

    hResult = psDev->lpVtbl->BeginScene( psDev );
    if ( hResult != D3D_OK )
	{
		DBERROR( ("BeginScene failed in SetRenderState.\n%s",
					DDErrorToString(hResult)) );
		return FALSE;
	}

	hResult = psDev->lpVtbl->SetCurrentViewport( psDev, g_sD3DGlob.psD3DViewport3 );
	if (hResult !=D3D_OK)
	{
        DBERROR( ("SetViewport failed in SetRenderState.\n%s",
					DDErrorToString(hResult)) );
		return FALSE;
    }

	/* set clip state */
	ATTEMPTD3D( psDev->lpVtbl->SetClipStatus( psDev, &status ) );

    hResult = psDev->lpVtbl->EndScene( psDev );
    if ( hResult != D3D_OK )
	{
		DBERROR( ("EndScene failed in SetRenderState.\n%s",
					DDErrorToString(hResult)) );
		return FALSE;
	}

	D3DSetCulling( FALSE );

	if ( D3DGetAlphaKey() == TRUE )
	{
		ATTEMPTD3D(hResult = psDev->lpVtbl->SetRenderState( psDev,
					D3DRENDERSTATE_ALPHAFUNC, D3DCMP_NOTEQUAL));
		ATTEMPTD3D(hResult = psDev->lpVtbl->SetRenderState( psDev,
					D3DRENDERSTATE_ALPHAREF, 0x00000000));
		ATTEMPTD3D(hResult = psDev->lpVtbl->SetRenderState( psDev,
					D3DRENDERSTATE_ALPHATESTENABLE, TRUE ));
	}

	return TRUE;

exit_with_error:

	DBERROR( ("Error in SetRenderState.\n%s", DDErrorToString(hResult)) );

	return FALSE;
}
/***************************************************************************/
#if 0
void
RestoreAllD3DTextures( void )
{
	int				i;
	TEXPAGE_D3D		*psTexPage;
	LPDDPIXELFORMAT	psCurTexSurfDesc;

	psCurTexSurfDesc =
		&g_sD3DGlob.TextureFormat[g_sD3DGlob.uwCurrTextureFormat].ddsd;

	/* free and reload D3D texture pages */
	for ( i=0; i<iV_TEX_MAX; i++ )
	{
		/* reload D3D texture if ivis texture loaded */
		if ( _TEX_PAGE[i].tex.width > 0 )
		{
			psTexPage = _TEX_PAGE[i].tex.pTex;

			if ( psTexPage != NULL )
			{
				D3DFreeTexturePage( psTexPage );
			}

			D3DGetD3DTexturePage( &_TEX_PAGE[i].tex,
								  _TEX_PAGE[i].tex.pPal );
		}
	}
}
#endif
/***************************************************************************/


/***************************************************************************/

void
D3DFreeTexturePage( TEXPAGE_D3D *psTexPage )
{
	if ( psTexPage != NULL )
	{
		/* release surface created by surfCreate in framework's surface.c */
//		surfRelease( psTexPage->psSurface );//surfaces not stored
		RELEASE( psTexPage->psSurface );

		RELEASE( psTexPage->psPalette );
		RELEASE( psTexPage->psTexture );
		RELEASE( psTexPage->psMaterial );
	}
}

/***************************************************************************/
/*
 * BitsPerPixelToBitDepth
 * Convert an integer bit per pixel number to a DirectDraw bit depth flag
 */
/***************************************************************************/

DWORD
BitsPerPixelToBitDepth( int iBpp )
{
	BOOL	bValFound = TRUE;

	switch( iBpp )
	{
		case 1:
			return DDBD_1;
		case 2:
			return DDBD_2;
		case 4:
			return DDBD_4;
		case 8:
			return DDBD_8;
		case 16:
			return DDBD_16;
		case 24:
			return DDBD_24;
		case 32:
			return DDBD_32;

		default:
			bValFound = FALSE;
	}

	/* assert if got to here */
	ASSERT( ( bValFound == TRUE,
			  "BitsPerPixelToBitDepth: Unknown bitsperpixel\n") );

	return (DWORD) 0;
}

/***************************************************************************/
/*
 * GetWindowsMode
 *
 * Record the current display mode in g_sD3DGlob.WindowsDisplay
 */
/***************************************************************************/

BOOL
GetWindowsMode(void)
{
	DDSURFACEDESC2	ddsd;
	HRESULT			hResult;

	/* init structure */
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);

	/* get current mode */
	hResult = g_sD3DGlob.psDD4->lpVtbl->GetDisplayMode(
					g_sD3DGlob.psDD4, &ddsd );
    
	if ( hResult != DD_OK )
	{
		DBERROR( ("GetDisplayMode failed:\n%s", DDErrorToString(hResult)) );

        return FALSE;
    }

	/* store values */
	g_sD3DGlob.WindowsDisplay.w = ddsd.dwWidth;
	g_sD3DGlob.WindowsDisplay.h = ddsd.dwHeight;
	g_sD3DGlob.WindowsDisplay.bpp = ddsd.ddpfPixelFormat.dwRGBBitCount;

    return TRUE;
}

/***************************************************************************/
/*
 * EnumDeviceFunc
 *
 * Device enumeration callback.  Record information about the D3D device
 * reported by D3D.
 *
 * Returns value showing whether to cancel or continue enumeration.
 */
/***************************************************************************/

static HRESULT
WINAPI EnumDeviceFunc(  LPGUID lpGuid, LPSTR lpDeviceDescription,
						LPSTR lpDeviceName, LPD3DDEVICEDESC lpHWDesc,
						LPD3DDEVICEDESC lpHELDesc, LPVOID lpContext   )
{
	lpContext;

	DBPRINTF( ("Direct3D device: desc %s, name %s\n", lpDeviceDescription,
				lpDeviceName ) );

	/*
	 * Don't accept any hardware D3D devices if emulation only option is set
	 */
/*	//always enumerate all devices
	if (lpHWDesc->dcmColorModel && !g_sD3Dinfo.bHardware)//not for reference renderer either
	{
		return D3DENUMRET_OK;
	}
*/
	// reject old renders
	if (strcmp(lpDeviceName,"Ramp Emulation") == 0)
	{
		return D3DENUMRET_OK;
	}

	if (strcmp(lpDeviceName,"MMX Emulation") == 0)
	{
		return D3DENUMRET_OK;
	}

#ifndef JEREMY
	if (strcmp(lpDeviceName,"Reference Rasterizer") == 0)
	{
		return D3DENUMRET_OK;
	}
#endif

	/*
	 * Record the D3D driver's information
	 */

	memcpy(&g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].Guid, lpGuid, sizeof(GUID));
	lstrcpy(g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].szAbout, lpDeviceDescription);
	lstrcpy(g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].szName, lpDeviceName);

	/*
	 * Is this a hardware device or software emulation?  Checking the color
	 * model for a valid model works.
	 */
	if (lpHWDesc->dcmColorModel)
	{
		g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].bIsHardware = TRUE;
		memcpy( &g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].Desc, lpHWDesc,
					sizeof(D3DDEVICEDESC));
	}
	else
	{
		g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].bIsHardware = FALSE;
		memcpy( &g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].Desc, lpHELDesc,
				sizeof(D3DDEVICEDESC));
    }

	/*
	 * Does this driver do texture mapping?
	 */
	g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].bDoesTextures =
		(g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].Desc.dpcTriCaps.dwTextureCaps &
			D3DPTEXTURECAPS_PERSPECTIVE) ? TRUE : FALSE;

	/*
	 * Can this driver use a z-buffer?
	 */
	g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].bDoesZBuffer =
		g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].Desc.dwDeviceZBufferBitDepth
		? TRUE : FALSE;

	/*
     * Can this driver render to the Windows display depth?
     */
	g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].bCanDoWindow =
		(g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].Desc.dwDeviceRenderBitDepth &
		 BitsPerPixelToBitDepth(g_sD3DGlob.WindowsDisplay.bpp)) ? TRUE : FALSE;
	if (!g_sD3DGlob.bIsPrimary)
		g_sD3DGlob.Driver[g_sD3DGlob.uwNumDrivers].bCanDoWindow = FALSE;

	g_sD3DGlob.uwNumDrivers++;

	if (g_sD3DGlob.uwNumDrivers == MAXD3DDRIVERS)
		return (D3DENUMRET_CANCEL);

	return (D3DENUMRET_OK);
}

/***************************************************************************/
/*
 * GetSurfDesc
 *
 * Get the description of the given surface.  Returns the result of the
 * GetSurfaceDesc call.
 */
/***************************************************************************/

HRESULT
GetSurfDesc(LPDDSURFACEDESC2 lpDDSurfDesc,LPDIRECTDRAWSURFACE4 lpDDSurf)
{
    HRESULT result;
    memset(lpDDSurfDesc, 0, sizeof(DDSURFACEDESC2));
    lpDDSurfDesc->dwSize = sizeof(DDSURFACEDESC2);
    result = lpDDSurf->lpVtbl->GetSurfaceDesc(lpDDSurf, lpDDSurfDesc);
    return result;
}


    //-------------------------------------------------------------------------
    // Create the z-buffer AFTER creating the back buffer and BEFORE creating
    // the d3ddevice.
    //
    // Note: before creating the z-buffer, apps may want to check the device
    // caps for the D3DPRASTERCAPS_ZBUFFERLESSHSR flag. This flag is true for
    // certain hardware that can do HSR (hidden-surface-removal) without a
    // z-buffer. For those devices, there is no need to create a z-buffer.
    //-------------------------------------------------------------------------
 
 
static HRESULT WINAPI EnumZBufferCallback( DDPIXELFORMAT* pddpf,
                                           VOID* pddpfDesired )
{
    // If this is ANY type of depth-buffer, stop.
    if( pddpf->dwFlags == DDPF_ZBUFFER )
	{
		if (pddpf->dwZBufferBitDepth == ((DDPIXELFORMAT*)pddpfDesired)->dwZBufferBitDepth)
		{
			memcpy( pddpfDesired, pddpf, sizeof(DDPIXELFORMAT) );
 
			// Return with D3DENUMRET_CANCEL to end the search.
			return D3DENUMRET_CANCEL;
		}
    }
 
    // Return with D3DENUMRET_OK to continue the search.
    return D3DENUMRET_OK;
}
 

/***************************************************************************/
/*
 * CreateZBuffer
 *
 * Create a Z-Buffer of the appropriate depth and attach it to the back
 * buffer.
 */
/***************************************************************************/

BOOL
CreateZBuffer(int w, int h, int driver)
{
	DDSURFACEDESC2	ddsd;
	DWORD			devDepth;
	HRESULT			hResult;
    DDPIXELFORMAT	ddpfZBuffer;  // destination for callback
	GUID*			pDeviceGUID;
	/*
	 * Release any Z-Buffer that might be around just in case.
	 */
	RELEASE(g_sD3DGlob.psZBuffer);

	pDeviceGUID = &g_sD3DGlob.Driver[g_sD3DGlob.uwCurrDriver].Guid;
   	
	/* get Z buffer bit depth from surface description for current surface */
	devDepth = 16;

	//set up the desired zBuffer format 
	memset(&ddpfZBuffer, 0 ,sizeof(DDPIXELFORMAT));
	ddpfZBuffer.dwFlags = DDPF_ZBUFFER;
	ddpfZBuffer.dwZBufferBitDepth = devDepth;


    hResult = g_sD3DGlob.psD3D3->lpVtbl->EnumZBufferFormats( g_sD3DGlob.psD3D3,
								(REFCLSID)pDeviceGUID, //(REFCLSID)
                                EnumZBufferCallback,
								&ddpfZBuffer );

	if( hResult != DD_OK )
	{
		DBERROR( ( "Enumerate Z-buffer formats failed.\n%s",
					DDErrorToString(hResult) ) );

		goto exit_with_error;
	}

	/* reset surface description structure */
	memset(&ddsd, 0 ,sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof( ddsd );

	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS |
					DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
	ddsd.dwHeight = h;
	ddsd.dwWidth = w;
	memcpy(&ddsd.ddpfPixelFormat,&ddpfZBuffer,sizeof(DDPIXELFORMAT));

	/*
	 * If this is a hardware D3D driver, the Z-Buffer MUST end up in video
	 * memory.  Otherwise, it MUST end up in system memory.
	 */
	if (g_sD3DGlob.Driver[driver].bIsHardware)
		ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
	else
		ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;


	
	//create the zBuffer
	hResult = g_sD3DGlob.psDD4->lpVtbl->CreateSurface( g_sD3DGlob.psDD4,
				&ddsd, &g_sD3DGlob.psZBuffer, NULL);

	if( hResult != DD_OK )
	{
		if (hResult == DDERR_OUTOFMEMORY || hResult == DDERR_OUTOFVIDEOMEMORY)
		{
			if ( screenGetMode() == SCREEN_FULLSCREEN )
			{
				DBERROR( ("There was not enough video memory to create the Z-buffer surface.\nPlease restart the program and try another fullscreen mode with less resolution or lower bit depth.") );
            }
			else
			{                
				DBERROR( ("There was not enough video memory to create the Z-buffer surface.\nTo run this program in a window of this size, please adjust your display settings for a smaller desktop area or a lower palette size and restart the program.") );
			}       
		}
		else
		{
			DBERROR( ( "CreateSurface for Z-buffer failed.\n%s",
						DDErrorToString(hResult) ) );
		}

		goto exit_with_error;
	}

	/*
	 * Attach the Z-buffer to the back buffer so D3D will find it
	 */
	hResult = g_sD3DGlob.psBack4->lpVtbl->AddAttachedSurface(
		g_sD3DGlob.psBack4, g_sD3DGlob.psZBuffer );

	if(hResult != DD_OK)
	{
		DBERROR( ("AddAttachedBuffer failed for Z-Buffer.\n%s",
								DDErrorToString(hResult)) );
		goto exit_with_error;
	}

	/*
	 * Find out if it ended up in video memory.
	 */
	hResult = GetSurfDesc(&ddsd, g_sD3DGlob.psZBuffer);

	if (hResult != DD_OK)
	{
		DBERROR( ("Failed to get surface description of Z buffer.\n%s",
								DDErrorToString(hResult)) );
		goto exit_with_error;
	}

	g_sD3DGlob.bZBufferInVideo =
		(ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ? TRUE : FALSE;

    if (g_sD3DGlob.Driver[driver].bIsHardware && !g_sD3DGlob.bZBufferInVideo)
	{
		DBERROR( ("Could not fit the Z-buffer in video memory for this hardware device.\n") );
		goto exit_with_error;
	}

	return TRUE;

exit_with_error:

	RELEASE( g_sD3DGlob.psZBuffer );
	
	return FALSE;
}

/***************************************************************************/
/*
 * GetClientWin
 *
 * Gets the client window size and sets it in the g_sD3DGlob structure
 */
/***************************************************************************/
/*
void
GetClientWin( HWND hWnd )
{
	RECT	rc;

	g_sD3DGlob.posClientOnPrimary.x = 0;
	g_sD3DGlob.posClientOnPrimary.y = 0;

	if ( screenGetMode() == SCREEN_WINDOWED )
	{
		ClientToScreen( hWnd, &g_sD3DGlob.posClientOnPrimary );
		GetClientRect( hWnd, &rc );
		g_sD3DGlob.sizeClient.cx = rc.right;
		g_sD3DGlob.sizeClient.cy = rc.bottom;
	}
	else
	{
		// In the fullscreen case, we must be careful because if the window
		// frame has been drawn, the client size has shrunk and this can
		// cause problems, so it's best to report the entire screen.
		
		g_sD3DGlob.sizeClient.cx = 640;
		g_sD3DGlob.sizeClient.cy = 480;
	}
}
*/
/***************************************************************************/
/*
 * EnumTextureFormatsCallback
 *
 * Record information about each texture format the current D3D driver can
 * support. Choose one as the default format (paletted formats are prefered)
 * and return it through lpContext.
 */
/***************************************************************************/

static HRESULT
CALLBACK EnumTextureFormatsCallback(LPDDPIXELFORMAT lpDDPF, LPVOID lpContext)
{
	unsigned long	m;
	int				r, g, b;
	int				*lpStartFormat = (int *)lpContext;
	BOOL			bOK;

	/* Record the DDSURFACEDESC2 of this texture format */
	memset( &g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats], 0,
			sizeof(TEXTUREFORMAT));
	memcpy(&g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].ddsd, lpDDPF,
			sizeof(DDSURFACEDESC2));
	
	/* Is this format palettized?  How many bits?
	 * Otherwise, how many RGB bits?
	 */
	if (lpDDPF->dwFlags & DDPF_PALETTEINDEXED8)
	{
		g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].bPalettized = TRUE;
		g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].uwIndexBPP = 8;
	}
	else if (lpDDPF->dwFlags & DDPF_PALETTEINDEXED4)
	{
		g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].bPalettized = TRUE;
		g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].uwIndexBPP = 4;
	}
	else
	{
		if (lpDDPF->dwFlags & DDPF_ALPHAPIXELS)
		{
			if ( D3DGetAlphaKey() == TRUE )
			{
				g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].bAlphaKeyed = TRUE;
			}
			else
			{
				/* don't enum alphakeyed formats */
				return DDENUMRET_OK;
			}
		}

		g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].bPalettized = FALSE;
		g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].uwIndexBPP = 0;
		for (r = 0, m = lpDDPF->dwRBitMask; !(m & 1);
				r++, m >>= 1);
		for (r = 0; m & 1; r++, m >>= 1);
		for (g = 0, m = lpDDPF->dwGBitMask; !(m & 1);
				g++, m >>= 1);
		for (g = 0; m & 1; g++, m >>= 1);
		for (b = 0, m = lpDDPF->dwBBitMask; !(m & 1);
				b++, m >>= 1);
		for (b = 0; m & 1; b++, m >>= 1);

		g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].uwRedBPP = r;
		g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].uwGreenBPP = g;
		g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].uwBlueBPP = b;
	}

	/*
	 * If lpStartFormat is -1, this is the first format.  Select it.
	 */
	if (*lpStartFormat == -1)
		*lpStartFormat = g_sD3DGlob.uwNumTextureFormats;

	if ( D3DGetAlphaKey() == TRUE )
	{
		bOK = FALSE;

		/* 
		 * If this format is 16-bit alpha keyed, select it.
		 */
		if ( g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].bAlphaKeyed )
		{
			bOK = TRUE;
		}
		/* 
		 * If this format is paletted, select it.
		 */
		else if ( g_sD3DGlob.TextureFormat[g_sD3DGlob.uwNumTextureFormats].bPalettized )
		{
			bOK = TRUE;
		}

		if ( bOK == TRUE )
		{
			*lpStartFormat = g_sD3DGlob.uwNumTextureFormats;
			g_sD3DGlob.uwCurrTextureFormat = g_sD3DGlob.uwNumTextureFormats;
			return DDENUMRET_CANCEL;
		}
	}

	g_sD3DGlob.uwNumTextureFormats++;

	return DDENUMRET_OK;
}


extern BOOL d3d_bHardware(void)
{
	return g_sD3DGlob.Driver[g_sD3DGlob.uwCurrDriver].bIsHardware;
}



LPDDPIXELFORMAT d3d_GetCurTexSurfDesc(void)
{
	if ((g_sD3DGlob.uwNumTextureFormats == 0) || (g_sD3DGlob.uwCurrTextureFormat > g_sD3DGlob.uwNumTextureFormats))
	{
		ASSERT((FALSE,"d3d_GetCurTexSurfDesc, Surface not initialised"));
		return NULL;
	}
	return	&g_sD3DGlob.TextureFormat[g_sD3DGlob.uwCurrTextureFormat].ddsd;
}

LPDIRECT3DDEVICE3 d3d_GetpsD3DDevice3(void)
{
	ASSERT((g_sD3DGlob.psD3DDevice3 != NULL,"dx6_GetpsD3DDevice3(), Device not set."));
	return g_sD3DGlob.psD3DDevice3;
}

LPDIRECT3D3 d3d_GetpsD3D(void)
{
	return g_sD3DGlob.psD3D3;
}

LPDIRECTDRAW4 d3d_GetpsDD4(void)
{
	return g_sD3DGlob.psDD4;
}
/***************************************************************************/
/*
 * CreateDevice
 *
 * Create the D3D device and enumerate the texture formats
 */
/***************************************************************************/

BOOL
CreateDevice( int iDriver )
{
	HRESULT			hResult;
	DDSURFACEDESC2	sDDSurfDesc;

	iDriver;

	/* release previous device (if any) */
	RELEASE( g_sD3DGlob.psD3DDevice3 );

	/* init surface description struct */
	memset( &sDDSurfDesc, 0, sizeof(DDSURFACEDESC2) );
    sDDSurfDesc.dwSize = sizeof(DDSURFACEDESC2);

	/* get back buffer surface description */
	hResult = g_sD3DGlob.psBack4->lpVtbl->GetSurfaceDesc(
							g_sD3DGlob.psBack4, &sDDSurfDesc );
	if (hResult != DD_OK)
	{
		DBERROR( ("CreateDevice: GetSurfaceDesc for back buffer failed.\n%s",
				  DDErrorToString(hResult) ));
		goto exit_with_error;
	}

	/* check whether back buffer in video memory */
	g_sD3DGlob.bBackBufferInVideo =
		(sDDSurfDesc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ? TRUE : FALSE;

	/* check back buffer in video memory for hardware rendering */
	if ( g_sD3DGlob.Driver[g_sD3DGlob.uwCurrDriver].bIsHardware &&
		!g_sD3DGlob.bBackBufferInVideo )
	{
		DBERROR(("Back buffer must be in video memory for hardware rendering.\n"));
		goto exit_with_error;
	}

	/* create D3D device */
	hResult = g_sD3DGlob.psD3D3->lpVtbl->CreateDevice( g_sD3DGlob.psD3D3,
				&g_sD3DGlob.Driver[g_sD3DGlob.uwCurrDriver].Guid,
				g_sD3DGlob.psBack4, &g_sD3DGlob.psD3DDevice3,NULL);

	if (hResult != DD_OK)
	{
		DBERROR( ("Create D3D device failed.\n%s",
				  DDErrorToString(hResult) ));
		goto exit_with_error;
	}

	return TRUE;

exit_with_error:

	RELEASE( g_sD3DGlob.psD3DDevice3 );

	return FALSE;
}

/***************************************************************************/
/*
 * ChooseDriver
 *
 * Enumerates Direct3D drivers and picks one
 */
/***************************************************************************/

BOOL
ChooseDriver( void )
{
	HRESULT		hResult;
	BOOL		bDriverFound = FALSE;
	UWORD		uwDriver;
	SBYTE		seps[] = " ,\t\n";
//	SBYTE		*token;

	/* zero drivers */
	g_sD3DGlob.uwNumDrivers = 0;

	/* get available devices */
	hResult = g_sD3DGlob.psD3D3->lpVtbl->EnumDevices(
					g_sD3DGlob.psD3D3, EnumDeviceFunc, NULL );

	if ( hResult != DD_OK )
	{
		DBERROR( ( "ChooseDriver: Error in enumeration of drivers:\n%s",
					DDErrorToString(hResult)) );

		return FALSE;
	}
    
	/* init current driver */
	uwDriver = 0;

	// choose driver 
	while ( (uwDriver<g_sD3DGlob.uwNumDrivers) &&
			(bDriverFound == FALSE)           )
	{
		if ( strcmp(g_sD3DGlob.Driver[uwDriver].szName, pie_GetDirect3DDeviceName()) == 0)
		{
			bDriverFound = TRUE;
			g_sD3DGlob.uwCurrDriver = uwDriver;
		}
		else
		{
			/* check next driver */
			uwDriver++;
		}
	}

	/* if hardware driver not found use driver 0
	 * (assumed to be Microsoft Ramp Driver)
	 */
	if ( bDriverFound == FALSE )
	{
		g_sD3DGlob.uwCurrDriver = 0;
		uwDriver = 0;
		return TRUE;
	}

	return TRUE;
}


/***************************************************************************/
/*
 * CreateViewport
 *
 * Creates the D3D viewport object
 */
/***************************************************************************/

static BOOL
CreateViewport( DWORD dwWidth, DWORD dwHeight )
{
	HRESULT		hResult;
	D3DVIEWPORT2 viewData;

	/* release current viewport if assigned */
	RELEASE( g_sD3DGlob.psD3DViewport3 );

	/* Create the D3D viewport object */
	hResult = g_sD3DGlob.psD3D3->lpVtbl->CreateViewport(
				g_sD3DGlob.psD3D3, &g_sD3DGlob.psD3DViewport3, NULL );
	if (hResult != D3D_OK)
	{
		DBERROR( ("Create D3D viewport failed.\n%s",
					DDErrorToString(hResult)) );
		goto exit_with_error;
    }
    
	/* Add the viewport to the D3D device */
	hResult = g_sD3DGlob.psD3DDevice3->lpVtbl->AddViewport(
				g_sD3DGlob.psD3DDevice3, g_sD3DGlob.psD3DViewport3 );
	if (hResult != D3D_OK)
	{
		DBERROR( ("Add D3D viewport failed.\n%s",
					DDErrorToString(hResult) ));
		goto exit_with_error;
    }

    /* Setup the viewport for a reasonable viewing area */
	memset(&viewData, 0, sizeof(D3DVIEWPORT));
	viewData.dwSize = sizeof(D3DVIEWPORT);
	viewData.dwX = 0;
	viewData.dwY = 0;
	viewData.dwWidth = dwWidth;
	viewData.dwHeight = dwHeight;
 	viewData.dvClipX = D3DVAL(-1.0);
	viewData.dvClipY = D3DVAL(1.0);
	viewData.dvClipWidth = D3DVAL(2.0);
	viewData.dvClipHeight = D3DVAL(2.0);
	viewData.dvMinZ = D3DVAL(0.0);
	viewData.dvMaxZ = D3DVAL(1.0);

	hResult = g_sD3DGlob.psD3DViewport3->lpVtbl->SetViewport2(
						g_sD3DGlob.psD3DViewport3, &viewData );
	if (hResult != D3D_OK)
	{
		DBERROR( ("SetViewport failed.\n%s",
					DDErrorToString(hResult) ));
		goto exit_with_error;
    }

	return TRUE;

exit_with_error:

	RELEASE(g_sD3DGlob.psD3DViewport3);

	return FALSE;
}

/***************************************************************************/
/*
 * CreateMaterial
 *
 * Create a Direct3D material
 */
/***************************************************************************/

BOOL
CreateMaterial( LPDIRECT3D3 lpD3D, D3DMATERIALHANDLE * hMat,
				LPDIRECT3DMATERIAL3 *ppsMat, D3DTEXTUREHANDLE hTexture )
{
	D3DMATERIAL			sMat;
	HRESULT				hResult;

	hResult = lpD3D->lpVtbl->CreateMaterial( lpD3D, ppsMat, NULL);
	if ( hResult != DD_OK )
	{
		DBERROR( ("CreateMaterial: Couldn't create material.\n%s",
					DDErrorToString(hResult)) );

		goto exit_with_error;
	}

	/* init structure */
	memset(&sMat, 0, sizeof(D3DMATERIAL));

	sMat.diffuse.r  = D3DVAL(1);
	sMat.diffuse.g  = D3DVAL(1);
	sMat.diffuse.b  = D3DVAL(1);
	sMat.diffuse.a  = D3DVAL(1);

	sMat.ambient.r  = D3DVAL(1);
	sMat.ambient.g  = D3DVAL(1);
	sMat.ambient.b  = D3DVAL(1);
	sMat.ambient.a  = D3DVAL(0);

	sMat.specular.r = D3DVAL(0);
	sMat.specular.g = D3DVAL(0);
	sMat.specular.b = D3DVAL(0);
	sMat.specular.a = D3DVAL(1);

	sMat.emissive.r = D3DVAL(0);
	sMat.emissive.g = D3DVAL(0);
	sMat.emissive.b = D3DVAL(0);
	sMat.emissive.a = D3DVAL(1);

	sMat.hTexture   = hTexture;
	sMat.dwSize     = sizeof(D3DMATERIAL);
	sMat.power      = D3DVAL(1);

	sMat.dwRampSize = RAMP_SIZE_D3D;

	/* set material */
	hResult = (*ppsMat)->lpVtbl->SetMaterial( (*ppsMat), &sMat );
	if ( hResult != DD_OK )
	{
		DBERROR( ("CreateMaterial: Couldn't set material.\n%s",
					DDErrorToString(hResult)) );

		goto exit_with_error;
	}

	/* get handle to it */
	hResult = (*ppsMat)->lpVtbl->GetHandle( (*ppsMat),
					g_sD3DGlob.psD3DDevice3, hMat );
	if ( hResult != DD_OK )
	{
		DBERROR( ("CreateMaterial: Couldn't get material handle.\n%s",
					DDErrorToString(hResult)) );

		goto exit_with_error;
	}

	return TRUE;

exit_with_error:

	RELEASE( (*ppsMat) );

	return FALSE;
}

/***************************************************************************/
/*
 * disp3D_Initialise
 *
 * Initialise the 3D display system
 *
 * Returns FALSE on error.
 */
/***************************************************************************/

BOOL
rend_InitD3D( void )
{
    HRESULT		hRes; 
	
	/* init globals struct */
	memset( &g_sD3DGlob, 0, sizeof(DISP3DGLOBALS) );

	/* get local copy of DirectDraw object pointer from framework */
	g_sD3DGlob.psDD4 = screenGetDDObject();

	/* get local copy of back buffer pointer from framework */
	g_sD3DGlob.psBack4 = screenGetSurface();

//no handled in the framework	/* look for 2nd video card (3dfx?) */
//	D3DEnumDDrawDevices(); 

	/* Retrieve the Direct3D interface from the DirectDraw object */
	if ( g_sD3DGlob.psD3D3 == NULL )
	{
		hRes = g_sD3DGlob.psDD4->lpVtbl->QueryInterface( g_sD3DGlob.psDD4,
							&IID_IDirect3D3, &g_sD3DGlob.psD3D3 ); 
		if ( hRes != D3D_OK )
		{
			DBERROR( ("InitD3D: couldn't create Direct3D3 driver object") );
			return FALSE;
		}
	}

	/* get current Windows mode data */
	if ( GetWindowsMode() == FALSE )
	{
		DBERROR( ("disp3D_Initialise: Couldn't get WindowsMode.\n") );

		goto exit_with_error;
	}

	/* Get the available drivers from Direct3D and pick one */
	ATTEMPT( ChooseDriver() );

	/* get client window */
//	GetClientWin( screenGetHWnd() );

	if ( g_sD3Dinfo.bHardware == TRUE )
	{
		/* Create the Z-buffer */
		ATTEMPT( CreateZBuffer( pie_GetVideoBufferWidth(),
								pie_GetVideoBufferHeight(),
								g_sD3DGlob.uwCurrDriver ) );
	}

	/* create the D3D device */
	ATTEMPT( CreateDevice(g_sD3DGlob.uwCurrDriver) );

	/* check blending modes */
	D3DValidateDevice();
	D3DSetCardSpecificParams();
	D3DGetCaps();
//	D3DEnableFog( 0x00B08f5f );

	/* init texture manager */
	dtm_Initialise();

	/* create viewpoint */
	ATTEMPT( CreateViewport( pie_GetVideoBufferWidth(),
							 pie_GetVideoBufferHeight()) );

	/* set initial rendering state */
	ATTEMPT( SetRenderState() );

	return TRUE;

exit_with_error:

	RELEASE( g_sD3DGlob.psD3DDevice3 );
	RELEASE( g_sD3DGlob.psPalette );
	RELEASE( g_sD3DGlob.psD3DViewport3 );

	if ( g_sD3Dinfo.bHardware == TRUE )
	{
		RELEASE( g_sD3DGlob.psZBuffer );
	}

	return FALSE;
}

/***************************************************************************/

void
D3DReInit( void )
{
	ShutDownD3D();
	InitD3D( &g_sD3Dinfo );
	dtm_ReloadAllTextures();
	D3DSetColourKeying( TRUE );
	dx6_SetBilinear( pie_GetBilinear() );
}

/***************************************************************************/

void D3DTestCooperativeLevel( BOOL bGotFocus )
{
	HRESULT			hRes;
	static BOOL		bReInit = FALSE;

	hRes = screenGetDDObject()->lpVtbl->TestCooperativeLevel(
				screenGetDDObject() );

	if ( hRes == DDERR_NOEXCLUSIVEMODE )
	{
		bReInit = TRUE;
	}
	else if ( (bGotFocus == TRUE) && (bReInit == TRUE) )
	{
		screenReInit();
		D3DReInit();
		bReInit = FALSE;
	}
}

/***************************************************************************/

#endif		// #ifndef PIEPSX - at the top of the file - FFS JS/TC - GJ

/***************************************************************************/
