/***************************************************************************/

#ifndef _D3DRENDER_H_
#define _D3DRENDER_H_

/***************************************************************************/

#define DIRECT3D_VERSION 0x0700
#include <d3d.h>
#include "frame.h"
#include "texd3d.h"
#include "piestate.h"

/***************************************************************************/

#define RAMP_SIZE_D3D		64

#define D3D_TRI_COLOURKEY	0x0001
#define D3D_TRI_NOCULL		0x0002
#define D3D_TRI_ALPHABLEND	0x0004

typedef struct D3DINFO
{
	BOOL	bZBufferOn;
	BOOL	bHardware;
	BOOL	bReference;
	BOOL	bAlphaKey;
}
D3DINFO;

/***************************************************************************/

extern BOOL	InitD3D( D3DINFO *psD3Dinfo );
extern void	ShutDownD3D( void );
extern void	BeginSceneD3D( void );
extern void	EndSceneD3D( void );
extern void D3D_PIEPolygon( SDWORD numVerts, PIEVERTEX* pVrts);
extern void D3DDrawPoly( int nVerts, D3DTLVERTEX * psVert);
//extern BOOL	D3DGetD3DTexturePage( iTexture *pIvisTex, iPalette pIvisPal );
//extern void D3DFreeTexturePages( void );
//extern void RestoreAllD3DTextures( void );
extern void D3DSetAlphaBlending( BOOL bAlphaOn );
extern void D3DSetTranslucencyMode( TRANSLUCENCY_MODE transMode );

extern void	D3DSetColourKeying( BOOL bKeyingOn );
extern void D3DSetDepthBuffer( BOOL bDepthBufferOn );
extern void D3DSetDepthWrite( BOOL bWriteEnable );
extern void D3DSetDepthCompare( D3DCMPFUNC depthCompare );

extern void	D3DSetAlphaKey( BOOL bAlphaOn );
extern BOOL	D3DGetAlphaKey( void );

extern void	D3DSetTexelOffsetState( BOOL bOffsetOn );

extern void	D3DReInit( void );
extern void D3DTestCooperativeLevel( BOOL bGotFocus );

extern void D3DSetClipWindow(SDWORD xMin, SDWORD yMin, SDWORD xMax, SDWORD yMax);

extern BOOL d3d_bHardware(void);
extern LPDDPIXELFORMAT d3d_GetCurTexSurfDesc(void);
extern LPDIRECT3DDEVICE3 d3d_GetpsD3DDevice3(void) ;
extern LPDIRECT3D3 d3d_GetpsD3D(void);
extern LPDIRECTDRAW4 d3d_GetpsDD4(void);

extern BOOL CreateMaterial( LPDIRECT3D3 lpD3D, D3DMATERIALHANDLE * hMat,
				LPDIRECT3DMATERIAL3 *ppsMat, D3DTEXTUREHANDLE hTexture );

/***************************************************************************/

#endif	/* _D3DRENDER_H_ */

/***************************************************************************/
