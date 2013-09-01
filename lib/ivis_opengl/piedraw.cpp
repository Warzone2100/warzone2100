/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#include "lib/netplay/netplay.h"
#include "piematrix.h"
#include "screen.h"

#include <string.h>
#include <vector>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#define SHADOW_END_DISTANCE (8000*8000) // Keep in sync with lighting.c:FOG_END

// Shadow stencil stuff
static void ss_GL2_1pass();
static void ss_EXT_1pass();
static void ss_ATI_1pass();
static void ss_2pass();
static void (*ShadowStencilFunc)() = 0;
static GLenum ss_op_depth_pass_front = GL_INCR;
static GLenum ss_op_depth_pass_back = GL_DECR;

/*
 *	Local Variables
 */

static unsigned int pieCount = 0;
static unsigned int polyCount = 0;
static bool shadows = false;
static GLfloat lighting0[LIGHT_MAX][4];

/*
 *	Source
 */

void pie_InitLighting()
{
	const GLfloat defaultLight[LIGHT_MAX][4] = {{0.0f, 0.0f, 0.0f, 1.0f},  {0.5f, 0.5f, 0.5f, 1.0f},  {0.8f, 0.8f, 0.8f, 1.0f},  {1.0f, 1.0f, 1.0f, 1.0f}};
	memcpy(lighting0, defaultLight, sizeof(lighting0));
	pie_SetupLighting();
}

void pie_SetupLighting()
{
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lighting0[LIGHT_EMISSIVE]);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lighting0[LIGHT_AMBIENT]);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lighting0[LIGHT_DIFFUSE]);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lighting0[LIGHT_SPECULAR]);
}

void pie_Lighting0(LIGHTING_TYPE entry, float value[4])
{
	lighting0[entry][0] = value[0];
	lighting0[entry][1] = value[1];
	lighting0[entry][2] = value[2];
	lighting0[entry][3] = value[3];
}

void pie_setShadows(bool drawShadows)
{
	shadows = drawShadows;
}

void pie_BeginLighting(const Vector3f *light)
{
	const float pos[4] = { light->x, light->y, light->z, 0.0f };

	glLightfv(GL_LIGHT0, GL_POSITION, pos);
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
	glm::mat4	matrix;
	iIMDShape*	shape;
	int		flag;
	int		flag_data;
	glm::vec4	light;
};

typedef struct
{
	glm::mat4	matrix;
	iIMDShape*	shape;
	int		frame;
	PIELIGHT	colour;
	PIELIGHT	teamcolour;
	int		flag;
	int		flag_data;
} SHAPE;

static std::vector<ShadowcastingShape> scshapes;
static std::vector<SHAPE> tshapes;
static std::vector<SHAPE> shapes;

static void pie_Draw3DButton(iIMDShape *shape)
{
	const PIELIGHT colour = WZCOL_WHITE;
	const PIELIGHT teamcolour = pal_GetTeamColour(NetPlay.players[selectedPlayer].colour);
	pie_SetAlphaTest(true);
	pie_SetFogStatus(false);
	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
	pie_ActivateShader(SHADER_BUTTON, shape, teamcolour, colour);
	pie_SetRendMode(REND_OPAQUE);
	glColor4ubv(colour.vector);     // Only need to set once for entire model
	pie_SetTexturePage(shape->texpage);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, shape->buffers[VBO_VERTEX]); glVertexPointer(3, GL_FLOAT, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, shape->buffers[VBO_NORMAL]); glNormalPointer(GL_FLOAT, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, shape->buffers[VBO_TEXCOORD]); glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape->buffers[VBO_INDEX]);
	glDrawElements(GL_TRIANGLES, shape->npolys * 3, GL_UNSIGNED_SHORT, NULL);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	polyCount += shape->npolys;
	pie_DeactivateShader();
	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
}

static void pie_Draw3DShape2(iIMDShape *shape, int frame, PIELIGHT colour, PIELIGHT teamcolour, int pieFlag, int pieFlagData, glm::mat4 &matrix)
{
	bool light = true;

	glLoadMatrixf(&matrix[0][0]);
	pie_SetAlphaTest((pieFlag & pie_PREMULTIPLIED) == 0);

	/* Set fog status */
	if (!(pieFlag & pie_FORCE_FOG) && (pieFlag & pie_ADDITIVE || pieFlag & pie_TRANSLUCENT || pieFlag & pie_PREMULTIPLIED))
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
		pie_ActivateShader(SHADER_COMPONENT, shape, teamcolour, colour);
	}
	else
	{
		pie_DeactivateShader();
	}

	glColor4ubv(colour.vector);     // Only need to set once for entire model
	pie_SetTexturePage(shape->texpage);

	frame %= MAX(1, shape->numFrames);

	glBindBuffer(GL_ARRAY_BUFFER, shape->buffers[VBO_VERTEX]); glVertexPointer(3, GL_FLOAT, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, shape->buffers[VBO_NORMAL]); glNormalPointer(GL_FLOAT, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, shape->buffers[VBO_TEXCOORD]); glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape->buffers[VBO_INDEX]);
	glDrawElements(GL_TRIANGLES, shape->npolys * 3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(frame * shape->npolys * 3 * sizeof(uint16_t)));

	polyCount += shape->npolys;

	pie_SetShaderEcmEffect(false);
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
static void pie_DrawShadow(iIMDShape *shape, int flag, int flag_data, const glm::vec4 &light)
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
			glm::vec3 p[3];
			for(j = 0; j < 3; j++)
			{
				int current = pPolys->pindex[j];
				p[j] = glm::vec3(pVertices[current].x, scale_y(pVertices[current].y, flag, flag_data), pVertices[current].z);
			}
			if (glm::dot(glm::cross(p[2] - p[0], p[1] - p[0]), glm::vec3(light)) > 0.0f)
			{
				for (n = 1; n < 3; n++)
				{
					// link to the previous vertex
					addToEdgeList(pPolys->pindex[n-1], pPolys->pindex[n], edgelist);
				}
				// back to the first (FIXME - should be 0, not 2?)
				addToEdgeList(pPolys->pindex[2], pPolys->pindex[0], edgelist);
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
		glVertex3f(pVertices[b].x + light[0], scale_y(pVertices[b].y, flag, flag_data) + light[1], pVertices[b].z + light[2]);
		glVertex3f(pVertices[a].x + light[0], scale_y(pVertices[a].y, flag, flag_data) + light[1], pVertices[a].z + light[2]);
		glVertex3f(pVertices[a].x, scale_y(pVertices[a].y, flag, flag_data), pVertices[a].z);
	}
	glEnd();
}

void pie_SetUp(void)
{
	// initialise pie engine

	if (GLEW_EXT_stencil_wrap)
	{
		ss_op_depth_pass_front = GL_INCR_WRAP;
		ss_op_depth_pass_back = GL_DECR_WRAP;
	}

	if (GLEW_VERSION_2_0)
	{
		ShadowStencilFunc = ss_GL2_1pass;
	}
	else if (GLEW_EXT_stencil_two_side)
	{
		ShadowStencilFunc = ss_EXT_1pass;
	}
	else if (GLEW_ATI_separate_stencil)
	{
		ShadowStencilFunc = ss_ATI_1pass;
	}
	else
	{
		ShadowStencilFunc = ss_2pass;
	}
}

void pie_CleanUp( void )
{
	tshapes.clear();
	shapes.clear();
	scshapes.clear();
}

void pie_Draw3DShape(iIMDShape *shape, int frame, int team, PIELIGHT colour, int pieFlag, int pieFlagData)
{
	pieCount++;

	ASSERT(frame >= 0, "Negative frame %d", frame);
	ASSERT(team >= 0, "Negative team %d", team);

	if (pieFlag & pie_BUTTON)
	{
		pie_Draw3DButton(shape);
	}
	else
	{
		const PIELIGHT teamcolour = pal_GetTeamColour(team);
		SHAPE tshape;
		tshape.shape = shape;
		tshape.frame = frame;
		tshape.colour = colour;
		tshape.teamcolour = teamcolour;
		tshape.flag = pieFlag;
		tshape.flag_data = pieFlagData;
		pie_GetMatrix(&tshape.matrix[0][0]);

		if (pieFlag & pie_HEIGHT_SCALED)	// construct
		{
			tshape.matrix = glm::scale(tshape.matrix, glm::vec3(1.0f, (float)pieFlagData / (float)pie_RAISE_SCALE, 1.0f));
		}
		if (pieFlag & pie_RAISE)		// collapse
		{
			tshape.matrix = glm::translate(tshape.matrix, glm::vec3(1.0f, (-shape->max.y * (pie_RAISE_SCALE - pieFlagData)) * (1.0f / pie_RAISE_SCALE), 1.0f));
		}

		if (pieFlag & (pie_ADDITIVE | pie_TRANSLUCENT | pie_PREMULTIPLIED))
		{
			tshapes.push_back(tshape);
		}
		else
		{
			if (shadows && (pieFlag & pie_SHADOW || pieFlag & pie_STATIC_SHADOW))
			{
				float distance;

				// draw a shadow
				ShadowcastingShape scshape;
				pie_GetMatrix(&scshape.matrix[0][0]);
				distance = scshape.matrix[3][0] * scshape.matrix[3][0];
				distance += scshape.matrix[3][1] * scshape.matrix[3][1];
				distance += scshape.matrix[3][2] * scshape.matrix[3][2];

				// if object is too far in the fog don't generate a shadow.
				if (distance < SHADOW_END_DISTANCE)
				{
					glm::vec4 pos_light0;
					glm::mat4 invmat = glm::inverse(scshape.matrix);

					// Calculate the light position relative to the object
					glGetLightfv(GL_LIGHT0, GL_POSITION, &pos_light0[0]);
					scshape.light = invmat * pos_light0;
					scshape.shape = shape;
					scshape.flag = pieFlag;
					scshape.flag_data = pieFlagData;

					scshapes.push_back(scshape);
				}
			}
			shapes.push_back(tshape);
		}
	}
}

static void pie_ShadowDrawLoop(void)
{
	for (unsigned i = 0; i < scshapes.size(); i++)
	{
		glLoadMatrixf(&scshapes[i].matrix[0][0]);
		pie_DrawShadow(scshapes[i].shape, scshapes[i].flag, scshapes[i].flag_data, scshapes[i].light);
	}
}

static void pie_DrawShadows(void)
{
	const float width = pie_GetVideoBufferWidth();
	const float height = pie_GetVideoBufferHeight();

	pie_SetTexturePage(TEXPAGE_NONE);

	glPushMatrix();

	pie_SetAlphaTest(false);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_FALSE);
	glEnable(GL_STENCIL_TEST);

	ShadowStencilFunc();

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

	scshapes.resize(0);
}

void pie_RemainingPasses(void)
{
	GL_DEBUG("Remaining passes - shadows");
	glEnable(GL_LIGHT0);
	// Draw shadows
	if (shadows)
	{
		pie_DrawShadows();
	}
	// Draw models
	// TODO, sort list to reduce state changes
	GL_DEBUG("Remaining passes - opaque models");
	glPushMatrix();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	for (unsigned i = 0; i < shapes.size(); ++i)
	{
		pie_Draw3DShape2(shapes[i].shape, shapes[i].frame, shapes[i].colour, shapes[i].teamcolour, shapes[i].flag, shapes[i].flag_data, shapes[i].matrix);
	}
	// Draw translucent models last
	// TODO, sort list by Z order to do translucency correctly
	GL_DEBUG("Remaining passes - translucent models");
	for (unsigned i = 0; i < tshapes.size(); ++i)
	{
		pie_Draw3DShape2(tshapes[i].shape, tshapes[i].frame, tshapes[i].colour, tshapes[i].teamcolour, tshapes[i].flag, tshapes[i].flag_data, tshapes[i].matrix);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_LIGHT0);
	pie_DeactivateShader();
	glPopMatrix();
	tshapes.resize(0);
	shapes.resize(0);
	GL_DEBUG("Remaining passes - done");
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

// GL 2.0 1-pass version
static void ss_GL2_1pass()
{
	glDisable(GL_CULL_FACE);
	glStencilMask(~0);
	glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
	glStencilFunc(GL_ALWAYS, 0, ~0);

	pie_ShadowDrawLoop();
}

// generic 1-pass version
static void ss_EXT_1pass()
{
	glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	glDisable(GL_CULL_FACE);
	glStencilMask(~0);
	glActiveStencilFaceEXT(GL_BACK);
	glStencilOp(GL_KEEP, GL_KEEP, ss_op_depth_pass_back);
	glStencilFunc(GL_ALWAYS, 0, ~0);
	glActiveStencilFaceEXT(GL_FRONT);
	glStencilOp(GL_KEEP, GL_KEEP, ss_op_depth_pass_front);
	glStencilFunc(GL_ALWAYS, 0, ~0);

	pie_ShadowDrawLoop();

	glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
}

// ATI-specific 1-pass version
static void ss_ATI_1pass()
{
	glDisable(GL_CULL_FACE);
	glStencilMask(~0);
	glStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_KEEP, ss_op_depth_pass_back);
	glStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_KEEP, ss_op_depth_pass_front);
	glStencilFunc(GL_ALWAYS, 0, ~0);

	pie_ShadowDrawLoop();
}

// generic 2-pass version
static void ss_2pass()
{
	glStencilMask(~0);
	glStencilFunc(GL_ALWAYS, 0, ~0);
	glEnable(GL_CULL_FACE);

	// Setup stencil for front-facing polygons
	glCullFace(GL_BACK);
	glStencilOp(GL_KEEP, GL_KEEP, ss_op_depth_pass_front);

	pie_ShadowDrawLoop();

	// Setup stencil for back-facing polygons
	glCullFace(GL_FRONT);
	glStencilOp(GL_KEEP, GL_KEEP, ss_op_depth_pass_back);

	pie_ShadowDrawLoop();
}
