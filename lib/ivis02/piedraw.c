/***************************************************************************/
/*
 * piedraw.c
 *
 * updated render routines for 3D coloured shaded transparency rendering
 *
 */
/***************************************************************************/

#include "frame.h"

#include "ivisdef.h"

#include "imd.h"
#include "rendmode.h"
//#include "d3dmode.h"
#include "pieFunc.h"
#include "pieMatrix.h"
#include "tex.h"

#include "piedef.h"
#include "pieState.h"
#include "pieTexture.h"
#include "pieClip.h"

#include "d3d.h"
#include "d3drender.h"

#ifdef INC_GLIDE
	#include "dGlide.h"
	#include "3dfxFunc.h"
	#include "3dfxText.h"
#endif

#ifndef PIEPSX		// was #ifdef WIN32
#define MIST

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

static PIEPIXEL		scrPoints[pie_MAX_POINTS];
static PIEVERTEX	pieVrts[pie_MAX_POLY_VERTS];
static PIEVERTEX	clippedVrts[pie_MAX_POLY_VERTS];
static D3DTLVERTEX	d3dVrts[pie_MAX_POLY_VERTS];
static iVertex		imdVrts[pie_MAX_POLY_VERTS];
static SDWORD		pieCount = 0;
static SDWORD		tileCount = 0;
static SDWORD		polyCount = 0;

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/

//old ivis style draw poly (low level) software mode only
static void pie_IvisPoly(SDWORD texPage, iIMDPoly *poly, BOOL bClip);
static void pie_IvisPolyFrame(SDWORD texPage, iIMDPoly *poly, SDWORD frame, BOOL bClip);
//d3d draw poly (low level) D3D mode only
void pie_D3DPoly(PIED3DPOLY *poly);
static void pie_D3DPolyFrame(PIED3DPOLY *poly, SDWORD frame);
//pievertex draw poly (low level) //all modes from PIEVERTEX data
static void pie_PiePoly(PIEPOLY *poly, BOOL bClip);
static void pie_PiePolyFrame(PIEPOLY *poly, SDWORD frame, BOOL bClip);

void DrawTriangleList(BSPPOLYID PolygonNumber);

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

#ifdef BSPIMD
	extern iIMDShape *BSPimd;	// global defined here for speed 
extern	iIMDPoly *BSPScrVertices;
UDWORD ShapeTexPage;
UDWORD ShapeFrame;

	// This is a BSP routine that draws a linked list of polygon
	// .. Its in here becuase it uses in inline function "DrawPoly"
	#define IMD_POLYGON(poly) (	&BSPimd->polys[(poly)])

void DrawTriangleList(BSPPOLYID PolygonNumber)
	{
		iIMDPoly *pPolys;
		UDWORD n;

		VERTEXID	*index;
		iIMDPoly	imdPoly;


//	DBPRINTF(("poly %d\n",PolygonNumber));

		while(PolygonNumber!=BSPPOLYID_TERMINATE)
		{
			pPolys= IMD_POLYGON(PolygonNumber);	

				index = pPolys->pindex;
				imdPoly.flags = pPolys->flags;
				for (n=0; n<(SDWORD)(pPolys->npnts); n++, index++)
				{
					imdVrts[n].x = MAKEINT(scrPoints[*index].d3dx);
					imdVrts[n].y = MAKEINT(scrPoints[*index].d3dy);
					imdVrts[n].z = 0;

					//cull triangles with off screen points
					if (scrPoints[*index].d3dy > (float)LONG_TEST)
						imdPoly.flags = 0;

					imdVrts[n].u = pPolys->vrt[n].u;
					imdVrts[n].v = pPolys->vrt[n].v;
					imdVrts[n].g  = 128;//(red + green + blue + alpha)>>2;
				}
				imdPoly.npnts = pPolys->npnts;
				imdPoly.vrt = &imdVrts[0];
				imdPoly.pTexAnim = pPolys->pTexAnim;
				if (imdPoly.flags > 0)
				{
					pie_IvisPolyFrame(ShapeTexPage, &imdPoly,ShapeFrame,TRUE);	   // draw the polygon ... this is an inline function
				}

				PolygonNumber=pPolys->BSP_NextPoly;
		}
	}

// BSP object position
static iVector BSPObject;
static iVector BSPCamera;
static SDWORD BSPObject_Yaw=0,BSPObject_Pitch=0;


void SetBSPObjectPos(SDWORD x,SDWORD y,SDWORD z)
{

	
	BSPObject.x=x;
	BSPObject.y=y;
	BSPObject.z=z;
	// Reset the yaw & pitch

		// these values must be set every time they are used ...
	BSPObject_Yaw=0;
	BSPObject_Pitch=0;


}

// This MUST be called after SetBSPObjectPos ...

void SetBSPObjectRot(SDWORD Yaw, SDWORD Pitch)
{
	BSPObject_Yaw=Yaw;
	BSPObject_Pitch=Pitch;
}


// This must be called once per frame after the terrainMidX & player.p values have been updated
void SetBSPCameraPos(SDWORD x,SDWORD y,SDWORD z)
{
	BSPCamera.x=x;
	BSPCamera.y=y;
	BSPCamera.z=z;
}



static void AddIMDPrimativesBSP2(iIMDShape *IMDdef,iIMDPoly *ScrVertices, UDWORD frame)
{
	iVector pPos;

	SDWORD xDif,zDif;

// Camera ---- X is map colums +ve is going left, Z is map rows  +ve is going down the map, Y is map height +ve is up in the air

	ShapeTexPage=IMDdef->texpage;
	ShapeFrame=frame;

	xDif= (BSPCamera.x - BSPObject.x);
	zDif= (BSPCamera.z - BSPObject.z);

//	pPos.x=xDif;		// mapX
//	pPos.z=-zDif;		// mapY

	pPos.x = (((COS(BSPObject_Yaw)*xDif)/4096)-((SIN(BSPObject_Yaw)*zDif)/4096));
	pPos.z = (-(((SIN(BSPObject_Yaw)*xDif)/4096)+((COS(BSPObject_Yaw)*zDif)/4096)));

	pPos.y = (BSPCamera.y - BSPObject.y);		// map height (up in the air)

//	DBPRINTF(("cam(%d,%d,%d) obj(%d,%d,%d) \n",BSPCamera.x,BSPCamera.y,BSPCamera.z,BSPObject.x,BSPObject.y,BSPObject.z));
//	DBPRINTF((" pos(%d,%d,%d)\n",pPos.x,pPos.y,pPos.z));




	DrawBSPIMD( IMDdef, &pPos , ScrVertices);		// in bspimd.c
}



#endif

/***************************************************************************
 * pie_Draw3dShape
 *
 * Project and render a pumpkin image to render surface
 * Will support zbuffering, texturing, coloured lighting and alpha effects
 * Avoids recalculating vertex projections for every poly 
 ***************************************************************************/

//#define _3DNOW

//#ifdef _3DNOW
#include "amd3d.h"
//#endif

// ---
// DEV STUDIO 5, 3D NOW and INTEL VERSION.
#if (_MSC_VER != 1000) && (_MSC_VER != 1020)
void pie_Draw3DShape(iIMDShape *shape, int frame, int team, UDWORD col, UDWORD spec, int pieFlag, int pieFlagData)
{

	// needed for AMD
	int amd_scale = 0x3a800000;				// 2^-10
	int amd_pie_RAISE_SCALE = 0x3b800000;	// 2^-8
	int amd_sign = 0x80000000;
	int	amd_RAISE = 0;
	int	amd_HEIGHT_SCALED = 0x3f800000;

	// needed for intel
	int32		rx, ry, rz;
	int32		tzx, tzy;
	int32		tempY;

	int i, n;
	iVector		*pVertices;
	PIEPIXEL	*pPixels;
	iIMDPoly	*pPolys;
	PIEPOLY		piePoly;
	iIMDPoly	imdPoly;
	VERTEXID	*index;
	PIELIGHT	colour, specular;
	UBYTE		alpha;

	pieCount++;

	// Fix for transparent buildings and features!! */
	if( (pieFlag & pie_TRANSLUCENT) AND (pieFlagData>220) )
	{
		pieFlag = pieFlagData = 0;	// force to bilinear and non-transparent
	}
	// Fix for transparent buildings and features!! */

	

// WARZONE light as byte passed in colour so expand
	if (col <= MAX_UB_LIGHT)
	{
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			colour.byte.a = 0;//no fog
		}
		else
		{
			colour.byte.a = 255;//no fog
		}
		colour.byte.r = (UBYTE)col;
		colour.byte.g = (UBYTE)col;
		colour.byte.b = (UBYTE)col;
	}
	else
	{
		colour.argb = col;
	}
	specular.argb = spec;

	if (frame == 0) 
	{
		frame = team;
	}

	if (!pie_Translucent())
	{
		if ((pieFlag & pie_ADDITIVE) || (pieFlag & pie_TRANSLUCENT))
		{
			pieFlag &= (pie_FLAG_MASK - pie_TRANSLUCENT - pie_ADDITIVE);
			pieFlag |= pie_NO_BILINEAR;
		}
	}
	else if (!pie_Additive())
	{
		if (pieFlag & pie_ADDITIVE)//Assume also translucent
		{
			pieFlag -= pie_ADDITIVE;
			pieFlag |= pie_TRANSLUCENT;
			pieFlag |= pie_NO_BILINEAR;
			pieFlagData /= 2;
		}
	}
	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE)//Assume also translucent
	{
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ADDITIVE_TEX);
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			alpha = 255-colour.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
		}
		else if (pie_GetRenderEngine() == ENGINE_D3D)
		{
			alpha = 255-specular.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
			colour.byte.r = pie_ByteScale(alpha, colour.byte.r);
			colour.byte.g = pie_ByteScale(alpha, colour.byte.g);
			colour.byte.b = pie_ByteScale(alpha, colour.byte.b);
			specular.argb = 0;
		}
		else
		{
			colour.byte.a = (UBYTE)pieFlagData;
		}
		pie_SetBilinear(TRUE);
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ALPHA_TEX);
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			alpha = 255-colour.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
		}
		else if (pie_GetRenderEngine() == ENGINE_D3D)
		{
			alpha = 255-specular.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
			specular.argb = 0;
		}
		else
		{
			colour.byte.a = (UBYTE)pieFlagData;
		}
		pie_SetBilinear(FALSE);//never bilinear with constant alpha, gives black edges 
	}
	else
	{
		if (pieFlag & pie_BUTTON)
		{
			pie_SetFogStatus(FALSE);
			pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
		}
		else
		{
			pie_SetFogStatus(TRUE);
		}
		pie_SetRendMode(REND_GOURAUD_TEX);
		//if hardware fog then alpha is set else unused in decal mode
		//colour.byte.a = MAX_UB_LIGHT;
		if (pieFlag & pie_NO_BILINEAR)
		{
			pie_SetBilinear(FALSE);
		}
		else
		{
			pie_SetBilinear(TRUE);
		}
	}


	if (pieFlag & pie_RAISE)
	{
			pieFlagData = (shape->ymax * (pie_RAISE_SCALE - pieFlagData))/pie_RAISE_SCALE;
	}

	pie_SetTexturePage(shape->texpage);


//now draw the shape	
	//rotate and project points from shape->points to scrPoints
	pVertices = shape->points;
	pPixels = &scrPoints[0];

	if(weHave3DNow())  // call alex m's AMD detection stuff - uses _emit?!?!?
	{
		// Mike Goddard's funky code replacement.
		_asm {
			FEMMS

			movd		mm2, amd_RAISE			// defaults
			mov			eax, pieFlag
			test		eax, pie_RAISE
			jz			no_RAISE
			movd		mm0, pieFlagData
			pi2fd		(m0, m0)
			movd		amd_RAISE, mm0
			movq		mm2, mm0				// for first iteration
		no_RAISE:
			movd		mm3, amd_HEIGHT_SCALED	// defaults
			test		eax, pie_HEIGHT_SCALED
			jz			no_HEIGHT_SCALED
			movd		mm0, pieFlagData
			movd		mm1, amd_pie_RAISE_SCALE
			pi2fd		(m0, m0)
			pfmul		(m0, m1)
			movd		amd_HEIGHT_SCALED, mm0
			movq		mm3, mm0				// for first iteration
		no_HEIGHT_SCALED:

			mov			eax, pVertices
			mov			ebx, psMatrix
			mov			ecx, shape
			mov			edx, pPixels
			mov			ecx, [ecx]iIMDShape.npoints
			mov			edi, psRendSurface
			test		ecx, ecx
			je			tloop_done
		tloop:
			movd		mm1, [eax+4]			// 0 | y
			;
			movd		mm0, [eax]				// 0 | x
			;
			pi2fd		(m1, m1)
			;
			pi2fd		(m0, m0)
			;
			pfsub		(m1, m2)				// 0 | RAISE y
			movd		mm2, [eax+8]			// 0 | z
			movq		mm7, [ebx]				// b | a
			punpckldq	mm0, mm0				// x | x
			movq		mm6, [ebx+12]			// e | d
			pfmul		(m1, m3)				// 0 | SCALE y
			movq		mm5, [ebx+24]			// h | g
			punpckldq	mm2, mm2				// z | z
			movd		mm4, [ebx+8]			// 0 | c
			punpckldq	mm1, mm1				// y | y
			pi2fd		(m2, m2)
			;
			pi2fd		(m7, m7)
			;
			pi2fd		(m6, m6)
			;
			pi2fd		(m5, m5)
			pfmul		(m7, m0)				// x*b | x*a
			pi2fd		(m4, m4)
			pfmul		(m6, m1)				// y*e | y*d
			pfmul		(m5, m2)				// z*h | z*g
			;
			pfadd		(m6, m7)				// y*e+x*b | y*d+x*a
			movd		mm7, [ebx+20]			// 0 | f
			pfmul		(m4, m0)				// 0 | x*c
			;
			pi2fd		(m7, m7)
			;
			pfadd		(m5, m6)				// z*h+y*e+x*b | z*g+y*d+x*a
			movd		mm6, [ebx+32]			// 0 | i
			pfmul		(m7, m1)				// 0 | y*f
			movq		mm0, [ebx+36]			// k | j
			pi2fd		(m6, m6)
			movd		mm1, [ebx+44]			// 0 | l
			pfadd		(m7, m4)				// 0 | y*f+x*c
			movd		mm4, amd_scale
			pfmul		(m6, m2)				// 0 | z*i
			pi2fd		(m0, m0)
			pi2fd		(m1, m1)
			;
			pfadd		(m6, m7)				// 0 | z*i+y*f+x*c
			;
			pfadd		(m5, m0)				// ry | rx
			pxor		mm0, mm0
			pfadd		(m6, m1)				//  0 | rz
			punpckldq	mm0, amd_sign
			movq		mm1, [edi]iSurface.xcentre
			pxor		mm2, mm2
			pfmul		(m6, m4)				// 0 | srz
			movd		[edx]PIEPIXEL.d3dz, mm6
			pxor		mm5, mm0
			pi2fd		(m1, m1)
			pfmax		(m6, m2)				// chop less than zero to max
			movd		mm2, amd_RAISE			// restore
			;
			;
			pfrcp		(m7, m6)				// 1/srz | 1/srz
			movd		mm3, amd_HEIGHT_SCALED	// restore
			;
			;
			pfmul		(m5, m7)				// ry/srz | rx/srz
			add			eax, 12
			add			edx, 12
			dec			ecx
			pfadd		(m5, m1)
			movq		[edx-12]PIEPIXEL.d3dx, mm5
			jne			tloop
		tloop_done:
			FEMMS
		}
	}
	else	// run the intel one
	//--
		for (i=0; i<shape->npoints; i++, pVertices++, pPixels++)
	{
				tempY = pVertices->y;
				if (pieFlag & pie_RAISE)
				{
					tempY = pVertices->y - pieFlagData;
					if (tempY < 0) tempY = 0;

				}
				else if (pieFlag & pie_HEIGHT_SCALED)
				{
					if(pVertices->y>0)
					{
						tempY = (pVertices->y * pieFlagData)/pie_RAISE_SCALE;
					}
					//if (tempY < 0) tempY = 0;
				}
				rx = pVertices->x * psMatrix->a + tempY * psMatrix->d + pVertices->z * psMatrix->g + psMatrix->j;
				ry = pVertices->x * psMatrix->b + tempY * psMatrix->e + pVertices->z * psMatrix->h + psMatrix->k;
				rz = pVertices->x * psMatrix->c + tempY * psMatrix->f + pVertices->z * psMatrix->i + psMatrix->l;



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


	//--


#ifdef BSPIMD

	if ((!pie_Hardware()) AND shape->BSPNode!=NULL)
	{
		AddIMDPrimativesBSP2(shape,scrPoints,frame);
		return;
	}

#endif

	//build and render polygons
	if (pie_GetRenderEngine() == ENGINE_4101)
	{
		pPolys = shape->polys;
		for (i=0; i<shape->npolys; i++, pPolys++)
		{

				index = pPolys->pindex;
				imdPoly.flags = pPolys->flags;
				for (n=0; n<pPolys->npnts; n++, index++)
				{
					imdVrts[n].x = MAKEINT(scrPoints[*index].d3dx);
					imdVrts[n].y = MAKEINT(scrPoints[*index].d3dy);
					imdVrts[n].z = 0;

					//cull triangles with off screen points
					if (scrPoints[*index].d3dy > (float)LONG_TEST)
						imdPoly.flags = 0;

					imdVrts[n].u = pPolys->vrt[n].u;
					imdVrts[n].v = pPolys->vrt[n].v;
					imdVrts[n].g  = 128;//(red + green + blue + alpha)>>2;
				}
				imdPoly.npnts = pPolys->npnts;
				imdPoly.vrt = &imdVrts[0];
#ifndef PIEPSX   // was #ifdef WIN32
				imdPoly.pTexAnim = pPolys->pTexAnim;
#endif
				if (imdPoly.flags > 0)
				{
					pie_IvisPolyFrame(shape->texpage, &imdPoly,frame,TRUE);	   // draw the polygon ... this is an inline function
				}
		}
	}
	else //if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		pPolys = shape->polys;
		for (i=0; i<shape->npolys; i++, pPolys++)
		{
				index = pPolys->pindex;
				piePoly.flags = pPolys->flags;
				if (pieFlag & pie_TRANSLUCENT)
				{
					piePoly.flags |= PIE_ALPHA;
				}
				else if (pieFlag & pie_ADDITIVE)
				{
					piePoly.flags &= (0xffffffff-PIE_COLOURKEYED);//dont treat additive images as colour keyed
				}
				for (n=0; n<pPolys->npnts; n++, index++)
				{
					pieVrts[n].sx = MAKEINT(scrPoints[*index].d3dx);
					pieVrts[n].sy = MAKEINT(scrPoints[*index].d3dy);
					//cull triangles with off screen points
					if (scrPoints[*index].d3dy > (float)LONG_TEST)
					{
						piePoly.flags = 0;
					}
					pieVrts[n].sz  = MAKEINT(scrPoints[*index].d3dz);
					pieVrts[n].tu = pPolys->vrt[n].u;
					pieVrts[n].tv = pPolys->vrt[n].v;
					pieVrts[n].light.argb = colour.argb;
					pieVrts[n].specular.argb = specular.argb;
				}
				piePoly.nVrts = pPolys->npnts;
				piePoly.pVrts = &pieVrts[0];
#ifndef PIEPSX   // was #ifdef WIN32
				piePoly.pTexAnim = pPolys->pTexAnim;
#endif
				if (piePoly.flags > 0)
				{
						pie_PiePolyFrame(&piePoly,frame,TRUE);	   // draw the polygon ... this is an inline function
				}

			}
	}
	if (pieFlag & pie_BUTTON)
	{
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	}
}
#else
// DEV STUDIO 4, 3D NOW and INTEL ONLY VERSION.

// ---
//
//void pie_Draw3DIntelShape(iIMDShape *shape, int frame, int team, UDWORD col, UDWORD spec, int pieFlag, int pieFlagData)
void pie_Draw3DShape(iIMDShape *shape, int frame, int team, UDWORD col, UDWORD spec, int pieFlag, int pieFlagData)
{

	int32		rx, ry, rz;
	int32		tzx, tzy;
	int32		tempY;
	int i, n;
	iVector		*pVertices;
	PIEPIXEL	*pPixels;
	iIMDPoly	*pPolys;
	PIEPOLY		piePoly;
	iIMDPoly	imdPoly;
	VERTEXID	*index;
	PIELIGHT	colour, specular;
	UBYTE		alpha;

	pieCount++;

	// Fix for transparent buildings and features!! */
	if( (pieFlag & pie_TRANSLUCENT) AND (pieFlagData>220) )
	{
		pieFlag = pieFlagData = 0;	// force to bilinear and non-transparent
	}
	// Fix for transparent buildings and features!! */

	

// WARZONE light as byte passed in colour so expand
	if (col <= MAX_UB_LIGHT)
	{
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			colour.byte.a = 0;//no fog
		}
		else
		{
			colour.byte.a = 255;//no fog
		}
		colour.byte.r = (UBYTE)col;
		colour.byte.g = (UBYTE)col;
		colour.byte.b = (UBYTE)col;
	}
	else
	{
		colour.argb = col;
	}
	specular.argb = spec;

	if (frame == 0) 
	{
		frame = team;
	}

	if (!pie_Translucent())
	{
		if ((pieFlag & pie_ADDITIVE) || (pieFlag & pie_TRANSLUCENT))
		{
			pieFlag &= (pie_FLAG_MASK - pie_TRANSLUCENT - pie_ADDITIVE);
			pieFlag |= pie_NO_BILINEAR;
		}
	}
	else if (!pie_Additive())
	{
		if (pieFlag & pie_ADDITIVE)//Assume also translucent
		{
			pieFlag -= pie_ADDITIVE;
			pieFlag |= pie_TRANSLUCENT;
			pieFlag |= pie_NO_BILINEAR;
			pieFlagData /= 2;
		}
	}
	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE)//Assume also translucent
	{
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ADDITIVE_TEX);
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			alpha = 255-colour.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
		}
		else if (pie_GetRenderEngine() == ENGINE_D3D)
		{
			alpha = 255-specular.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
			colour.byte.r = pie_ByteScale(alpha, colour.byte.r);
			colour.byte.g = pie_ByteScale(alpha, colour.byte.g);
			colour.byte.b = pie_ByteScale(alpha, colour.byte.b);
			specular.argb = 0;
		}
		else
		{
			colour.byte.a = (UBYTE)pieFlagData;
		}
		pie_SetBilinear(TRUE);
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ALPHA_TEX);
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			alpha = 255-colour.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
		}
		else if (pie_GetRenderEngine() == ENGINE_D3D)
		{
			alpha = 255-specular.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
			specular.argb = 0;
		}
		else
		{
			colour.byte.a = (UBYTE)pieFlagData;
		}
		pie_SetBilinear(FALSE);//never bilinear with constant alpha, gives black edges 
	}
	else
	{
		if (pieFlag & pie_BUTTON)
		{
			pie_SetFogStatus(FALSE);
			pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
		}
		else
		{
			pie_SetFogStatus(TRUE);
		}
		pie_SetRendMode(REND_GOURAUD_TEX);
		//if hardware fog then alpha is set else unused in decal mode
		//colour.byte.a = MAX_UB_LIGHT;
		if (pieFlag & pie_NO_BILINEAR)
		{
			pie_SetBilinear(FALSE);
		}
		else
		{
			pie_SetBilinear(TRUE);
		}
	}


	if (pieFlag & pie_RAISE)
	{
			pieFlagData = (shape->ymax * (pie_RAISE_SCALE - pieFlagData))/pie_RAISE_SCALE;
	}

	pie_SetTexturePage(shape->texpage);


//now draw the shape	
	//rotate and project points from shape->points to scrPoints
	pVertices = shape->points;
	pPixels = &scrPoints[0];


	for (i=0; i<shape->npoints; i++, pVertices++, pPixels++)
	{
				tempY = pVertices->y;
				if (pieFlag & pie_RAISE)
				{
					tempY = pVertices->y - pieFlagData;
					if (tempY < 0) tempY = 0;

				}
				else if (pieFlag & pie_HEIGHT_SCALED)
				{
					if(pVertices->y>0)
					{
						tempY = (pVertices->y * pieFlagData)/pie_RAISE_SCALE;
					}
					//if (tempY < 0) tempY = 0;
				}
				rx = pVertices->x * psMatrix->a + tempY * psMatrix->d + pVertices->z * psMatrix->g + psMatrix->j;
				ry = pVertices->x * psMatrix->b + tempY * psMatrix->e + pVertices->z * psMatrix->h + psMatrix->k;
				rz = pVertices->x * psMatrix->c + tempY * psMatrix->f + pVertices->z * psMatrix->i + psMatrix->l;



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


#ifdef BSPIMD

	if ((!pie_Hardware()) AND shape->BSPNode!=NULL)
	{
		AddIMDPrimativesBSP2(shape,scrPoints,frame);
		return;
	}

#endif

	//build and render polygons
	if (pie_GetRenderEngine() == ENGINE_4101)
	{
		pPolys = shape->polys;
		for (i=0; i<shape->npolys; i++, pPolys++)
		{

				index = pPolys->pindex;
				imdPoly.flags = pPolys->flags;
				for (n=0; n<pPolys->npnts; n++, index++)
				{
					imdVrts[n].x = MAKEINT(scrPoints[*index].d3dx);
					imdVrts[n].y = MAKEINT(scrPoints[*index].d3dy);
					imdVrts[n].z = 0;

					//cull triangles with off screen points
					if (scrPoints[*index].d3dy > (float)LONG_TEST)
						imdPoly.flags = 0;

					imdVrts[n].u = pPolys->vrt[n].u;
					imdVrts[n].v = pPolys->vrt[n].v;
					imdVrts[n].g  = 128;//(red + green + blue + alpha)>>2;
				}
				imdPoly.npnts = pPolys->npnts;
				imdPoly.vrt = &imdVrts[0];
#ifndef PIEPSX   // was #ifdef WIN32
				imdPoly.pTexAnim = pPolys->pTexAnim;
#endif
				if (imdPoly.flags > 0)
				{
					pie_IvisPolyFrame(shape->texpage, &imdPoly,frame,TRUE);	   // draw the polygon ... this is an inline function
				}
		}
	}
	else //if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		pPolys = shape->polys;
		for (i=0; i<shape->npolys; i++, pPolys++)
		{
				index = pPolys->pindex;
				piePoly.flags = pPolys->flags;
				if (pieFlag & pie_TRANSLUCENT)
				{
					piePoly.flags |= PIE_ALPHA;
				}
				else if (pieFlag & pie_ADDITIVE)
				{
					piePoly.flags &= (0xffffffff-PIE_COLOURKEYED);//dont treat additive images as colour keyed
				}
				for (n=0; n<pPolys->npnts; n++, index++)
				{
					pieVrts[n].sx = MAKEINT(scrPoints[*index].d3dx);
					pieVrts[n].sy = MAKEINT(scrPoints[*index].d3dy);
					//cull triangles with off screen points
					if (scrPoints[*index].d3dy > (float)LONG_TEST)
					{
						piePoly.flags = 0;
					}
					pieVrts[n].sz  = MAKEINT(scrPoints[*index].d3dz);
					pieVrts[n].tu = pPolys->vrt[n].u;
					pieVrts[n].tv = pPolys->vrt[n].v;
					pieVrts[n].light.argb = colour.argb;
					pieVrts[n].specular.argb = specular.argb;
				}
				piePoly.nVrts = pPolys->npnts;
				piePoly.pVrts = &pieVrts[0];
#ifndef PIEPSX   // was #ifdef WIN32
				piePoly.pTexAnim = pPolys->pTexAnim;
#endif
				if (piePoly.flags > 0)
				{
						pie_PiePolyFrame(&piePoly,frame,TRUE);	   // draw the polygon ... this is an inline function
				}

			}
	}
	if (pieFlag & pie_BUTTON)
	{
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	}
}
#endif
// THE VERSION MIKE CHANGED.
// 3D NOW Specific (and FASTER) shape renderer.
#if 0 
/*
void pie_Draw3DNowShape(iIMDShape *shape, int frame, int team, UDWORD col, UDWORD spec, int pieFlag, int pieFlagData)
{

	int amd_scale = 0x3a800000;				// 2^-10
	int amd_pie_RAISE_SCALE = 0x3b800000;	// 2^-8
	int amd_sign = 0x80000000;
	int	amd_RAISE = 0;
	int	amd_HEIGHT_SCALED = 0x3f800000;

	int i, n;
	iVector		*pVertices;
	PIEPIXEL	*pPixels;
	iIMDPoly	*pPolys;
	PIEPOLY		piePoly;
	iIMDPoly	imdPoly;
	VERTEXID	*index;
	PIELIGHT	colour, specular;
	UBYTE		alpha;

	pieCount++;

	// Fix for transparent buildings and features!!
	if( (pieFlag & pie_TRANSLUCENT) AND (pieFlagData>220) )
	{
		pieFlag = pieFlagData = 0;	// force to bilinear and non-transparent
	}
	// Fix for transparent buildings and features!!

	

// WARZONE light as byte passed in colour so expand
	if (col <= MAX_UB_LIGHT)
	{
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			colour.byte.a = 0;//no fog
		}
		else
		{
			colour.byte.a = 255;//no fog
		}
		colour.byte.r = (UBYTE)col;
		colour.byte.g = (UBYTE)col;
		colour.byte.b = (UBYTE)col;
	}
	else
	{
		colour.argb = col;
	}
	specular.argb = spec;

	if (frame == 0) 
	{
		frame = team;
	}

	if (!pie_Translucent())
	{
		if ((pieFlag & pie_ADDITIVE) || (pieFlag & pie_TRANSLUCENT))
		{
			pieFlag &= (pie_FLAG_MASK - pie_TRANSLUCENT - pie_ADDITIVE);
			pieFlag |= pie_NO_BILINEAR;
		}
	}
	else if (!pie_Additive())
	{
		if (pieFlag & pie_ADDITIVE)//Assume also translucent
		{
			pieFlag -= pie_ADDITIVE;
			pieFlag |= pie_TRANSLUCENT;
			pieFlag |= pie_NO_BILINEAR;
			pieFlagData /= 2;
		}
	}
	// Set tranlucency
	if (pieFlag & pie_ADDITIVE)//Assume also translucent
	{
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ADDITIVE_TEX);
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			alpha = 255-colour.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
		}
		else if (pie_GetRenderEngine() == ENGINE_D3D)
		{
			alpha = 255-specular.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
			colour.byte.r = pie_ByteScale(alpha, colour.byte.r);
			colour.byte.g = pie_ByteScale(alpha, colour.byte.g);
			colour.byte.b = pie_ByteScale(alpha, colour.byte.b);
			specular.argb = 0;
		}
		else
		{
			colour.byte.a = (UBYTE)pieFlagData;
		}
		pie_SetBilinear(TRUE);
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ALPHA_TEX);
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			alpha = 255-colour.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
		}
		else if (pie_GetRenderEngine() == ENGINE_D3D)
		{
			alpha = 255-specular.byte.a;
			alpha = pie_ByteScale(alpha, (UBYTE)pieFlagData);//scale transparency by fog value
			colour.byte.a = alpha;
			specular.argb = 0;
		}
		else
		{
			colour.byte.a = (UBYTE)pieFlagData;
		}
		pie_SetBilinear(FALSE);//never bilinear with constant alpha, gives black edges 
	}
	else
	{
		if (pieFlag & pie_BUTTON)
		{
			pie_SetFogStatus(FALSE);
			pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
		}
		else
		{
			pie_SetFogStatus(TRUE);
		}
		pie_SetRendMode(REND_GOURAUD_TEX);
		//if hardware fog then alpha is set else unused in decal mode
		//colour.byte.a = MAX_UB_LIGHT;
		if (pieFlag & pie_NO_BILINEAR)
		{
			pie_SetBilinear(FALSE);
		}
		else
		{
			pie_SetBilinear(TRUE);
		}
	}


	if (pieFlag & pie_RAISE)
	{
			pieFlagData = (shape->ymax * (pie_RAISE_SCALE - pieFlagData))/pie_RAISE_SCALE;
	}

	pie_SetTexturePage(shape->texpage);


//now draw the shape	
	//rotate and project points from shape->points to scrPoints
	pVertices = shape->points;
	pPixels = &scrPoints[0];

	// Mike Goddard's funky code replacement.
	_asm {
		FEMMS

		movd		mm2, amd_RAISE			// defaults
		mov			eax, pieFlag
		test		eax, pie_RAISE
		jz			no_RAISE
		movd		mm0, pieFlagData
		pi2fd		(m0, m0)
		movd		amd_RAISE, mm0
		movq		mm2, mm0				// for first iteration
	no_RAISE:
		movd		mm3, amd_HEIGHT_SCALED	// defaults
		test		eax, pie_HEIGHT_SCALED
		jz			no_HEIGHT_SCALED
		movd		mm0, pieFlagData
		movd		mm1, amd_pie_RAISE_SCALE
		pi2fd		(m0, m0)
		pfmul		(m0, m1)
		movd		amd_HEIGHT_SCALED, mm0
		movq		mm3, mm0				// for first iteration
	no_HEIGHT_SCALED:

		mov			eax, pVertices
		mov			ebx, psMatrix
		mov			ecx, shape
		mov			edx, pPixels
		mov			ecx, [ecx]iIMDShape.npoints
		mov			edi, psRendSurface
		test		ecx, ecx
		je			tloop_done
	tloop:
		movd		mm1, [eax+4]			// 0 | y
		;
		movd		mm0, [eax]				// 0 | x
		;
		pi2fd		(m1, m1)
		;
		pi2fd		(m0, m0)
		;
		pfsub		(m1, m2)				// 0 | RAISE y
		movd		mm2, [eax+8]			// 0 | z
		movq		mm7, [ebx]				// b | a
		punpckldq	mm0, mm0				// x | x
		movq		mm6, [ebx+12]			// e | d
		pfmul		(m1, m3)				// 0 | SCALE y
		movq		mm5, [ebx+24]			// h | g
		punpckldq	mm2, mm2				// z | z
		movd		mm4, [ebx+8]			// 0 | c
		punpckldq	mm1, mm1				// y | y
		pi2fd		(m2, m2)
		;
		pi2fd		(m7, m7)
		;
		pi2fd		(m6, m6)
		;
		pi2fd		(m5, m5)
		pfmul		(m7, m0)				// x*b | x*a
		pi2fd		(m4, m4)
		pfmul		(m6, m1)				// y*e | y*d
		pfmul		(m5, m2)				// z*h | z*g
		;
		pfadd		(m6, m7)				// y*e+x*b | y*d+x*a
		movd		mm7, [ebx+20]			// 0 | f
		pfmul		(m4, m0)				// 0 | x*c
		;
		pi2fd		(m7, m7)
		;
		pfadd		(m5, m6)				// z*h+y*e+x*b | z*g+y*d+x*a
		movd		mm6, [ebx+32]			// 0 | i
		pfmul		(m7, m1)				// 0 | y*f
		movq		mm0, [ebx+36]			// k | j
		pi2fd		(m6, m6)
		movd		mm1, [ebx+44]			// 0 | l
		pfadd		(m7, m4)				// 0 | y*f+x*c
		movd		mm4, amd_scale
		pfmul		(m6, m2)				// 0 | z*i
		pi2fd		(m0, m0)
		pi2fd		(m1, m1)
		;
		pfadd		(m6, m7)				// 0 | z*i+y*f+x*c
		;
		pfadd		(m5, m0)				// ry | rx
		pxor		mm0, mm0
		pfadd		(m6, m1)				//  0 | rz
		punpckldq	mm0, amd_sign
		movq		mm1, [edi]iSurface.xcentre
		pxor		mm2, mm2
		pfmul		(m6, m4)				// 0 | srz
		movd		[edx]PIEPIXEL.d3dz, mm6
		pxor		mm5, mm0
		pi2fd		(m1, m1)
		pfmax		(m6, m2)				// chop less than zero to max
		movd		mm2, amd_RAISE			// restore
		;
		;
		pfrcp		(m7, m6)				// 1/srz | 1/srz
		movd		mm3, amd_HEIGHT_SCALED	// restore
		;
		;
		pfmul		(m5, m7)				// ry/srz | rx/srz
		add			eax, 12
		add			edx, 12
		dec			ecx
		pfadd		(m5, m1)
		movq		[edx-12]PIEPIXEL.d3dx, mm5
		jne			tloop
	tloop_done:
		FEMMS
	}


#ifdef BSPIMD

	if ((!pie_Hardware()) AND shape->BSPNode!=NULL)
	{
		AddIMDPrimativesBSP2(shape,scrPoints,frame);
		return;
	}

#endif

	//build and render polygons
	if (pie_GetRenderEngine() == ENGINE_4101)
	{
		pPolys = shape->polys;
		for (i=0; i<shape->npolys; i++, pPolys++)
		{

				index = pPolys->pindex;
				imdPoly.flags = pPolys->flags;
				for (n=0; n<pPolys->npnts; n++, index++)
				{
					imdVrts[n].x = MAKEINT(scrPoints[*index].d3dx);
					imdVrts[n].y = MAKEINT(scrPoints[*index].d3dy);
					imdVrts[n].z = 0;

					//cull triangles with off screen points
					if (scrPoints[*index].d3dy > (float)LONG_TEST)
						imdPoly.flags = 0;

					imdVrts[n].u = pPolys->vrt[n].u;
					imdVrts[n].v = pPolys->vrt[n].v;
					imdVrts[n].g  = 128;//(red + green + blue + alpha)>>2;
				}
				imdPoly.npnts = pPolys->npnts;
				imdPoly.vrt = &imdVrts[0];
#ifndef PIEPSX   // was #ifdef WIN32
				imdPoly.pTexAnim = pPolys->pTexAnim;
#endif
				if (imdPoly.flags > 0)
				{
					pie_IvisPolyFrame(shape->texpage, &imdPoly,frame,TRUE);	   // draw the polygon ... this is an inline function
				}
		}
	}
	else //if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		pPolys = shape->polys;
		for (i=0; i<shape->npolys; i++, pPolys++)
		{
				index = pPolys->pindex;
				piePoly.flags = pPolys->flags;
				if (pieFlag & pie_TRANSLUCENT)
				{
					piePoly.flags |= PIE_ALPHA;
				}
				for (n=0; n<pPolys->npnts; n++, index++)
				{
					pieVrts[n].sx = MAKEINT(scrPoints[*index].d3dx);
					pieVrts[n].sy = MAKEINT(scrPoints[*index].d3dy);
					//cull triangles with off screen points
					if (scrPoints[*index].d3dy > (float)LONG_TEST)
					{
						piePoly.flags = 0;
					}
					pieVrts[n].sz  = MAKEINT(scrPoints[*index].d3dz);
					pieVrts[n].tu = pPolys->vrt[n].u;
					pieVrts[n].tv = pPolys->vrt[n].v;
					pieVrts[n].light.argb = colour.argb;
					pieVrts[n].specular.argb = specular.argb;
				}
				piePoly.nVrts = pPolys->npnts;
				piePoly.pVrts = &pieVrts[0];
#ifndef PIEPSX   // was #ifdef WIN32
				piePoly.pTexAnim = pPolys->pTexAnim;
#endif
				if (piePoly.flags > 0)
				{
						pie_PiePolyFrame(&piePoly,frame,TRUE);	   // draw the polygon ... this is an inline function
				}

			}
	}
	if (pieFlag & pie_BUTTON)
	{
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	}
}
*/
#endif
/***************************************************************************
 * pie_Drawimage
 *
 * General purpose blit function
 * Will support zbuffering, non_textured, coloured lighting and alpha effects
 *
 * replaces all ivis blit functions 
 *
 ***************************************************************************/
//d3d loses edge pixels in triangle draw
//this is a temporary correction that may become an option 

# define EDGE_CORRECTION 0

void pie_DrawImage(PIEIMAGE *image, PIERECT *dest, PIESTYLE *style)
{
	/* Set transparent color to be 0 red, 0 green, 0 blue, 0 alpha */
	polyCount++;

	pie_SetTexturePage(image->texPage);

	style->colour.argb = pie_GetColour();
	style->specular.argb = 0x00000000;

	//set up 4 pie verts
	pieVrts[0].sx = dest->x;
	pieVrts[0].sy = dest->y;
	pieVrts[0].sz  = (SDWORD)INTERFACE_DEPTH;
	pieVrts[0].tu = image->tu;
	pieVrts[0].tv = image->tv;
	pieVrts[0].light.argb = style->colour.argb;
	pieVrts[0].specular.argb = style->specular.argb;

	pieVrts[1].sx = dest->x + dest->w + EDGE_CORRECTION;
	pieVrts[1].sy = dest->y;
	pieVrts[1].sz  = (SDWORD)INTERFACE_DEPTH;
	pieVrts[1].tu = image->tu + image->tw + EDGE_CORRECTION;
	pieVrts[1].tv = image->tv;
	pieVrts[1].light.argb = style->colour.argb;
	pieVrts[1].specular.argb = style->specular.argb;

	pieVrts[2].sx = dest->x + dest->w + EDGE_CORRECTION;
	pieVrts[2].sy = dest->y + dest->h + EDGE_CORRECTION;
	pieVrts[2].sz  = (SDWORD)INTERFACE_DEPTH;
	pieVrts[2].tu = image->tu + image->tw + EDGE_CORRECTION;
	pieVrts[2].tv = image->tv + image->th + EDGE_CORRECTION;
	pieVrts[2].light.argb = style->colour.argb;
	pieVrts[2].specular.argb = style->specular.argb;

	pieVrts[3].sx = dest->x;
	pieVrts[3].sy = dest->y + dest->h + EDGE_CORRECTION;
	pieVrts[3].sz  = (SDWORD)INTERFACE_DEPTH;
	pieVrts[3].tu = image->tu;
	pieVrts[3].tv = image->tv + image->th + EDGE_CORRECTION;
	pieVrts[3].light.argb = style->colour.argb;
	pieVrts[3].specular.argb = style->specular.argb;

	if (pie_GetRenderEngine() == ENGINE_D3D)
	{
		D3D_PIEPolygon(4, pieVrts);
	}
	else if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		gl_PIEPolygon(4, pieVrts);
	}
}

/***************************************************************************
 * pie_Drawimage270
 *
 * General purpose blit function
 * Will support zbuffering, non_textured, coloured lighting and alpha effects
 *
 * replaces all ivis blit functions 
 *
 ***************************************************************************/

void pie_DrawImage270(PIEIMAGE *image, PIERECT *dest, PIESTYLE *style)
{
	/* Set transparent color to be 0 red, 0 green, 0 blue, 0 alpha */
	polyCount++;

	pie_SetTexturePage(image->texPage);

	style->colour.argb = pie_GetColour();
	style->specular.argb = 0x00000000;

	//set up 4 pie verts
	//set up 4 pie verts
	pieVrts[0].sx = dest->x;
	pieVrts[0].sy = dest->y;
	pieVrts[0].sz  = (SDWORD)INTERFACE_DEPTH;
	pieVrts[3].tu = image->tu;
	pieVrts[3].tv = image->tv;
	pieVrts[0].light.argb = style->colour.argb;
	pieVrts[0].specular.argb = style->specular.argb;

	pieVrts[1].sx = dest->x + dest->h + EDGE_CORRECTION;
	pieVrts[1].sy = dest->y;
	pieVrts[1].sz  = (SDWORD)INTERFACE_DEPTH;
	pieVrts[0].tu = image->tu + image->tw + EDGE_CORRECTION;
	pieVrts[0].tv = image->tv;
	pieVrts[1].light.argb = style->colour.argb;
	pieVrts[1].specular.argb = style->specular.argb;

	pieVrts[2].sx = dest->x + dest->h + EDGE_CORRECTION;
	pieVrts[2].sy = dest->y + dest->w + EDGE_CORRECTION;
	pieVrts[2].sz  = (SDWORD)INTERFACE_DEPTH;
	pieVrts[1].tu = image->tu + image->tw + EDGE_CORRECTION;
	pieVrts[1].tv = image->tv + image->th + EDGE_CORRECTION;
	pieVrts[2].light.argb = style->colour.argb;
	pieVrts[2].specular.argb = style->specular.argb;

	pieVrts[3].sx = dest->x;
	pieVrts[3].sy = dest->y + dest->w + EDGE_CORRECTION;
	pieVrts[3].sz  = (SDWORD)INTERFACE_DEPTH;
	pieVrts[2].tu = image->tu;
	pieVrts[2].tv = image->tv + image->th + EDGE_CORRECTION;
	pieVrts[3].light.argb = style->colour.argb;
	pieVrts[3].specular.argb = style->specular.argb;

	if (pie_GetRenderEngine() == ENGINE_D3D)
	{
		D3D_PIEPolygon(4, pieVrts);
	}
	else if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		gl_PIEPolygon(4, pieVrts);
	}
}

/***************************************************************************
 * pie_DrawLine
 *
 * universal line function for hardware
 *
 * Assumes render mode set up externally
 *
 ***************************************************************************/

void pie_DrawLine(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour, BOOL bClip)
{
	SDWORD n;
	polyCount++;

	pie_SetTexturePage(-1);

	if (bClip)
	{
		n = pie_ClipFlat2dLine(x0, y0, x1, y1);
		if (n != 2)
		{
			return;
		}
	}
	if (pie_GetRenderEngine() == ENGINE_D3D)
	{
		d3dVrts[0].sx = (float)x0;
		d3dVrts[0].sy = (float)y0;
		
		d3dVrts[0].sz  = (float)INTERFACE_DEPTH * (float)INV_MAX_Z;
		d3dVrts[0].rhw = (float)1.0/d3dVrts[0].sz;

		d3dVrts[0].tu = (float)0.0;
		d3dVrts[0].tv = (float)0.0;
		d3dVrts[0].color = colour;
		d3dVrts[0].specular = 0;

		memcpy(&d3dVrts[1],&d3dVrts[0],sizeof(D3DTLVERTEX));
		d3dVrts[1].sx = (float)x1;
		d3dVrts[1].sy = (float)y1;

#ifndef NO_RENDER
		D3DDrawPoly(2,&d3dVrts[0]); 
#endif
	}
}

/***************************************************************************
 * pie_DrawRect
 *
 * universal rectangle function for hardware
 *
 * Assumes render mode set up externally, draws filled rectangle
 *
 ***************************************************************************/
# define D3D_RECT_CORRECTION 0

void pie_DrawRect(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour, BOOL bClip)
{
	SDWORD swap;
	polyCount++;


	if (bClip)
	{
		if (x0>psRendSurface->clip.right || x1<psRendSurface->clip.left ||
			y0>psRendSurface->clip.bottom || y1<psRendSurface->clip.top)
		return;

		if (x0<psRendSurface->clip.left)
			x0 = psRendSurface->clip.left;
		if (x1>psRendSurface->clip.right)
			x1 = psRendSurface->clip.right;
		if (y0<psRendSurface->clip.top)
			y0 = psRendSurface->clip.top;
		if (y1>psRendSurface->clip.bottom)
			y1 = psRendSurface->clip.bottom;
	}
	if (pie_GetRenderEngine() == ENGINE_D3D)
	{
		if (x1 < x0)
		{
			swap = x0;
			x0 = x1;
			x1 = swap;		

		}
		if (y1 < y0)
		{
			swap = y0;
			y0 = y1;
			y1 = swap;

		}
		d3dVrts[0].sx = (float)x0;
		d3dVrts[0].sy = (float)y0;
		//cull triangles with off screen points
		d3dVrts[0].sz  = (float)INTERFACE_DEPTH * (float)INV_MAX_Z;
		d3dVrts[0].rhw = (float)1.0/d3dVrts[0].sz;

		d3dVrts[0].tu = (float)0.0;
		d3dVrts[0].tv = (float)0.0;
		d3dVrts[0].color = colour;
		d3dVrts[0].specular = 0;

		memcpy(&d3dVrts[1],&d3dVrts[0],sizeof(D3DTLVERTEX));
		memcpy(&d3dVrts[2],&d3dVrts[0],sizeof(D3DTLVERTEX));
		memcpy(&d3dVrts[3],&d3dVrts[0],sizeof(D3DTLVERTEX));
		memcpy(&d3dVrts[4],&d3dVrts[0],sizeof(D3DTLVERTEX));

		d3dVrts[1].sx = (float)x1 + D3D_RECT_CORRECTION;
		d3dVrts[1].sy = (float)y0;

		d3dVrts[2].sx = (float)x1 + D3D_RECT_CORRECTION;
		d3dVrts[2].sy = (float)y1 + D3D_RECT_CORRECTION;

		d3dVrts[3].sx = (float)x0;
		d3dVrts[3].sy = (float)y1 + D3D_RECT_CORRECTION;

#ifndef NO_RENDER
		D3DDrawPoly(3,&d3dVrts[2]); 
		D3DDrawPoly(3,&d3dVrts[0]); 
#endif
	}
}

/***************************************************************************
 * pie_PiePoly
 *
 * universal poly draw function for hardware
 *
 * Assumes render mode set up externally
 *
 ***************************************************************************/

static void pie_PiePoly(PIEPOLY *poly, BOOL bClip)
{
	SDWORD n;
static BOOL bBilinear;

	polyCount++;
	// handle texture animated polygons
	if (!(poly->flags & PIE_NO_CULL) && (poly->nVrts >= 3))
	{
		//cull if backfaced
		if(!pie_PieClockwise(poly->pVrts))
		{
			return;//culled
		}
	}

	if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		if (bClip)
		{
			n = pie_ClipTextured(poly->nVrts,poly->pVrts,&clippedVrts[0],FALSE);
			poly->nVrts = n;
			poly->pVrts = &clippedVrts[0];
		}
		if (poly->nVrts >= 3)
		{
			gl_PIEPolygon(poly->nVrts,poly->pVrts);
		}
	}
	else if (pie_GetRenderEngine() == ENGINE_D3D)
	{
		if (bClip)
		{
			n = pie_ClipTextured(poly->nVrts,poly->pVrts,&clippedVrts[0],TRUE);
			poly->nVrts = n;
			poly->pVrts = &clippedVrts[0];
		}
		if (poly->nVrts >= 3)
		{
			if (poly->flags & PIE_COLOURKEYED)
			{
				bBilinear = pie_GetBilinear();
				pie_SetBilinear(FALSE);
				D3D_PIEPolygon(poly->nVrts,poly->pVrts);
				pie_SetBilinear(bBilinear);
			}
			else
			{
				D3D_PIEPolygon(poly->nVrts,poly->pVrts);
			}
		}
	}
}

static void pie_PiePolyFrame(PIEPOLY *poly, int frame, BOOL bClip)
{
int	uFrame, vFrame, j, framesPerLine;

	// handle texture animated polygons
	if (!(poly->flags & PIE_NO_CULL) && (poly->nVrts >= 3))
	{
		//cull if backfaced
		if(!pie_PieClockwise(poly->pVrts))
		{
			return;//culled
		}
		poly->flags |= PIE_NO_CULL;//dont check culling again for this poly
	}

	if ((poly->flags & iV_IMD_TEXANIM) && (frame != 0))
	{
#ifndef PIEPSX   // was #ifdef WIN32
		if (poly->pTexAnim != NULL)
		{
			if (poly->pTexAnim->nFrames >=0)
			{
				frame %= poly->pTexAnim->nFrames;
			}
			else //frame is colour key
			{
				frame %= (-poly->pTexAnim->nFrames);
			}
			if (frame > 0)
			{
// HACK - fix this!!!!
				framesPerLine = 256 / poly->pTexAnim->textureWidth;
//should be		framesPerLine = iV_TEXTEX(texPage)->width / poly->pTexAnim->textureWidth;
				vFrame = 0;
				while (frame >= framesPerLine)
				{
					frame -= framesPerLine;
					vFrame += poly->pTexAnim->textureHeight;
				}
				uFrame = frame * poly->pTexAnim->textureWidth;
				
				for (j=0; j<poly->nVrts; j++) 
				{
					poly->pVrts[j].tu += uFrame;
					poly->pVrts[j].tv += vFrame;
				}
			}
		}
#endif
	}
#ifndef NO_RENDER
	//draw with new texture data
	pie_PiePoly(poly, bClip);
#endif
}

/***************************************************************************
 * pie_D3DPoly
 *
 * D3D specific poly draw function should change to use hardpoly
 *
 * Assumes render mode NOT set up externally
 *                     ---   
 ***************************************************************************/

void pie_D3DPoly(PIED3DPOLY *poly)
{
#ifdef NO_RENDER
	return;
#else
	polyCount++;
	D3DDrawPoly(poly->nVrts, &poly->pVrts[0]);
#endif
}

static void pie_D3DPolyFrame(PIED3DPOLY *poly, int frame)
{
	int	uFrame, vFrame, j, framesPerLine;
	// handle texture animated polygons

	if (poly->flags & iV_IMD_TEXANIM)
	{
		if (poly->pTexAnim != NULL)
		{
			if (poly->pTexAnim->nFrames >=0)
			{
				frame %= poly->pTexAnim->nFrames;
			}
			else //frame is colour key
			{
				frame %= (-poly->pTexAnim->nFrames);
			}


			if (frame > 0)
			{
				framesPerLine = 256 / poly->pTexAnim->textureWidth;//use TexPage WIDTH define

				vFrame = 0;
				while (frame >= framesPerLine)
				{
					frame -= framesPerLine;
					vFrame += poly->pTexAnim->textureHeight;
				}
				uFrame = frame * poly->pTexAnim->textureWidth;
				
				// no clip for d3d render??
				// shift the clipped textures for animation
				for (j=0; j<poly->nVrts; j++) 
				{
					poly->pVrts[j].tu += (float)uFrame/(float)256.0;
					poly->pVrts[j].tv += (float)vFrame/(float)256.0;
				}
			}
		}
	}

	pie_D3DPoly(poly);
}

//old ivis style draw poly
/***************************************************************************
 * pie_IvisPoly
 *
 * optimised poly draw function for software
 *
 * Assumes render mode NOT set up externally
 *                     ---
 ***************************************************************************/
static void pie_IvisPoly(SDWORD texPage, iIMDPoly *poly, BOOL bClip)
{
	int c;
	iVertex clip[iV_POLY_MAX_POINTS+4];

	polyCount++;

	// handle texture animated polygons
	if (!(poly->flags & PIE_NO_CULL) && (poly->npnts >= 3))
	{
		//cull if backfaced
		if(!pie_Clockwise(poly->vrt))
		{
			return;//culled
		}
	}

	if (bClip)
	{
		// call correct clipper (FLAT, GOUR, TEX)
		if (poly->flags & iV_IMD_TEX)
		{
			c = pie_PolyClipTex2D(poly->npnts,&poly->vrt[0],clip);
		}
		else
		{
			return;//no clip mode
		}
		poly->npnts = c;
		poly->vrt= &clip[0];
	}
/*	else if (poly->flags & iV_IMD_FLAT)
		c = iV_PolyClip2D(poly->npnts,&poly->vrt[0],clip);
*/
/*
  	if (c == 2)
		iV_pLine(clip[0].x,clip[0].y,clip[1].x,clip[1].y,poly->flags & 0xff);
*/

	if (poly->npnts >= 3)
	{
		if (poly->npnts == 3)
		{
			if (poly->flags & iV_IMD_TEX)
			{
				if (poly->flags & PIE_COLOURKEYED)
				{
						_tttriangle_4101(&poly->vrt[0],iV_TEXTEX(texPage));
				}
				else
				{
						/* this one! */
						_ttriangle_4101(&poly->vrt[0],iV_TEXTEX(texPage));
				}
			}
		}
		else
		{
			if (poly->flags & iV_IMD_TEX)
			{
				if (poly->flags & PIE_COLOURKEYED)
				{
						/* This one */
						_ttpolygon_4101(poly->npnts,&poly->vrt[0],iV_TEXTEX(texPage));
				}
				else
				{
						/* this one! */
						_tpolygon_4101(poly->npnts,&poly->vrt[0],iV_TEXTEX(texPage));
				}
			}
		}
	}
}

static void pie_IvisPolyFrame(SDWORD texPage, iIMDPoly *poly, int frame, BOOL bClip)
{
	int	uFrame, vFrame, j, framesPerLine;

	polyCount++;

	// handle texture animated polygons
	if (!(poly->flags & PIE_NO_CULL) && (poly->npnts >= 3))
	{
		//cull if backfaced
		if(!pie_Clockwise(poly->vrt))
		{
			return;//culled
		}
		poly->flags |= PIE_NO_CULL;//dont check culling again for this poly
	}

	if ((poly->flags & iV_IMD_TEXANIM) && (frame != 0))
	{
#ifndef PIEPSX   // was #ifdef WIN32
		if (poly->pTexAnim != NULL)
		{
			if (poly->pTexAnim->nFrames >=0)
			{
				frame %= poly->pTexAnim->nFrames;
			}
			else //frame is colour key
			{
				frame %= (-poly->pTexAnim->nFrames);
			}
			if (frame > 0)
			{
				framesPerLine = iV_TEXTEX(texPage)->width / poly->pTexAnim->textureWidth;
				vFrame = 0;
				while (frame >= framesPerLine)
				{
					frame -= framesPerLine;
					vFrame += poly->pTexAnim->textureHeight;
				}
				uFrame = frame * poly->pTexAnim->textureWidth;
				// shift the textures for animation
				if (poly->flags & iV_IMD_TEXANIM) 
				{
					for (j=0; j<poly->npnts; j++) 
					{
						poly->vrt[j].u += uFrame;
						poly->vrt[j].v += vFrame;
					}
				}
			}
		}
#endif
	}
#ifndef NO_RENDER
	pie_IvisPoly(texPage, poly, bClip);
#endif
}


/***************************************************************************
 *
 *
 *
 ***************************************************************************/

//ivis style draw function
void pie_DrawTriangle(iVertex *pv, iTexture* texPage, UDWORD renderFlags, iPoint *offset)
{
	UDWORD	n,f;
	iVertex clip[iV_POLY_MAX_POINTS];
   
   	if ( !pie_CLOCKWISE( pv[0].x, pv[0].y, pv[1].x, pv[1].y, pv[2].x, pv[2].y ) )
	{
		return;
	}
	// CULL test - only used on landscape?
	/*
	if(((v2->sy - v1->sy) * (v3->sx - v2->sx)) <= ((v2->sx - v1->sx) * (v3->sy - v2->sy)) )
	{
		bClockwise = TRUE;
	}
	else
	{
		return;
	}
	*/
	n = pie_PolyClipTex2D(3,pv,&clip[0]);
	if(n==0) return;
	if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		for(f=0;f<n;f++)
		{
			clip[f].u += offset->x;
			clip[f].v += offset->y;
		}
	}
/*	else
	{
		//temp d3d hack
		if (texPage == NULL)
		{
			texPage = iV_TEXTEX(4);
		}
	}
*/
	// Dealing with triangles?
	if (n==3) 
	{

	  	if ((rendSurface.usr == iV_MODE_4101) || (rendSurface.usr == REND_GLIDE_3DFX))
  		 //  	iV_tgPolygon(3,clip,texPage);
				   	   	iV_tgTriangle(clip,texPage);
//		else
//			p_Triangle(clip,texPage,0);
	} 
   	else if (n>3) // If not, then it's a n sided polygon with n > 3
   	{
	 	if ((rendSurface.usr == iV_MODE_4101) || (rendSurface.usr == REND_GLIDE_3DFX))
  			iV_tgPolygon(n,clip,texPage);
// 		else
//			iV_Polygon(n,clip,texPage,0);
   	}
	return;
}

void	pie_DrawFastTriangle(PIEVERTEX *v1, PIEVERTEX *v2, PIEVERTEX *v3, iTexture* texPage, int pieFlag, int pieFlagData )
{
	UDWORD	n;
	BOOL	bClockwise;
	UBYTE   alpha;

	if(((v2->sy - v1->sy) * (v3->sx - v2->sx)) <= ((v2->sx - v1->sx) * (v3->sy - v2->sy)) )
	{
		bClockwise = TRUE;
	}
	else
	{
		return;
	}
	
	tileCount++;

	n = pie_ClipTexturedTriangleFast(v1,v2,v3,&clippedVrts[0], FALSE);//no specular for glide

	if(n<3)
	{
		/* Offscreen, so do one */
		return;
	}

	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE)//Assume also translucent
	{
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ADDITIVE_TEX);
		alpha = (UBYTE)pieFlagData;
		pie_SetBilinear(TRUE);
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ALPHA_TEX);
		alpha = (UBYTE)pieFlagData;
		pie_SetBilinear(FALSE);//never bilinear with constant alpha, gives black edges 
	}
	else
	{
		pie_SetFogStatus(TRUE);
		pie_SetRendMode(REND_GOURAUD_TEX);
		alpha = MAX_UB_LIGHT;
		if (pieFlag & pie_NO_BILINEAR)
		{
			pie_SetBilinear(FALSE);
		}
		else
		{
			pie_SetBilinear(TRUE);
		}
	}

	if(n==3)
	{
	if (rendSurface.usr == REND_GLIDE_3DFX)
		{
  			gl_PIETriangle(clippedVrts);
		}
	}
	else 
	{
	 	if (rendSurface.usr == REND_GLIDE_3DFX)
		{
			gl_PIEPolygon(n,clippedVrts);
		}
   	}
}


void pie_DrawPoly(SDWORD numVrts, PIEVERTEX *aVrts, SDWORD texPage, void* psEffects)
{
	SDWORD	i;
	iIMDPoly	imdPoly;
	PIED3DPOLY	renderPoly;
	BOOL	bClockwise;
	UBYTE	alpha, *psAlpha;

	/*	Since this is only used from within source for the terrain draw - we can backface cull the
		polygons.


	*/
  	if(((aVrts[1].sy - aVrts[0].sy) * (aVrts[2].sx - aVrts[1].sx)) <= ((aVrts[1].sx - aVrts[0].sx) * (aVrts[2].sy - aVrts[1].sy)) )
	{
		bClockwise = TRUE;
	}
	else
	{
		return;
	}

	tileCount++;
	pie_SetTexturePage(texPage);
	pie_SetFogStatus(TRUE);
	if (psEffects == NULL)//jps 15apr99 translucent water code
	{
		pie_SetRendMode(REND_GOURAUD_TEX);//jps 15apr99 old solid water code
	}
	else//jps 15apr99 translucent water code
	{
		pie_SetRendMode(REND_ALPHA_TEX);//jps 15apr99 old solid water code
	}
	pie_SetBilinear(TRUE);

	if (rendSurface.usr == iV_MODE_4101)
	{
		imdPoly.flags = iV_IMD_TEX;
		imdPoly.npnts = 3;
		for(i = 0; i < 3; i++)
		{
			imdVrts[i].x = (SDWORD)aVrts[i].sx;
			imdVrts[i].y = (SDWORD)aVrts[i].sy;
			imdVrts[i].u = aVrts[i].tu;
			imdVrts[i].v = aVrts[i].tv;
			imdVrts[i].g  = (255 + aVrts[i].light.byte.r + aVrts[i].light.byte.g + aVrts[i].light.byte.b)>>2;
		}
		imdPoly.vrt = &imdVrts[0];
		pie_IvisPoly(texPage, &imdPoly, TRUE);	   // draw the polygon ... this is an inline function
	}
	else if (pie_GetRenderEngine() == ENGINE_GLIDE)//d3d and glide 
	{
		renderPoly.nVrts = pie_ClipTextured(3, &aVrts[0], &clippedVrts[0], FALSE);
		if (renderPoly.nVrts >= 3)
		{
			gl_PIEPolygon(renderPoly.nVrts, &clippedVrts[0]);
		}
		//lines not drawn
	}
	else//d3d 
	{
		renderPoly.nVrts = pie_ClipTextured(numVrts, &aVrts[0], &clippedVrts[0], TRUE);
		renderPoly.flags = 0x08;
		renderPoly.pVrts = &d3dVrts[0];
		for(i = 0; i < renderPoly.nVrts; i++)
		{
			d3dVrts[i].sx = (float)clippedVrts[i].sx;
			d3dVrts[i].sy = (float)clippedVrts[i].sy;
			//cull triangles with off screen points
			if (d3dVrts[i].sy > (float)LONG_TEST)
			{
				renderPoly.flags = 0;
			}
			d3dVrts[i].sz = (float)clippedVrts[i].sz * (float)INV_MAX_Z;
			d3dVrts[i].rhw = (float)1.0 / (float)clippedVrts[i].sz;
			d3dVrts[i].tu = (float)clippedVrts[i].tu * (float)INV_TEX_SIZE;
			d3dVrts[i].tv = (float)clippedVrts[i].tv * (float)INV_TEX_SIZE;
			if (psEffects == NULL)//jps 15apr99 translucent water code
			{
				d3dVrts[i].color = clippedVrts[i].light.argb;//jps 15apr99 old solid water code
				d3dVrts[i].specular = clippedVrts[i].specular.argb;//jps 15apr99 old solid water code
			}
			else//jps 15apr99 translucent water code
			{
				psAlpha = psEffects;
//				alpha = 255-clippedVrts[i].specular.byte.a;
//				alpha = pie_ByteScale(alpha, *psAlpha);//scale transparency by fog value
				alpha = *psAlpha;//dont scale transparency by fog value
				clippedVrts[i].light.byte.a = alpha;
				d3dVrts[i].color = clippedVrts[i].light.argb;
				d3dVrts[i].specular = clippedVrts[i].specular.argb;
			}

		}
		if (renderPoly.nVrts >= 3)
		{
			pie_D3DPoly(&renderPoly);	   // draw the polygon ... this is an inline function
		}
	}
}

//#ifdef NECROMANCER
void pie_DrawTile(PIEVERTEX *pv0, PIEVERTEX *pv1, PIEVERTEX *pv2, PIEVERTEX *pv3, SDWORD texPage)
{
	SDWORD i;
	DWORD	colour, specular;
	iIMDPoly imdPoly;
	PIED3DPOLY renderPoly;
	PIEVERTEX *pv;

	tileCount++;

	pie_SetRendMode(TRANS_DECAL);
	pie_SetTexturePage(texPage);
	pie_SetBilinear(TRUE);

	if ((pie_GetRenderEngine() == ENGINE_4101) || (pie_GetRenderEngine() == ENGINE_SR))
	{
		imdPoly.flags = iV_IMD_TEX;
		imdPoly.npnts = 3;
		for(i = 0; i < 4; i++)
		{
			switch(i)
			{
				case 0:
					pv = pv0;
					break;
				case 1:
					pv = pv1;
					break;
				case 2:
					pv = pv2;
					break;
				case 3:
				default:
					pv = pv3;
					break;
			}

			imdVrts[i].x = (SDWORD)pv->sx;
			imdVrts[i].y = (SDWORD)pv->sy;
			imdVrts[i].u = pv->tu;
			imdVrts[i].v = pv->tv;
			imdVrts[i].g  = (255 + pv->light.byte.r + pv->light.byte.g + pv->light.byte.b)>>2;
		}
	}
	else if (pie_GetRenderEngine() == ENGINE_GLIDE)//d3d and glide 
	{
		memcpy(&pieVrts[0],pv0,sizeof(PIEVERTEX));
		memcpy(&pieVrts[1],pv1,sizeof(PIEVERTEX));
		memcpy(&pieVrts[2],pv2,sizeof(PIEVERTEX));
		memcpy(&pieVrts[3],pv3,sizeof(PIEVERTEX));
		renderPoly.nVrts = pie_ClipTextured(4, &pieVrts[0], &clippedVrts[0], FALSE);
	}
	else//d3d 
	{
		memcpy(&pieVrts[0],pv0,sizeof(PIEVERTEX));
		memcpy(&pieVrts[1],pv1,sizeof(PIEVERTEX));
		memcpy(&pieVrts[2],pv2,sizeof(PIEVERTEX));
		memcpy(&pieVrts[3],pv3,sizeof(PIEVERTEX));
		renderPoly.nVrts = pie_ClipTextured(4, &pieVrts[0], &clippedVrts[0], TRUE);
		renderPoly.flags = 0x08;
		renderPoly.pVrts = &d3dVrts[0];
		for(i = 0; i < renderPoly.nVrts; i++)
		{
			colour = (clippedVrts[i].light.byte.a << 24) + (clippedVrts[i].light.byte.r << 16) + (clippedVrts[i].light.byte.g << 8) + clippedVrts[i].light.byte.b;
			specular = (clippedVrts[i].specular.byte.a << 24) + (clippedVrts[i].specular.byte.r << 16) + (clippedVrts[i].specular.byte.g << 8) + clippedVrts[i].specular.byte.b;

			d3dVrts[i].sx = (float)clippedVrts[i].sx;
			d3dVrts[i].sy = (float)clippedVrts[i].sy;
			//cull triangles with off screen points
			if (d3dVrts[i].sy > (float)LONG_TEST)
			{
				renderPoly.flags = 0;
			}
			d3dVrts[i].sz = (float)clippedVrts[i].sz * (float)INV_MAX_Z;
			d3dVrts[i].rhw = (float)1.0 / d3dVrts[i].sz;
			d3dVrts[i].tu = (float)clippedVrts[i].tu * (float)INV_TEX_SIZE;
			d3dVrts[i].tv = (float)clippedVrts[i].tv * (float)INV_TEX_SIZE;
			d3dVrts[i].color = colour;
			d3dVrts[i].specular = specular;
		}
	}


	if (rendSurface.usr == iV_MODE_4101)
	{
		imdPoly.vrt = &imdVrts[0];
		pie_IvisPoly(texPage, &imdPoly, TRUE);	   // draw the polygon ... this is an inline function

		memcpy(&imdVrts[4],&imdVrts[0],sizeof(iVertex));
		imdPoly.vrt = &imdVrts[2];
		pie_IvisPoly(texPage, &imdPoly, TRUE);	   // draw the polygon ... this is an inline function
	}
	else if (rendSurface.usr == REND_GLIDE_3DFX)//glide 
	{
		if (renderPoly.nVrts == 3)
		{
			gl_PIETriangle(&clippedVrts[0]);
		}
		else if (renderPoly.nVrts > 3)
		{
			gl_PIEPolygon(renderPoly.nVrts, &clippedVrts[0]);
		}
		//lines not drawn
	}
	else//d3d
	{
		pie_D3DPoly(&renderPoly);	   // draw the polygon ... this is an inline function
	}
}
//#endif

void pie_GetResetCounts(SDWORD* pPieCount, SDWORD* pTileCount, SDWORD* pPolyCount, SDWORD* pStateCount)
{
	*pPieCount  = pieCount; 
	*pTileCount = tileCount;
	*pPolyCount = polyCount;
	*pStateCount = pieStateCount;

	pieCount = 0;
	tileCount = 0;
	polyCount = 0;
	pieStateCount = 0;
	return;
}




#endif