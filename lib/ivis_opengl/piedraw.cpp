/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/imd.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/pieclip.h"
#include "piematrix.h"
#include "screen.h"


#include <string.h>
#include <vector>
#include <algorithm>


#define SHADOW_END_DISTANCE (8000*8000) // Keep in sync with lighting.c:FOG_END

#define VERTICES_PER_TRIANGLE 3
#define COLOUR_COMPONENTS 4
#define TEXCOORD_COMPONENTS 2
#define VERTEX_COMPONENTS 3
#define TRIANGLES_PER_TILE 2
#define VERTICES_PER_TILE (TRIANGLES_PER_TILE * VERTICES_PER_TRIANGLE)

extern bool drawing_interface;

/*
 *	Local Variables
 */

static unsigned int pieCount = 0;
static unsigned int polyCount = 0;
static bool shadows = false;
static GLfloat lighting0[LIGHT_MAX][4] = {{0.0f, 0.0f, 0.0f, 1.0f},  {0.5f, 0.5f, 0.5f, 1.0f},  {0.8f, 0.8f, 0.8f, 1.0f},  {1.0f, 1.0f, 1.0f, 1.0f}};

/*
 *	Source
 */

void pie_Lighting0(LIGHTING_TYPE entry, float value[4])
{
	lighting0[entry][0] = value[0];
	lighting0[entry][1] = value[1];
	lighting0[entry][2] = value[2];
	lighting0[entry][3] = value[3];
}

void pie_BeginLighting(const Vector3f *light, bool drawshadows)
{
	const float pos[4] = {light->x, light->y, light->z, 0.0f};

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lighting0[LIGHT_EMISSIVE]);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lighting0[LIGHT_AMBIENT]);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lighting0[LIGHT_DIFFUSE]);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lighting0[LIGHT_SPECULAR]);
	glEnable(GL_LIGHT0);

	if (drawshadows)
	{
		shadows = true;
	}
}

bool pie_GetLightingState(void)
{
	return true;
}

void pie_EndLighting(void)
{
	shadows = false;
}

/***************************************************************************
 * pie_Draw3dShape
 *
 * Project and render a pumpkin image to render surface
 * Will support zbuffering, texturing, coloured lighting and alpha effects
 * Avoids recalculating vertex projections for every poly
 ***************************************************************************/

struct ShadowcastingShape
{
	float		matrix[16];
	iIMDShape*	shape;
	int		flag;
	int		flag_data;
	Vector3f	light;
};

struct TranslucentShape
{
	float		matrix[16];
	iIMDShape*	shape;
	int		frame;
	PIELIGHT	colour;
	int		flag;
	int		flag_data;
};

static std::vector<ShadowcastingShape> scshapes;
static std::vector<TranslucentShape> tshapes;

static void pie_Draw3DShape2(iIMDShape *shape, int frame, PIELIGHT colour, PIELIGHT teamcolour, int pieFlag, int pieFlagData)
{
	iIMDPoly *pPolys;
	bool light = true;
	bool shaders = pie_GetShaderAvailability();

	pie_SetAlphaTest((pieFlag & pie_PREMULTIPLIED) == 0);

	/* Set fog status */
	if (!(pieFlag & pie_FORCE_FOG) && 
		(pieFlag & pie_ADDITIVE || pieFlag & pie_TRANSLUCENT || pieFlag & pie_BUTTON || pieFlag & pie_PREMULTIPLIED))
	{
		pie_SetFogStatus(false);
	}
	else
	{
		pie_SetFogStatus(true);
	}

	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE)
	{
		pie_SetRendMode(REND_ADDITIVE);
		colour.byte.a = (UBYTE)pieFlagData;
		light = false;
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		pie_SetRendMode(REND_ALPHA);
		colour.byte.a = (UBYTE)pieFlagData;
		light = false;
	}
	else if (pieFlag & pie_PREMULTIPLIED)
	{
		pie_SetRendMode(REND_PREMULTIPLIED);
		light = false;
	}
	else
	{
		if (pieFlag & pie_BUTTON)
		{
			pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
			light = false;
			if (shaders)
			{
				pie_ActivateShader(SHADER_BUTTON, shape, teamcolour, colour);
			}
			else
			{
				pie_ActivateFallback(SHADER_BUTTON, shape, teamcolour, colour);
			}
		}
		pie_SetRendMode(REND_OPAQUE);
	}
	if (pieFlag & pie_ECM)
	{
		pie_SetRendMode(REND_ALPHA);
		light = true;
		pie_SetShaderEcmEffect(true);
	}

	if (light)
	{
		glMaterialfv(GL_FRONT, GL_AMBIENT, shape->material[LIGHT_AMBIENT]);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, shape->material[LIGHT_DIFFUSE]);
		glMaterialfv(GL_FRONT, GL_SPECULAR, shape->material[LIGHT_SPECULAR]);
		glMaterialf(GL_FRONT, GL_SHININESS, shape->shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, shape->material[LIGHT_EMISSIVE]);
		if (shaders)
		{
			pie_ActivateShader(SHADER_COMPONENT, shape, teamcolour, colour);
		}
		else
		{
			pie_ActivateFallback(SHADER_COMPONENT, shape, teamcolour, colour);
		}
	}

	if (pieFlag & pie_HEIGHT_SCALED)	// construct
	{
		glScalef(1.0f, (float)pieFlagData / (float)pie_RAISE_SCALE, 1.0f);
	}
	if (pieFlag & pie_RAISE)		// collapse
	{
		glTranslatef(1.0f, (-shape->max.y * (pie_RAISE_SCALE - pieFlagData)) * (1.0f / pie_RAISE_SCALE), 1.0f);
	}

	glColor4ubv(colour.vector);     // Only need to set once for entire model
	pie_SetTexturePage(shape->texpage);

	frame %= MAX(1, shape->numFrames);

	glBegin(GL_TRIANGLES);
	for (pPolys = shape->polys; pPolys < shape->polys + shape->npolys; pPolys++)
	{
		Vector3f	vertexCoords[3];
		unsigned int	n, frameidx = frame;
		int	*index;

		if (!(pPolys->flags & iV_IMD_TEXANIM))
		{
			frameidx = 0;
		}

		for (n = 0, index = pPolys->pindex;
				n < pPolys->npnts;
				n++, index++)
		{
			vertexCoords[n].x = shape->points[*index].x;
			vertexCoords[n].y = shape->points[*index].y;
			vertexCoords[n].z = shape->points[*index].z;
		}

		polyCount++;

		glNormal3fv((GLfloat*)&pPolys->normal);
		for (n = 0; n < pPolys->npnts; n++)
		{
			GLfloat* texCoord = (GLfloat*)&pPolys->texCoord[frameidx * pPolys->npnts + n];
			glTexCoord2fv(texCoord);
			if (!shaders)
			{
				glMultiTexCoord2fv(GL_TEXTURE1, texCoord);
			}
			glVertex3fv((GLfloat*)&vertexCoords[n]);
		}
	}
	glEnd();

	if (light || (pieFlag & pie_BUTTON))
	{
		if (shaders)
		{
			pie_DeactivateShader();
		}
		else
		{
			pie_DeactivateFallback();
		}
	}
	pie_SetShaderEcmEffect(false);

	if (pieFlag & pie_BUTTON)
	{
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	}
}

static inline bool edgeLessThan(EDGE const &e1, EDGE const &e2)
{
	if (e1.from != e2.from) return e1.from < e2.from;
	return e1.to < e2.to;
}

static inline bool edgeEqual(EDGE const &e1, EDGE const &e2)
{
	return e1.from == e2.from && e1.to == e2.to;
}

static inline void flipEdge(EDGE &e)
{
	std::swap(e.from, e.to);
}

/// Add an edge to an edgelist
/// Makes sure only silhouette edges are present
static inline void addToEdgeList(int a, int b, std::vector<EDGE> &edgelist)
{
	EDGE newEdge = {a, b};
	edgelist.push_back(newEdge);
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
	static std::vector<EDGE> edgelist;  // Static, to save allocations.
	static std::vector<EDGE> edgelistFlipped;  // Static, to save allocations.
	static std::vector<EDGE> edgelistFiltered;  // Static, to save allocations.
	EDGE *drawlist = NULL;

	unsigned edge_count;
	pVertices = shape->points;
	if( flag & pie_STATIC_SHADOW && shape->shadowEdgeList )
	{
		drawlist = shape->shadowEdgeList;
		edge_count = shape->nShadowEdges;
	}
	else
	{
		edgelist.clear();
		for (i = 0, pPolys = shape->polys; i < shape->npolys; ++i, ++pPolys)
		{
			Vector3f p[3];
			for(j = 0; j < 3; j++)
			{
				int current = pPolys->pindex[j];
				p[j] = Vector3f(pVertices[current].x, scale_y(pVertices[current].y, flag, flag_data), pVertices[current].z);
			}

			Vector3f normal = crossProduct(p[2] - p[0], p[1] - p[0]);
			if (normal * *light > 0)
			{
				for (n = 1; n < pPolys->npnts; n++)
				{
					// link to the previous vertex
					addToEdgeList(pPolys->pindex[n-1], pPolys->pindex[n], edgelist);
				}
				// back to the first
				addToEdgeList(pPolys->pindex[pPolys->npnts-1], pPolys->pindex[0], edgelist);
			}
		}

		// Remove duplicate pairs from the edge list. For example, in the list ((1 2), (2 6), (6 2), (3, 4)), remove (2 6) and (6 2).
		edgelistFlipped = edgelist;
		std::for_each(edgelistFlipped.begin(), edgelistFlipped.end(), flipEdge);
		std::sort(edgelist.begin(), edgelist.end(), edgeLessThan);
		std::sort(edgelistFlipped.begin(), edgelistFlipped.end(), edgeLessThan);
		edgelistFiltered.resize(edgelist.size());
		edgelistFiltered.erase(std::set_difference(edgelist.begin(), edgelist.end(), edgelistFlipped.begin(), edgelistFlipped.end(), edgelistFiltered.begin(), edgeLessThan), edgelistFiltered.end());

		drawlist = &edgelistFiltered[0];
		edge_count = edgelistFiltered.size();
		//debug(LOG_WARNING, "we have %i edges", edge_count);

		if(flag & pie_STATIC_SHADOW)
		{
			// then store it in the imd
			shape->nShadowEdges = edge_count;
			shape->shadowEdgeList = (EDGE *)realloc(shape->shadowEdgeList, sizeof(EDGE) * shape->nShadowEdges);
			std::copy(drawlist, drawlist + edge_count, shape->shadowEdgeList);
		}
	}

	// draw the shadow volume
	glBegin(GL_QUADS);
	glNormal3f(0.0, 1.0, 0.0);
	for(i=0;i<edge_count;i++)
	{
		int a = drawlist[i].from, b = drawlist[i].to;

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

void pie_SetUp(void)
{
	// initialise pie engine (just a placeholder for now)
}

void pie_CleanUp( void )
{
	tshapes.clear();
	scshapes.clear();
}

void pie_Draw3DShape(iIMDShape *shape, int frame, int team, PIELIGHT colour, int pieFlag, int pieFlagData)
{
	PIELIGHT teamcolour;

	ASSERT_OR_RETURN(, shape, "Attempting to draw null sprite");

	teamcolour = pal_GetTeamColour(team);

	pieCount++;

	ASSERT(frame >= 0, "Negative frame %d", frame);
	ASSERT(team >= 0, "Negative team %d", team);
	if (frame == 0)
	{
		frame = team;
	}

	if (drawing_interface || !shadows)
	{
		pie_Draw3DShape2(shape, frame, colour, teamcolour, pieFlag, pieFlagData);
	}
	else
	{
		if (pieFlag & (pie_ADDITIVE | pie_TRANSLUCENT | pie_PREMULTIPLIED) && !(pieFlag & pie_FORCE_IMMEDIATE))
		{
			TranslucentShape tshape;
			glGetFloatv(GL_MODELVIEW_MATRIX, tshape.matrix);
			tshape.shape = shape;
			tshape.frame = frame;
			tshape.colour = colour;
			tshape.flag = pieFlag;
			tshape.flag_data = pieFlagData;
			tshapes.push_back(tshape);
		}
		else
		{
			if(pieFlag & pie_SHADOW || pieFlag & pie_STATIC_SHADOW)
			{
				float distance;

				// draw a shadow
				ShadowcastingShape scshape;
				glGetFloatv(GL_MODELVIEW_MATRIX, scshape.matrix);
				distance = scshape.matrix[12] * scshape.matrix[12];
				distance += scshape.matrix[13] * scshape.matrix[13];
				distance += scshape.matrix[14] * scshape.matrix[14];

				// if object is too far in the fog don't generate a shadow.
				if (distance < SHADOW_END_DISTANCE)
				{
					float invmat[9], pos_light0[4];

					inverse_matrix( scshape.matrix, invmat );

					// Calculate the light position relative to the object
					glGetLightfv(GL_LIGHT0, GL_POSITION, pos_light0);
					scshape.light.x = invmat[0] * pos_light0[0] + invmat[3] * pos_light0[1] + invmat[6] * pos_light0[2];
					scshape.light.y = invmat[1] * pos_light0[0] + invmat[4] * pos_light0[1] + invmat[7] * pos_light0[2];
					scshape.light.z = invmat[2] * pos_light0[0] + invmat[5] * pos_light0[1] + invmat[8] * pos_light0[2];

					scshape.shape = shape;
					scshape.flag = pieFlag;
					scshape.flag_data = pieFlagData;

					scshapes.push_back(scshape);
				}
			}

			pie_Draw3DShape2(shape, frame, colour, teamcolour, pieFlag, pieFlagData);
		}
	}
}

static void pie_ShadowDrawLoop(void)
{
	for (unsigned i = 0; i < scshapes.size(); i++)
	{
		glLoadMatrixf(scshapes[i].matrix);
		pie_DrawShadow(scshapes[i].shape, scshapes[i].flag, scshapes[i].flag_data, &scshapes[i].light);
	}
}

static void pie_DrawShadows(void)
{
	const float width = pie_GetVideoBufferWidth();
	const float height = pie_GetVideoBufferHeight();
	GLenum op_depth_pass_front = GL_INCR, op_depth_pass_back = GL_DECR;

	pie_SetTexturePage(TEXPAGE_NONE);

	glPushMatrix();

	pie_SetAlphaTest(false);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_FALSE);
	glEnable(GL_STENCIL_TEST);

	// Check if we have the required extensions
	if (GLEW_EXT_stencil_wrap)
	{
		op_depth_pass_front = GL_INCR_WRAP_EXT;
		op_depth_pass_back = GL_DECR_WRAP_EXT;
	}

	// generic 1-pass version
	if (GLEW_EXT_stencil_two_side)
	{
		glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		glDisable(GL_CULL_FACE);
		glStencilMask(~0);
		glActiveStencilFaceEXT(GL_BACK);
		glStencilOp(GL_KEEP, GL_KEEP, op_depth_pass_back);
		glStencilFunc(GL_ALWAYS, 0, ~0);
		glActiveStencilFaceEXT(GL_FRONT);
		glStencilOp(GL_KEEP, GL_KEEP, op_depth_pass_front);
		glStencilFunc(GL_ALWAYS, 0, ~0);

		pie_ShadowDrawLoop();
		
		glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	}
	// check for ATI-specific 1-pass version
	else if (GLEW_ATI_separate_stencil)
	{
		glDisable(GL_CULL_FACE);
		glStencilMask(~0);
		glStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_KEEP, op_depth_pass_back);
		glStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_KEEP, op_depth_pass_front);
		glStencilFunc(GL_ALWAYS, 0, ~0);

		pie_ShadowDrawLoop();	
	}
	// fall back to default 2-pass version
	else
	{
		glStencilMask(~0);
		glStencilFunc(GL_ALWAYS, 0, ~0);
		glEnable(GL_CULL_FACE);
		
		// Setup stencil for front-facing polygons
		glCullFace(GL_BACK);
		glStencilOp(GL_KEEP, GL_KEEP, op_depth_pass_front);

		pie_ShadowDrawLoop();

		// Setup stencil for back-facing polygons
		glCullFace(GL_FRONT);
		glStencilOp(GL_KEEP, GL_KEEP, op_depth_pass_back);

		pie_ShadowDrawLoop();
	}

	pie_SetRendMode(REND_ALPHA);
	glEnable(GL_CULL_FACE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilMask(~0);
	glStencilFunc(GL_LESS, 0, ~0);
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

	pie_SetRendMode(REND_OPAQUE);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	glPopMatrix();

	scshapes.clear();
}

static void pie_DrawRemainingTransShapes(void)
{
	glPushMatrix();
	for (unsigned i = 0; i < tshapes.size(); ++i)
	{
		glLoadMatrixf(tshapes[i].matrix);
		pie_Draw3DShape2(tshapes[i].shape, tshapes[i].frame, tshapes[i].colour, tshapes[i].colour,
				 tshapes[i].flag, tshapes[i].flag_data);
	}
	glPopMatrix();

	tshapes.clear();
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
void pie_DrawImage(const PIEIMAGE *image, const PIERECT *dest)
{
	pie_DrawImage(image, dest, WZCOL_WHITE);
}

void pie_DrawImage(const PIEIMAGE *image, const PIERECT *dest, PIELIGHT colour)
{
	/* Set transparent color to be 0 red, 0 green, 0 blue, 0 alpha */
	polyCount++;

	pie_SetTexturePage(image->texPage);

	glColor4ubv(colour.vector);

	glBegin(GL_TRIANGLE_STRIP);
		//set up 4 pie verts
		glTexCoord2f(image->tu / OLD_TEXTURE_SIZE_FIX, image->tv / OLD_TEXTURE_SIZE_FIX);
		glVertex2f(dest->x, dest->y);

		glTexCoord2f((image->tu + image->tw) / OLD_TEXTURE_SIZE_FIX, image->tv / OLD_TEXTURE_SIZE_FIX);
		glVertex2f(dest->x + dest->w, dest->y);

		glTexCoord2f(image->tu / OLD_TEXTURE_SIZE_FIX, (image->tv + image->th) / OLD_TEXTURE_SIZE_FIX);
		glVertex2f(dest->x, dest->y + dest->h);

		glTexCoord2f((image->tu + image->tw) / OLD_TEXTURE_SIZE_FIX, (image->tv + image->th) / OLD_TEXTURE_SIZE_FIX);
		glVertex2f(dest->x + dest->w, dest->y + dest->h);
	glEnd();
}

void pie_GetResetCounts(unsigned int* pPieCount, unsigned int* pPolyCount, unsigned int* pStateCount)
{
	*pPieCount  = pieCount;
	*pPolyCount = polyCount;
	*pStateCount = pieStateCount;

	pieCount = 0;
	polyCount = 0;
	pieStateCount = 0;
	return;
}
