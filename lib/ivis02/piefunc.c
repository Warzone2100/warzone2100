/***************************************************************************/
/*
 * piefunc.c
 *
 * extended render routines for 3D rendering
 *
 */
/***************************************************************************/

#include "lib/framework/frame.h"

#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piefunc.h"
#include "lib/ivis_common/piestate.h"
#include "piematrix.h"
#include "pietexture.h"
#include "lib/ivis_common/pieclip.h"

#include "lib/gamelib/gtime.h"



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
static PIEPIXEL		scrPoints[pie_MAX_POLYS];
static PIEVERTEX	pieVrts[pie_MAX_POLY_VERTS];
static PIEVERTEX	clippedVrts[pie_MAX_POLY_VERTS];
static D3DTLVERTEX	d3dVrts[pie_MAX_POLY_VERTS];
static UBYTE	aByteScale[256][256];

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

void pie_DownLoadBufferToScreen(void *pSrcData, UDWORD destX, UDWORD destY,UDWORD srcWidth,UDWORD srcHeight,UDWORD srcStride)
{
	return;
}

/***************************************************************************/
/*
 *	void pie_RectFilter(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour)
 *
 * Draws rectangular filter to screen ivis mode defaults to
 *
 */
/***************************************************************************/
void pie_RectFilter(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour)
{

	{
		iV_TransBoxFill(x0, y0, x1, y1);
	}
	return;
}

/* ---------------------------------------------------------------------------------- */
void	pie_CornerBox(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour,
					  UBYTE a, UBYTE b, UBYTE c, UBYTE d)
{

}

/* ---------------------------------------------------------------------------------- */
#define D3D_VIEW_WINDOW
#ifndef D3D_VIEW_WINDOW
void	pie_DrawViewingWindow( iVector *v, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2,UDWORD colour)
{

}
#else
void	pie_DrawViewingWindow(iVector *v,UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, UDWORD colour)
{
	SDWORD clip, i;


	{
		pie_SetTexturePage(-1);
		pie_SetRendMode(REND_ALPHA_FLAT);
//PIE verts
		pieVrts[0].sx = v[1].x;
		pieVrts[0].sy = v[1].y;
		//cull triangles with off screen points
		pieVrts[0].sz  = INTERFACE_DEPTH;


		pieVrts[0].tu = 0.0;
		pieVrts[0].tv = 0.0;
		pieVrts[0].light.argb = colour;//0x7fffffff;
		pieVrts[0].specular.argb = 0;

		memcpy(&pieVrts[1],&pieVrts[0],sizeof(PIEVERTEX));
		memcpy(&pieVrts[2],&pieVrts[0],sizeof(PIEVERTEX));
		memcpy(&pieVrts[3],&pieVrts[0],sizeof(PIEVERTEX));
		memcpy(&pieVrts[4],&pieVrts[0],sizeof(PIEVERTEX));

		pieVrts[1].sx = v[0].x;
		pieVrts[1].sy = v[0].y;

		pieVrts[2].sx = v[2].x;
		pieVrts[2].sy = v[2].y;

		pieVrts[3].sx = v[3].x;
		pieVrts[3].sy = v[3].y;

		pie_Set2DClip(x1,y1,x2-1,y2-1);
		clip = pie_ClipTextured(4, &pieVrts[0], &clippedVrts[0], FALSE);
		pie_Set2DClip(CLIP_BORDER,CLIP_BORDER,psRendSurface->width-CLIP_BORDER,psRendSurface->height-CLIP_BORDER);


#ifndef NO_RENDER

		if (clip >= 3)
		{
			if(!pie_Translucent())
			{
				for (i = 0;i < (clip - 1);i++)
				{
					pie_Line(clippedVrts[i].sx, clippedVrts[i].sy, clippedVrts[i+1].sx, clippedVrts[i+1].sy, colour);
				}
				pie_Line(clippedVrts[clip-1].sx, clippedVrts[clip-1].sy, clippedVrts[0].sx, clippedVrts[0].sy, colour);
			}
		}
#endif
  }
}
#endif
/* ---------------------------------------------------------------------------------- */
void	pie_TransColouredTriangle(PIEVERTEX *vrt, UDWORD rgb, UDWORD trans)
{
}

/* ---------------------------------------------------------------------------------- */
/* Returns number of buffers pending */
int pie_Num3dfxBuffersPending( void )
{
int	retVal=0;


	return(retVal);
}
/* ---------------------------------------------------------------------------------- */

void pie_DrawBoundingDisc(iIMDShape *shape, int pieFlag)
{

	int i, n, radR2;
	iVector vertex;
	PIEPIXEL *pPixels;
	PIED3DPOLY renderPoly;
	DWORD	colour, specular;
	int32 rx, ry, rz;
	int32 tzx, tzy;

//	pieCount++;

	pie_SetTexturePage(-1);

	colour = 0xff000000;
	specular = 0;
	//draw bounding sphere
	if (pieFlag & pie_DRAW_DISC_RED)
	{
		colour |= 0xff0000;
	}
	if (pieFlag & pie_DRAW_DISC_GREEN)
	{
		colour |= 0xff00;
	}
	if (pieFlag & pie_DRAW_DISC_YELLOW)
	{
		colour |= 0xffff00;
	}
	if (pieFlag & pie_DRAW_DISC_BLUE)
	{
		colour |= 0xff;
	}

	//rotate and project four new points
	pPixels = &scrPoints[0];
	radR2 = (shape->oradius * 71)/100;
	if (radR2 < 10)
	{
		radR2 = 10;
	}
	for (i=0; i<8; i++, pPixels++)
	{
		vertex.x = 0;
		vertex.y = 0;
		vertex.z = 0;
		switch (i)
		{
			case 0:
				vertex.x = shape->oradius;
				break;
			case 1:
				vertex.x = radR2;
				vertex.z = -radR2;
				break;
			case 2:
				vertex.z = -shape->oradius;
				break;
			case 3:
				vertex.x = -radR2;
				vertex.z = -radR2;
				break;
 			case 4:
				vertex.x = -shape->oradius;
				break;
			case 5:
				vertex.x = -radR2;
				vertex.z = radR2;
				break;
			case 6:
				vertex.z = shape->oradius;
				break;
			case 7:
				vertex.x = radR2;
				vertex.z = radR2;
				break;
		}
		rx = vertex.x * psMatrix->a + vertex.y * psMatrix->d + vertex.z * psMatrix->g + psMatrix->j;
		ry = vertex.x * psMatrix->b + vertex.y * psMatrix->e + vertex.z * psMatrix->h + psMatrix->k;
		rz = vertex.x * psMatrix->c + vertex.y * psMatrix->f + vertex.z * psMatrix->i + psMatrix->l;

		pPixels->d3dz = D3DVAL((rz>>STRETCHED_Z_SHIFT));

		tzx = rz >> psRendSurface->xpshift;
		tzy = rz >> psRendSurface->ypshift;

		if ((tzx<=0) || (tzy<=0))
		{
			pPixels->d3dx = (float)LONG_WAY;//just along way off screen
			pPixels->d3dy = (float)LONG_WAY;
		}
		else if (pPixels->d3dz < D3DVAL(MIN_STRETCHED_Z))
		{
			pPixels->d3dx = (float)LONG_WAY;//just along way off screen
			pPixels->d3dy = (float)LONG_WAY;
		}
		else
		{
			pPixels->d3dx = D3DVAL((psRendSurface->xcentre + (rx / tzx)));
			pPixels->d3dy = D3DVAL((psRendSurface->ycentre - (ry / tzy)));
		}
	}


	renderPoly.flags = PIE_NO_CULL | PIE_ALPHA;
	for (n=0; n<8; n++)
	{
		d3dVrts[n].sx = scrPoints[n].d3dx;
		d3dVrts[n].sy = scrPoints[n].d3dy;

		//cull triangles with off screen points
		if (scrPoints[n].d3dy > (float)LONG_TEST)
			renderPoly.flags = 0;

		d3dVrts[n].sz = (float)pPixels->d3dz * (float)INV_MAX_Z;
		d3dVrts[n].rhw = (float)1.0 / d3dVrts[n].sz;
		d3dVrts[n].tu = (float)0.0;
		d3dVrts[n].tv = (float)0.0;
		d3dVrts[n].color = colour;
		d3dVrts[n].specular = specular;
	}
	renderPoly.nVrts = 8;
	renderPoly.pVrts = &d3dVrts[0];
	renderPoly.pTexAnim = NULL;
	pie_D3DPoly(&renderPoly);	   // draw the polygon ... this is an inline function

}

void pie_Blit(SDWORD texPage, SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1)
{
	SDWORD i;
	PIED3DPOLY renderPoly;

	for(i = 0; i < 4; i++)
	{
		switch(i)
		{
			case 0:
				d3dVrts[i].sx = (float)x0;
				d3dVrts[i].sy = (float)y0;
				d3dVrts[i].tu = (float)0.0;
				d3dVrts[i].tv = (float)0.0;
				break;
			case 1:
				d3dVrts[i].sx = (float)x1;
				d3dVrts[i].sy = (float)y0;
				d3dVrts[i].tu = (float)1.0;
				d3dVrts[i].tv = (float)0.0;
				break;
			case 2:
				d3dVrts[i].sx = (float)x1;
				d3dVrts[i].sy = (float)y1;
				d3dVrts[i].tu = (float)1.0;
				d3dVrts[i].tv = (float)1.0;
				break;
			case 3:
			default:
				d3dVrts[i].sx = (float)x0;
				d3dVrts[i].sy = (float)y1;
				d3dVrts[i].tu = (float)0.0;
				d3dVrts[i].tv = (float)1.0;
				break;
		}
		d3dVrts[i].sz = (float)0.001;//scrPoints[*index].d3dz;
		d3dVrts[i].rhw = (float)1000.0;
		d3dVrts[i].color = (D3DCOLOR)((255 << 24) + (255 << 16) + (255 << 8) + 255);
		d3dVrts[i].specular = 0;
	}

	{
		renderPoly.flags = PIE_NO_CULL | PIE_TEXTURED;
		renderPoly.nVrts = 4;
		renderPoly.pVrts = &d3dVrts[0];
		pie_SetTexturePage(texPage);
		pie_D3DPoly(&renderPoly);	   // draw the polygon ... this is an inline function
	}
}

void pie_Sky(SDWORD texPage, PIEVERTEX* aSky)
{
	SDWORD i;
	PIED3DPOLY renderPoly;

	if ((rendSurface.usr == iV_MODE_4101) || (rendSurface.usr == REND_GLIDE_3DFX))
	{
	}
	else
	{
		renderPoly.flags = PIE_NO_CULL | PIE_TEXTURED;
		renderPoly.nVrts = 4;
		renderPoly.pVrts = &d3dVrts[0];
	}
	for(i = 0; i < 4; i++)
	{
		d3dVrts[i].sx = (float)aSky[i].sx;
		d3dVrts[i].sy = (float)aSky[i].sy;
			if (d3dVrts[i].sy == -9999)
			{
				renderPoly.flags = 0;
			}
		d3dVrts[i].tu = (float)aSky[i].tu * (float)INV_TEX_SIZE;
		d3dVrts[i].tv = (float)aSky[i].tv * (float)INV_TEX_SIZE;
		d3dVrts[i].sz = (float)aSky[i].sz * (float)INV_MAX_Z;//scr Points[*index].d3dz;
		d3dVrts[i].rhw = (float)1.0 / d3dVrts[i].sz;
		d3dVrts[i].color = (D3DCOLOR)((255 << 24) + (255 << 16) + (255 << 8) + 255);
		d3dVrts[i].specular = 0;
	}
	if ((rendSurface.usr == iV_MODE_4101) || (rendSurface.usr == REND_GLIDE_3DFX))
	{
	}
	else
	{
		pie_SetTexturePage(texPage);
		pie_D3DPoly(&renderPoly);	   // draw the polygon ... this is an inline function
	}
}

void pie_Water(SDWORD texPage, SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, SDWORD height, SDWORD translucency)
{
	SDWORD i;
	PIEPIXEL pPixel;
	iVector vertex;
	PIED3DPOLY renderPoly;
	int32 rx, ry, rz;
	int32 tzx, tzy;

	vertex.y = height;

	if ((rendSurface.usr == iV_MODE_4101) || (rendSurface.usr == REND_GLIDE_3DFX))
	{
	}
	else
	{
		renderPoly.flags = PIE_NO_CULL | PIE_TEXTURED | PIE_COLOURKEYED | PIE_ALPHA;
		renderPoly.nVrts = 4;
		renderPoly.pVrts = &d3dVrts[0];
	}

	for(i = 0; i < 4; i++)
	{
		switch(i)
		{
			case 0:
				vertex.x = x0;
				vertex.z = y0;
				d3dVrts[i].tu = (float)0.0;
				d3dVrts[i].tv = (float)0.0;
				break;
			case 1:
				vertex.x = x1;
				vertex.z = y0;
				d3dVrts[i].tu = (float)1.0;
				d3dVrts[i].tv = (float)0.0;
				break;
			case 2:
				vertex.x = x1;
				vertex.z = y1;
				d3dVrts[i].tu = (float)1.0;
				d3dVrts[i].tv = (float)0.5;
				break;
			case 3:
			default:
				vertex.x = x0;
				vertex.z = y1;
				d3dVrts[i].tu = (float)0.0;
				d3dVrts[i].tv = (float)0.5;
				break;
		}
		rx = vertex.x * psMatrix->a + vertex.y * psMatrix->d + vertex.z * psMatrix->g + psMatrix->j;
		ry = vertex.x * psMatrix->b + vertex.y * psMatrix->e + vertex.z * psMatrix->h + psMatrix->k;
		rz = vertex.x * psMatrix->c + vertex.y * psMatrix->f + vertex.z * psMatrix->i + psMatrix->l;

		pPixel.d3dz = D3DVAL((rz>>STRETCHED_Z_SHIFT));

		tzx = rz >> psRendSurface->xpshift;
		tzy = rz >> psRendSurface->ypshift;

		if ((tzx<=0) || (tzy<=0))
		{
			pPixel.d3dx = (float)LONG_WAY;//just along way off screen
			pPixel.d3dy = (float)LONG_WAY;
		}
		else if (pPixel.d3dz < D3DVAL(MIN_STRETCHED_Z))
		{
			pPixel.d3dx = (float)LONG_WAY;//just along way off screen
			pPixel.d3dy = (float)LONG_WAY;
		}
		else
		{
			pPixel.d3dx = D3DVAL((psRendSurface->xcentre + (rx / tzx)));
			pPixel.d3dy = D3DVAL((psRendSurface->ycentre - (ry / tzy)));
		}

		d3dVrts[i].sx = (float)pPixel.d3dx;
		d3dVrts[i].sy = (float)pPixel.d3dy;
		if (d3dVrts[i].sy > LONG_TEST)
		{
			renderPoly.flags = 0;
		}
		d3dVrts[i].sz = (float)pPixel.d3dz * (float)INV_MAX_Z;
		d3dVrts[i].rhw = (float)1.0 / d3dVrts[i].sz;
		d3dVrts[i].color = (D3DCOLOR)((translucency << 24) + (translucency << 16) + (translucency << 8) + translucency);
		d3dVrts[i].specular = 0;
	}
	if ((rendSurface.usr == iV_MODE_4101) || (rendSurface.usr == REND_GLIDE_3DFX))
	{
	}
	else
	{
		pie_SetTexturePage(texPage);
		pie_D3DPoly(&renderPoly);	   // draw the polygon ... this is an inline function
	}
}

#define FOG_RED 00
#define FOG_GREEN 00
#define FOG_BLUE 80
#define MIST_RED 00
#define MIST_GREEN 80
#define MIST_BLUE 00
#define FOG_DEPTH 3000
#define FOG_RATE 4
#define MIST_HEIGHT 100
#define SPECULAR_FOG_AND_MIST 0

void pie_AddFogandMist(SDWORD depth, SDWORD height, PIELIGHT* pColour, PIELIGHT* pSpecular)
{
#if SPECULAR_FOG_AND_MIST
	SDWORD fogLevel;//0 to 256
	SDWORD mistLevel;//0 to 256
	DWORD cRed,cGreen,cBlue;
	DWORD sRed,sGreen,sBlue;

	/// get colour components
/*	cRed = pColour->r;
	cGreen = pColour->g;
	cBlue = pColour->b;
*/
	/// get specular components
/*	sRed = pSpecular->r;
	sGreen = pSpecular->g;
	sBlue = pSpecular->b;
*/
	// fog level is gradual upto FOG_DEPTH then ramps up suddenly
	if (depth < (FOG_DEPTH/2))
	{
		fogLevel = 256;
	}
	else if (depth < FOG_DEPTH)
	{
		depth -= (FOG_DEPTH/2);
		fogLevel = 192 * depth/(FOG_DEPTH/2);
	}
	else
	{
		depth -= FOG_DEPTH;
		fogLevel = 64 * depth/FOG_RATE;
		fogLevel += 192;
		if (fogLevel > 256)
		{
			fogLevel = 256;
		}
	}

	pColour->a = fogLevel;
/*
	cRed = (cRed * (256 - fogLevel))>>8;
	cGreen = (cGreen * (256 - fogLevel))>>8;
	cBlue = (cBlue * (256 - fogLevel))>>8;

	sRed += (FOG_RED * fogLevel)>>8;
	sGreen += (FOG_GREEN * fogLevel)>>8;
	sBlue += (FOG_BLUE * fogLevel)>>8;

	// set colour components
	pColour->r = cRed;
	pColour->g = cGreen;
	pColour->b = cBlue;
	// set specular components
	pSpecular->r = sRed;
	pSpecular->g = sGreen;
	pSpecular->b = sBlue;
*/
	// mist level is high upto MIST_HEIGHT then ramps up gently
#endif
}

void pie_InitMaths(void)
{
	UBYTE c;
	UDWORD a,b,bigC;

	for(a=0; a<=UBYTE_MAX; a++)
	{
		for(b=0; b<=UBYTE_MAX; b++)
		{
			bigC = a * b;
			bigC /= UBYTE_MAX;
			ASSERT((bigC <= UBYTE_MAX,"light_InitMaths; rounding error"));
			c = (UBYTE)bigC;
			aByteScale[a][b] = c;
		}
	}
}

UBYTE pie_ByteScale(UBYTE a, UBYTE b)
{
	return aByteScale[a][b];
}

void	pie_doWeirdBoxFX(UDWORD x, UDWORD y, UDWORD x2, UDWORD y2, UDWORD	trans)
{
UDWORD	val;
UDWORD	xDif;
UDWORD	yDif;
UDWORD	radius;

	val = getTimeValueRange(5760,360);
	radius = 100;
	xDif = radius * (SIN(DEG(val)));
	yDif = radius * (COS(DEG(val)));

	xDif = xDif/4096;	 // cos it's fixed point
	yDif = yDif/4096;

 	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
   	pie_CornerBox(x,y,x2,y2,trans,20+(radius+xDif),20+(radius+yDif),20+(radius-xDif),20+(radius-yDif));
	/*
	val = 360-getTimeValueRange(2880,360);
	xDif = radius * (SIN(DEG(val)));
	yDif = radius * (COS(DEG(val)));

	xDif = xDif/4096;	 // cos it's fixed point
	yDif = yDif/4096;
//	pie_BoxFill(100,100,200,200,234);
   	pie_CornerBox(x,y,x2,y2,trans,20+(radius+xDif),20+(radius+yDif),20+(radius-xDif),20+(radius-yDif));
   	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
	*/
}


//render raw data in system memory to direct draw surface
//use outside of D3D sceen only
void pie_RenderImageToSurface(LPDIRECTDRAWSURFACE4 lpDDS4, SDWORD surfaceOffsetX, SDWORD surfaceOffsetY, UWORD* pSrcData, SDWORD srcWidth, SDWORD srcHeight, SDWORD srcStride)
{

}


