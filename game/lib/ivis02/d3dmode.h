/***************************************************************************/

#ifndef _D3DMODE_H_
#define _D3DMODE_H_

/***************************************************************************/

iBool	_mode_D3D_RGB( void );
iBool	_mode_D3D_HAL( void );
iBool	_mode_D3D_REF( void );
void	_close_D3D( void );
void	_renderBegin_D3D( void );
void	_renderEnd_D3D( void );
//void	_triangle_D3D( iVertex *vrt, iTexture *tex, uint32 type );
//void	_quad_D3D( iVertex *vrt, iTexture *tex, uint32 type );
//void	_polygon_D3D(  int npoints, iVertex *vrt, iTexture *tex, uint32 type );
void	_palette_D3D( int i, int r, int g, int b );
void	_dummyFunc_D3D( void );

void	_dummyFunc1_D3D(int ,int ,unsigned int );
void	_dummyFunc2_D3D(int ,int ,int ,int ,unsigned int );
void	_dummyFunc3_D3D(int ,int ,int ,unsigned int );
void	_dummyFunc4_D3D(unsigned char *,int ,int ,int ,int );
void	_dummyFunc5_D3D(unsigned char *,int ,int ,int ,int ,int );
void	_dummyFunc6_D3D(unsigned char *,int ,int ,int ,int ,int ,int );
void	_dummyFunc7_D3D(int ,int ,int ,unsigned int );
void	_dummyFunc8_D3D(iVertex *vrt, iTexture *tex, uint32 type);
void	SetTransFilter_D3D(UDWORD rgb,UDWORD tablenumber);
void	TransBoxFill_D3D(UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1);

/**********************************************************************/

#endif	/* _IVID3D_H_ */

/***************************************************************************/
