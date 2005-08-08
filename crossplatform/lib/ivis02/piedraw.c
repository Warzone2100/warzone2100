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
#include "piefunc.h"
#include "piematrix.h"
#include "tex.h"

#include "piedef.h"
#include "piestate.h"
#include "pietexture.h"
#include "pieclip.h"
#include "bspfunc.h"

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
// DEV STUDIO 5, 3D NOW and INTEL VERSION.
#if (_MSC_VER != 1000) && (_MSC_VER != 1020)
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

		if (pie_GetRenderEngine() == ENGINE_D3D)
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

		if (pie_GetRenderEngine() == ENGINE_D3D)
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

				imdPoly.pTexAnim = pPolys->pTexAnim;

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

				piePoly.pTexAnim = pPolys->pTexAnim;

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

		if (pie_GetRenderEngine() == ENGINE_D3D)
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

		if (pie_GetRenderEngine() == ENGINE_D3D)
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

				imdPoly.pTexAnim = pPolys->pTexAnim;

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

				piePoly.pTexAnim = pPolys->pTexAnim;

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
#endif
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
	UDWORD	n;
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




//#endif  //line29
