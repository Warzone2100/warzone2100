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
#include <unordered_set>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#endif
#include "3rdparty/ska_sort.hpp"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#define BUFFER_OFFSET(i) (reinterpret_cast<char *>(i))
#define SHADOW_END_DISTANCE (8000*8000) // Keep in sync with lighting.c:FOG_END

/*
 *	Local Variables
 */

static size_t pieCount = 0;
static size_t polyCount = 0;
static size_t drawCallsCount = 0;
static bool shadows = false;
static gfx_api::gfxFloat lighting0[LIGHT_MAX][4];
static gfx_api::gfxFloat lightingDefault[LIGHT_MAX][4];

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
	memcpy(lightingDefault, defaultLight, sizeof(lightingDefault));
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

void pie_Draw3DButton(iIMDShape *shape, PIELIGHT teamcolour, const glm::mat4 &matrix)
{
	auto* tcmask = shape->tcmaskpage != iV_TEX_INVALID ? &pie_Texture(shape->tcmaskpage) : nullptr;
	auto* normalmap = shape->normalpage != iV_TEX_INVALID ? &pie_Texture(shape->normalpage) : nullptr;
	auto* specularmap = shape->specularpage != iV_TEX_INVALID ? &pie_Texture(shape->specularpage) : nullptr;

	gfx_api::buffer* pTangentBuffer = (shape->buffers[VBO_TANGENT] != nullptr) ? shape->buffers[VBO_TANGENT] : getZeroedVertexBuffer(shape->vertexCount * 4 * sizeof(gfx_api::gfxFloat));

	const PIELIGHT colour = WZCOL_WHITE;
	glm::vec4 sceneColor(lightingDefault[LIGHT_EMISSIVE][0], lightingDefault[LIGHT_EMISSIVE][1], lightingDefault[LIGHT_EMISSIVE][2], lightingDefault[LIGHT_EMISSIVE][3]);
	glm::vec4 ambient(lightingDefault[LIGHT_AMBIENT][0], lightingDefault[LIGHT_AMBIENT][1], lightingDefault[LIGHT_AMBIENT][2], lightingDefault[LIGHT_AMBIENT][3]);
	glm::vec4 diffuse(lightingDefault[LIGHT_DIFFUSE][0], lightingDefault[LIGHT_DIFFUSE][1], lightingDefault[LIGHT_DIFFUSE][2], lightingDefault[LIGHT_DIFFUSE][3]);
	glm::vec4 specular(lightingDefault[LIGHT_SPECULAR][0], lightingDefault[LIGHT_SPECULAR][1], lightingDefault[LIGHT_SPECULAR][2], lightingDefault[LIGHT_SPECULAR][3]);
	gfx_api::Draw3DShapeOpaque::get().bind();

	gfx_api::Draw3DShapeGlobalUniforms globalUniforms {
		pie_PerspectiveGet(),
		glm::vec4(getDefaultSunPosition(), 0.f),
		sceneColor, ambient, diffuse, specular, glm::vec4(0.f),
		0.f, 0.f, 0.f, 0
	};

	gfx_api::Draw3DShapePerMeshUniforms meshUniforms {
		tcmask ? 1 : 0, normalmap != nullptr, specularmap != nullptr, shape->buffers[VBO_TANGENT] != nullptr
	};

	gfx_api::Draw3DShapePerInstanceUniforms instanceUniforms {
		matrix,
		glm::transpose(glm::inverse(matrix)),
		pal_PIELIGHTtoVec4(colour), pal_PIELIGHTtoVec4(teamcolour),
		0.f, 0.f, 0, !(shape->flags & pie_PREMULTIPLIED)
	};

	gfx_api::Draw3DShapeOpaque::get().set_uniforms(globalUniforms, meshUniforms, instanceUniforms);

	gfx_api::Draw3DShapeOpaque::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);
	gfx_api::Draw3DShapeOpaque::get().bind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
	gfx_api::context::get().bind_index_buffer(*shape->buffers[VBO_INDEX], gfx_api::index_type::u16);
	gfx_api::Draw3DShapeOpaque::get().draw_elements(shape->polys.size() * 3, 0);
	polyCount += shape->polys.size();
	gfx_api::Draw3DShapeOpaque::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
	gfx_api::context::get().unbind_index_buffer(*shape->buffers[VBO_INDEX]);
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

namespace std {
	template <> struct hash<templatedState>
	{
		size_t operator()(const templatedState & x) const
		{
			std::size_t h = 0;
			hash_combine(h, static_cast<size_t>(x.shader), x.shape, x.pieFlag);
			return h;
		}
	};
}

// Ensure the uniforms are set for a shader once until reset
struct ShaderOnce
{
public:
	template<typename T>
	void perform_once(const std::function<void (void)>& func)
	{
		auto type = std::type_index(typeid(T));
		if (performed_once.count(type) > 0)
		{
			return;
		}
		func();
		performed_once.insert(type);
	}

	void reset()
	{
		performed_once.clear();
	}

private:
	std::unordered_set<std::type_index> performed_once;
};

template<SHADER_MODE shader, typename AdditivePSO, typename AlphaPSO, typename AlphaNoDepthWRTPSO, typename PremultipliedPSO, typename OpaquePSO>
static void draw3dShapeTemplated(const templatedState &lastState, ShaderOnce& globalsOnce, const gfx_api::Draw3DShapeGlobalUniforms& globalUniforms, const PIELIGHT &colour, const PIELIGHT &teamcolour, const float& stretch, const int& ecmState, const glm::mat4 & matrix, const iIMDShape * shape, int pieFlag, int frame)
{
	templatedState currentState = templatedState(shader, shape, pieFlag);

	auto* tcmask = shape->tcmaskpage != iV_TEX_INVALID ? &pie_Texture(shape->tcmaskpage) : nullptr;
	auto* normalmap = shape->normalpage != iV_TEX_INVALID ? &pie_Texture(shape->normalpage) : nullptr;
	auto* specularmap = shape->specularpage != iV_TEX_INVALID ? &pie_Texture(shape->specularpage) : nullptr;

	gfx_api::Draw3DShapePerMeshUniforms meshUniforms {
		tcmask ? 1 : 0, normalmap != nullptr, specularmap != nullptr, shape->buffers[VBO_TANGENT] != nullptr
	};

	gfx_api::Draw3DShapePerInstanceUniforms instanceUniforms {
		matrix,
		glm::transpose(glm::inverse(matrix)),
		pal_PIELIGHTtoVec4(colour), pal_PIELIGHTtoVec4(teamcolour),
		stretch, float(frame), ecmState, !(pieFlag & pie_PREMULTIPLIED)
	};

	gfx_api::buffer* pTangentBuffer = (shape->buffers[VBO_TANGENT] != nullptr) ? shape->buffers[VBO_TANGENT] : getZeroedVertexBuffer(shape->vertexCount * 4 * sizeof(gfx_api::gfxFloat));

	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE)
	{
		AdditivePSO::get().bind();
		globalsOnce.perform_once<AdditivePSO>([&globalUniforms]{
			AdditivePSO::get().set_uniforms_at(0, globalUniforms);
		});
		if (currentState != lastState)
		{
			AdditivePSO::get().set_uniforms_at(1, meshUniforms);
			AdditivePSO::get().bind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
			AdditivePSO::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);
		}
		AdditivePSO::get().set_uniforms_at(2, instanceUniforms);
		AdditivePSO::get().draw_elements(shape->polys.size() * 3, 0);
//		AdditivePSO::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD]);
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		if (!(pieFlag & pie_NODEPTHWRITE))
		{
			AlphaPSO::get().bind();
			globalsOnce.perform_once<AlphaPSO>([&globalUniforms]{
				AlphaPSO::get().set_uniforms_at(0, globalUniforms);
			});
			if (currentState != lastState)
			{
				AlphaPSO::get().set_uniforms_at(1, meshUniforms);
				AlphaPSO::get().bind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
				AlphaPSO::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);
			}
			AlphaPSO::get().set_uniforms_at(2, instanceUniforms);
			AlphaPSO::get().draw_elements(shape->polys.size() * 3, 0);
	//		AlphaPSO::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD]);
		}
		else
		{
			AlphaNoDepthWRTPSO::get().bind();
			globalsOnce.perform_once<AlphaNoDepthWRTPSO>([&globalUniforms]{
				AlphaNoDepthWRTPSO::get().set_uniforms_at(0, globalUniforms);
			});
			if (currentState != lastState)
			{
				AlphaNoDepthWRTPSO::get().set_uniforms_at(1, meshUniforms);
				AlphaNoDepthWRTPSO::get().bind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
				AlphaNoDepthWRTPSO::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);
			}
			AlphaNoDepthWRTPSO::get().set_uniforms_at(2, instanceUniforms);
			AlphaNoDepthWRTPSO::get().draw_elements(shape->polys.size() * 3, 0);
	//		AlphaPSO::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD]);
		}
	}
	else if (pieFlag & pie_PREMULTIPLIED)
	{
		PremultipliedPSO::get().bind();
		globalsOnce.perform_once<PremultipliedPSO>([&globalUniforms]{
			PremultipliedPSO::get().set_uniforms_at(0, globalUniforms);
		});
		if (currentState != lastState)
		{
			PremultipliedPSO::get().set_uniforms_at(1, meshUniforms);
			PremultipliedPSO::get().bind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
			PremultipliedPSO::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);
		}
		PremultipliedPSO::get().set_uniforms_at(2, instanceUniforms);
		PremultipliedPSO::get().draw_elements(shape->polys.size() * 3, 0);
//		PremultipliedPSO::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD]);
	}
	else
	{
		OpaquePSO::get().bind();
		globalsOnce.perform_once<OpaquePSO>([&globalUniforms]{
			OpaquePSO::get().set_uniforms_at(0, globalUniforms);
		});
		if (currentState != lastState)
		{
			OpaquePSO::get().set_uniforms_at(1, meshUniforms);
			OpaquePSO::get().bind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD], pTangentBuffer);
			OpaquePSO::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);
		}
		OpaquePSO::get().set_uniforms_at(2, instanceUniforms);
		OpaquePSO::get().draw_elements(shape->polys.size() * 3, 0);
//		OpaquePSO::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD]);
	}
}

static inline gfx_api::Draw3DShapePerInstanceInterleavedData GenerateInstanceData(int frame, PIELIGHT colour, PIELIGHT teamcolour, int pieFlag, int pieFlagData, glm::mat4 const &matrix, float stretchDepth)
{
	int ecmState = (pieFlag & pie_ECM) ? 1 : 0;

	/* Set translucency */
	if (pieFlag & pie_ADDITIVE)
	{
		colour.byte.a = (UBYTE)pieFlagData;
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		colour.byte.a = (UBYTE)pieFlagData;
	}

	return gfx_api::Draw3DShapePerInstanceInterleavedData {
		matrix,
		glm::vec4(stretchDepth, ecmState, !(pieFlag & pie_PREMULTIPLIED), float(frame)),
		colour.rgba, teamcolour.rgba
	};
}

static templatedState pie_Draw3DShape2(const templatedState &lastState, ShaderOnce& globalsOnce, const gfx_api::Draw3DShapeGlobalUniforms& globalUniforms, const iIMDShape *shape, int frame, PIELIGHT colour, PIELIGHT teamcolour, int pieFlag, int pieFlagData, glm::mat4 const &matrix, float stretchDepth)
{
	bool light = true;
	int ecmState = 0;

	++drawCallsCount;

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
		ecmState = 1;
	}

	frame %= std::max<int>(1, shape->numFrames);

	templatedState currentState = templatedState((light) ? SHADER_COMPONENT : SHADER_NOLIGHT, shape, pieFlag);
	if (currentState != lastState)
	{
		gfx_api::context::get().bind_index_buffer(*shape->buffers[VBO_INDEX], gfx_api::index_type::u16);
	}

	if (light)
	{
		draw3dShapeTemplated<SHADER_COMPONENT, gfx_api::Draw3DShapeAdditive, gfx_api::Draw3DShapeAlpha, gfx_api::Draw3DShapeAlphaNoDepthWRT, gfx_api::Draw3DShapePremul, gfx_api::Draw3DShapeOpaque>(lastState, globalsOnce, globalUniforms, colour, teamcolour, stretchDepth, ecmState, matrix, shape, pieFlag, frame);
	}
	else
	{
		draw3dShapeTemplated<SHADER_NOLIGHT, gfx_api::Draw3DShapeNoLightAdditive, gfx_api::Draw3DShapeNoLightAlpha, gfx_api::Draw3DShapeNoLightAlphaNoDepthWRT, gfx_api::Draw3DShapeNoLightPremul, gfx_api::Draw3DShapeNoLightOpaque>(lastState, globalsOnce, globalUniforms, colour, teamcolour, stretchDepth, ecmState, matrix, shape, pieFlag, frame);
	}

	polyCount += shape->polys.size();

	return currentState;
}

static inline bool edgeLessThan(EDGE const &e1, EDGE const &e2)
{
	return e1.sort_key < e2.sort_key;
}

static inline uint64_t edgeSortKey(uint32_t from, uint32_t to)
{
	return static_cast<uint64_t>(from) << 32 | to;
}

static inline void flipEdge(EDGE &e)
{
	std::swap(e.from, e.to);
	e.sort_key = edgeSortKey(e.from, e.to);
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
		float mat_a = modelViewMatrix[0].x;
		float mat_b = modelViewMatrix[1].x;
		float mat_c = modelViewMatrix[2].x;
		float mat_d = modelViewMatrix[3].x;
		float mat_e = modelViewMatrix[0].y;
		float mat_f = modelViewMatrix[1].y;
		float mat_g = modelViewMatrix[2].y;
		float mat_h = modelViewMatrix[3].y;
		float mat_i = modelViewMatrix[0].z;
		float mat_j = modelViewMatrix[1].z;
		float mat_k = modelViewMatrix[2].z;
		float mat_l = modelViewMatrix[3].z;
		float premult_x;
		float premult_y;
		float premult_z;
		for (auto &vertex : cachedData.vertexes)
		{
			premult_x = vertex.x*mat_a + vertex.y*mat_b + vertex.z*mat_c + mat_d;
			premult_y = vertex.x*mat_e + vertex.y*mat_f + vertex.z*mat_g + mat_h;
			premult_z = vertex.x*mat_i + vertex.y*mat_j + vertex.z*mat_k + mat_l;
			vertexes.push_back(Vector3f(premult_x, premult_y, premult_z));
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
		const Vector3f *pVertices = shape->pShadowPoints->data();
		if (flag & pie_STATIC_SHADOW && shape->shadowEdgeList)
		{
			drawlist = shape->shadowEdgeList;
			edge_count = shape->nShadowEdges;
		}
		else
		{
			edgelist.clear();
			glm::vec3 p[3];
			for (const iIMDPoly &poly : *(shape->pShadowPolys))
			{
				for (int j = 0; j < 3; ++j)
				{
					uint32_t current = poly.pindex[j];
					p[j] = glm::vec3(pVertices[current].x, scale_y(pVertices[current].y, flag, flag_data), pVertices[current].z);
				}
				if (glm::dot(glm::cross(p[2] - p[0], p[1] - p[0]), glm::vec3(light)) > 0.0f)
				{
					for (int n = 0; n < 3; ++n)
					{
						// Add the edges
						uint32_t from = poly.pindex[n];
						uint32_t to = poly.pindex[(n + 1)%3];
						edgelist.push_back({from, to, edgeSortKey(from, to)});
					}
				}
			}

			// Remove duplicate pairs from the edge list. For example, in the list ((1 2), (2 6), (6 2), (3, 4)), remove (2 6) and (6 2).
			edgelistFlipped = edgelist;
			std::for_each(edgelistFlipped.begin(), edgelistFlipped.end(), flipEdge);
			ska_sort(edgelist.begin(), edgelist.end(), [](const EDGE & edge)
			{
				return edge.sort_key;
			});
			ska_sort(edgelistFlipped.begin(), edgelistFlipped.end(), [](const EDGE & edge)
			{
				return edge.sort_key;
			});
			edgelistFiltered.resize(edgelist.size());
			edgelistFiltered.erase(std::set_difference(edgelist.begin(), edgelist.end(), edgelistFlipped.begin(), edgelistFlipped.end(), edgelistFiltered.begin(), edgeLessThan), edgelistFiltered.end());

			edge_count = edgelistFiltered.size();
			drawlist = edgelistFiltered.data();
			//debug(LOG_WARNING, "we have %i edges", edge_count);

			if (flag & pie_STATIC_SHADOW)
			{
				// then store it in the imd
				shape->nShadowEdges = edge_count;
				if (edge_count > 0)
				{
					shape->shadowEdgeList = (EDGE *)realloc(shape->shadowEdgeList, sizeof(EDGE) * shape->nShadowEdges);
					std::copy(drawlist, drawlist + edge_count, shape->shadowEdgeList);
				}
			}
		}

		std::vector<Vector3f> vertexes;
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
		cache.vertexes = std::move(vertexes);
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

class InstancedMeshRenderer
{
public:
	bool Draw3DShape(iIMDShape *shape, int frame, PIELIGHT teamcolour, PIELIGHT colour, int pieFlag, int pieFlagData, const glm::mat4 &modelView, float stretchDepth);
	bool DrawAll(uint64_t currentGameFrame);
public:
	// New, instanced rendering
	void Draw3DShapes_Instanced(uint64_t currentGameFrame, ShaderOnce& globalsOnce, const gfx_api::Draw3DShapeGlobalUniforms& globalUniforms);
	// Old, non-instanced rendering
	void Draw3DShapes_Old(uint64_t currentGameFrame, ShaderOnce& globalsOnce, const gfx_api::Draw3DShapeGlobalUniforms& globalUniforms);
	templatedState Draw3DShapeInstance_Old(const templatedState &lastState, ShaderOnce& globalsOnce, const gfx_api::Draw3DShapeGlobalUniforms& globalUniforms, const iIMDShape *shape, int frame, PIELIGHT colour, PIELIGHT teamcolour, int pieFlag, int pieFlagData, glm::mat4 const &matrix, float stretchDepth);
public:
	bool initialize();
	void clear();
	void reset();
private:
	bool useInstancedRendering = false;

	typedef templatedState MeshInstanceKey;
	std::unordered_map<MeshInstanceKey, std::vector<SHAPE>> instanceMeshes;
	std::unordered_map<MeshInstanceKey, std::vector<SHAPE>> instanceTranslucentMeshes;
	size_t instancesCount = 0;
	size_t translucentInstancesCount = 0;

	std::vector<SHAPE> tshapes;
	std::vector<SHAPE> shapes;

	std::vector<gfx_api::Draw3DShapePerInstanceInterleavedData> instancesData;
	std::vector<gfx_api::buffer*> instanceDataBuffers;
	size_t currInstanceBufferIdx = 0;
};

bool InstancedMeshRenderer::initialize()
{
	reset();

	if (!gfx_api::context::get().supportsInstancedRendering())
	{
		useInstancedRendering = false;
		return true;
	}

	// check for minimum required vertex attributes (and vertex outputs) for the instanced shaders
	int32_t max_vertex_attribs = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_VERTEX_ATTRIBS);
	int32_t max_vertex_output_components = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_VERTEX_OUTPUT_COMPONENTS);
	size_t maxInstancedMeshShaderVertexAttribs = std::max({gfx_api::instance_modelMatrix, gfx_api::instance_packedValues, gfx_api::instance_Colour, gfx_api::instance_TeamColour}) + 1;
	constexpr size_t min_required_vertex_output_components = 11 * 4; // from the shaders - see tcmask_instanced.vert / nolight_instanced.vert
	if (max_vertex_attribs < maxInstancedMeshShaderVertexAttribs)
	{
		debug(LOG_INFO, "Disabling instanced rendering - Insufficient MAX_VERTEX_ATTRIBS (%" PRIi32 "; need: %zu)", max_vertex_attribs, maxInstancedMeshShaderVertexAttribs);
		useInstancedRendering = false;
		return true;
	}
	if (max_vertex_output_components < min_required_vertex_output_components)
	{
		debug(LOG_INFO, "Disabling instanced rendering - Insufficient MAX_VERTEX_OUTPUT_COMPONENTS (%" PRIi32 "; need: %zu)", max_vertex_output_components, min_required_vertex_output_components);
		useInstancedRendering = false;
		return true;
	}

	instanceDataBuffers.resize(gfx_api::context::get().maxFramesInFlight() + 1);
	for (size_t i = 0; i < instanceDataBuffers.size(); ++i)
	{
		instanceDataBuffers[i] = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::stream_draw);
	}
	useInstancedRendering = true;
	return true;
}

void InstancedMeshRenderer::clear()
{
	instanceMeshes.clear();
	instanceTranslucentMeshes.clear();
	instancesCount = 0;
	translucentInstancesCount = 0;
	tshapes.clear();
	shapes.clear();
}

void InstancedMeshRenderer::reset()
{
	clear();
	for (auto buffer : instanceDataBuffers)
	{
		delete buffer;
	}
	instanceDataBuffers.clear();
}

bool InstancedMeshRenderer::Draw3DShape(iIMDShape *shape, int frame, PIELIGHT teamcolour, PIELIGHT colour, int pieFlag, int pieFlagData, const glm::mat4 &modelView, float stretchDepth)
{
	bool light = true;

	if (pieFlag & pie_ADDITIVE)
	{
		light = false;
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		light = false;
	}
	else if (pieFlag & pie_PREMULTIPLIED)
	{
		light = false;
	}

	if (pieFlag & pie_ECM)
	{
		light = true;
	}

	frame %= std::max<int>(1, shape->numFrames);

	templatedState currentState = templatedState((light) ? ((useInstancedRendering) ? SHADER_COMPONENT_INSTANCED : SHADER_COMPONENT) : ((useInstancedRendering) ? SHADER_NOLIGHT_INSTANCED : SHADER_NOLIGHT), shape, pieFlag);

	SHAPE tshape;
	tshape.shape = shape;
	tshape.frame = frame;
	tshape.colour = colour;
	tshape.teamcolour = teamcolour;
	tshape.flag = pieFlag;
	tshape.flag_data = pieFlagData;
	tshape.stretch = stretchDepth;
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
		if (useInstancedRendering)
		{
			instanceTranslucentMeshes[currentState].push_back(tshape);
		}
		else
		{
			tshapes.push_back(tshape);
		}
		++translucentInstancesCount;
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
		if (useInstancedRendering)
		{
			instanceMeshes[currentState].push_back(tshape);
		}
		else
		{
			shapes.push_back(tshape);
		}

		++instancesCount;
	}

	return true;
}

static InstancedMeshRenderer instancedMeshRenderer;

void pie_InitializeInstancedRenderer()
{
	instancedMeshRenderer.initialize();
}

void pie_CleanUp()
{
	instancedMeshRenderer.reset();
	scshapes.clear();
	if (pZeroedVertexBuffer)
	{
		delete pZeroedVertexBuffer;
		pZeroedVertexBuffer = nullptr;
	}
}

bool pie_Draw3DShape(iIMDShape *shape, int frame, int team, PIELIGHT colour, int pieFlag, int pieFlagData, const glm::mat4 &modelView, float stretchDepth, bool onlySingleLevel)
{
	pieCount++;

	ASSERT(frame >= 0, "Negative frame %d", frame);
	ASSERT(team >= 0, "Negative team %d", team);

	bool retVal = false;
	const bool drawAllLevels = (shape->modelLevel == 0) && !onlySingleLevel;
	const PIELIGHT teamcolour = pal_GetTeamColour(team);

	iIMDShape *pCurrShape = shape;
	do
	{
		if (pieFlag & pie_BUTTON)
		{
			pie_Draw3DButton(pCurrShape, teamcolour, modelView);
			retVal = true;
		}
		else
		{
			retVal = instancedMeshRenderer.Draw3DShape(pCurrShape, frame, teamcolour, colour, pieFlag, pieFlagData, modelView, stretchDepth);
		}

		pCurrShape = pCurrShape->next;

	} while (drawAllLevels && pCurrShape && retVal);

	return retVal;
}

static void pie_ShadowDrawLoop(ShadowCache &shadowCache)
{
//	size_t cachedShadowDraws = 0;
//	size_t uncachedShadowDraws = 0;
	for (unsigned i = 0; i < scshapes.size(); i++)
	{
		/*DrawShadowResult result =*/ pie_DrawShadow(shadowCache, scshapes[i].shape, scshapes[i].flag, scshapes[i].flag_data, scshapes[i].light, scshapes[i].matrix);
//		if (result == DRAW_SUCCESS_CACHED)
//		{
//			++cachedShadowDraws;
//		}
//		else
//		{
//			++uncachedShadowDraws;
//		}
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
	const int width = pie_GetVideoBufferWidth();
	const int height = pie_GetVideoBufferHeight();
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

static ShaderOnce perFrameUniformsShaderOnce;

void pie_RemainingPasses(uint64_t currentGameFrame)
{
	instancedMeshRenderer.DrawAll(currentGameFrame);
	instancedMeshRenderer.clear();
}

bool InstancedMeshRenderer::DrawAll(uint64_t currentGameFrame)
{
	perFrameUniformsShaderOnce.reset();

	// Generate global (per-frame) uniforms
	glm::vec4 sceneColor(lighting0[LIGHT_EMISSIVE][0], lighting0[LIGHT_EMISSIVE][1], lighting0[LIGHT_EMISSIVE][2], lighting0[LIGHT_EMISSIVE][3]);
	glm::vec4 ambient(lighting0[LIGHT_AMBIENT][0], lighting0[LIGHT_AMBIENT][1], lighting0[LIGHT_AMBIENT][2], lighting0[LIGHT_AMBIENT][3]);
	glm::vec4 diffuse(lighting0[LIGHT_DIFFUSE][0], lighting0[LIGHT_DIFFUSE][1], lighting0[LIGHT_DIFFUSE][2], lighting0[LIGHT_DIFFUSE][3]);
	glm::vec4 specular(lighting0[LIGHT_SPECULAR][0], lighting0[LIGHT_SPECULAR][1], lighting0[LIGHT_SPECULAR][2], lighting0[LIGHT_SPECULAR][3]);

	const auto &renderState = getCurrentRenderState();
	const glm::vec4 fogColor = renderState.fogEnabled ? glm::vec4(
		renderState.fogColour.vector[0] / 255.f,
		renderState.fogColour.vector[1] / 255.f,
		renderState.fogColour.vector[2] / 255.f,
		renderState.fogColour.vector[3] / 255.f
	) : glm::vec4(0.f);

	gfx_api::Draw3DShapeGlobalUniforms globalUniforms {
		pie_PerspectiveGet(),
		glm::vec4(currentSunPosition, 0.f), sceneColor, ambient, diffuse, specular, fogColor,
		renderState.fogBegin, renderState.fogEnd, pie_GetShaderTime(), renderState.fogEnabled
	};

	if (useInstancedRendering)
	{
		Draw3DShapes_Instanced(currentGameFrame, perFrameUniformsShaderOnce, globalUniforms);
	}
	else
	{
		Draw3DShapes_Old(currentGameFrame, perFrameUniformsShaderOnce, globalUniforms);
	}

	return true;
}

template<SHADER_MODE shader, typename Draw3DInstancedPSO>
static void drawInstanced3dShapeTemplated_Inner(ShaderOnce& globalsOnce, const gfx_api::Draw3DShapeGlobalUniforms& globalUniforms, const iIMDShape * shape, gfx_api::buffer* instanceDataBuffer, size_t instanceBufferOffset, size_t instance_count)
{
	auto* tcmask = shape->tcmaskpage != iV_TEX_INVALID ? &pie_Texture(shape->tcmaskpage) : nullptr;
	auto* normalmap = shape->normalpage != iV_TEX_INVALID ? &pie_Texture(shape->normalpage) : nullptr;
	auto* specularmap = shape->specularpage != iV_TEX_INVALID ? &pie_Texture(shape->specularpage) : nullptr;

	gfx_api::Draw3DShapePerMeshUniforms meshUniforms {
		tcmask ? 1 : 0, normalmap != nullptr, specularmap != nullptr, shape->buffers[VBO_TANGENT] != nullptr
	};

	gfx_api::buffer* pTangentBuffer = (shape->buffers[VBO_TANGENT] != nullptr) ? shape->buffers[VBO_TANGENT] : getZeroedVertexBuffer(shape->vertexCount * 4 * sizeof(gfx_api::gfxFloat));

	Draw3DInstancedPSO::get().bind();
	globalsOnce.perform_once<Draw3DInstancedPSO>([&globalUniforms]{
		Draw3DInstancedPSO::get().set_uniforms_at(0, globalUniforms);
	});

	Draw3DInstancedPSO::get().set_uniforms_at(1, meshUniforms);
	gfx_api::context::get().bind_vertex_buffers(0, {
		std::make_tuple(shape->buffers[VBO_VERTEX], 0),
		std::make_tuple(shape->buffers[VBO_NORMAL], 0),
		std::make_tuple(shape->buffers[VBO_TEXCOORD], 0),
		std::make_tuple(pTangentBuffer, 0),
		std::make_tuple(instanceDataBuffer, instanceBufferOffset) });
	Draw3DInstancedPSO::get().bind_textures(&pie_Texture(shape->texpage), tcmask, normalmap, specularmap);

	Draw3DInstancedPSO::get().draw_elements_instanced(shape->polys.size() * 3, 0, instance_count);
//	Draw3DInstancedPSO::get().unbind_vertex_buffers(shape->buffers[VBO_VERTEX], shape->buffers[VBO_NORMAL], shape->buffers[VBO_TEXCOORD]);
}

template<SHADER_MODE shader, typename AdditivePSO, typename AdditiveNoDepthWRTPSO, typename AlphaPSO, typename AlphaNoDepthWRTPSO, typename PremultipliedPSO, typename OpaquePSO>
static void drawInstanced3dShapeTemplated(ShaderOnce& globalsOnce, const gfx_api::Draw3DShapeGlobalUniforms& globalUniforms, const iIMDShape * shape, int pieFlag, gfx_api::buffer* instanceDataBuffer, size_t instanceBufferOffset, size_t instance_count)
{
	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE)
	{
		if (!(pieFlag & pie_NODEPTHWRITE))
		{
			return drawInstanced3dShapeTemplated_Inner<shader, AdditivePSO>(globalsOnce, globalUniforms, shape, instanceDataBuffer, instanceBufferOffset, instance_count);
		}
		else
		{
			return drawInstanced3dShapeTemplated_Inner<shader, AdditiveNoDepthWRTPSO>(globalsOnce, globalUniforms, shape, instanceDataBuffer, instanceBufferOffset, instance_count);
		}
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		if (!(pieFlag & pie_NODEPTHWRITE))
		{
			return drawInstanced3dShapeTemplated_Inner<shader, AlphaPSO>(globalsOnce, globalUniforms, shape, instanceDataBuffer, instanceBufferOffset, instance_count);
		}
		else
		{
			return drawInstanced3dShapeTemplated_Inner<shader, AlphaNoDepthWRTPSO>(globalsOnce, globalUniforms, shape, instanceDataBuffer, instanceBufferOffset, instance_count);
		}
	}
	else if (pieFlag & pie_PREMULTIPLIED)
	{
		return drawInstanced3dShapeTemplated_Inner<shader, PremultipliedPSO>(globalsOnce, globalUniforms, shape, instanceDataBuffer, instanceBufferOffset, instance_count);
	}
	else
	{
		return drawInstanced3dShapeTemplated_Inner<shader, OpaquePSO>(globalsOnce, globalUniforms, shape, instanceDataBuffer, instanceBufferOffset, instance_count);
	}
}

static void pie_Draw3DShape2_Instanced(ShaderOnce& globalsOnce, const gfx_api::Draw3DShapeGlobalUniforms& globalUniforms, const iIMDShape *shape, int pieFlag, gfx_api::buffer* instanceDataBuffer, size_t instanceBufferOffset, size_t instance_count)
{
	bool light = true;

	++drawCallsCount;

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
		light = false;
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		light = false;
	}
	else if (pieFlag & pie_PREMULTIPLIED)
	{
		light = false;
	}

	if (pieFlag & pie_ECM)
	{
		light = true;
	}

	gfx_api::context::get().bind_index_buffer(*shape->buffers[VBO_INDEX], gfx_api::index_type::u16);

	if (light)
	{
		drawInstanced3dShapeTemplated<SHADER_COMPONENT_INSTANCED, gfx_api::Draw3DShapeAdditive_Instanced, gfx_api::Draw3DShapeAdditiveNoDepthWRT_Instanced, gfx_api::Draw3DShapeAlpha_Instanced, gfx_api::Draw3DShapeAlphaNoDepthWRT_Instanced, gfx_api::Draw3DShapePremul_Instanced, gfx_api::Draw3DShapeOpaque_Instanced>(globalsOnce, globalUniforms, shape, pieFlag, instanceDataBuffer, instanceBufferOffset, instance_count);
	}
	else
	{
		drawInstanced3dShapeTemplated<SHADER_NOLIGHT_INSTANCED, gfx_api::Draw3DShapeNoLightAdditive_Instanced, gfx_api::Draw3DShapeNoLightAdditiveNoDepthWRT_Instanced, gfx_api::Draw3DShapeNoLightAlpha_Instanced, gfx_api::Draw3DShapeNoLightAlphaNoDepthWRT_Instanced, gfx_api::Draw3DShapeNoLightPremul_Instanced, gfx_api::Draw3DShapeNoLightOpaque_Instanced>(globalsOnce, globalUniforms, shape, pieFlag, instanceDataBuffer, instanceBufferOffset, instance_count);
	}

	polyCount += shape->polys.size();
}

void InstancedMeshRenderer::Draw3DShapes_Instanced(uint64_t currentGameFrame, ShaderOnce& globalsOnce, const gfx_api::Draw3DShapeGlobalUniforms& globalUniforms)
{
	if (instancesCount + translucentInstancesCount == 0)
	{
		return;
	}

	instancesData.clear();
	instancesData.reserve(instancesCount + translucentInstancesCount);

	struct InstancedDrawCall
	{
		templatedState state;
		size_t instance_count;
		size_t startingIdxInInstancesBuffer = 0;

		InstancedDrawCall(const templatedState& state, size_t instance_count, size_t startingIdxInInstancesBuffer)
		:	state(state),
			instance_count(instance_count),
			startingIdxInInstancesBuffer(startingIdxInInstancesBuffer)
		{ }
	};
	std::vector<InstancedDrawCall> drawCalls;

	for (const auto& mesh : instanceMeshes)
	{
		const auto& meshInstances = mesh.second;
		size_t startingIdxInInstancesBuffer = instancesData.size();
		for (const auto& instance : meshInstances)
		{
			instancesData.push_back(GenerateInstanceData(instance.frame, instance.colour, instance.teamcolour, instance.flag, instance.flag_data, instance.matrix, instance.stretch));
		}
		drawCalls.emplace_back(mesh.first, meshInstances.size(), startingIdxInInstancesBuffer);
	}

	size_t startIdxTranslucentDrawCalls = drawCalls.size();

	for (const auto& mesh : instanceTranslucentMeshes)
	{
		const auto& meshInstances = mesh.second;
		size_t startingIdxInInstancesBuffer = instancesData.size();
		for (const auto& instance : meshInstances)
		{
			instancesData.push_back(GenerateInstanceData(instance.frame, instance.colour, instance.teamcolour, instance.flag, instance.flag_data, instance.matrix, instance.stretch));
		}
		drawCalls.emplace_back(mesh.first, meshInstances.size(), startingIdxInInstancesBuffer);
	}

	// Upload buffer
	++currInstanceBufferIdx;
	if (currInstanceBufferIdx >= instanceDataBuffers.size())
	{
		currInstanceBufferIdx = 0;
	}
	instanceDataBuffers[currInstanceBufferIdx]->upload(instancesData.size() * sizeof(gfx_api::Draw3DShapePerInstanceInterleavedData), instancesData.data());

	instancesData.clear();

	// Draw opaque models
	gfx_api::context::get().debugStringMarker("Remaining passes - opaque models");
	for (size_t i = 0; i < startIdxTranslucentDrawCalls; ++i)
	{
		const auto& call = drawCalls[i];
		const iIMDShape * shape = call.state.shape;
		size_t instanceBufferOffset = static_cast<size_t>(sizeof(gfx_api::Draw3DShapePerInstanceInterleavedData) * call.startingIdxInInstancesBuffer);
		pie_Draw3DShape2_Instanced(perFrameUniformsShaderOnce, globalUniforms, shape, call.state.pieFlag, instanceDataBuffers[currInstanceBufferIdx], instanceBufferOffset, call.instance_count);
	}
	if (startIdxTranslucentDrawCalls > 0)
	{
		// unbind last index buffer bound inside pie_Draw3DShape2
		gfx_api::context::get().unbind_index_buffer(*((drawCalls[startIdxTranslucentDrawCalls-1].state.shape)->buffers[VBO_INDEX]));
	}

	// Draw shadows
	gfx_api::context::get().debugStringMarker("Remaining passes - shadows");
	if (shadows)
	{
		pie_DrawShadows(currentGameFrame);
	}

	// Draw translucent models last
	gfx_api::context::get().debugStringMarker("Remaining passes - translucent models");
	for (size_t i = startIdxTranslucentDrawCalls; i < drawCalls.size(); ++i)
	{
		const auto& call = drawCalls[i];
		const iIMDShape * shape = call.state.shape;
		size_t instanceBufferOffset = static_cast<size_t>(sizeof(gfx_api::Draw3DShapePerInstanceInterleavedData) * call.startingIdxInInstancesBuffer);
		pie_Draw3DShape2_Instanced(perFrameUniformsShaderOnce, globalUniforms, shape, call.state.pieFlag, instanceDataBuffers[currInstanceBufferIdx], instanceBufferOffset, call.instance_count);
	}
	gfx_api::context::get().disable_all_vertex_buffers();
	if (startIdxTranslucentDrawCalls < drawCalls.size())
	{
		// unbind last index buffer bound inside pie_Draw3DShape2
		gfx_api::context::get().unbind_index_buffer(*((drawCalls.back().state.shape)->buffers[VBO_INDEX]));
	}

	gfx_api::context::get().debugStringMarker("Remaining passes - done");
}

void InstancedMeshRenderer::Draw3DShapes_Old(uint64_t currentGameFrame, ShaderOnce& globalsOnce, const gfx_api::Draw3DShapeGlobalUniforms& globalUniforms)
{
	// Draw models
	// sort list to reduce state changes
	std::sort(shapes.begin(), shapes.end(), less_than_shape());
	gfx_api::context::get().debugStringMarker("Remaining passes - opaque models");
	templatedState lastState;
	for (SHAPE const &shape : shapes)
	{
		lastState = pie_Draw3DShape2(lastState, perFrameUniformsShaderOnce, globalUniforms, shape.shape, shape.frame, shape.colour, shape.teamcolour, shape.flag, shape.flag_data, shape.matrix, shape.stretch);
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
		lastState = pie_Draw3DShape2(lastState, perFrameUniformsShaderOnce, globalUniforms, shape.shape, shape.frame, shape.colour, shape.teamcolour, shape.flag, shape.flag_data, shape.matrix, shape.stretch);
	}
	gfx_api::context::get().disable_all_vertex_buffers();
	if (!tshapes.empty())
	{
		// unbind last index buffer bound inside pie_Draw3DShape2
		gfx_api::context::get().unbind_index_buffer(*((tshapes.back().shape)->buffers[VBO_INDEX]));
	}

	gfx_api::context::get().debugStringMarker("Remaining passes - done");
}

void pie_GetResetCounts(size_t *pPieCount, size_t *pPolyCount)
{
	*pPieCount  = pieCount;
	*pPolyCount = polyCount;

	pieCount = 0;
	polyCount = 0;
	drawCallsCount = 0;
}
