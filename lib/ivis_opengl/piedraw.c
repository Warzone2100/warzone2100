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
/** \file
 *  Render routines for 3D coloured and shaded transparency rendering.
 */

#include <string.h>
#include <SDL_opengl.h>
#include <SDL_video.h>

#include "lib/framework/frame.h"
#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/imd.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piefunc.h"
#include "lib/ivis_common/tex.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/pieclip.h"
#include "piematrix.h"

#define VERTICES_PER_TRIANGLE 3
#define COLOUR_COMPONENTS 4
#define TEXCOORD_COMPONENTS 2
#define VERTEX_COMPONENTS 3
#define MAP_TRIANGLES (64 * 64 * 2) // two triangles per tile
#define MAP_VERTICES (VERTICES_PER_TRIANGLE * MAP_TRIANGLES)

static GLubyte aColour[COLOUR_COMPONENTS * MAP_VERTICES];
static GLfloat aTexCoord[TEXCOORD_COMPONENTS * MAP_VERTICES];
static GLfloat aVertex[VERTEX_COMPONENTS * MAP_VERTICES];

extern BOOL drawing_interface;

/*
 *	OpenGL extensions for shadows
 */

BOOL check_extension(const char *extName)
{
	char *p = (char *) glGetString(GL_EXTENSIONS);
	char *end;
	size_t extNameLen= strlen(extName);

	end = p + strlen(p);    
	while (p < end)
	{
		int n = strcspn(p, " ");
		if ((extNameLen == n) && (strncmp(extName, p, n) == 0))
		{
			return TRUE;
		}
		p += (n + 1);
	}
	return FALSE;
}

// EXT_stencil_two_side
#ifndef GL_EXT_stencil_two_side
# define GL_EXT_stencil_two_side 1
# define GL_STENCIL_TEST_TWO_SIDE_EXT      0x8910
# define GL_ACTIVE_STENCIL_FACE_EXT        0x8911
typedef void (APIENTRY * PFNGLACTIVESTENCILFACEEXTPROC) (GLenum face);
#endif

#ifndef WZ_OS_MAC
PFNGLACTIVESTENCILFACEEXTPROC glActiveStencilFaceEXT;
#endif


/** Check if we can use one-pass stencil in the shadow draw code. */
static BOOL stencil_one_pass(void)
{
	// tribool, -1: uninitialized, 0: FALSE, 1: TRUE
	static int can_do_stencil_one_pass = -1;

	if (can_do_stencil_one_pass < 0) {
		can_do_stencil_one_pass = 0; // can't use it until we decide otherwise

		// let's check if we have the needed extensions
#ifdef WZ_OS_MAC
		can_do_stencil_one_pass = 1;
#else
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
#endif /* WZ_OS_MAC */
	}

	return (1 == can_do_stencil_one_pass); // to get the types right
}

/*
 *	Local Variables
 */

static TERRAIN_VERTEXF pieVrts[pie_MAX_VERTICES_PER_POLYGON];
static unsigned int pieCount = 0;
static unsigned int tileCount = 0;
static unsigned int polyCount = 0;
static BOOL lighting = FALSE;
static BOOL shadows = FALSE;

/*
 *	Source
 */

void pie_BeginLighting(const Vector3f * light)
{
	const float pos[4] = {light->x, light->y, light->z, 0.0f};
	const float zero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	const float ambient[4] = {0.3f, 0.3f, 0.3f, 1.0f};
	const float diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
	const float specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glEnable(GL_LIGHT0);

//	lighting = TRUE;
	shadows = TRUE;
}

void pie_EndLighting(void)
{
	shadows = FALSE;
	lighting = FALSE;
}

static inline void pie_Polygon(const unsigned int numVerts, const TERRAIN_VERTEXF* pVrts, const BOOL light)
{
	unsigned int i = 0;

	if (numVerts < 1)
	{
		return;
	}
	else if (numVerts == 1)
	{
		glBegin(GL_POINTS);
	}
	else if (numVerts == 2)
	{
		glBegin(GL_LINE_STRIP);
	}
	else
	{
		if (light)
		{
			float ambient[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			float diffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			float specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			float shininess = 10;

			glEnable(GL_LIGHTING);
			glEnable(GL_NORMALIZE);

			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
		}

		glBegin(GL_TRIANGLE_FAN);

		if (light)
		{
			const Vector3f
					p1 = { pVrts[0].x, pVrts[0].y, pVrts[0].z },
					p2 = { pVrts[1].x, pVrts[1].y, pVrts[1].z },
					p3 = { pVrts[2].x, pVrts[2].y, pVrts[2].z },
					v1 = Vector3f_Sub(p3, p1),
					v2 = Vector3f_Sub(p2, p1),
					normal = Vector3f_CrossP(v1, v2);

			STATIC_ASSERT(sizeof(Vector3f) == sizeof(float[3]));

			glNormal3fv((float*)&normal);
		}
	}

	for (i = 0; i < numVerts; i++)
	{
		glColor4ubv(pVrts[i].light.vector);
		glTexCoord2f(pVrts[i].u, pVrts[i].v);
		glVertex3f(pVrts[i].x, pVrts[i].y, pVrts[i].z);
	}

	glEnd();

	if (light)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_NORMALIZE);
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
static inline void pie_PiePoly(const PIEPOLY *poly, const BOOL light)
{
	polyCount++;

	if (poly->nVrts >= 3)
	{
		if (poly->flags & PIE_COLOURKEYED)
		{
			pie_SetAlphaTest(TRUE);
		}
		else
		{
			pie_SetAlphaTest(FALSE);
		}
		pie_SetAlphaTest(TRUE);
		if (poly->flags & PIE_NO_CULL)
		{
			glDisable(GL_CULL_FACE);
		}
		pie_Polygon(poly->nVrts, poly->pVrts, light);
		if (poly->flags & PIE_NO_CULL)
		{
			glEnable(GL_CULL_FACE);
		}
	}
}

static inline void pie_PiePolyFrame(PIEPOLY *poly, SDWORD frame, const BOOL light)
{
	if ( (poly->flags & iV_IMD_TEXANIM) && poly->pTexAnim != NULL && frame != 0 )
	{
		frame %= abs(poly->pTexAnim->nFrames);

		if (frame > 0)
		{
			const unsigned int framesPerLine = 256 / poly->pTexAnim->textureWidth;
			const unsigned int
					uFrame = (frame % framesPerLine) * poly->pTexAnim->textureWidth,
					vFrame = (frame / framesPerLine) * poly->pTexAnim->textureHeight;
			unsigned int j = 0;

			for (j = 0; j < poly->nVrts; j++)
			{
				poly->pVrts[j].u += uFrame;
				poly->pVrts[j].v += vFrame;
			}
		}
	}
#ifndef NO_RENDER
	//draw with new texture data
	pie_PiePoly(poly, light);
#endif
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
	Vector3f	light;
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

static void pie_Draw3DShape2(iIMDShape *shape, int frame, PIELIGHT colour, PIELIGHT specular, int pieFlag, int pieFlagData)
{
	unsigned int n;
	Vector3f *pVertices, *pPixels, scrPoints[pie_MAX_VERTICES];
	iIMDPoly *pPolys;
	PIEPOLY piePoly;
	VERTEXID *index;
	BOOL light = lighting;

	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE)
	{ //Assume also translucent
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ADDITIVE_TEX);
		colour.byte.a = (UBYTE)pieFlagData;
		light = FALSE;
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		pie_SetFogStatus(FALSE);
		pie_SetRendMode(REND_ALPHA_TEX);
		colour.byte.a = (UBYTE)pieFlagData;
		light = FALSE;
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
	}

	if (pieFlag & pie_RAISE)
	{
		pieFlagData = (shape->max.y * (pie_RAISE_SCALE - pieFlagData)) / pie_RAISE_SCALE;
	}

	pie_SetTexturePage(shape->texpage);

	//now draw the shape
	//rotate and project points from shape->points to scrPoints
	for (pVertices = shape->points, pPixels = scrPoints;
			pVertices < shape->points + shape->npoints;
			pVertices++, pPixels++)
	{
		float tempY = pVertices->y;
		if (pieFlag & pie_RAISE)
		{
			tempY = pVertices->y - pieFlagData;
			if (tempY < 0)
				tempY = 0;

		}
		else if ( (pieFlag & pie_HEIGHT_SCALED) && pVertices->y > 0 )
		{
			tempY = (pVertices->y * pieFlagData) / pie_RAISE_SCALE;
		}
		pPixels->x = pVertices->x;
		pPixels->y = tempY;
		pPixels->z = pVertices->z;
	}

	for (pPolys = shape->polys;
			pPolys < shape->polys + shape->npolys;
			pPolys++)
	{
		piePoly.flags = pPolys->flags;

		if (pieFlag & pie_TRANSLUCENT)
		{
			/* There are no PIE files with PIE_ALPHA set, this is the only user, and
			 * this flag is never checked anywhere, except we check below that _some_
			 * flag is set. This is weird. FIXME. - Per */
			piePoly.flags |= PIE_ALPHA;
		}
		else if (pieFlag & pie_ADDITIVE)
		{
			piePoly.flags &= (0xffffffff-PIE_COLOURKEYED);//dont treat additive images as colour keyed
		}

		for (n = 0, index = pPolys->pindex;
				n < pPolys->npnts;
				n++, index++)
		{
			pieVrts[n].x = scrPoints[*index].x;
			pieVrts[n].y = scrPoints[*index].y;
			pieVrts[n].z = scrPoints[*index].z;
			pieVrts[n].u = pPolys->texCoord[n].x;
			pieVrts[n].v = pPolys->texCoord[n].y;
			pieVrts[n].light = colour;
			pieVrts[n].specular = specular;
		}
		piePoly.nVrts = pPolys->npnts;
		piePoly.pVrts = pieVrts;

		piePoly.pTexAnim = pPolys->pTexAnim;

		if (piePoly.flags > 0)
		{
			pie_PiePolyFrame(&piePoly, frame, light); // draw the polygon ...
		}
	}

	if (pieFlag & pie_BUTTON)
	{
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	}
}

/// returns true if the edges are adjacent
static int compare_edge (EDGE *A, EDGE *B, const Vector3f *pVertices )
{
	if(A->from == B->to)
	{
		if(A->to == B->from)
		{
			return TRUE;
		}
		return Vector3f_Compare(pVertices[A->to], pVertices[B->from]);
	}

	if(!Vector3f_Compare(pVertices[A->from], pVertices[B->to]))
	{
		return FALSE;
	}

	if(A->to == B->from)
	{
		return TRUE;
	}
	return Vector3f_Compare(pVertices[A->to], pVertices[B->from]);
}

/// Add an edge to an edgelist
/// Makes sure only silhouette edges are present
static void addToEdgeList(int a, int b, EDGE *edgelist, unsigned int* edge_count, Vector3f *pVertices)
{
	EDGE newEdge = {a, b};
	unsigned int i;
	BOOL foundMatching = FALSE;

	for(i = 0; i < *edge_count; i++)
	{
		if(edgelist[i].from < 0)
		{
			// does not exist anymore
			continue;
		}
		if(compare_edge(&newEdge, &edgelist[i], pVertices)) {
			// remove the other too
			edgelist[i].from = -1;
			foundMatching = TRUE;
			break;
		}
	}
	if(!foundMatching)
	{
		edgelist[*edge_count] = newEdge;
		(*edge_count)++;
	}
}

/// scale the height according to the flags
static inline float scale_y(float y, int flag, int flag_data)
{
	float tempY = y;
	if (flag & pie_RAISE) {
		tempY = y - flag_data;
		if (y - flag_data < 0) tempY = 0;
	} else if (flag & pie_HEIGHT_SCALED) {
		if(y>0) {
			tempY = (y * flag_data)/pie_RAISE_SCALE;
		}
	}
	return tempY;
}

/// Draw the shadow for a shape
static void pie_DrawShadow(iIMDShape *shape, int flag, int flag_data, Vector3f* light)
{
	unsigned int i, j, n;
	Vector3f *pVertices;
	iIMDPoly *pPolys;
	unsigned int edge_count = 0;
	static EDGE *edgelist = NULL;
	static unsigned int edgelistsize = 256;
	EDGE *drawlist = NULL;

	if(!edgelist)
	{
		edgelist = (EDGE*)malloc(sizeof(EDGE)*edgelistsize);
	}
	pVertices = shape->points;
	if( flag & pie_STATIC_SHADOW && shape->shadowEdgeList )
	{
		drawlist = shape->shadowEdgeList;
		edge_count = shape->nShadowEdges;
	}
	else
	{

		for (i = 0, pPolys = shape->polys; i < shape->npolys; ++i, ++pPolys) {
			Vector3f p[3], v[2], normal = {0.0f, 0.0f, 0.0f};
			VERTEXID current, first;
			for(j = 0; j < 3; j++)
			{
				current = pPolys->pindex[j];
				Vector3f_Set(&p[j], pVertices[current].x, scale_y(pVertices[current].y, flag, flag_data), pVertices[current].z);
			}

			v[0] = Vector3f_Sub(p[2], p[0]);
			v[1] = Vector3f_Sub(p[1], p[0]);
			normal = Vector3f_CrossP(v[0], v[1]);
			if (Vector3f_ScalarP(normal, *light) > 0)
			{
				first = pPolys->pindex[0];
				for (n = 1; n < pPolys->npnts; n++) {
					// link to the previous vertex
					addToEdgeList(pPolys->pindex[n-1], pPolys->pindex[n], edgelist, &edge_count, pVertices);
					// check if the edgelist is still large enough
					if(edge_count >= edgelistsize-1)
					{
						// enlarge
						EDGE* newstack;
						edgelistsize *= 2;
						newstack = realloc(edgelist, sizeof(EDGE) * edgelistsize);
						if (newstack == NULL)
						{
							debug(LOG_ERROR, "pie_DrawShadow: Out of memory!");
							abort();
							return;
						}

						edgelist = newstack;

						debug(LOG_WARNING, "new edge list size: %u", edgelistsize);
					}
				}
				// back to the first
				addToEdgeList(pPolys->pindex[pPolys->npnts-1], first, edgelist, &edge_count, pVertices);
			}
		}
		//debug(LOG_WARNING, "we have %i edges", edge_count);
		drawlist = edgelist;

		if(flag & pie_STATIC_SHADOW)
		{
			// first compact the current edgelist
			for(i = 0, j = 0; i < edge_count; i++)
			{
				if(edgelist[i].from < 0)
				{
					continue;
				}
				edgelist[j] = edgelist[i];
				j++;
			}
			edge_count = j;
			// then store it in the imd
			shape->nShadowEdges = edge_count;
			shape->shadowEdgeList = realloc(shape->shadowEdgeList, sizeof(EDGE) * shape->nShadowEdges);
			memcpy(shape->shadowEdgeList, edgelist, sizeof(EDGE) * shape->nShadowEdges);
		}
	}

	// draw the shadow volume
	glBegin(GL_QUADS);
	for(i=0;i<edge_count;i++)
	{
		int a = drawlist[i].from, b = drawlist[i].to;
		if(a < 0)
		{
			continue;
		}

		glVertex3f(pVertices[b].x, scale_y(pVertices[b].y, flag, flag_data), pVertices[b].z);
		glVertex3f(pVertices[b].x+light->x, scale_y(pVertices[b].y, flag, flag_data)+light->y, pVertices[b].z+light->z);
		glVertex3f(pVertices[a].x+light->x, scale_y(pVertices[a].y, flag, flag_data)+light->y, pVertices[a].z+light->z);
		glVertex3f(pVertices[a].x, scale_y(pVertices[a].y, flag, flag_data), pVertices[a].z);
	}
	glEnd();

#ifdef SHOW_SHADOW_EDGES
	glDisable(GL_DEPTH_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glColor4ub(0xFF, 0, 0, 0xFF);
	glBegin(GL_LINES);
	for(i = 0; i < edge_count; i++)
	{
		int a = drawlist[i].from, b = drawlist[i].to;
		if(a < 0)
		{
			continue;
		}

		glVertex3f(pVertices[b].x, scale_y(pVertices[b].y, flag, flag_data), pVertices[b].z);
		glVertex3f(pVertices[a].x, scale_y(pVertices[a].y, flag, flag_data), pVertices[a].z);
	}
	glEnd();
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glEnable(GL_DEPTH_TEST);
#endif
}

static void inverse_matrix(const float * src, float * dst)
{
	const float det = src[0]*src[5]*src[10] + src[4]*src[9]*src[2] + src[8]*src[1]*src[6] - src[2]*src[5]*src[8] - src[6]*src[9]*src[0] - src[10]*src[1]*src[4];
	const float invdet = 1.0f/det;

	dst[0] = invdet * (src[5]*src[10] - src[9]*src[6]);
	dst[1] = invdet * (src[9]*src[2] - src[1]*src[10]);
	dst[2] = invdet * (src[1]*src[6] - src[5]*src[2]);
	dst[3] = invdet * (src[8]*src[6] - src[4]*src[10]);
	dst[4] = invdet * (src[0]*src[10] - src[8]*src[2]);
	dst[5] = invdet * (src[4]*src[2] - src[0]*src[6]);
	dst[6] = invdet * (src[4]*src[9] - src[8]*src[5]);
	dst[7] = invdet * (src[8]*src[1] - src[0]*src[9]);
	dst[8] = invdet * (src[0]*src[5] - src[4]*src[1]);
}

void pie_CleanUp( void )
{
	free( tshapes );
	free( scshapes );
	tshapes = NULL;
	scshapes = NULL;
}

void pie_Draw3DShape(iIMDShape *shape, int frame, int team, PIELIGHT colour, PIELIGHT specular, int pieFlag, int pieFlagData)
{
	pieCount++;

	// Fix for transparent buildings and features!!
	if( (pieFlag & pie_TRANSLUCENT) && (pieFlagData > 220) )
	{
		// force to bilinear and non-transparent
		pieFlag = pieFlag & ~pie_TRANSLUCENT;
		pieFlagData = 0;
	}

	if (frame == 0)
	{
		frame = team;
	}

	if (drawing_interface || !shadows)
	{
		pie_Draw3DShape2(shape, frame, colour, specular, pieFlag, pieFlagData);
	}
	else
	{
		if (pieFlag & (pie_ADDITIVE | pie_TRANSLUCENT))
		{
			if (tshapes_size <= nb_tshapes)
			{
				if (tshapes_size == 0)
				{
					tshapes_size = 64;
					tshapes = (transluscent_shape_t*)malloc(tshapes_size*sizeof(transluscent_shape_t));
					memset( tshapes, 0, tshapes_size*sizeof(transluscent_shape_t) );
				}
				else
				{
					const unsigned int old_size = tshapes_size;
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
		}
		else
		{
			if(pieFlag & pie_SHADOW || pieFlag & pie_STATIC_SHADOW)
			{
				float distance;

				// draw a shadow
				if (scshapes_size <= nb_scshapes)
				{
					if (scshapes_size == 0)
					{
						scshapes_size = 64;
						scshapes = (shadowcasting_shape_t*)malloc(scshapes_size*sizeof(shadowcasting_shape_t));
						memset( scshapes, 0, scshapes_size*sizeof(shadowcasting_shape_t) );
					}
					else
					{
						const unsigned int old_size = scshapes_size;
						scshapes_size <<= 1;
						scshapes = (shadowcasting_shape_t*)realloc(scshapes, scshapes_size*sizeof(shadowcasting_shape_t));
						memset( &scshapes[old_size], 0, (scshapes_size-old_size)*sizeof(shadowcasting_shape_t) );
					}
				}

				glGetFloatv(GL_MODELVIEW_MATRIX, scshapes[nb_scshapes].matrix);
				distance = scshapes[nb_scshapes].matrix[12] * scshapes[nb_scshapes].matrix[12];
				distance += scshapes[nb_scshapes].matrix[13] * scshapes[nb_scshapes].matrix[13];
				distance += scshapes[nb_scshapes].matrix[14] * scshapes[nb_scshapes].matrix[14];

				// if object is too far in the fog don't generate a shadow.
				if (distance < 6000*6000)
				{
					float invmat[9], pos_light0[4];

					inverse_matrix( scshapes[nb_scshapes].matrix, invmat );

					// Calculate the light position relative to the object
					glGetLightfv(GL_LIGHT0, GL_POSITION, pos_light0);
					scshapes[nb_scshapes].light.x = invmat[0] * pos_light0[0] + invmat[3] * pos_light0[1] + invmat[6] * pos_light0[2];
					scshapes[nb_scshapes].light.y = invmat[1] * pos_light0[0] + invmat[4] * pos_light0[1] + invmat[7] * pos_light0[2];
					scshapes[nb_scshapes].light.z = invmat[2] * pos_light0[0] + invmat[5] * pos_light0[1] + invmat[8] * pos_light0[2];

					scshapes[nb_scshapes].shape = shape;
					scshapes[nb_scshapes].flag = pieFlag;
					scshapes[nb_scshapes].flag_data = pieFlagData;

					nb_scshapes++;
				}
			}
			pie_Draw3DShape2(shape, frame, colour, specular, pieFlag, pieFlagData);
		}
	}
}

static void pie_ShadowDrawLoop(void)
{
	unsigned int i = 0;

	for (i = 0; i < nb_scshapes; i++)
	{
		glLoadMatrixf(scshapes[i].matrix);
		pie_DrawShadow(scshapes[i].shape, scshapes[i].flag, scshapes[i].flag_data, &scshapes[i].light);
	}
}

static void pie_DrawShadows(void)
{
	const float width = pie_GetVideoBufferWidth();
	const float height = pie_GetVideoBufferHeight();

	pie_SetTexturePage(-1);

	glPushMatrix();

	pie_SetAlphaTest(FALSE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_FALSE);
	glEnable(GL_STENCIL_TEST);

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

		pie_ShadowDrawLoop();
		glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);

	} else {
		// Setup stencil for back faces.
		glStencilMask(~0);
		glStencilFunc(GL_ALWAYS, 0, ~0);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

		pie_ShadowDrawLoop();

		// Setup stencil for front faces.
		glCullFace(GL_FRONT);
		glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

		// Draw shadows again
		pie_ShadowDrawLoop();
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
	unsigned int i = 0;

	glPushMatrix();
	for (i = 0; i < nb_tshapes; ++i)
	{
		glLoadMatrixf(tshapes[i].matrix);
		pie_Draw3DShape2(tshapes[i].shape, tshapes[i].frame, tshapes[i].colour,
				 tshapes[i].specular, tshapes[i].flag, tshapes[i].flag_data);
	}
	glPopMatrix();

	nb_tshapes = 0;
}

void pie_RemainingPasses(void)
{
	if(shadows)
	{
		pie_DrawShadows();
	}
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
void pie_DrawImage(PIEIMAGE *image, PIERECT *dest)
{
	PIELIGHT colour = WZCOL_WHITE;

	/* Set transparent color to be 0 red, 0 green, 0 blue, 0 alpha */
	polyCount++;

	pie_SetTexturePage(image->texPage);

	glColor4ubv(colour.vector);

	glBegin(GL_TRIANGLE_STRIP);
		//set up 4 pie verts
		glTexCoord2f(image->tu, image->tv);
		glVertex2f(dest->x, dest->y);

		glTexCoord2f(image->tu + image->tw, image->tv);
		glVertex2f(dest->x + dest->w, dest->y);

		glTexCoord2f(image->tu, image->tv + image->th);
		glVertex2f(dest->x, dest->y + dest->h);

		glTexCoord2f(image->tu + image->tw, image->tv + image->th);
		glVertex2f(dest->x + dest->w, dest->y + dest->h);
	glEnd();
}

void pie_DrawTerrainDone(int mapx, int mapy)
{
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glColorPointer(COLOUR_COMPONENTS, GL_UNSIGNED_BYTE, 0, aColour);
	glTexCoordPointer(TEXCOORD_COMPONENTS, GL_FLOAT, 0, aTexCoord);
	glVertexPointer(VERTEX_COMPONENTS, GL_FLOAT, 0, aVertex);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glDrawArrays(GL_TRIANGLES, 0, VERTICES_PER_TRIANGLE * mapx * mapy * 2);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

// index gives us the triangle
void pie_DrawTerrainTriangle(int index, const TERRAIN_VERTEX *aVrts)
{
	unsigned int i = 0, j = index * VERTICES_PER_TRIANGLE;

	assert(index < MAP_TRIANGLES);
	assert(j < MAP_VERTICES);
	tileCount++;

	for ( i = 0; i < 3; i++ )
	{
		aColour[j * COLOUR_COMPONENTS + 0] = aVrts[i].light.byte.r;
		aColour[j * COLOUR_COMPONENTS + 1] = aVrts[i].light.byte.g;
		aColour[j * COLOUR_COMPONENTS + 2] = aVrts[i].light.byte.b;
		aColour[j * COLOUR_COMPONENTS + 3] = aVrts[i].light.byte.a;
		aTexCoord[j * TEXCOORD_COMPONENTS + 0] = aVrts[i].u;
		aTexCoord[j * TEXCOORD_COMPONENTS + 1] = aVrts[i].v;
		aVertex[j * VERTEX_COMPONENTS + 0] = aVrts[i].pos.x;
		aVertex[j * VERTEX_COMPONENTS + 1] = aVrts[i].pos.y;
		aVertex[j * VERTEX_COMPONENTS + 2] = aVrts[i].pos.z;
		j++;
	}
}

void pie_DrawWaterTriangle(const TERRAIN_VERTEX *aVrts)
{
	unsigned int i = 0;

	/* Since this is only used from within source for the terrain draw - we can backface cull the polygons. */
	tileCount++;

	glBegin(GL_TRIANGLE_FAN);
		for ( i = 0; i < 3; i++ )
		{
			glColor4ubv(aVrts[i].light.vector);
			glTexCoord2f(aVrts[i].u, aVrts[i].v);
			glVertex3f( aVrts[i].pos.x, aVrts[i].pos.y, aVrts[i].pos.z );
		}
	glEnd();
}

void pie_GetResetCounts(unsigned int* pPieCount, unsigned int* pTileCount, unsigned int* pPolyCount, unsigned int* pStateCount)
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
