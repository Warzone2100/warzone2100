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

#include "shader_catalog.h"

#include "lib/framework/frame.h"

#include <array>

namespace gfx_api
{

namespace
{

constexpr shader_spec_mask SPEC_SHADOW =
	spec_shadow_mode | spec_shadow_filter_size | spec_shadow_cascades;
constexpr shader_spec_mask SPEC_SHADOW_AND_LIGHTS =
	SPEC_SHADOW | spec_point_light_enabled;

constexpr shader_program_desc make_program(
	const char* friendly_name,
	const char* vert,
	const char* frag,
	const char* tesc = nullptr,
	const char* tese = nullptr,
	shader_spec_mask spec = 0,
	shader_es_version max_es = shader_es_version::es_300)
{
	return shader_program_desc{friendly_name, vert, frag, tesc, tese, spec, max_es};
}

constexpr std::array<shader_program_desc, SHADER_MAX> make_catalog_table()
{
	std::array<shader_program_desc, SHADER_MAX> t{};
	t[SHADER_COMPONENT] = make_program("Component program", "tcmask.vert", "tcmask.frag");
	t[SHADER_COMPONENT_INSTANCED] = make_program("Component program", "tcmask_instanced.vert", "tcmask_instanced.frag", nullptr, nullptr, SPEC_SHADOW_AND_LIGHTS);
	t[SHADER_COMPONENT_DEPTH_INSTANCED] = make_program("Component program", "tcmask_depth_instanced.vert", "tcmask_depth_instanced.frag");
	t[SHADER_COMPONENT_DEPTH_PREPASS_INSTANCED] = make_program("Component depth prepass program", "tcmask_depth_prepass_instanced.vert", "tcmask_depth_prepass_instanced.frag");
	t[SHADER_COMPONENT_DEPTH_PREPASS_DEPTHONLY_INSTANCED] = make_program("Component depth-only prepass program", "tcmask_depth_prepass_depthonly_instanced.vert", "prepass_depth_only.frag");
	t[SHADER_NOLIGHT] = make_program("Plain program", "nolight.vert", "nolight.frag");
	t[SHADER_NOLIGHT_INSTANCED] = make_program("Plain program", "nolight_instanced.vert", "nolight_instanced.frag");
	t[SHADER_TERRAIN_DEPTH] = make_program("terrain_depth program", "terrain_depth.vert", "terraindepth.frag");
	t[SHADER_TERRAIN_DEPTHMAP] = make_program("terrain_depthmap program", "terrain_depth_only.vert", "terrain_depth_only.frag");
	t[SHADER_TERRAIN_DEPTH_PREPASS] = make_program("terrain_depth_prepass program", "terrain_depth_prepass.vert", "terrain_depth_prepass.frag");
	t[SHADER_TERRAIN_DEPTH_PREPASS_DEPTHONLY] = make_program("terrain_depth_prepass depth-only program", "terrain_depth_prepass.vert", "prepass_depth_only.frag");
	t[SHADER_WATER] = make_program("water program", "terrain_water.vert", "water.frag");
	t[SHADER_WATER_DEPTH_PREPASS] = make_program("water_depth_prepass program", "water_depth_prepass.vert", "water_depth_prepass.frag");
	t[SHADER_WATER_DEPTH_PREPASS_DEPTHONLY] = make_program("water_depth_prepass depth-only program", "water_depth_prepass.vert", "prepass_depth_only.frag");
	t[SHADER_RECT] = make_program("Rect program", "rect.vert", "rect.frag");
	t[SHADER_RECT_INSTANCED] = make_program("Rect instanced program", "rect_instanced.vert", "rect_instanced.frag");
	t[SHADER_TEXRECT] = make_program("Textured rect program", "rect.vert", "texturedrect.frag");
	t[SHADER_GFX_COLOUR] = make_program("gfx_color program", "gfx_color.vert", "gfx.frag");
	t[SHADER_GFX_TEXT] = make_program("gfx_text program", "gfx_text.vert", "texturedrect.frag");
	t[SHADER_SKYBOX] = make_program("skybox program", "skybox.vert", "skybox.frag");
	t[SHADER_GENERIC_COLOR] = make_program("generic color program", "generic.vert", "rect.frag");
	t[SHADER_CONSTRUCTION_LINE] = make_program("construction line program", "construction_line.vert", "construction_line.frag");
	t[SHADER_LINE] = make_program("line program", "line.vert", "rect.frag");
	t[SHADER_TEXT] = make_program("Text program", "rect.vert", "text.frag");
	t[SHADER_TERRAIN_COMBINED_CLASSIC] = make_program("terrain decals program", "terrain_combined.vert", "terrain_combined_classic.frag", nullptr, nullptr, SPEC_SHADOW);
	t[SHADER_TERRAIN_COMBINED_MEDIUM] = make_program("terrain decals program", "terrain_combined.vert", "terrain_combined_medium.frag", nullptr, nullptr, SPEC_SHADOW);
	t[SHADER_TERRAIN_COMBINED_HIGH] = make_program("terrain decals program", "terrain_combined.vert", "terrain_combined_high.frag", nullptr, nullptr, SPEC_SHADOW_AND_LIGHTS);
	t[SHADER_TERRAIN_DEPTHMAP_TESS] = make_program("terrain_depthmap tess program", "terrain_depth_tess.vert", "terrain_depth_only.frag", "terrain_depth_tess.tesc", "terrain_depthmap_tess.tese");
	t[SHADER_TERRAIN_DEPTH_PREPASS_TESS] = make_program("terrain_depth_prepass tess program", "terrain_depth_prepass_tess.vert", "terrain_depth_prepass_tess.frag", "terrain_depth_prepass_tess.tesc", "terrain_depth_prepass_tess.tese");
	t[SHADER_TERRAIN_DEPTH_PREPASS_TESS_DEPTHONLY] = make_program("terrain_depth_prepass tess depth-only program", "terrain_depth_prepass_tess.vert", "prepass_depth_only.frag", "terrain_depth_prepass_tess.tesc", "terrain_depth_prepass_tess.tese");
	t[SHADER_TERRAIN_COMBINED_MEDIUM_TESS] = make_program("terrain decals tess program", "terrain_combined_tess.vert", "terrain_combined_medium.frag", "terrain_combined_tess.tesc", "terrain_combined_tess.tese", SPEC_SHADOW);
	t[SHADER_TERRAIN_COMBINED_HIGH_TESS] = make_program("terrain decals tess program", "terrain_combined_tess.vert", "terrain_combined_high.frag", "terrain_combined_tess.tesc", "terrain_combined_tess.tese", SPEC_SHADOW_AND_LIGHTS);
	t[SHADER_WATER_CLASSIC] = make_program("classic water program", "terrain_water_classic.vert", "terrain_water_classic.frag");
	t[SHADER_WATER_HIGH] = make_program("high water program", "terrain_water_high.vert", "terrain_water_high.frag", nullptr, nullptr, SPEC_SHADOW_AND_LIGHTS);
	t[SHADER_WORLD_TO_SCREEN] = make_program("World to screen quad program", "world_to_screen.vert", "world_to_screen.frag");
	t[SHADER_FSR1_EASU] = make_program("FSR1 EASU program", "world_to_screen.vert", "fsr1_easu.frag", nullptr, nullptr, 0, shader_es_version::es_310);
	t[SHADER_FSR1_RCAS] = make_program("FSR1 RCAS program", "world_to_screen.vert", "fsr1_rcas.frag");
	t[SHADER_SMAA_EDGES] = make_program("SMAA edge detection program", "smaa_edges.vert", "smaa_edges.frag");
	t[SHADER_SMAA_WEIGHTS] = make_program("SMAA blending weights program", "smaa_weights.vert", "smaa_weights.frag");
	t[SHADER_SMAA_BLEND] = make_program("SMAA neighborhood blending program", "smaa_blend.vert", "smaa_blend.frag");
	t[SHADER_SSAO_GENERATE] = make_program("SSAO generate program", "postprocess_fullscreen.vert", "ssao_generate.frag");
	t[SHADER_SSAO_BLUR] = make_program("SSAO blur program", "postprocess_fullscreen.vert", "ssao_blur.frag");
	t[SHADER_SSAO_DOWNSAMPLE] = make_program("SSAO downsample program", "postprocess_fullscreen.vert", "ssao_downsample.frag");
	t[SHADER_SCENE_COMPOSE_SSAO] = make_program("Scene compose SSAO program", "postprocess_fullscreen.vert", "scene_compose_ssao.frag");
	t[SHADER_SCENE_FOG] = make_program("Scene fog program", "postprocess_fullscreen.vert", "scene_fog.frag");
	t[SHADER_RANGE_RING_SDF] = make_program("Range ring SDF program", "range_ring_sdf.vert", "range_ring_sdf.frag");
	t[SHADER_RANGE_RING_COMPOSITE] = make_program("Range ring composite program", "postprocess_fullscreen.vert", "range_ring_composite.frag");
	t[SHADER_DEBUG_TEXTURE2D_QUAD] = make_program("Debug texture quad program", "quad_texture2d.vert", "quad_texture2d.frag");
	t[SHADER_DEBUG_TEXTURE2DARRAY_QUAD] = make_program("Debug texture array quad program", "quad_texture2darray.vert", "quad_texture2darray.frag");
	t[SHADER_DEBUG_TESS_QUAD] = make_program("Debug tessellated quad program", "tess_quad.vert", "tess_quad.frag", "tess_quad.tesc", "tess_quad.tese");

	return t;
}

constexpr bool catalog_is_complete(const std::array<shader_program_desc, SHADER_MAX>& t)
{
	for (int i = SHADER_NONE + 1; i < SHADER_MAX; ++i)
	{
		if (t[static_cast<size_t>(i)].stage_count < 2)
		{
			return false;
		}
	}
	return true;
}

constexpr auto CATALOG_TABLE = make_catalog_table();
static_assert(catalog_is_complete(CATALOG_TABLE), "missing catalog row for a SHADER_MODE");

} // namespace

const shader_program_desc& shader_catalog(SHADER_MODE mode)
{
	ASSERT(mode > SHADER_NONE && mode < SHADER_MAX, "Invalid SHADER_MODE %d", static_cast<int>(mode));
	return CATALOG_TABLE[static_cast<size_t>(mode)];
}

std::string gl_shader_path(const char* basename)
{
	ASSERT_OR_RETURN({}, basename != nullptr && basename[0] != '\0', "Empty shader basename");
	return std::string("shaders/") + basename;
}

std::string vk_spv_path(const char* basename)
{
	ASSERT_OR_RETURN({}, basename != nullptr && basename[0] != '\0', "Empty shader basename");
	return std::string("shaders/vk/") + basename + ".spv";
}

const shader_stage_file* find_stage(const shader_program_desc& desc, shader_stage_kind kind)
{
	for (uint8_t i = 0; i < desc.stage_count && i < MAX_SHADER_STAGES; ++i)
	{
		if (desc.stages[i].kind == kind && desc.stages[i].basename != nullptr)
		{
			return &desc.stages[i];
		}
	}
	return nullptr;
}

bool shader_has_tessellation(const shader_program_desc& desc)
{
	return find_stage(desc, shader_stage_kind::tess_control) != nullptr
		|| find_stage(desc, shader_stage_kind::tess_eval) != nullptr;
}

} // namespace gfx_api
