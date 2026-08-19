// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file range_rings.cpp
 * Instanced cone SDF range rings (generate + terrain composite).
 */

#include "range_rings.h"

#include "baseobject.h"
#include "display3d_render_graph.h"
#include "display3d_render_internal.h"
#include "droid.h"
#include "game_world.h"
#include "map.h"
#include "projectile.h"
#include "profiling.h"
#include "stats.h"
#include "structure.h"
#include "visibility.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/piestate.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cmath>
#include <vector>

namespace range_rings
{

namespace
{

constexpr int CONE_SIDES = 32;
constexpr float CONE_RIM_RADIUS = 1.25f; // coverage skirt past the true disk (SDF=0 at 1.0)
// Cone slope: drop in y per unit of planar distance, constant across instances.
// The apex sits at CONE_SLOPE * R, so the rasterized depth is an affine function
// of (r - R) with an instance-independent offset, and the LEQ depth union keeps
// the instance with the smallest true signed distance. (An apex at y = R would
// bias the winner by (1 - CONE_SLOPE) * (R1 - R2) wherever rings of different
// radii overlap, cutting gaps into the merged contour near ring junctions.)
constexpr float CONE_SLOPE = 1.f / CONE_RIM_RADIUS;
constexpr size_t CONE_VERTEX_COUNT = static_cast<size_t>(CONE_SIDES) + 1;
constexpr size_t CONE_INDEX_COUNT = static_cast<size_t>(CONE_SIDES) * 3;
constexpr float ORTHO_PAD = static_cast<float>(TILE_UNITS);
constexpr float CAMERA_HEIGHT_SLACK = 64.f;
constexpr float SDF_BAND_TEXELS = 8.f;
constexpr float FILL_ALPHA = 0.06f; // inner disk tint

gfx_api::buffer* s_coneVBO = nullptr;
gfx_api::buffer* s_coneIBO = nullptr;
gfx_api::buffer* s_instanceSensor = nullptr;
gfx_api::buffer* s_instanceWeapon = nullptr;
gfx_api::buffer* s_instanceMin = nullptr;

std::vector<gfx_api::RangeRingInstance> s_sensor;
std::vector<gfx_api::RangeRingInstance> s_weapon;
std::vector<gfx_api::RangeRingInstance> s_minRange;

uint32_t s_gatheredFrame = 0;
bool s_gatheredThisFrame = false;

float s_debugX = 0.f;
float s_debugY = 0.f;
float s_debugRadius = 0.f;

struct SdfCamera
{
	glm::mat4 viewProj{1.f};
	glm::vec4 originExtent{0.f, 0.f, 1.f, 1.f};
	glm::vec4 mapOriginExtent{0.f}; // 0 size = do not clip to map
	float maxR = 1.f;
	float sdfBand = 1.f;
};

SdfCamera s_sdfCamera;

glm::vec4 mapWorldOriginExtent()
{
	if (gameWorld.map.width <= 0 || gameWorld.map.height <= 0)
	{
		return glm::vec4(0.f);
	}
	const float w = static_cast<float>(world_coord(gameWorld.map.width));
	const float h = static_cast<float>(world_coord(gameWorld.map.height));
	return glm::vec4(0.f, -h, w, h);
}

void includeXZ(float x, float z, float& minX, float& maxX, float& minZ, float& maxZ, bool& any)
{
	if (!any)
	{
		minX = maxX = x;
		minZ = maxZ = z;
		any = true;
		return;
	}
	minX = std::min(minX, x);
	maxX = std::max(maxX, x);
	minZ = std::min(minZ, z);
	maxZ = std::max(maxZ, z);
}

void scanMaxR(const std::vector<gfx_api::RangeRingInstance>& list, float& maxR)
{
	for (const gfx_api::RangeRingInstance& inst : list)
	{
		maxR = std::max(maxR, inst.centerRadius.z);
	}
}

float sdfBandForExtent(float extentX, float extentZ)
{
	std::pair<uint32_t, uint32_t> rt = gfx_api::context::get().usedPipelineSurfaceExtent(gfx_api::PipelineSurfaceId::RangeRingSdf);
	const float w = std::max(static_cast<float>(rt.first), 1.f);
	const float h = std::max(static_cast<float>(rt.second), 1.f);
	return SDF_BAND_TEXELS * std::max(extentX / w, extentZ / h);
}

SdfCamera fitOrtho(const InGame3DFrameContext& fc)
{
	float minX = 0.f;
	float maxX = 0.f;
	float minZ = 0.f;
	float maxZ = 0.f;
	bool any = false;
	float maxR = 1.f;
	scanMaxR(s_sensor, maxR);
	scanMaxR(s_weapon, maxR);
	scanMaxR(s_minRange, maxR);

	const glm::mat4 inv = glm::inverse(fc.perspectiveViewMatrix);
	std::array<glm::vec3, 8> corners {};
	bool frustumOk = true;
	for (int i = 0; i < 8; ++i)
	{
		const glm::vec4 clip((i & 1) ? 1.f : -1.f, (i & 2) ? 1.f : -1.f, (i & 4) ? 1.f : -1.f, 1.f);
		glm::vec4 world = inv * clip;
		if (std::abs(world.w) < 1e-6f || !std::isfinite(world.w))
		{
			frustumOk = false;
			break;
		}
		world /= world.w;
		if (!std::isfinite(world.x) || !std::isfinite(world.z))
		{
			frustumOk = false;
			break;
		}
		corners[static_cast<size_t>(i)] = glm::vec3(world);
		includeXZ(world.x, world.z, minX, maxX, minZ, maxZ, any);
	}
	if (frustumOk)
	{
		for (int i = 0; i < 4; ++i)
		{
			const glm::vec3 a = corners[static_cast<size_t>(i)];
			const glm::vec3 b = corners[static_cast<size_t>(i + 4)];
			const float dy = b.y - a.y;
			if (std::abs(dy) < 1e-5f)
			{
				continue;
			}
			const float t = -a.y / dy;
			if (t < 0.f)
			{
				continue;
			}
			const glm::vec3 p = a + t * (b - a);
			if (std::isfinite(p.x) && std::isfinite(p.z))
			{
				includeXZ(p.x, p.z, minX, maxX, minZ, maxZ, any);
			}
		}
	}

	if (any)
	{
		minX -= ORTHO_PAD;
		maxX += ORTHO_PAD;
		minZ -= ORTHO_PAD;
		maxZ += ORTHO_PAD;
	}

	if (gameWorld.map.width > 0 && gameWorld.map.height > 0)
	{
		const float mapMinX = 0.f;
		const float mapMaxX = static_cast<float>(world_coord(gameWorld.map.width));
		const float mapMinZ = static_cast<float>(-world_coord(gameWorld.map.height));
		const float mapMaxZ = 0.f;
		if (any)
		{
			minX = std::max(minX, mapMinX);
			maxX = std::min(maxX, mapMaxX);
			minZ = std::max(minZ, mapMinZ);
			maxZ = std::min(maxZ, mapMaxZ);
		}
		if (!any || minX >= maxX || minZ >= maxZ)
		{
			minX = mapMinX;
			maxX = mapMaxX;
			minZ = mapMinZ;
			maxZ = mapMaxZ;
			any = true;
		}
	}

	if (!any || (maxX - minX < 1.f && maxZ - minZ < 1.f))
	{
		SdfCamera degenerate;
		degenerate.viewProj = glm::mat4(1.f);
		degenerate.originExtent = glm::vec4(0.f, 0.f, 1.f, 1.f);
		degenerate.mapOriginExtent = mapWorldOriginExtent();
		degenerate.maxR = 1.f;
		degenerate.sdfBand = 1.f;
		return degenerate;
	}

	if (maxX - minX < 1.f)
	{
		const float c = 0.5f * (minX + maxX);
		minX = c - 0.5f;
		maxX = c + 0.5f;
	}
	if (maxZ - minZ < 1.f)
	{
		const float c = 0.5f * (minZ + maxZ);
		minZ = c - 0.5f;
		maxZ = c + 0.5f;
	}

	const float cx = 0.5f * (minX + maxX);
	const float cz = 0.5f * (minZ + maxZ);
	const float halfW = 0.5f * (maxX - minX);
	const float halfH = 0.5f * (maxZ - minZ);
	const glm::vec3 eye(cx, maxR + CAMERA_HEIGHT_SLACK, cz);
	const glm::vec3 at(cx, 0.f, cz);
	const glm::mat4 view = glm::lookAt(eye, at, glm::vec3(0.f, 0.f, -1.f));
	// Rim vertices sit below y=0 (down to -CONE_SLOPE * (CONE_RIM_RADIUS - 1) * R)
	// Extend the far plane so the skirt is not clipped
	const float rimDrop = CONE_SLOPE * (CONE_RIM_RADIUS - 1.f) * maxR;
	const glm::mat4 proj = glm::ortho(-halfW, halfW, -halfH, halfH, 1.f, eye.y + rimDrop + 1.f);

	SdfCamera cam;
	cam.viewProj = proj * view;
	cam.originExtent = glm::vec4(minX, minZ, maxX - minX, maxZ - minZ);
	cam.mapOriginExtent = mapWorldOriginExtent();
	cam.maxR = maxR;
	cam.sdfBand = sdfBandForExtent(cam.originExtent.z, cam.originExtent.w);
	return cam;
}

void push(std::vector<gfx_api::RangeRingInstance>& dst, float x, float yGame, float radius)
{
	if (radius <= 0.f)
	{
		return;
	}
	dst.push_back({ glm::vec4(x, -yGame, radius, 0.f) });
}

void push(std::vector<gfx_api::RangeRingInstance>& dst, const BASE_OBJECT* psObj, float radius)
{
	if (psObj == nullptr)
	{
		return;
	}
	const Spacetime st = interpolateObjectSpacetime(psObj, graphicsTime);
	push(dst, static_cast<float>(st.pos.x), static_cast<float>(st.pos.y), radius);
}

void pushWeaponAndMin(const BASE_OBJECT* psObj)
{
	if (psObj == nullptr || psObj->numWeaps == 0)
	{
		return;
	}
	const unsigned nStat = psObj->asWeaps[0].nStat;
	if (nStat == 0 || nStat >= asWeaponStats.size())
	{
		return;
	}
	const WEAPON_STATS& stats = asWeaponStats[nStat];
	push(s_weapon, psObj, static_cast<float>(proj_GetLongRange(stats, static_cast<int>(psObj->player))));
	const unsigned minRange = static_cast<unsigned>(proj_GetMinRange(stats, static_cast<int>(psObj->player)));
	if (minRange > 0)
	{
		push(s_minRange, psObj, static_cast<float>(minRange));
	}
}

void gatherSelected()
{
	s_sensor.clear();
	s_weapon.clear();
	s_minRange.clear();

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}

	for (DROID* psDroid : gameWorld.objects.droids[selectedPlayer])
	{
		if (psDroid == nullptr || !psDroid->selected)
		{
			continue;
		}
		push(s_sensor, psDroid, static_cast<float>(objSensorRange(psDroid)));
		pushWeaponAndMin(psDroid);
	}

	for (STRUCTURE* psStruct : gameWorld.objects.structures[selectedPlayer])
	{
		if (psStruct == nullptr || !psStruct->selected)
		{
			continue;
		}
		push(s_sensor, psStruct, static_cast<float>(objSensorRange(psStruct)));
		pushWeaponAndMin(psStruct);
	}

	if (s_debugRadius > 0.f)
	{
		push(s_weapon, s_debugX, s_debugY, s_debugRadius);
	}
}

void gatherIfNeeded()
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	const auto& fc = pie_GetInGame3DFrameContext();
	if (s_gatheredThisFrame && s_gatheredFrame == fc.currentGameFrame)
	{
		return;
	}
	s_gatheredFrame = fc.currentGameFrame;
	s_gatheredThisFrame = true;
	gatherSelected();
	s_sdfCamera = fitOrtho(fc);
}

template<typename Pso>
void recordSdf(const std::vector<gfx_api::RangeRingInstance>& instances, gfx_api::buffer* instanceBuf)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	if (!gfx_api::context::get().supportsInstancedRendering())
	{
		return;
	}
	if (instances.empty() || instanceBuf == nullptr || s_coneVBO == nullptr || s_coneIBO == nullptr)
	{
		return;
	}

	instanceBuf->upload(instances.size() * sizeof(gfx_api::RangeRingInstance), instances.data());

	gfx_api::constant_buffer_type<SHADER_RANGE_RING_SDF> constants {};
	constants.orthoViewProj = s_sdfCamera.viewProj;
	constants.mapOriginExtent = s_sdfCamera.mapOriginExtent;
	constants.sdfParams = glm::vec4(s_sdfCamera.sdfBand, 0.f, 0.f, 0.f);
	auto& pso = Pso::get();
	pso.bind();
	pso.bind_constants(constants);
	gfx_api::context::get().bind_index_buffer(*s_coneIBO, gfx_api::index_type::u16);
	pso.bind_vertex_buffers(s_coneVBO, instanceBuf);
	pso.draw_elements_instanced(CONE_INDEX_COUNT, 0, instances.size());
	pso.unbind_vertex_buffers(s_coneVBO, instanceBuf);
	gfx_api::context::get().unbind_index_buffer(*s_coneIBO);
}

} // namespace

bool init()
{
	if (s_coneVBO != nullptr)
	{
		return true;
	}

	std::array<glm::vec4, CONE_VERTEX_COUNT> vertices {};
	vertices[0] = glm::vec4(0.f, CONE_SLOPE, 0.f, 1.f); // apex over the ring center
	const float rimY = CONE_SLOPE * (1.f - CONE_RIM_RADIUS); // below 0: keeps the slope constant out to the skirt
	for (int i = 0; i < CONE_SIDES; ++i)
	{
		const float a = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(CONE_SIDES);
		vertices[static_cast<size_t>(i + 1)] = glm::vec4(CONE_RIM_RADIUS * std::cos(a), rimY, CONE_RIM_RADIUS * std::sin(a), 1.f);
	}

	std::array<uint16_t, CONE_INDEX_COUNT> indices {};
	for (int i = 0; i < CONE_SIDES; ++i)
	{
		const size_t base = static_cast<size_t>(i) * 3;
		indices[base + 0] = 0;
		indices[base + 1] = static_cast<uint16_t>(1 + i);
		indices[base + 2] = static_cast<uint16_t>(1 + ((i + 1) % CONE_SIDES));
	}

	auto& ctx = gfx_api::context::get();
	s_coneVBO = ctx.create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::static_draw, "range_rings::coneVBO");
	s_coneIBO = ctx.create_buffer_object(gfx_api::buffer::usage::index_buffer, gfx_api::context::buffer_storage_hint::static_draw, "range_rings::coneIBO");
	s_instanceSensor = ctx.create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::stream_draw, "range_rings::instanceSensor");
	s_instanceWeapon = ctx.create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::stream_draw, "range_rings::instanceWeapon");
	s_instanceMin = ctx.create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::stream_draw, "range_rings::instanceMin");
	if (s_coneVBO == nullptr || s_coneIBO == nullptr
		|| s_instanceSensor == nullptr || s_instanceWeapon == nullptr || s_instanceMin == nullptr)
	{
		shutdown();
		return false;
	}

	s_coneVBO->upload(vertices.size() * sizeof(glm::vec4), vertices.data());
	s_coneIBO->upload(indices.size() * sizeof(uint16_t), indices.data());
	return true;
}

void shutdown()
{
	delete s_coneVBO;
	s_coneVBO = nullptr;
	delete s_coneIBO;
	s_coneIBO = nullptr;
	delete s_instanceSensor;
	s_instanceSensor = nullptr;
	delete s_instanceWeapon;
	s_instanceWeapon = nullptr;
	delete s_instanceMin;
	s_instanceMin = nullptr;
	s_sensor.clear();
	s_weapon.clear();
	s_minRange.clear();
	s_gatheredThisFrame = false;
	s_sdfCamera = SdfCamera{};
}

void setDebugRange(float centerX, float centerYGame, float radius)
{
	if (radius <= 0.f)
	{
		s_debugRadius = 0.f;
		return;
	}
	s_debugX = centerX;
	s_debugY = centerYGame;
	s_debugRadius = radius;
}

void recordSdfSensor(const gfx_api::RenderPassContext&)
{
	WZ_PROFILE_SCOPE(rangeRingSdfSensor);
	gatherIfNeeded();
	recordSdf<gfx_api::RangeRingSdfSensorPSO>(s_sensor, s_instanceSensor);
}

void recordSdfWeapon(const gfx_api::RenderPassContext&)
{
	WZ_PROFILE_SCOPE(rangeRingSdfWeapon);
	gatherIfNeeded();
	recordSdf<gfx_api::RangeRingSdfWeaponPSO>(s_weapon, s_instanceWeapon);
}

void recordSdfMin(const gfx_api::RenderPassContext&)
{
	WZ_PROFILE_SCOPE(rangeRingSdfMin);
	gatherIfNeeded();
	recordSdf<gfx_api::RangeRingSdfMinPSO>(s_minRange, s_instanceMin);
}

void recordComposite(const gfx_api::RenderPassContext& passCtx)
{
	WZ_PROFILE_SCOPE(rangeRingComposite);
	ASSERT(passCtx.readCount() == 3, "RangeRingComposite: 0 color, 1 prepass depth, 2 sdf");
	gfx_api::abstract_texture* scene = passCtx.getRead(0);
	gfx_api::abstract_texture* depth = passCtx.getRead(1);
	gfx_api::abstract_texture* sdf = passCtx.getRead(2);
	if (scene == nullptr || depth == nullptr || sdf == nullptr
		|| !pie_IsInGame3DFrameContextReady())
	{
		return;
	}

	gatherIfNeeded();

	const auto& fc = pie_GetInGame3DFrameContext();
	gfx_api::constant_buffer_type<SHADER_RANGE_RING_COMPOSITE> constants {};
	constants.invProjectionMatrix = glm::inverse(fc.perspectiveMatrix);
	constants.invViewMatrix = glm::inverse(fc.viewMatrix);
	display3d_fillPassReadUvScaleClamp(passCtx, 1, constants.uvScaleClamp);
	constants.sdfOriginExtent = s_sdfCamera.originExtent;
	display3d_fillPassReadUvScaleClamp(passCtx, 2, constants.sdfUvScaleClamp);
	constants.sensorColor = glm::vec4(0.20f, 0.85f, 1.00f, 1.f);
	constants.weaponColor = glm::vec4(1.00f, 0.45f, 0.15f, 1.f);
	constants.minRangeColor = glm::vec4(0.75f, 0.35f, 1.00f, 1.f);
	constants.fillAlpha = FILL_ALPHA;
	constants.sdfBand = s_sdfCamera.sdfBand;
	display3d_drawFullscreenTriangle<gfx_api::RangeRingCompositePSO>(constants, scene, depth, sdf);
}

} // namespace range_rings
