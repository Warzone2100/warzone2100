/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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

#include <unordered_map>
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/imd.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "piematrix.h"
#include "screen.h"

#include <string.h>
#include <vector>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define BUFFER_OFFSET(i) (reinterpret_cast<char *>(i))
#define SHADOW_END_DISTANCE (8000*8000) // Keep in sync with lighting.c:FOG_END

/*
 *	Local Variables
 */

static size_t pieCount = 0;
static size_t polyCount = 0;
static bool shadows = false;
static gfx_api::gfxFloat lighting0[LIGHT_MAX][4];

/*
 *	Source
 */

void pie_InitLighting()
{
	// set scene color, ambient, diffuse and specular light intensities of sun
	// diffuse lighting is turned off because players dislike it
	const gfx_api::gfxFloat defaultLight[LIGHT_MAX][4] = {
		{0.0f, 0.0f, 0.0f, 1.0f}, // LIGHT_EMISSIVE
		{0.5f, 0.5f, 0.5f, 1.0f}, // LIGHT_AMBIENT
		{1.0f, 1.0f, 1.0f, 1.0f}, // LIGHT_DIFFUSE
		{1.0f, 1.0f, 1.0f, 1.0f}  // LIGHT_SPECULAR
	};
	memcpy(lighting0, defaultLight, sizeof(lighting0));
}

void pie_Lighting0(LIGHTING_TYPE entry, const float value[4])
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

static Vector3f currentSunPosition(0.f, 0.f, 0.f);

void pie_BeginLighting(const Vector3f &light)
{
	currentSunPosition = light;
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
	iIMDShape	*shape;
	int		flag;
	int		flag_data;
	glm::vec4	light;
};

struct SHAPE
{
	glm::mat4	matrix;
	iIMDShape	*shape;
	int		frame;
	PIELIGHT	colour;
	PIELIGHT	teamcolour;
	int		flag;
	int		flag_data;
	float		stretch;
};

static std::vector<ShadowcastingShape> scshapes;
static std::vector<SHAPE> tshapes;
static std::vector<SHAPE> shapes;
static gfx_api::buffer* pZeroedVertexBuffer = nullptr;

static gfx_api::buffer* getZeroedVertexBuffer(size_t size)
{
	static size_t currentSize = 0;
	if (!pZeroedVertexBuffer || (currentSize < size))
	{
		if (pZeroedVertexBuffer)
		{
			delete pZeroedVertexBuffer;
		}
		pZeroedVertexBuffer = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer);
		std::vector<UBYTE> tempZeroes(size, 0);
		pZeroedVertexBuffer->upload(size, tempZeroes.data());
		currentSize = size;
	}
	return pZeroedVertexBuffer;
}

static void pie_Draw3DButton(iIMDShape *shape, PIELIGHT teamcolour, const glm::mat4 &matrix)
{
	auto* tcmask = shape->tcmaskpage != iV_TEX_INVALID ? &pie_Texture(shape->tcmaskpage) : nullptr;
	auto* normalmap = shape->normalpage != iV_TEX_INVALID ? &pie_Texture(shape->normalpage) : nullptr;
	auto* specularmap = shape->specularpage != iV_TEX_INVALID ? &pie_Texture(shape->specularpage) : nullptr;

	gfx_api::buffer* pTangentBuffer = (shape->buffers[VBO_TANGENT] != nullptr) ? shape->buffers[VBO_TANGENT] : getZeroedVertexBuffer(shape->vertexCount * 4 * sizeof(gfx_api::gfxFloat));

	const PIELIGHT colour = WZCOL_WHITE;
	gfx_api::Draw3DButtonPSO::get().bind();
	gfx_api::constant_buffer_type<SHADER_BUTTON> cbuf{
		pal_PIELIGHTtoVec4(colour), pal_PIELIGHTtoVec4(teamcolour), 0.f, tcmask ? 1 : 0, 0, normalmap != nullptr, specularmap != nullptr, 0, 0, 0.f, matrix, pie_PerspectiveGet() * matrix, glm::transpose(glm::inverse(matrix)),
		glm::vec4(0.f), glm::vec4(0.f), glm::vec4(0.f), glm::vec4(0.f), glm::vec4(0.f), glm::vec4(0.f), 0.f, 0.f, shape->buffers[VBO_TANGENT] != nullptr };
	gfx_api::Draw3DButtonPSO::get().bind_constants(cbuf);

	gfx_api::Draw3DButtonPSO::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);
	gfx_api::Draw3DButtonPSO::get().bind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
	gfx_api::context::get().bind_index_buffer(*shape->buffers[VBO_INDEX], gfx_api::index_type::u16);
	gfx_api::Draw3DButtonPSO::get().draw_elements(shape->polys.size() * 3, 0);
	polyCount += shape->polys.size();
	gfx_api::Draw3DButtonPSO::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
	gfx_api::context::get().unbind_index_buffer(*shape->buffers[VBO_INDEX]);
}

struct templatedState
{
	SHADER_MODE shader = SHADER_NONE;
	const iIMDShape * shape = nullptr;
	int pieFlag = 0;

	templatedState()
	: shader(SHADER_NONE), shape(nullptr), pieFlag(0)
	{ }

	templatedState(SHADER_MODE shader, const iIMDShape * shape, int pieFlag)
	: shader(shader), shape(shape), pieFlag(pieFlag)
	{ }

	bool operator==(const templatedState& rhs) const
	{
		return (shader == rhs.shader)
		&& (shape == rhs.shape)
		&& (pieFlag == rhs.pieFlag);
	}
	bool operator!=(const templatedState& rhs) const
	{
		return !operator==(rhs);
	}
};

template<SHADER_MODE shader, typename AdditivePSO, typename AlphaPSO, typename PremultipliedPSO, typename OpaquePSO>
static void draw3dShapeTemplated(const templatedState &lastState, const PIELIGHT &colour, const PIELIGHT &teamcolour, const float& stretch, const int& ecmState, const float& timestate, const glm::mat4 & matrix, glm::vec4 &sceneColor, glm::vec4 &ambient, glm::vec4 &diffuse, glm::vec4 &specular, const iIMDShape * shape, int pieFlag, int frame)
{
	templatedState currentState = templatedState(shader, shape, pieFlag);

	auto* tcmask = shape->tcmaskpage != iV_TEX_INVALID ? &pie_Texture(shape->tcmaskpage) : nullptr;
	auto* normalmap = shape->normalpage != iV_TEX_INVALID ? &pie_Texture(shape->normalpage) : nullptr;
	auto* specularmap = shape->specularpage != iV_TEX_INVALID ? &pie_Texture(shape->specularpage) : nullptr;

	gfx_api::constant_buffer_type<shader> cbuf{
		pal_PIELIGHTtoVec4(colour), pal_PIELIGHTtoVec4(teamcolour), stretch, tcmask ? 1 : 0, 0, normalmap != nullptr, specularmap != nullptr, ecmState, !(pieFlag & pie_PREMULTIPLIED), timestate, matrix, pie_PerspectiveGet() * matrix, glm::transpose(glm::inverse(matrix)),
		glm::vec4(currentSunPosition, 0.f), sceneColor, ambient, diffuse, specular, glm::vec4(0.f), 0.f, 0.f, shape->buffers[VBO_TANGENT] != nullptr };

	gfx_api::buffer* pTangentBuffer = (shape->buffers[VBO_TANGENT] != nullptr) ? shape->buffers[VBO_TANGENT] : getZeroedVertexBuffer(shape->vertexCount * 4 * sizeof(gfx_api::gfxFloat));

	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE)
	{
		AdditivePSO::get().bind();
		AdditivePSO::get().bind_constants(cbuf);
		if (currentState != lastState)
		{
			AdditivePSO::get().bind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
			AdditivePSO::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);
		}
		AdditivePSO::get().draw_elements(shape->polys.size() * 3, frame * shape->polys.size() * 3 * sizeof(uint16_t));
//		AdditivePSO::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD]);
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		AlphaPSO::get().bind();
		AlphaPSO::get().bind_constants(cbuf);
		if (currentState != lastState)
		{
			AlphaPSO::get().bind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
			AlphaPSO::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);
		}
		AlphaPSO::get().draw_elements(shape->polys.size() * 3, frame * shape->polys.size() * 3 * sizeof(uint16_t));
//		AlphaPSO::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD]);
	}
	else if (pieFlag & pie_PREMULTIPLIED)
	{
		PremultipliedPSO::get().bind();
		PremultipliedPSO::get().bind_constants(cbuf);
		if (currentState != lastState)
		{
			PremultipliedPSO::get().bind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
			PremultipliedPSO::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);
		}
		PremultipliedPSO::get().draw_elements(shape->polys.size() * 3, frame * shape->polys.size() * 3 * sizeof(uint16_t));
//		PremultipliedPSO::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD]);
	}
	else
	{
		OpaquePSO::get().bind();
		OpaquePSO::get().bind_constants(cbuf);
		if (currentState != lastState)
		{
			OpaquePSO::get().bind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
			OpaquePSO::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);
		}
		OpaquePSO::get().draw_elements(shape->polys.size() * 3, frame * shape->polys.size() * 3 * sizeof(uint16_t));
//		OpaquePSO::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD]);
	}
}

static templatedState pie_Draw3DShape2(const templatedState &lastState, const iIMDShape *shape, int frame, PIELIGHT colour, PIELIGHT teamcolour, int pieFlag, int pieFlagData, glm::mat4 const &matrix)
{
	bool light = true;

	/* Set fog status */
	if (!(pieFlag & pie_FORCE_FOG) && (pieFlag & pie_ADDITIVE || pieFlag & pie_TRANSLUCENT || pieFlag & pie_PREMULTIPLIED))
	{
		pie_SetFogStatus(false);
	}
	else
	{
		pie_SetFogStatus(true);
	}

	/* Set translucency */
	if (pieFlag & pie_ADDITIVE)
	{
		colour.byte.a = (UBYTE)pieFlagData;
		light = false;
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		colour.byte.a = (UBYTE)pieFlagData;
		light = false;
	}
	else if (pieFlag & pie_PREMULTIPLIED)
	{
		light = false;
	}

	if (pieFlag & pie_ECM)
	{
		light = true;
		pie_SetShaderEcmEffect(true);
	}

	glm::vec4 sceneColor(lighting0[LIGHT_EMISSIVE][0], lighting0[LIGHT_EMISSIVE][1], lighting0[LIGHT_EMISSIVE][2], lighting0[LIGHT_EMISSIVE][3]);
	glm::vec4 ambient(lighting0[LIGHT_AMBIENT][0], lighting0[LIGHT_AMBIENT][1], lighting0[LIGHT_AMBIENT][2], lighting0[LIGHT_AMBIENT][3]);
	glm::vec4 diffuse(lighting0[LIGHT_DIFFUSE][0], lighting0[LIGHT_DIFFUSE][1], lighting0[LIGHT_DIFFUSE][2], lighting0[LIGHT_DIFFUSE][3]);
	glm::vec4 specular(lighting0[LIGHT_SPECULAR][0], lighting0[LIGHT_SPECULAR][1], lighting0[LIGHT_SPECULAR][2], lighting0[LIGHT_SPECULAR][3]);

	frame %= std::max<int>(1, shape->numFrames);

	templatedState currentState = templatedState((light) ? SHADER_COMPONENT : SHADER_NOLIGHT, shape, pieFlag);
	if (currentState != lastState)
	{
		gfx_api::context::get().bind_index_buffer(*shape->buffers[VBO_INDEX], gfx_api::index_type::u16);
	}

	if (light)
	{
		draw3dShapeTemplated<SHADER_COMPONENT, gfx_api::Draw3DShapeAdditive, gfx_api::Draw3DShapeAlpha, gfx_api::Draw3DShapePremul, gfx_api::Draw3DShapeOpaque>(lastState, colour, teamcolour, pie_GetShaderStretchDepth(), pie_GetShaderEcmEffect(), pie_GetShaderTime(), matrix, sceneColor, ambient, diffuse, specular, shape, pieFlag, frame);
	}
	else
	{
		draw3dShapeTemplated<SHADER_NOLIGHT, gfx_api::Draw3DShapeNoLightAdditive, gfx_api::Draw3DShapeNoLightAlpha, gfx_api::Draw3DShapeNoLightPremul, gfx_api::Draw3DShapeNoLightOpaque>(lastState, colour, teamcolour, pie_GetShaderStretchDepth(), pie_GetShaderEcmEffect(), pie_GetShaderTime(), matrix, sceneColor, ambient, diffuse, specular, shape, pieFlag, frame);
	}

	polyCount += shape->polys.size();

	pie_SetShaderEcmEffect(false);

	return currentState;
}

static inline bool edgeLessThan(EDGE const &e1, EDGE const &e2)
{
	if (e1.from != e2.from)
	{
		return e1.from < e2.from;
	}
	return e1.to < e2.to;
}

static inline void flipEdge(EDGE &e)
{
	std::swap(e.from, e.to);
}

/// scale the height according to the flags
static inline float scale_y(float y, int flag, int flag_data)
{
	float tempY = y;
	if (flag & pie_RAISE)
	{
		tempY = y - flag_data;
		if (y - flag_data < 0)
		{
			tempY = 0;
		}
	}
	else if (flag & pie_HEIGHT_SCALED)
	{
		if (y > 0)
		{
			tempY = (y * flag_data) / pie_RAISE_SCALE;
		}
	}
	return tempY;
}

inline void hash_combine(std::size_t& seed) { }

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
	std::hash<T> hasher;
#if SIZE_MAX >= UINT64_MAX
	seed ^= hasher(v) + 0x9e3779b97f4a7c15L + (seed<<6) + (seed>>2);
#else
	seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
#endif
	hash_combine(seed, rest...);
}

std::size_t hash_vec4(const glm::vec4& vec)
{
	std::size_t h=0;
	hash_combine(h, vec[0], vec[1], vec[2], vec[3]);
	return h;
}

struct ShadowDrawParameters {
	int flag;
	int flag_data;
	//		glm::mat4 modelViewMatrix; // the modelViewMatrix doesn't change any of the vertex / buffer calculations
	glm::vec4 light;

	ShadowDrawParameters(int flag, int flag_data, const glm::vec4 &light)
	: flag(flag)
	, flag_data(flag_data)
	, light(light)
	{ }

	bool operator ==(const ShadowDrawParameters &b) const
	{
		return (flag == b.flag) && (flag_data == b.flag_data) && (light == b.light);
	}
};

namespace std {
	template <>
	struct hash<ShadowDrawParameters>
	{
		std::size_t operator()(const ShadowDrawParameters& k) const
		{
			std::size_t h = 0;
			hash_combine(h, k.flag, k.flag_data, hash_vec4(k.light));
			return h;
		}
	};
}

struct ShadowCache {

	struct CachedShadowData {
		uint64_t lastQueriedFrameCount = 0;
		std::vector<Vector3f> vertexes;

		CachedShadowData() { }
	};

	typedef std::unordered_map<ShadowDrawParameters, CachedShadowData> ShadowDrawParametersToCachedDataMap;
	typedef std::unordered_map<iIMDShape *, ShadowDrawParametersToCachedDataMap, std::hash<iIMDShape *>> ShapeMap;

	const CachedShadowData* findCacheForShadowDraw(iIMDShape *shape, int flag, int flag_data, const glm::vec4 &light)
	{
		auto it = shapeMap.find(shape);
		if (it == shapeMap.end()) {
			return nullptr;
		}
		auto it_cachedData = it->second.find(ShadowDrawParameters(flag, flag_data, light));
		if (it_cachedData == it->second.end()) {
			return nullptr;
		}
		// update the frame in which we requested this shadow cache
		it_cachedData->second.lastQueriedFrameCount = _currentFrame;
		return &(it_cachedData->second);
	}

	CachedShadowData& createCacheForShadowDraw(iIMDShape *shape, int flag, int flag_data, const glm::vec4 &light)
	{
		auto result = shapeMap[shape].emplace(ShadowDrawParameters(flag, flag_data, light), CachedShadowData());
		result.first->second.lastQueriedFrameCount = _currentFrame;
		return result.first->second;
	}

	void addPremultipliedVertexes(const CachedShadowData& cachedData, const glm::mat4 &modelViewMatrix)
	{
		for (auto &vertex : cachedData.vertexes)
		{
			vertexes.emplace_back(modelViewMatrix * glm::vec4(vertex, 1.0));
		}
	}

	const std::vector<Vector3f>& getPremultipliedVertexes()
	{
		return vertexes;
	}

	void clearPremultipliedVertexes()
	{
		vertexes.clear();
	}

	void setCurrentFrame(uint64_t currentFrame)
	{
		_currentFrame = currentFrame;
	}

	size_t removeUnused()
	{
		std::vector<ShapeMap::iterator> unusedShapes;
		size_t oldItemsRemoved = 0;
		for (auto it_shape = shapeMap.begin(); it_shape != shapeMap.end(); ++it_shape)
		{
			std::vector<ShadowDrawParametersToCachedDataMap::iterator> unusedBuffersForShape;
			for (auto it_shadowDrawParams = it_shape->second.begin(); it_shadowDrawParams != it_shape->second.end(); ++it_shadowDrawParams)
			{
				if (it_shadowDrawParams->second.lastQueriedFrameCount != _currentFrame)
				{
					unusedBuffersForShape.push_back(it_shadowDrawParams);
				}
			}
			for (auto &item : unusedBuffersForShape)
			{
				it_shape->second.erase(item);
				++oldItemsRemoved;
			}
			if (it_shape->second.empty())
			{
				// remove from the root shapeMap
				unusedShapes.push_back(it_shape);
			}
		}
		for (auto &item : unusedShapes)
		{
			shapeMap.erase(item);
		}
		return oldItemsRemoved;
	}
private:
	uint64_t _currentFrame = 0;
	ShapeMap shapeMap;
	std::vector<Vector3f> vertexes;
};

enum DrawShadowResult {
	DRAW_SUCCESS_CACHED,
	DRAW_SUCCESS_UNCACHED
};

/// Draw the shadow for a shape
/// Prequisite:
///		Caller must call the following before all calls to pie_DrawShadow():
///			const auto &program = pie_ActivateShader(SHADER_GENERIC_COLOR, pie_PerspectiveGet(), glm::vec4());
///			glEnableVertexAttribArray(program.locVertex);
///		and must call the following after all calls to pie_DrawShadow():
///			glDisableVertexAttribArray(program.locVertex);
///			pie_DeactivateShader();
///		The only place this is currently called is pie_ShadowDrawLoop(), which handles this properly.
static inline DrawShadowResult pie_DrawShadow(ShadowCache &shadowCache, iIMDShape *shape, int flag, int flag_data, const glm::vec4 &light, const glm::mat4 &modelViewMatrix)
{
	static std::vector<EDGE> edgelist;  // Static, to save allocations.
	static std::vector<EDGE> edgelistFlipped;  // Static, to save allocations.
	static std::vector<EDGE> edgelistFiltered;  // Static, to save allocations.
	EDGE *drawlist = nullptr;

	size_t edge_count;
	DrawShadowResult result;

	// Find cached data (if available)
	// Note: The modelViewMatrix is not used for calculating the sorted / filtered vertices, so it's not included
	const ShadowCache::CachedShadowData *pCached = shadowCache.findCacheForShadowDraw(shape, flag, flag_data, light);
	if (pCached == nullptr)
	{
		const Vector3f *pVertices = shape->points.data();
		if (flag & pie_STATIC_SHADOW && shape->shadowEdgeList)
		{
			drawlist = shape->shadowEdgeList;
			edge_count = shape->nShadowEdges;
		}
		else
		{
			edgelist.clear();
			glm::vec3 p[3];
			for (const iIMDPoly &poly : shape->polys)
			{
				for (int j = 0; j < 3; ++j)
				{
					int current = poly.pindex[j];
					p[j] = glm::vec3(pVertices[current].x, scale_y(pVertices[current].y, flag, flag_data), pVertices[current].z);
				}
				if (glm::dot(glm::cross(p[2] - p[0], p[1] - p[0]), glm::vec3(light)) > 0.0f)
				{
					for (int n = 0; n < 3; ++n)
					{
						// Add the edges
						edgelist.push_back({poly.pindex[n], poly.pindex[(n + 1)%3]});
					}
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

			if (flag & pie_STATIC_SHADOW)
			{
				// then store it in the imd
				shape->nShadowEdges = edge_count;
				shape->shadowEdgeList = (EDGE *)realloc(shape->shadowEdgeList, sizeof(EDGE) * shape->nShadowEdges);
				std::copy(drawlist, drawlist + edge_count, shape->shadowEdgeList);
			}
		}

		static std::vector<Vector3f> vertexes;
		vertexes.clear();
		vertexes.reserve(edge_count * 6);
		for (size_t i = 0; i < edge_count; i++)
		{
			int a = drawlist[i].from, b = drawlist[i].to;

			glm::vec3 v1(pVertices[b].x, scale_y(pVertices[b].y, flag, flag_data), pVertices[b].z);
			glm::vec3 v3(pVertices[a].x + light[0], scale_y(pVertices[a].y, flag, flag_data) + light[1], pVertices[a].z + light[2]);

			vertexes.push_back(v1);
			vertexes.push_back(glm::vec3(pVertices[b].x + light[0], scale_y(pVertices[b].y, flag, flag_data) + light[1], pVertices[b].z + light[2])); //v2
			vertexes.push_back(v3);

			vertexes.push_back(v3);
			vertexes.push_back(glm::vec3(pVertices[a].x, scale_y(pVertices[a].y, flag, flag_data), pVertices[a].z)); //v4
			vertexes.push_back(v1);
		}

		ShadowCache::CachedShadowData& cache = shadowCache.createCacheForShadowDraw(shape, flag, flag_data, light);
		cache.vertexes = vertexes;
		result = DRAW_SUCCESS_UNCACHED;
		pCached = &cache;
	}
	else
	{
		result = DRAW_SUCCESS_CACHED;
	}

	// Aggregate the vertexes (pre-computed with the modelViewMatrix)
	shadowCache.addPremultipliedVertexes(*pCached, modelViewMatrix);

	return result;
}

void pie_CleanUp()
{
	tshapes.clear();
	shapes.clear();
	scshapes.clear();
	if (pZeroedVertexBuffer)
	{
		delete pZeroedVertexBuffer;
		pZeroedVertexBuffer = nullptr;
	}
}

bool pie_Draw3DShape(iIMDShape *shape, int frame, int team, PIELIGHT colour, int pieFlag, int pieFlagData, const glm::mat4 &modelView)
{
	pieCount++;

	ASSERT(frame >= 0, "Negative frame %d", frame);
	ASSERT(team >= 0, "Negative team %d", team);

	const PIELIGHT teamcolour = pal_GetTeamColour(team);
	if (pieFlag & pie_BUTTON)
	{
		pie_Draw3DButton(shape, teamcolour, modelView);
	}
	else
	{
		SHAPE tshape;
		tshape.shape = shape;
		tshape.frame = frame;
		tshape.colour = colour;
		tshape.teamcolour = teamcolour;
		tshape.flag = pieFlag;
		tshape.flag_data = pieFlagData;
		tshape.stretch = pie_GetShaderStretchDepth();
		tshape.matrix = modelView;

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
				scshape.matrix = modelView;
				distance = scshape.matrix[3][0] * scshape.matrix[3][0];
				distance += scshape.matrix[3][1] * scshape.matrix[3][1];
				distance += scshape.matrix[3][2] * scshape.matrix[3][2];

				// if object is too far in the fog don't generate a shadow.
				if (distance < SHADOW_END_DISTANCE)
				{
					// Calculate the light position relative to the object
					glm::vec4 pos_light0 = glm::vec4(currentSunPosition, 0.f);
					glm::mat4 invmat = glm::inverse(scshape.matrix);

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

	return true;
}

static void pie_ShadowDrawLoop(ShadowCache &shadowCache)
{
	size_t cachedShadowDraws = 0;
	size_t uncachedShadowDraws = 0;
	for (unsigned i = 0; i < scshapes.size(); i++)
	{
		DrawShadowResult result = pie_DrawShadow(shadowCache, scshapes[i].shape, scshapes[i].flag, scshapes[i].flag_data, scshapes[i].light, scshapes[i].matrix);
		if (result == DRAW_SUCCESS_CACHED)
		{
			++cachedShadowDraws;
		}
		else
		{
			++uncachedShadowDraws;
		}
	}

	const auto &premultipliedVertexes = shadowCache.getPremultipliedVertexes();
	if (premultipliedVertexes.size() > 0)
	{
		// Draw the shadow volume
		gfx_api::DrawStencilShadow::get().bind();
		// The vertexes returned by shadowCache.getPremultipliedVertexes() are pre-multiplied by the modelViewMatrix
		// Thus we only need to include the perspective matrix
		gfx_api::DrawStencilShadow::get().bind_constants({ pie_PerspectiveGet(), glm::vec2(0.f), glm::vec2(0.f), glm::vec4(0.f) });
		gfx_api::context::get().bind_streamed_vertex_buffers(premultipliedVertexes.data(), sizeof(Vector3f) * premultipliedVertexes.size());

		// Batch into glDrawArrays calls of <= SHADOW_BATCH_MAX
		static const size_t SHADOW_BATCH_MAX = 8192 * 3; // must be divisible by 3
		size_t vertex_count = premultipliedVertexes.size();
		for (size_t startingIndex = 0; startingIndex < vertex_count; startingIndex += SHADOW_BATCH_MAX)
		{
			gfx_api::DrawStencilShadow::get().draw(std::min(vertex_count - startingIndex, SHADOW_BATCH_MAX), startingIndex);
		}

		gfx_api::context::get().disable_all_vertex_buffers();
	}

	shadowCache.clearPremultipliedVertexes();

//	debug(LOG_INFO, "Cached shadow draws: %lu, uncached shadow draws: %lu", cachedShadowDraws, uncachedShadowDraws);
}

static ShadowCache shadowCache;

static void pie_DrawShadows(uint64_t currentGameFrame)
{
	const float width = pie_GetVideoBufferWidth();
	const float height = pie_GetVideoBufferHeight();
	shadowCache.setCurrentFrame(currentGameFrame);

	pie_ShadowDrawLoop(shadowCache);

	PIELIGHT grey;
	grey.byte = { 0, 0, 0, 128 };
	pie_BoxFill_alpha(0, 0, width, height, grey);

	scshapes.resize(0);
	shadowCache.removeUnused();
}

struct less_than_shape
{
	inline bool operator() (const SHAPE& shape1, const SHAPE& shape2)
	{
		return (shape1.shape < shape2.shape);
	}
};

void pie_RemainingPasses(uint64_t currentGameFrame)
{
	// Draw models
	// sort list to reduce state changes
	std::sort(shapes.begin(), shapes.end(), less_than_shape());
	gfx_api::context::get().debugStringMarker("Remaining passes - opaque models");
	templatedState lastState;
	for (SHAPE const &shape : shapes)
	{
		pie_SetShaderStretchDepth(shape.stretch);
		lastState = pie_Draw3DShape2(lastState, shape.shape, shape.frame, shape.colour, shape.teamcolour, shape.flag, shape.flag_data, shape.matrix);
	}
	gfx_api::context::get().disable_all_vertex_buffers();
	if (!shapes.empty())
	{
		// unbind last index buffer bound inside pie_Draw3DShape2
		gfx_api::context::get().unbind_index_buffer(*((shapes.back().shape)->buffers[VBO_INDEX]));
	}
	gfx_api::context::get().debugStringMarker("Remaining passes - shadows");
	// Draw shadows
	if (shadows)
	{
		pie_DrawShadows(currentGameFrame);
	}
	// Draw translucent models last
	// TODO, sort list by Z order to do translucency correctly
	gfx_api::context::get().debugStringMarker("Remaining passes - translucent models");
	lastState = templatedState();
	for (SHAPE const &shape : tshapes)
	{
		pie_SetShaderStretchDepth(shape.stretch);
		lastState = pie_Draw3DShape2(lastState, shape.shape, shape.frame, shape.colour, shape.teamcolour, shape.flag, shape.flag_data, shape.matrix);
	}
	gfx_api::context::get().disable_all_vertex_buffers();
	if (!tshapes.empty())
	{
		// unbind last index buffer bound inside pie_Draw3DShape2
		gfx_api::context::get().unbind_index_buffer(*((tshapes.back().shape)->buffers[VBO_INDEX]));
	}
	pie_SetShaderStretchDepth(0);
	tshapes.clear();
	shapes.clear();
	gfx_api::context::get().debugStringMarker("Remaining passes - done");
}

void pie_GetResetCounts(size_t *pPieCount, size_t *pPolyCount)
{
	*pPieCount  = pieCount;
	*pPolyCount = polyCount;

	pieCount = 0;
	polyCount = 0;
}
