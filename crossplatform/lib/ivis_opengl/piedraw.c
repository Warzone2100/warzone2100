/***************************************************************************/
/*
 * piedraw.c
 *
 * updated render routines for 3D coloured shaded transparency rendering
 *
 */
/***************************************************************************/

#include <string.h>
#ifdef _MSC_VER	
#include <windows.h>  //needed for gl.h!  --Qamly
#endif
#include <GL/gl.h>

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
//static void pie_D3DPolyFrame(PIED3DPOLY *poly, SDWORD frame);
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

void DrawTriangleList(BSPPOLYID PolygonNumber) {
	iIMDPoly *pPolys;
	UDWORD n;

	VERTEXID	*index;
	iIMDPoly	imdPoly;

//	DBPRINTF(("poly %d\n",PolygonNumber));

	while (PolygonNumber!=BSPPOLYID_TERMINATE) {
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
		if (imdPoly.flags > 0) {
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



/*
static void AddIMDPrimativesBSP2(iIMDShape *IMDdef,iIMDPoly *ScrVertices, UDWORD frame) {
	iVector pPos;

	SDWORD xDif,zDif;

// Camera ---- X is map colums +ve is going left, Z is map rows  +ve is going down the map, Y is map height +ve is up in the air

	ShapeTexPage=IMDdef->texpage;
	ShapeFrame=frame;

	xDif= (BSPCamera.x - BSPObject.x);
	zDif= (BSPCamera.z - BSPObject.z);

	pPos.x = (((COS(BSPObject_Yaw)*xDif)/4096)-((SIN(BSPObject_Yaw)*zDif)/4096));
	pPos.z = (-(((SIN(BSPObject_Yaw)*xDif)/4096)+((COS(BSPObject_Yaw)*zDif)/4096)));

	pPos.y = (BSPCamera.y - BSPObject.y);		// map height (up in the air)

	DrawBSPIMD(IMDdef, &pPos ,ScrVertices);		// in bspimd.c
}
*/
#endif

void
pie_Polygon(SDWORD numVerts, PIEVERTEX* pVrts, FRACT texture_offset) {
	SDWORD i;

	if (numVerts < 1) {
		return;
	} else if (numVerts == 1) {
		glBegin(GL_POINTS);
	} else if (numVerts == 2) {
		glBegin(GL_LINE_STRIP);
	} else {
		glBegin(GL_TRIANGLE_FAN);
	}
	for (i = 0; i < numVerts; i++) {
		glColor4ub(pVrts[i].light.byte.r, pVrts[i].light.byte.g, pVrts[i].light.byte.b, pVrts[i].light.byte.a);
		glTexCoord2f(pVrts[i].tu, pVrts[i].tv+texture_offset);
		//d3dVrts[i].specular = pVrts[i].specular.argb;
		glVertex3f(pVrts[i].sx, pVrts[i].sy, pVrts[i].sz * INV_MAX_Z);
	}
	glEnd();
}

/***************************************************************************
 * pie_Draw3dShape
 *
 * Project and render a pumpkin image to render surface
 * Will support zbuffering, texturing, coloured lighting and alpha effects
 * Avoids recalculating vertex projections for every poly 
 ***************************************************************************/

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
	VERTEXID	*index;
	PIELIGHT	colour, specular;

	pieCount++;

	// Fix for transparent buildings and features!! */
	if( (pieFlag & pie_TRANSLUCENT) AND (pieFlagData>220) )
	{
		pieFlag = pieFlagData = 0;	// force to bilinear and non-transparent
	}
	// Fix for transparent buildings and features!! */

	

// WARZONE light as byte passed in colour so expand
	if (col <= MAX_UB_LIGHT) {
		colour.byte.a = 255;//no fog
		colour.byte.r = (UBYTE)col;
		colour.byte.g = (UBYTE)col;
		colour.byte.b = (UBYTE)col;
	} else {
		colour.argb = col;
	}
	specular.argb = spec;

	if (frame == 0) 
	{
		frame = team;
	}

	/*
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
	*/

	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE) { //Assume also translucent
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ADDITIVE_TEX);
		colour.byte.a = (UBYTE)pieFlagData;
		pie_SetBilinear(TRUE);
	} else if (pieFlag & pie_TRANSLUCENT) {
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ALPHA_TEX);
		colour.byte.a = (UBYTE)pieFlagData;
		pie_SetBilinear(FALSE);//never bilinear with constant alpha, gives black edges 
	} else {
		if (pieFlag & pie_BUTTON)
		{
			pie_SetFogStatus(FALSE);
			pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
		} else {
			pie_SetFogStatus(TRUE);
		}
		pie_SetRendMode(REND_GOURAUD_TEX);
		//if hardware fog then alpha is set else unused in decal mode
		//colour.byte.a = MAX_UB_LIGHT;
		if (pieFlag & pie_NO_BILINEAR) {
			pie_SetBilinear(FALSE);
		} else {
			pie_SetBilinear(TRUE);
		}
	}

	if (pieFlag & pie_RAISE) {
		pieFlagData = (shape->ymax * (pie_RAISE_SCALE - pieFlagData))/pie_RAISE_SCALE;
	}

	pie_SetTexturePage(shape->texpage);

	//now draw the shape	
	//rotate and project points from shape->points to scrPoints
	pVertices = shape->points;
	pPixels = &scrPoints[0];

	//--
	for (i=0; i<shape->npoints; i++, pVertices++, pPixels++) {
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
		} else {
			pPixels->d3dx = D3DVAL((psRendSurface->xcentre + (rx / tzx)));
			pPixels->d3dy = D3DVAL((psRendSurface->ycentre - (ry / tzy)));
		}
	}

	pPolys = shape->polys;
	for (i=0; i<shape->npolys; i++, pPolys++) {
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
			pieVrts[n].sz  = MAKEINT(scrPoints[*index].d3dz);
			pieVrts[n].tu = pPolys->vrt[n].u;
			pieVrts[n].tv = pPolys->vrt[n].v;
			pieVrts[n].light.argb = colour.argb;
			pieVrts[n].specular.argb = specular.argb;
		}
		piePoly.nVrts = pPolys->npnts;
		piePoly.pVrts = &pieVrts[0];

		piePoly.pTexAnim = pPolys->pTexAnim;

		if (piePoly.flags > 0) {
			pie_PiePolyFrame(&piePoly,frame,TRUE);	   // draw the polygon ... this is an inline function
		}
	}
	if (pieFlag & pie_BUTTON) {
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	}
}

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

	glBegin(GL_TRIANGLE_STRIP);
	glColor4ub(style->colour.byte.r, style->colour.byte.g, style->colour.byte.b, style->colour.byte.a);
	//set up 4 pie verts
	glTexCoord2f(image->tu, image->tv);
	glVertex2f(dest->x, dest->y);
	//pieVrts[0].sz  = (SDWORD)INTERFACE_DEPTH;
	//pieVrts[0].specular.argb = style->specular.argb;
	glTexCoord2f(image->tu + image->tw + EDGE_CORRECTION, image->tv);
	glVertex2f(dest->x + dest->w + EDGE_CORRECTION, dest->y);
	//pieVrts[1].sz  = (SDWORD)INTERFACE_DEPTH;
	//pieVrts[1].specular.argb = style->specular.argb;
	glTexCoord2f(image->tu, image->tv + image->th + EDGE_CORRECTION);
	glVertex2f(dest->x, dest->y + dest->h + EDGE_CORRECTION);
	//pieVrts[3].sz  = (SDWORD)INTERFACE_DEPTH;
	//pieVrts[3].specular.argb = style->specular.argb;
	glTexCoord2f(image->tu + image->tw + EDGE_CORRECTION, image->tv + image->th + EDGE_CORRECTION);
	glVertex2f(dest->x + dest->w + EDGE_CORRECTION, dest->y + dest->h + EDGE_CORRECTION);
	//pieVrts[2].sz  = (SDWORD)INTERFACE_DEPTH;
	//pieVrts[2].specular.argb = style->specular.argb;
	glEnd();
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

	pie_Polygon(4, pieVrts, 0.0);
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
	polyCount++;

	pie_SetTexturePage(-1);
	pie_SetColourKeyedBlack(FALSE);
	pie_SetColour(colour);

	glBegin(GL_LINE_STRIP);
	glVertex3f(x0, y0, INTERFACE_DEPTH);
	glVertex3f(x1, y1, INTERFACE_DEPTH);
	glEnd();
}

/***************************************************************************
 * pie_DrawRect
 *
 * universal rectangle function for hardware
 *
 * Assumes render mode set up externally, draws filled rectangle
 *
 ***************************************************************************/

void pie_DrawRect(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour, BOOL bClip)
{
//	SDWORD swap;
	PIELIGHT c;
	polyCount++;

	c.argb = colour;
	pie_SetColourKeyedBlack(FALSE);

	glColor4ub(c.byte.r, c.byte.g, c.byte.b, c.byte.a);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex2i(x0, y0);
	glVertex2i(x0, y1);
	glVertex2i(x1, y0);
	glVertex2i(x1, y1);
	glEnd();
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

	if (poly->nVrts >= 3) {
		if (poly->flags & PIE_COLOURKEYED) {
			pie_SetColourKeyedBlack(TRUE);
		} else {
			pie_SetColourKeyedBlack(FALSE);
		}
		pie_Polygon(poly->nVrts, poly->pVrts, 0.0);
	}
}

static void pie_PiePolyFrame(PIEPOLY *poly, int frame, BOOL bClip)
{
int	uFrame, vFrame, j, framesPerLine;

	if (poly->flags & PIE_COLOURKEYED) {
		pie_SetColourKeyedBlack(TRUE);
	} else {
		pie_SetColourKeyedBlack(FALSE);
	}

	if ((poly->flags & iV_IMD_TEXANIM) && (frame != 0)) {
		if (poly->pTexAnim != NULL) {
			if (poly->pTexAnim->nFrames >=0) {
				frame %= poly->pTexAnim->nFrames;
			} else {
				frame %= (-poly->pTexAnim->nFrames);
			}
			if (frame > 0) {
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

/*
void pie_D3DPoly(PIED3DPOLY *poly)
{
#ifdef NO_RENDER
	return;
#else
	polyCount++;
	D3DDrawPoly(poly->nVrts, &poly->pVrts[0]);
#endif
}

static void pie_D3DPolyFrame(PIED3DPOLY *poly, int frame) {
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
				for (j=0; j<poly->nVrts; j++) {
					poly->pVrts[j].tu += (float)uFrame/(float)256.0;
					poly->pVrts[j].tv += (float)vFrame/(float)256.0;
				}
			}
		}
	}

	pie_D3DPoly(poly);
}
*/

//old ivis style draw poly
/***************************************************************************
 * pie_IvisPoly
 *
 * optimised poly draw function for software
 *
 * Assumes render mode NOT set up externally
 *                     ---
 ***************************************************************************/
static void pie_IvisPoly(SDWORD texPage, iIMDPoly *poly, BOOL bClip) {
	polyCount++;

	if (poly->npnts >= 3) {
		SDWORD i;

		if (poly->flags & PIE_COLOURKEYED) {
			pie_SetColourKeyedBlack(TRUE);
		} else {
			pie_SetColourKeyedBlack(FALSE);
		}
		glBegin(GL_TRIANGLE_FAN);
		for (i = 0; i < poly->npnts; ++i) {
			glColor3ub(poly->vrt[i].g*16, poly->vrt[i].g*16, poly->vrt[i].g*16);
			glTexCoord2f(poly->vrt[i].u, poly->vrt[i].v);
			glVertex3f(poly->vrt[i].x, poly->vrt[i].y, poly->vrt[i].z);
		}
		glEnd();
	}
}

static void pie_IvisPolyFrame(SDWORD texPage, iIMDPoly *poly, int frame, BOOL bClip)
{
	int	uFrame, vFrame, j, framesPerLine;

	polyCount++;

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
	UDWORD	i;

   	if ( !pie_CLOCKWISE( pv[0].x, pv[0].y, pv[1].x, pv[1].y, pv[2].x, pv[2].y ) ) {
		return;
	}

	glBegin(GL_TRIANGLE_FAN);
	for (i = 0; i < 3; i++) {
		glColor4ub(pv[i].g*16, pv[i].g*16, pv[i].g*16, 255);
		glTexCoord2f(pv[i].u, pv[i].v);
		glVertex3f(pv[i].x, pv[i].y, pv[i].z);
	}
	glEnd();
}

void	pie_DrawFastTriangle(PIEVERTEX *v1, PIEVERTEX *v2, PIEVERTEX *v3, iTexture* texPage, int pieFlag, int pieFlagData )
{
}


void pie_DrawPoly(SDWORD numVrts, PIEVERTEX *aVrts, SDWORD texPage, void* psEffects) {
	BOOL		bClockwise;
	FRACT		offset = 0;

	/*	Since this is only used from within source for the terrain draw - we can backface cull the
		polygons.


	*/
  	if(((aVrts[1].sy - aVrts[0].sy) * (aVrts[2].sx - aVrts[1].sx)) <= ((aVrts[1].sx - aVrts[0].sx) * (aVrts[2].sy - aVrts[1].sy)) ) {
		bClockwise = TRUE;
	} else {
		return;
	}

	tileCount++;
	pie_SetTexturePage(texPage);
	pie_SetFogStatus(TRUE);
	if (psEffects == NULL)//jps 15apr99 translucent water code
	{
		pie_SetRendMode(REND_GOURAUD_TEX);//jps 15apr99 old solid water code
		pie_SetColourKeyedBlack(TRUE);
	}
	else//jps 15apr99 translucent water code
	{
		pie_SetRendMode(REND_ALPHA_TEX);//jps 15apr99 old solid water code
		pie_SetColourKeyedBlack(FALSE);
		offset = *((float*)psEffects);
	}
	pie_SetBilinear(TRUE);

	if (numVrts >= 3) {
		pie_Polygon(numVrts, aVrts, offset);
	}
}

//#ifdef NECROMANCER
void pie_DrawTile(PIEVERTEX *pv0, PIEVERTEX *pv1, PIEVERTEX *pv2, PIEVERTEX *pv3, SDWORD texPage)
{
	tileCount++;

	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetTexturePage(texPage);
	pie_SetBilinear(TRUE);

	memcpy(&pieVrts[0],pv0,sizeof(PIEVERTEX));
	memcpy(&pieVrts[1],pv1,sizeof(PIEVERTEX));
	memcpy(&pieVrts[2],pv2,sizeof(PIEVERTEX));
	memcpy(&pieVrts[3],pv3,sizeof(PIEVERTEX));

	pie_Polygon(4, pieVrts, 0.0);
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

