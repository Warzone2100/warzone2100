/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/***************************************************************************/
/*
 * piedraw.c
 *
 * updated render routines for 3D coloured shaded transparency rendering
 *
 */
/***************************************************************************/

#include <string.h>
#include <SDL/SDL_opengl.h>
#include <SDL/SDL_video.h>

#include "lib/framework/frame.h"
#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/imd.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piefunc.h"
#include "piematrix.h"
#include "lib/ivis_common/tex.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piestate.h"
#include "pietexture.h"
#include "lib/ivis_common/pieclip.h"

#define MIST

extern BOOL drawing_interface;

/***************************************************************************/
/*
 *	OpenGL extensions for shadows
 */
/***************************************************************************/

static BOOL check_extension(const char* extension_name)
{
	const char *extension_list = (const char *)glGetString(GL_EXTENSIONS);
	unsigned int extension_name_length = strlen(extension_name);
	const char *tmp = extension_list;
	unsigned int first_extension_length;

	if (!extension_name || !extension_list) return FALSE;

	while (tmp[0]) {
		first_extension_length = strcspn(tmp, " ");

		if (   extension_name_length == first_extension_length
		    && strncmp(extension_name, tmp, first_extension_length) == 0) {
			debug( LOG_3D, "%s is supported.\n", extension_name );
			return TRUE;
		}
		tmp += first_extension_length + 1;
	}

	debug( LOG_3D, "%s is not supported.\n", extension_name );

	return FALSE;
}

// EXT_stencil_two_side
#ifndef GL_EXT_stencil_two_side
#define GL_STENCIL_TEST_TWO_SIDE_EXT      0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT        0x8911
#endif

#ifndef GL_EXT_stencil_two_side
#define GL_EXT_stencil_two_side 1
typedef void (APIENTRY * PFNGLACTIVESTENCILFACEEXTPROC) (GLenum face);
#endif

PFNGLACTIVESTENCILFACEEXTPROC glActiveStencilFaceEXT;

/// Check if we can use one-pass stencil in the shadow draw code
static BOOL stencil_one_pass(void)
{
	// tribool, -1: uninitialized, 0: FALSE, 1: TRUE
	static int can_do_stencil_one_pass = -1;

	if (can_do_stencil_one_pass < 0) {
		can_do_stencil_one_pass = 0; // can't use it until we decide otherwise

		// let's check if we have the needed extensions
		if( check_extension("GL_EXT_stencil_two_side")
		 && check_extension("GL_EXT_stencil_wrap"))
		{
			// retrieve the function pointer
			glActiveStencilFaceEXT = (PFNGLACTIVESTENCILFACEEXTPROC) SDL_GL_GetProcAddress("glActiveStencilFaceEXT");
			if(glActiveStencilFaceEXT)
			{
				// all went well
				can_do_stencil_one_pass = 1;
			}
		}
	}

	return (1 == can_do_stencil_one_pass); // to get the types right
}

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
static SDWORD		pieCount = 0;
static SDWORD		tileCount = 0;
static SDWORD		polyCount = 0;

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/

//pievertex draw poly (low level) //all modes from PIEVERTEX data
static void pie_PiePoly(PIEPOLY *poly, BOOL bClip);
static void pie_PiePolyFrame(PIEPOLY *poly, SDWORD frame, BOOL bClip);

void DrawTriangleList(BSPPOLYID PolygonNumber);

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

static BOOL lighting = FALSE;
static BOOL shadows = FALSE;

void pie_BeginLighting(float x, float y, float z)
{
	float pos[4];
	float zero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float ambient[4] = {0.2f, 0.2f, 0.2f, 0.0f};
	float diffuse[4] = {0.5f, 0.5f, 0.5f, 0.0f};
	float specular[4] = {1.0f, 1.0f, 1.0f, 0.0f};

	pos[0] = x;
	pos[1] = y;
	pos[2] = z;
	pos[3] = 0;

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glEnable(GL_LIGHT0);

	//lighting = TRUE;
	shadows = TRUE;
}

void pie_EndLighting(void)
{
	shadows = FALSE;
	lighting = FALSE;
}

#ifdef BSPIMD

// BSP object position
static Vector3i BSPObject;
static Vector3i BSPCamera;
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

#endif

static void Vector3f_Set(Vector3f* v, float x, float y, float z)
{
	v->x = x;
	v->y = y;
	v->z = z;
}

static void Vector3f_Sub(Vector3f* dest, Vector3f* op1, Vector3f* op2)
{
	dest->x = op1->x - op2->x;
	dest->y = op1->y - op2->y;
	dest->z = op1->z - op2->z;
}

static float Vector3f_SP(Vector3f* op1, Vector3f* op2)
{
	return op1->x * op2->x + op1->y * op2->y + op1->z * op2->z;
}

static void Vector3f_CP(Vector3f* dest, Vector3f* op1, Vector3f* op2)
{
	dest->x = op1->y * op2->z - op1->z * op2->y;
	dest->y = op1->z * op2->x - op1->x * op2->z;
	dest->z = op1->x * op2->y - op1->y * op2->x;
}

static void
pie_Polygon(SDWORD numVerts, PIEVERTEX* pVrts, FRACT texture_offset, BOOL light)
{
	SDWORD i;

	if (numVerts < 1) {
		return;
	} else if (numVerts == 1) {
		glBegin(GL_POINTS);
	} else if (numVerts == 2) {
		glBegin(GL_LINE_STRIP);
	} else {
		if (light) {
			float ambient[4] = { 1, 1, 1, 1 };
			float diffuse[4] = { 1, 1, 1, 1 };
			float specular[4] = { 1, 1, 1, 1 };
			float shininess = 10;

			glEnable(GL_LIGHTING);
			glEnable(GL_NORMALIZE);

			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
		}
		glBegin(GL_TRIANGLE_FAN);
		if (light) {
			Vector3f p1, p2, p3, v1, v2, n;
			float l;

			Vector3f_Set(&p1, pVrts[0].sx, pVrts[0].sy, pVrts[0].sz);
			Vector3f_Set(&p2, pVrts[1].sx, pVrts[1].sy, pVrts[1].sz);
			Vector3f_Set(&p3, pVrts[2].sx, pVrts[2].sy, pVrts[2].sz);
			Vector3f_Sub(&v1, &p3, &p1);
			Vector3f_Sub(&v2, &p2, &p1);
			Vector3f_CP(&n, &v1, &v2);
			l = 1.0;

			glNormal3f(n.x*l, n.y*l, n.z*l);
			//printf("%f %f %f\n", n.x*l, n.y*l, n.z*l);
		}
	}
	for (i = 0; i < numVerts; i++) {
		glColor4ub(pVrts[i].light.byte.r, pVrts[i].light.byte.g, pVrts[i].light.byte.b, pVrts[i].light.byte.a);
		glTexCoord2f(pVrts[i].tu, pVrts[i].tv+texture_offset);
		//d3dVrts[i].specular = pVrts[i].specular.argb;
		glVertex3f(pVrts[i].sx, pVrts[i].sy, pVrts[i].sz);
	}
	glEnd();
	glDisable(GL_LIGHTING);
}

/***************************************************************************
 * pie_Draw3dShape
 *
 * Project and render a pumpkin image to render surface
 * Will support zbuffering, texturing, coloured lighting and alpha effects
 * Avoids recalculating vertex projections for every poly
 ***************************************************************************/

typedef struct {
	float		matrix[16];
	iIMDShape*	shape;
	int		flag;
	int		flag_data;
} shadowcasting_shape_t;

typedef struct {
	float		matrix[16];
	iIMDShape*	shape;
	int		frame;
	PIELIGHT	colour;
	PIELIGHT	specular;
	int		flag;
	int		flag_data;
} transluscent_shape_t;

static shadowcasting_shape_t* scshapes = NULL;
static unsigned int scshapes_size = 0;
static unsigned int nb_scshapes = 0;
static transluscent_shape_t* tshapes = NULL;
static unsigned int tshapes_size = 0;
static unsigned int nb_tshapes = 0;

static void pie_Draw3DShape2(iIMDShape *shape, int frame, PIELIGHT colour, PIELIGHT specular,
		      int pieFlag, int pieFlagData)
{
	Sint32		tempY;
	int i, n;
	Vector3i		*pVertices;
	PIEPIXEL	*pPixels;
	iIMDPoly	*pPolys;
	PIEPOLY		piePoly;
	VERTEXID	*index;
	BOOL		light = lighting;

	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE) { //Assume also translucent
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ADDITIVE_TEX);
		colour.byte.a = (UBYTE)pieFlagData;
		pie_SetBilinear(TRUE);
		light = FALSE;
	} else if (pieFlag & pie_TRANSLUCENT) {
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ALPHA_TEX);
		colour.byte.a = (UBYTE)pieFlagData;
		pie_SetBilinear(FALSE);//never bilinear with constant alpha, gives black edges
		light = FALSE;
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
		pPixels->d3dx = pVertices->x;
		pPixels->d3dy = tempY;
		pPixels->d3dz = pVertices->z;
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
			pieVrts[n].sz = MAKEINT(scrPoints[*index].d3dz);
			pieVrts[n].tu = pPolys->vrt[n].u;
			pieVrts[n].tv = pPolys->vrt[n].v;
			pieVrts[n].light.argb = colour.argb;
			pieVrts[n].specular.argb = specular.argb;
		}
		piePoly.nVrts = pPolys->npnts;
		piePoly.pVrts = &pieVrts[0];

		piePoly.pTexAnim = pPolys->pTexAnim;

		if (piePoly.flags > 0) {
			pie_PiePolyFrame(&piePoly,frame,light);	   // draw the polygon ... this is an inline function
		}
	}
	if (pieFlag & pie_BUTTON) {
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	}
}

static void pie_DrawShadow(iIMDShape *shape, int flag, int flag_data, Vector3f* light)
{
	Sint32		tempY;
	int i, n;
	Vector3i		*pVertices;
	PIEPIXEL	*pPixels;
	iIMDPoly	*pPolys;
	VERTEXID	*index;

	pVertices = shape->points;
	pPixels = scrPoints;
	for (i = 0; i < shape->npoints; i++, pVertices++, pPixels++) {
		tempY = pVertices->y;
		if (flag & pie_RAISE) {
			tempY = pVertices->y - flag_data;
			if (tempY < 0) tempY = 0;

		} else if (flag & pie_HEIGHT_SCALED) {
			if(pVertices->y>0) {
				tempY = (pVertices->y * flag_data)/pie_RAISE_SCALE;
			}
		}
		pPixels->d3dx = pVertices->x;
		pPixels->d3dy = tempY;
		pPixels->d3dz = pVertices->z;
	}

	pPolys = shape->polys;
	for (i = 0; i < shape->npolys; ++i, ++pPolys) {
		Vector3f p1, p2, p3, v1, v2, normal;

		index = pPolys->pindex;
		Vector3f_Set(&p1, scrPoints[*index].d3dx, scrPoints[*index].d3dy, scrPoints[*index].d3dz);
		++index;
		Vector3f_Set(&p2, scrPoints[*index].d3dx, scrPoints[*index].d3dy, scrPoints[*index].d3dz);
		++index;
		Vector3f_Set(&p3, scrPoints[*index].d3dx, scrPoints[*index].d3dy, scrPoints[*index].d3dz);
		Vector3f_Sub(&v1, &p3, &p1);
		Vector3f_Sub(&v2, &p2, &p1);
		Vector3f_CP(&normal, &v1, &v2);
		if (Vector3f_SP(&normal, light) > 0) {
			if (   pPolys->flags & PIE_COLOURKEYED
			    && pPolys->flags & PIE_NO_CULL) {
				VERTEXID i;

				glBegin(GL_TRIANGLE_FAN);
				index = pPolys->pindex;
				glVertex3f(scrPoints[*index].d3dx, scrPoints[*index].d3dy, scrPoints[*index].d3dz);
				for (n = pPolys->npnts-1; n > 0; --n) {
					i = pPolys->pindex[n];
					glVertex3f(scrPoints[i].d3dx, scrPoints[i].d3dy, scrPoints[i].d3dz);
				}
				glEnd();
			}
			index = pPolys->pindex;
			glBegin(GL_TRIANGLE_STRIP);
			for (n = 0; n < pPolys->npnts; ++n, ++index) {
				glVertex3f(scrPoints[*index].d3dx+light->x, scrPoints[*index].d3dy+light->y, scrPoints[*index].d3dz+light->z);
				glVertex3f(scrPoints[*index].d3dx, scrPoints[*index].d3dy, scrPoints[*index].d3dz);
			}
			index = pPolys->pindex;
			glVertex3f(scrPoints[*index].d3dx+light->x, scrPoints[*index].d3dy+light->y, scrPoints[*index].d3dz+light->z);
			glVertex3f(scrPoints[*index].d3dx, scrPoints[*index].d3dy, scrPoints[*index].d3dz);
		} else {
			if (   pPolys->flags & PIE_COLOURKEYED
			    && pPolys->flags & PIE_NO_CULL) {
				glBegin(GL_TRIANGLE_FAN);
				index = pPolys->pindex;
				for (n = 0; n < pPolys->npnts; ++n, ++index) {
					glVertex3f(scrPoints[*index].d3dx, scrPoints[*index].d3dy, scrPoints[*index].d3dz);
				}
				glEnd();
			}
			index = pPolys->pindex;
			glBegin(GL_TRIANGLE_STRIP);
			for (n = 0; n < pPolys->npnts; ++n, ++index) {
				glVertex3f(scrPoints[*index].d3dx, scrPoints[*index].d3dy, scrPoints[*index].d3dz);
				glVertex3f(scrPoints[*index].d3dx+light->x, scrPoints[*index].d3dy+light->y, scrPoints[*index].d3dz+light->z);
			}
			index = pPolys->pindex;
			glVertex3f(scrPoints[*index].d3dx, scrPoints[*index].d3dy, scrPoints[*index].d3dz);
			glVertex3f(scrPoints[*index].d3dx+light->x, scrPoints[*index].d3dy+light->y, scrPoints[*index].d3dz+light->z);
		}
		glEnd();
	}
}

void pie_CleanUp( void )
{
	free( tshapes );
	free( scshapes );
}

void pie_Draw3DShape(iIMDShape *shape, int frame, int team, UDWORD col, UDWORD spec, int pieFlag, int pieFlagData)
{
	PIELIGHT colour, specular;
	float distance;

	pieCount++;

	// Fix for transparent buildings and features!! */
	if( (pieFlag & pie_TRANSLUCENT) && (pieFlagData>220) )
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

	if (drawing_interface || !shadows) {
		pie_Draw3DShape2(shape, frame, colour, specular, pieFlag, pieFlagData);
	} else {
		if (pieFlag & (pie_ADDITIVE | pie_TRANSLUCENT)) {
			if (tshapes_size <= nb_tshapes) {
				if (tshapes_size == 0) {
					tshapes_size = 64;
					tshapes = (transluscent_shape_t*)malloc(tshapes_size*sizeof(transluscent_shape_t));
					memset( tshapes, 0, tshapes_size*sizeof(transluscent_shape_t) );
				} else {
					unsigned int old_size = tshapes_size;
					tshapes_size <<= 1;
					tshapes = (transluscent_shape_t*)realloc(tshapes, tshapes_size*sizeof(transluscent_shape_t));
					memset( &tshapes[old_size], 0, (tshapes_size-old_size)*sizeof(transluscent_shape_t) );
				}
			}
			glGetFloatv(GL_MODELVIEW_MATRIX, tshapes[nb_tshapes].matrix);
			tshapes[nb_tshapes].shape = shape;
			tshapes[nb_tshapes].frame = frame;
			tshapes[nb_tshapes].colour = colour;
			tshapes[nb_tshapes].specular = specular;
			tshapes[nb_tshapes].flag = pieFlag;
			tshapes[nb_tshapes].flag_data = pieFlagData;
			nb_tshapes++;
		} else {
			if (scshapes_size <= nb_scshapes) {
				if (scshapes_size == 0) {
					scshapes_size = 64;
					scshapes = (shadowcasting_shape_t*)malloc(scshapes_size*sizeof(shadowcasting_shape_t));
					memset( scshapes, 0, scshapes_size*sizeof(shadowcasting_shape_t) );
				} else {
					unsigned int old_size = scshapes_size;
					scshapes_size <<= 1;
					scshapes = (shadowcasting_shape_t*)realloc(scshapes, scshapes_size*sizeof(shadowcasting_shape_t));
					memset( &scshapes[old_size], 0, (scshapes_size-old_size)*sizeof(shadowcasting_shape_t) );
				}
			}
			glGetFloatv(GL_MODELVIEW_MATRIX, scshapes[nb_scshapes].matrix);
			distance = scshapes[nb_scshapes].matrix[12]*scshapes[nb_scshapes].matrix[12];
			distance += scshapes[nb_scshapes].matrix[13]*scshapes[nb_scshapes].matrix[13];
			distance += scshapes[nb_scshapes].matrix[14]*scshapes[nb_scshapes].matrix[14];
			// if object is too far in the fog don't generate a shadow.
			if (distance < 6000*6000) {
				scshapes[nb_scshapes].shape = shape;
				scshapes[nb_scshapes].flag = pieFlag;
				scshapes[nb_scshapes].flag_data = pieFlagData;
				nb_scshapes++;
			}
			pie_Draw3DShape2(shape, frame, colour, specular, pieFlag, pieFlagData);
		}
	}
}

static void inverse_matrix(float* src, float * dst)
{
	float det = src[0]*src[5]*src[10]+src[4]*src[9]*src[2]+src[8]*src[1]*src[6] -src[2]*src[5]*src[8]-src[6]*src[9]*src[0]-src[10]*src[1]*src[4];
	float invdet = 1.0f/det;

	dst[0] = invdet*(src[5]*src[10]-src[9]*src[6]);
	dst[1] = invdet*(src[9]*src[2]-src[1]*src[10]);
	dst[2] = invdet*(src[1]*src[6]-src[5]*src[2]);
	dst[3] = invdet*(src[8]*src[6]-src[4]*src[10]);
	dst[4] = invdet*(src[0]*src[10]-src[8]*src[2]);
	dst[5] = invdet*(src[4]*src[2]-src[0]*src[6]);
	dst[6] = invdet*(src[4]*src[9]-src[8]*src[5]);
	dst[7] = invdet*(src[8]*src[1]-src[0]*src[9]);
	dst[8] = invdet*(src[0]*src[5]-src[4]*src[1]);
}

static void pie_ShadowDrawLoop(float pos_lgt0[4])
{
	float invmat[9];
	Vector3f light;
	unsigned int i;

	for (i = 0; i < nb_scshapes; i++)
	{
		glLoadIdentity();
		glMultMatrixf( scshapes[i].matrix );
		inverse_matrix( scshapes[i].matrix, invmat );
		light.x = invmat[0] * pos_lgt0[0] + invmat[3] * pos_lgt0[1] + invmat[6] * pos_lgt0[2];
		light.y = invmat[1] * pos_lgt0[0] + invmat[4] * pos_lgt0[1] + invmat[7] * pos_lgt0[2];
		light.z = invmat[2] * pos_lgt0[0] + invmat[5] * pos_lgt0[1] + invmat[8] * pos_lgt0[2];
		pie_DrawShadow(scshapes[i].shape, scshapes[i].flag, scshapes[i].flag_data, &light);
	}
}

static void pie_DrawShadows(void)
{
	static BOOL dlist_defined = FALSE;
	static GLuint dlist;
	float pos_lgt0[4];
	float width = pie_GetVideoBufferWidth();
	float height = pie_GetVideoBufferHeight();

	glGetLightfv(GL_LIGHT0, GL_POSITION, pos_lgt0);

	pie_SetTexturePage(-1);

	glPushMatrix();

	pie_SetColourKeyedBlack(FALSE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_FALSE);
	glEnable(GL_STENCIL_TEST);
	glClear(GL_STENCIL_BUFFER_BIT);

	if (stencil_one_pass()) {
		glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		glDisable(GL_CULL_FACE);
		glStencilMask(~0);
		glActiveStencilFaceEXT(GL_BACK);
		glStencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP_EXT);
		glStencilFunc(GL_ALWAYS, 0, ~0);
		glActiveStencilFaceEXT(GL_FRONT);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP_EXT);
		glStencilFunc(GL_ALWAYS, 0, ~0);

		pie_ShadowDrawLoop(pos_lgt0);
		glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);

	} else {
		if (!dlist_defined) {
			dlist = glGenLists(1);
			dlist_defined = TRUE;
		}
		// Setup stencil for back faces.
		glStencilMask(~0);
		glStencilFunc(GL_ALWAYS, 0, ~0);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

		pie_ShadowDrawLoop(pos_lgt0);

		// Setup stencil for front faces.
		glCullFace(GL_FRONT);
		glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

		// Draw shadows again
		pie_ShadowDrawLoop(pos_lgt0);
	}

	glEnable(GL_CULL_FACE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilMask(~0);
	glStencilFunc(GL_LESS, 0, ~0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0, 0, 0, 0.5);

	pie_PerspectiveEnd();
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex2f(0, 0);
	glVertex2f(width, 0);
	glVertex2f(0, height);
	glVertex2f(width, height);
	glEnd();
	pie_PerspectiveBegin();

	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	glPopMatrix();

	nb_scshapes = 0;
}

static void pie_DrawRemainingTransShapes(void)
{
	unsigned int i;

	for (i = 0; i < nb_tshapes; ++i)
	{
		glPushMatrix();
		glLoadIdentity();
		glMultMatrixf(tshapes[i].matrix);
		pie_Draw3DShape2(tshapes[i].shape, tshapes[i].frame, tshapes[i].colour,
				 tshapes[i].specular, tshapes[i].flag, tshapes[i].flag_data);
		glPopMatrix();
	}

	nb_tshapes = 0;
}

void pie_RemainingPasses(void)
{
	pie_DrawShadows();
	pie_DrawRemainingTransShapes();
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

void pie_DrawImage270( PIEIMAGE *image, PIERECT *dest )
{
	PIELIGHT colour;

	polyCount++;

	pie_SetTexturePage(image->texPage);

	colour.argb = pie_GetColour();

	glBegin(GL_TRIANGLE_FAN);
	glColor4ub(colour.byte.r, colour.byte.g, colour.byte.b, colour.byte.a);
	glTexCoord2f(image->tu+image->tw+EDGE_CORRECTION, image->tv);
	glVertex2f(dest->x, dest->y);
	glTexCoord2f(image->tu+image->tw+EDGE_CORRECTION, image->tv+image->th+EDGE_CORRECTION);
	glVertex2f(dest->x+dest->h+EDGE_CORRECTION, dest->y);
	glTexCoord2f(image->tu, image->tv+image->th+EDGE_CORRECTION);
	glVertex2f(dest->x+dest->h+EDGE_CORRECTION, dest->y+dest->w+EDGE_CORRECTION);
	glTexCoord2f(image->tu, image->tv);
	glVertex2f(dest->x, dest->y+dest->w+EDGE_CORRECTION);
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

void pie_DrawRect( SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour )
{
	PIELIGHT c;
	polyCount++;

	c.argb = colour;
	pie_SetColourKeyedBlack(FALSE);

	glColor4ub(c.byte.r, c.byte.g, c.byte.b, c.byte.a);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex2i(x0, y0);
	glVertex2i(x1, y0);
	glVertex2i(x0, y1);
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

static void pie_PiePoly(PIEPOLY *poly, BOOL light)
{
	polyCount++;

	if (poly->nVrts >= 3) {
		if (poly->flags & PIE_COLOURKEYED) {
			pie_SetColourKeyedBlack(TRUE);
		} else {
			pie_SetColourKeyedBlack(FALSE);
		}
		pie_SetColourKeyedBlack(TRUE);
		if (poly->flags & PIE_NO_CULL) {
			glDisable(GL_CULL_FACE);
		}
		pie_Polygon(poly->nVrts, poly->pVrts, 0.0, light);
		if (poly->flags & PIE_NO_CULL) {
			glEnable(GL_CULL_FACE);
		}
	}
}

static void pie_PiePolyFrame(PIEPOLY *poly, SDWORD frame, BOOL light)
{
int	uFrame, vFrame, j, framesPerLine;

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
	pie_PiePoly(poly, light);
#endif
}


/***************************************************************************
 *
 *
 *
 ***************************************************************************/

//ivis style draw function
void pie_DrawTriangle( iVertex *pv )
{
	UDWORD	i;

	glBegin(GL_TRIANGLE_FAN);
	for (i = 0; i < 3; i++) {
		glColor4ub(pv[i].g*16, pv[i].g*16, pv[i].g*16, 255);
		glTexCoord2f(pv[i].u, pv[i].v);
		glVertex3f(pv[i].x, pv[i].y, pv[i].z);
	}
	glEnd();
}

void pie_DrawTexTriangle(PIEVERTEX *aVrts, SDWORD texPage, void* psEffects)
{
	GLfloat	offset = 0;
	int i;

	/*	Since this is only used from within source for the terrain draw - we can backface cull the
		polygons.
	*/
	tileCount++;
	pie_SetTexturePage(texPage);
	pie_SetFogStatus(TRUE);
	if (psEffects == NULL)
	{
		/* Solid terrain */
		pie_SetRendMode(REND_GOURAUD_TEX);
		pie_SetColourKeyedBlack(TRUE);
	}
	else
	{
		/* Translucent water with animation */
		pie_SetRendMode(REND_ALPHA_TEX);
		pie_SetColourKeyedBlack(FALSE);
		offset = *((GLfloat*)psEffects);
	}
	pie_SetBilinear(TRUE);

	glBegin(GL_TRIANGLE_FAN);
	for ( i = 0; i < 3; i++ )
	{
		glColor4ub( aVrts[i].light.byte.r, aVrts[i].light.byte.g, aVrts[i].light.byte.b, aVrts[i].light.byte.a );
		glTexCoord2f( aVrts[i].tu, aVrts[i].tv + offset );
		glVertex3f( aVrts[i].sx, aVrts[i].sy, aVrts[i].sz );
	}
	glEnd();
}

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
