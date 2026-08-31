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
/** @file shader_catalog.h
 * Shader program catalog:
 * SHADER_MODE -> stage basenames + spec-constant mask + GLES cap.
 * Backends own GPU objects (gl_pipeline_state_object / VkPSO).
 *
 * Adding a SHADER_MODE:
 * 1. Append the enumerator in pietypes.h (before SHADER_MAX).
 * 2. Add a catalog row in shader_catalog.cpp.
 * 3. Add GL sources under data/base/shaders/ and VK sources under data/base/shaders/vk/.
 * 4. List the new VK stage files in data/CMakeLists.txt VK_SHADERS (glob check will FATAL otherwise).
 * 5. Add UBO struct + pipeline_state_helper alias (gfx_pipelines.h).
 * 6. Draw with the PSO.
 * Do not add rows to gfx_api_gl.cpp / gfx_api_vk.cpp.
 *
 * Ownership:
 * - Recipe (pipeline_state_helper): raster state, vertex layout, UBO types, textures, SHADER_MODE.
 * - Catalog: stage basenames, spec-constant mask, friendly name, max GLES version. No GPU objects.
 * - C++ UBO types: std140 layout + GLSL block name.
 * - GLSL: algorithm, per dialect (shaders/ vs shaders/vk/). Not merged.
 */

#pragma once

#include "pietypes.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace gfx_api
{

enum class shader_stage_kind : uint8_t { vertex, tess_control, tess_eval, fragment };

struct shader_stage_file
{
	shader_stage_kind kind = shader_stage_kind::vertex;
	const char* basename = nullptr; // "tcmask.vert" -- no shaders/ prefix, no .spv

	constexpr shader_stage_file() = default;
	constexpr shader_stage_file(shader_stage_kind kind, const char* basename)
		: kind(kind), basename(basename)
	{}
};

enum class shader_es_version : uint8_t { es_100, es_300, es_310 };

enum shader_spec_constant : uint8_t
{
	spec_shadow_mode         = 1u << 0, // VK constant_id 1
	spec_shadow_filter_size  = 1u << 1, // 2
	spec_shadow_cascades     = 1u << 2, // 3
	spec_point_light_enabled = 1u << 3, // 4
};
using shader_spec_mask = uint8_t;

static constexpr size_t MAX_SHADER_STAGES = 4;

struct shader_program_desc
{
	const char* friendly_name = "";
	shader_stage_file stages[MAX_SHADER_STAGES] = {};
	uint8_t stage_count = 0;
	shader_spec_mask spec_constants = 0;
	shader_es_version max_es = shader_es_version::es_300;

	constexpr shader_program_desc() = default;

	constexpr shader_program_desc(
		const char* friendly_name,
		const char* vert,
		const char* frag,
		const char* tesc = nullptr,
		const char* tese = nullptr,
		shader_spec_mask spec = 0,
		shader_es_version max_es = shader_es_version::es_300)
		: friendly_name(friendly_name)
		, stages{}
		, stage_count(0)
		, spec_constants(spec)
		, max_es(max_es)
	{
		uint8_t n = 0;
		stages[n++] = shader_stage_file{shader_stage_kind::vertex, vert};
		if (tesc != nullptr)
		{
			stages[n++] = shader_stage_file{shader_stage_kind::tess_control, tesc};
		}
		if (tese != nullptr)
		{
			stages[n++] = shader_stage_file{shader_stage_kind::tess_eval, tese};
		}
		stages[n++] = shader_stage_file{shader_stage_kind::fragment, frag};
		stage_count = n;
	}
};

const shader_program_desc& shader_catalog(SHADER_MODE mode);

// "shaders/" + basename
std::string gl_shader_path(const char* basename);
// "shaders/vk/" + basename + ".spv"
std::string vk_spv_path(const char* basename);

const shader_stage_file* find_stage(const shader_program_desc& desc, shader_stage_kind kind);
bool shader_has_tessellation(const shader_program_desc& desc);

} // namespace gfx_api
