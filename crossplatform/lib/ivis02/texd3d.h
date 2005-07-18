/***************************************************************************/

#ifndef _TEX3D_H_
#define _TEX3D_H_

/***************************************************************************/

typedef enum TEXCOLOURDEPTH
{
	TCD_4BIT,			// 16 colour palette
	TCD_8BIT,			// 256 colour palette
	TCD_555RGB,			// 16 bit true colour using 5 bits for R,G and B
	TCD_565RGB,			// 16 bit true colour using 6 bits for G, but 5 for R and B
	TCD_24BIT,			// 24 bit true colour
	TCD_32BIT			// 32 bit true colour
}
TEXCOLOURDEPTH;

typedef struct TEXPAGE_D3D
{
	LPDIRECTDRAWSURFACE4	psSurface;		/* DD Surface			*/
	LPDIRECTDRAWPALETTE		psPalette;		/* surface palette		*/ //always use system palette
	LPDIRECT3DTEXTURE2		psTexture;		/* DD Texture			*/
	D3DTEXTUREHANDLE		hTexture;		/* texture handle		*/
	LPDIRECT3DMATERIAL3		psMaterial;		/* material pointer		*/
	D3DMATERIALHANDLE		hMat;			/* material handle		*/
	UWORD					iWidth;
	UWORD					iHeight;
	SWORD					widthShift;
	SWORD					heightShift;
	BOOL					bInVideo;
	BOOL					bColourKeyed;
}
TEXPAGE_D3D;

/***************************************************************************/
extern TEXPAGE_D3D			*psRadarTexpage;
/***************************************************************************/

BOOL	d3dLoadTextureSurf( char szFileName[], LPDIRECTDRAWSURFACE4 *ppsSurf,
							LPDIRECT3DTEXTURE2 *ppsTexture,
							PALETTEENTRY pal[],
							BOOL bHardware, BOOL* bInVideo );

BOOL	D3DTexCreateFromIvisTex( TEXPAGE_D3D			*psTexPage,
								 iTexture				*psIvisTex,
								 iPalette				psIvisPal,
								 LPDDPIXELFORMAT		pDDSurfDescTexture,
								 PALETTEENTRY			*psPal,
								 BOOL					bHardware );

/***************************************************************************/

#endif	/* _TEX3D_H_ */

/***************************************************************************/
