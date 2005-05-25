#include "frame.h"
#include "piedef.h"
#include "piestate.h"

void _dummyFunc2_D3D(int i,int j,int k,int l,unsigned int m) {}

void _dummyFunc3_D3D(int i,int j,int k,unsigned int l) {}

void _dummyFunc5_D3D(unsigned char *i,int j,int k,int l,int m,int n) {}

void _dummyFunc6_D3D(unsigned char *i,int j,int k,int l,int m,int n,int p) {}

void SetTransFilter_D3D(UDWORD filter,UDWORD tablenumber) {}

iBool _mode_D3D_HAL( void ) { return FALSE; }
iBool _mode_D3D_REF( void ) { return FALSE; }
iBool _mode_D3D_RGB( void ) { return FALSE; }
void _close_D3D( void ) {}
void _renderBegin_D3D( void ) {}
void _renderEnd_D3D( void ) {}

void D3D_PIEPolygon( SDWORD numVerts, PIEVERTEX* pVrts) {}
void D3DDrawPoly( int nVerts, D3DTLVERTEX * psVert) {}
void D3DSetTranslucencyMode( TRANSLUCENCY_MODE transMode ) {}
void D3DSetColourKeying( BOOL bKeyingOn ) {}
void D3DSetTexelOffsetState( BOOL bOffsetOn ) {}

BOOL dtm_LoadTexSurface( iTexture *psIvisTex, SDWORD index ) { return FALSE; }
BOOL dtm_ReLoadTexture(SDWORD i) { return FALSE; }
BOOL dtm_LoadRadarSurface(BYTE* radarBuffer) { return FALSE; }
SDWORD dtm_GetRadarTexImageSize(void) { return 0; }
void dtm_SetTexturePage(SDWORD i) {}
void dx6_SetBilinear( BOOL bBilinearOn ) {}
