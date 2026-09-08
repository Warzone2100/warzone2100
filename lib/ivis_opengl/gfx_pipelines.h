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
/** @file gfx_pipelines.h
 * Draw-side PSO recipes: pipeline_state_helper aliases and constant_buffer_type specializations.
 * Include this from draw TUs. gfx_api.h does not include this header.
 */

#pragma once

#include "gfx_api.h"

namespace gfx_api
{

	template<REND_MODE render_mode, SHADER_MODE shader, DEPTH_MODE depth_mode>
	using Draw3DShape = typename gfx_api::pipeline_state_helper<rasterizer_state<render_mode, depth_mode, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u16,
	std::tuple<
	Draw3DShapeGlobalUniforms,
	Draw3DShapePerMeshUniforms,
	Draw3DShapePerInstanceUniforms
	>,
	std::tuple<
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<normal, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float4, 0>>,
	vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<tangent, gfx_api::vertex_attribute_type::float4, 0>>
	>,
	std::tuple<
	texture_description<0, sampler_type::anisotropic, kSampler_Texture>, // diffuse
	texture_description<1, sampler_type::bilinear, kSampler_TextureTcmask>, // team color mask
	texture_description<2, sampler_type::anisotropic, kSampler_TextureNormal>, // normal map
	texture_description<3, sampler_type::anisotropic, kSampler_TextureSpecular> // specular map
	>, shader>;

	using Draw3DShapeOpaque = Draw3DShape<REND_OPAQUE, SHADER_COMPONENT, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeAlpha = Draw3DShape<REND_ALPHA, SHADER_COMPONENT, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapePremul = Draw3DShape<REND_PREMULTIPLIED, SHADER_COMPONENT, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeAdditive = Draw3DShape<REND_ADDITIVE, SHADER_COMPONENT, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeNoLightOpaque = Draw3DShape<REND_OPAQUE, SHADER_NOLIGHT, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeNoLightAlpha = Draw3DShape<REND_ALPHA, SHADER_NOLIGHT, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeNoLightPremul = Draw3DShape<REND_PREMULTIPLIED, SHADER_NOLIGHT, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeNoLightAdditive = Draw3DShape<REND_ADDITIVE, SHADER_NOLIGHT, DEPTH_CMP_LEQ_WRT_ON>;

	using Draw3DShapeAlphaNoDepthWRT = Draw3DShape<REND_ALPHA, SHADER_COMPONENT, DEPTH_CMP_LEQ_WRT_OFF>;
	using Draw3DShapeNoLightAlphaNoDepthWRT = Draw3DShape<REND_ALPHA, SHADER_NOLIGHT, DEPTH_CMP_LEQ_WRT_OFF>;

	template<REND_MODE render_mode, SHADER_MODE shader, DEPTH_MODE depth_mode>
	using Draw3DShapeInstanced = typename gfx_api::pipeline_state_helper<rasterizer_state<render_mode, depth_mode, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u16,
	std::tuple<
	Draw3DShapeInstancedGlobalUniforms,
	Draw3DShapeInstancedPerMeshUniforms,
	PointLightsUniforms
	>,
	std::tuple<
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<normal, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float4, 0>>,
	vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<tangent, gfx_api::vertex_attribute_type::float4, 0>>,
	// instance data
	vertex_buffer_description<sizeof(Draw3DShapePerInstanceInterleavedData), gfx_api::vertex_attribute_input_rate::instance,
		vertex_attribute_description<instance_modelMatrix, gfx_api::vertex_attribute_type::float4, 0>,
		vertex_attribute_description<instance_modelMatrix + 1, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)>,
		vertex_attribute_description<instance_modelMatrix + 2, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)*2>,
		vertex_attribute_description<instance_modelMatrix + 3, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)*3>,
		vertex_attribute_description<instance_packedValues, gfx_api::vertex_attribute_type::float4, offsetof(Draw3DShapePerInstanceInterleavedData, shaderStretch_ecmState_alphaTest_animFrameNumber)>,
		vertex_attribute_description<instance_Colour, gfx_api::vertex_attribute_type::u8x4_norm, offsetof(Draw3DShapePerInstanceInterleavedData, colour)>,
		vertex_attribute_description<instance_TeamColour, gfx_api::vertex_attribute_type::u8x4_norm, offsetof(Draw3DShapePerInstanceInterleavedData, teamcolour)>
		>
	>,
	std::tuple<
	texture_description<0, sampler_type::anisotropic, kSampler_Texture>, // diffuse
	texture_description<1, sampler_type::bilinear, kSampler_TextureTcmask>, // team color mask
	texture_description<2, sampler_type::anisotropic, kSampler_TextureNormal>, // normal map
	texture_description<3, sampler_type::anisotropic, kSampler_TextureSpecular>, // specular map
	texture_description<4, sampler_type::bilinear_border, kSampler_shadowMap, pixel_format_target::depth_map, border_color::opaque_white>,  // depth / shadow map
	texture_description<5, sampler_type::bilinear, kSampler_lightmap_tex> // lightmap
	>, shader>;

	using Draw3DShapeOpaque_Instanced = Draw3DShapeInstanced<REND_OPAQUE, SHADER_COMPONENT_INSTANCED, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeAlpha_Instanced = Draw3DShapeInstanced<REND_ALPHA, SHADER_COMPONENT_INSTANCED, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapePremul_Instanced = Draw3DShapeInstanced<REND_PREMULTIPLIED, SHADER_COMPONENT_INSTANCED, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeAdditive_Instanced = Draw3DShapeInstanced<REND_ADDITIVE, SHADER_COMPONENT_INSTANCED, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeNoLightOpaque_Instanced = Draw3DShapeInstanced<REND_OPAQUE, SHADER_NOLIGHT_INSTANCED, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeNoLightAlpha_Instanced = Draw3DShapeInstanced<REND_ALPHA, SHADER_NOLIGHT_INSTANCED, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeNoLightPremul_Instanced = Draw3DShapeInstanced<REND_PREMULTIPLIED, SHADER_NOLIGHT_INSTANCED, DEPTH_CMP_LEQ_WRT_ON>;
	using Draw3DShapeNoLightAdditive_Instanced = Draw3DShapeInstanced<REND_ADDITIVE, SHADER_NOLIGHT_INSTANCED, DEPTH_CMP_LEQ_WRT_ON>;

	using Draw3DShapeAlphaNoDepthWRT_Instanced = Draw3DShapeInstanced<REND_ALPHA, SHADER_COMPONENT_INSTANCED, DEPTH_CMP_LEQ_WRT_OFF>;
	using Draw3DShapeNoLightAlphaNoDepthWRT_Instanced = Draw3DShapeInstanced<REND_ALPHA, SHADER_NOLIGHT_INSTANCED, DEPTH_CMP_LEQ_WRT_OFF>;
	using Draw3DShapeAdditiveNoDepthWRT_Instanced = Draw3DShapeInstanced<REND_ADDITIVE, SHADER_COMPONENT_INSTANCED, DEPTH_CMP_LEQ_WRT_OFF>;
	using Draw3DShapeNoLightAdditiveNoDepthWRT_Instanced = Draw3DShapeInstanced<REND_ADDITIVE, SHADER_NOLIGHT_INSTANCED, DEPTH_CMP_LEQ_WRT_OFF>;
	using Draw3DShapePremulNoDepthWRT_Instanced = Draw3DShapeInstanced<REND_PREMULTIPLIED, SHADER_COMPONENT_INSTANCED, DEPTH_CMP_LEQ_WRT_OFF>;
	using Draw3DShapeNoLightPremulNoDepthWRT_Instanced = Draw3DShapeInstanced<REND_PREMULTIPLIED, SHADER_NOLIGHT_INSTANCED, DEPTH_CMP_LEQ_WRT_OFF>;

	using Draw3DShapeDepthOnly_Instanced = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::shadow_mapping>, primitive_type::triangles, index_type::u16,
	std::tuple<
	Draw3DShapeInstancedDepthOnlyGlobalUniforms
	>,
	std::tuple<
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<normal, gfx_api::vertex_attribute_type::float3, 0>>,
//	vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float4, 0>>,
//	vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<tangent, gfx_api::vertex_attribute_type::float4, 0>>,
	// instance data
	vertex_buffer_description<sizeof(Draw3DShapePerInstanceInterleavedData), gfx_api::vertex_attribute_input_rate::instance,
		vertex_attribute_description<instance_modelMatrix, gfx_api::vertex_attribute_type::float4, 0>,
		vertex_attribute_description<instance_modelMatrix + 1, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)>,
		vertex_attribute_description<instance_modelMatrix + 2, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)*2>,
		vertex_attribute_description<instance_modelMatrix + 3, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)*3>,
		vertex_attribute_description<instance_packedValues, gfx_api::vertex_attribute_type::float4, offsetof(Draw3DShapePerInstanceInterleavedData, shaderStretch_ecmState_alphaTest_animFrameNumber)>,
		vertex_attribute_description<instance_Colour, gfx_api::vertex_attribute_type::u8x4_norm, offsetof(Draw3DShapePerInstanceInterleavedData, colour)>,
		vertex_attribute_description<instance_TeamColour, gfx_api::vertex_attribute_type::u8x4_norm, offsetof(Draw3DShapePerInstanceInterleavedData, teamcolour)>
		>
	>, notexture, SHADER_COMPONENT_DEPTH_INSTANCED>;

	/// Mesh scene depth+normals prepass (view-space normals encoded to color RT0).
	using Draw3DShapeDepthPrepass_Instanced = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u16,
	std::tuple<
	Draw3DShapeInstancedDepthOnlyGlobalUniforms,
	Draw3DShapeDepthPrepassMeshUniforms
	>,
	std::tuple<
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<normal, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<sizeof(Draw3DShapePerInstanceInterleavedData), gfx_api::vertex_attribute_input_rate::instance,
		vertex_attribute_description<instance_modelMatrix, gfx_api::vertex_attribute_type::float4, 0>,
		vertex_attribute_description<instance_modelMatrix + 1, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)>,
		vertex_attribute_description<instance_modelMatrix + 2, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)*2>,
		vertex_attribute_description<instance_modelMatrix + 3, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)*3>,
		vertex_attribute_description<instance_packedValues, gfx_api::vertex_attribute_type::float4, offsetof(Draw3DShapePerInstanceInterleavedData, shaderStretch_ecmState_alphaTest_animFrameNumber)>,
		vertex_attribute_description<instance_Colour, gfx_api::vertex_attribute_type::u8x4_norm, offsetof(Draw3DShapePerInstanceInterleavedData, colour)>,
		vertex_attribute_description<instance_TeamColour, gfx_api::vertex_attribute_type::u8x4_norm, offsetof(Draw3DShapePerInstanceInterleavedData, teamcolour)>
		>
	>, notexture, SHADER_COMPONENT_DEPTH_PREPASS_INSTANCED>;

	/// Mesh scene depth-only prepass (no color attachment - PrepassNeed::Normals absent)
	using Draw3DShapeDepthPrepassDepthOnly_Instanced = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u16,
	std::tuple<
	Draw3DShapeInstancedDepthOnlyGlobalUniforms
	>,
	std::tuple<
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<normal, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<sizeof(Draw3DShapePerInstanceInterleavedData), gfx_api::vertex_attribute_input_rate::instance,
		vertex_attribute_description<instance_modelMatrix, gfx_api::vertex_attribute_type::float4, 0>,
		vertex_attribute_description<instance_modelMatrix + 1, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)>,
		vertex_attribute_description<instance_modelMatrix + 2, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)*2>,
		vertex_attribute_description<instance_modelMatrix + 3, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)*3>,
		vertex_attribute_description<instance_packedValues, gfx_api::vertex_attribute_type::float4, offsetof(Draw3DShapePerInstanceInterleavedData, shaderStretch_ecmState_alphaTest_animFrameNumber)>,
		vertex_attribute_description<instance_Colour, gfx_api::vertex_attribute_type::u8x4_norm, offsetof(Draw3DShapePerInstanceInterleavedData, colour)>,
		vertex_attribute_description<instance_TeamColour, gfx_api::vertex_attribute_type::u8x4_norm, offsetof(Draw3DShapePerInstanceInterleavedData, teamcolour)>
		>
	>, notexture, SHADER_COMPONENT_DEPTH_PREPASS_DEPTHONLY_INSTANCED>;

	template<>
	struct constant_buffer_type<SHADER_GENERIC_COLOR>
	{
		glm::mat4 transform_matrix;
		glm::vec2 unused;
		glm::vec2 unused2;
		glm::vec4 colour;
	};

	template<>
	struct constant_buffer_type<SHADER_CONSTRUCTION_LINE>
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ModelViewMatrix;
		glm::vec4 color;
		glm::vec4 fogColor;
		glm::vec4 fogRange;
	};

	using TransColouredTrianglePSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ADDITIVE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_CONSTRUCTION_LINE>>,
	std::tuple<
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>
	>, notexture, SHADER_CONSTRUCTION_LINE>;
	using DrawStencilShadow = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_OFF, 0, polygon_offset::disabled, stencil_mode::stencil_shadow_silhouette, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_GENERIC_COLOR>>,
	std::tuple<
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>
	>, notexture, SHADER_GENERIC_COLOR>;

	template<>
	struct constant_buffer_type<SHADER_TERRAIN_DEPTH>
	{
		glm::mat4 transform_matrix;
		glm::vec4 paramX;
		glm::vec4 paramY;
		glm::vec4 paramXLight;
		glm::vec4 paramYLight;
		glm::mat4 unused;
		glm::mat4 texture_matrix;
		int texture0;
		int texture1;
	};

	using TerrainDepth = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 0, polygon_offset::enabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_TERRAIN_DEPTH>>,
	std::tuple<
		TerrainVertexVBODescription
	>, std::tuple<texture_description<0, sampler_type::bilinear_repeat, kSampler_lightmap_tex>>, SHADER_TERRAIN_DEPTH>;

	template<>
	struct constant_buffer_type<SHADER_TERRAIN_DEPTHMAP>
	{
		glm::mat4 transform_matrix;
	};

	using TerrainDepthOnlyForDepthMap = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 0, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_TERRAIN_DEPTHMAP>>,
	std::tuple<
		TerrainVertexVBODescription
	>, std::tuple<texture_description<0, sampler_type::bilinear_repeat, kSampler_Texture>>, SHADER_TERRAIN_DEPTHMAP>;

	template<>
	struct constant_buffer_type<SHADER_TERRAIN_DEPTH_PREPASS>
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ViewMatrix;
	};

	using TerrainDepthPrepass = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::enabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_TERRAIN_DEPTH_PREPASS>>,
	std::tuple<
		TerrainDepthPrepassVertexVBODescription
	>, notexture, SHADER_TERRAIN_DEPTH_PREPASS>;

	/// Terrain scene depth-only prepass (no color attachment - PrepassNeed::Normals absent)
	using TerrainDepthPrepassDepthOnly = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::enabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_TERRAIN_DEPTH_PREPASS>>,
	std::tuple<
		TerrainDepthPrepassVertexVBODescription
	>, notexture, SHADER_TERRAIN_DEPTH_PREPASS_DEPTHONLY>;

	template<>
	struct constant_buffer_type<SHADER_WATER_DEPTH_PREPASS>
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ViewMatrix;
	};

	using WaterDepthPrepass = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_WATER_DEPTH_PREPASS>>,
	std::tuple<
		vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float4, 0>> // WaterVertex, w is depth
	>, notexture, SHADER_WATER_DEPTH_PREPASS>;

	/// Water scene depth-only prepass (no color attachment - PrepassNeed::Normals absent)
	using WaterDepthPrepassDepthOnly = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_WATER_DEPTH_PREPASS>>,
	std::tuple<
		vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float4, 0>> // WaterVertex, w is depth
	>, notexture, SHADER_WATER_DEPTH_PREPASS_DEPTHONLY>;


	template<REND_MODE render_mode, SHADER_MODE shader>
	using TerrainCombinedTemplate = typename gfx_api::pipeline_state_helper<rasterizer_state<render_mode, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<TerrainCombinedUniforms, PointLightsUniforms>,
	std::tuple<
	vertex_buffer_description<sizeof(TerrainDecalVertex), gfx_api::vertex_attribute_input_rate::vertex, // TerrainDecalVertex struct
	vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>,
	vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float2, offsetof(TerrainDecalVertex, decalUv)>,
	vertex_attribute_description<normal,   gfx_api::vertex_attribute_type::float3, offsetof(TerrainDecalVertex, normal)>,
	vertex_attribute_description<tangent,  gfx_api::vertex_attribute_type::float4, offsetof(TerrainDecalVertex, decalTangent)>,
	vertex_attribute_description<terrain_tileNo,   gfx_api::vertex_attribute_type::int1, offsetof(TerrainDecalVertex, decalNo)>,
	vertex_attribute_description<terrain_grounds,  gfx_api::vertex_attribute_type::u8x4_uint, offsetof(TerrainDecalVertex, grounds)>,
	vertex_attribute_description<terrain_groundWeights, gfx_api::vertex_attribute_type::u8x4_norm, offsetof(TerrainDecalVertex, groundWeights)>
	>
	>, std::tuple<
	texture_description<0, sampler_type::bilinear, kSampler_lightmap_tex>, // lightmap
	texture_description<1, sampler_type::anisotropic_repeat, kSampler_groundTex, pixel_format_target::texture_2d_array>, // ground
	texture_description<2, sampler_type::anisotropic_repeat, kSampler_groundNormal, pixel_format_target::texture_2d_array>, // ground normal
	texture_description<3, sampler_type::anisotropic_repeat, kSampler_groundSpecular, pixel_format_target::texture_2d_array>, // ground specular
	texture_description<4, sampler_type::anisotropic_repeat, kSampler_groundHeight, pixel_format_target::texture_2d_array>, // ground height
	texture_description<5, sampler_type::anisotropic, kSampler_decalTex, pixel_format_target::texture_2d_array>, // decal
	texture_description<6, sampler_type::anisotropic, kSampler_decalNormal, pixel_format_target::texture_2d_array>, // decal normal
	texture_description<7, sampler_type::anisotropic, kSampler_decalSpecular, pixel_format_target::texture_2d_array>, // decal specular
	texture_description<8, sampler_type::anisotropic, kSampler_decalHeight, pixel_format_target::texture_2d_array>,  // decal height
	texture_description<9, sampler_type::bilinear_border, kSampler_shadowMap, pixel_format_target::depth_map, border_color::opaque_white>  // depth / shadow map
	>, shader>;

	using TerrainCombined_Classic = TerrainCombinedTemplate<REND_ALPHA, SHADER_TERRAIN_COMBINED_CLASSIC>;
	using TerrainCombined_Medium = TerrainCombinedTemplate<REND_OPAQUE, SHADER_TERRAIN_COMBINED_MEDIUM>;
	using TerrainCombined_High = TerrainCombinedTemplate<REND_OPAQUE, SHADER_TERRAIN_COMBINED_HIGH>;

	// Hardware-tessellated terrain variants: one patch per tile over the tile-corner
	// vertices, with the TES evaluating the baked surface fields (requires
	// context::supportsTessellationShaders()).
	// Unlike the CPU path there is no separate terrain depth prepass: this pass
	// writes depth itself, with a polygon offset applied at draw time
	// so the depth buffer contents stay biased slightly farther than the terrain.
	// Structure baseplates rely on that bias to avoid z-fighting. The offset
	// biases depth only, not the rasterized position.
	template<SHADER_MODE shader>
	using TerrainCombinedTessTemplate = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::enabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::patch_list_4, index_type::u32,
	std::tuple<TerrainCombinedUniforms, PointLightsUniforms>,
	std::tuple<
	vertex_buffer_description<sizeof(TerrainDecalVertex), gfx_api::vertex_attribute_input_rate::vertex, // TerrainDecalVertex struct
	vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>,
	vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float2, offsetof(TerrainDecalVertex, decalUv)>,
	vertex_attribute_description<tangent,  gfx_api::vertex_attribute_type::float4, offsetof(TerrainDecalVertex, decalTangent)>,
	vertex_attribute_description<terrain_tileNo,   gfx_api::vertex_attribute_type::int1, offsetof(TerrainDecalVertex, decalNo)>,
	vertex_attribute_description<terrain_grounds,  gfx_api::vertex_attribute_type::u8x4_uint, offsetof(TerrainDecalVertex, grounds)>,
	vertex_attribute_description<terrain_groundWeights, gfx_api::vertex_attribute_type::u8x4_norm, offsetof(TerrainDecalVertex, groundWeights)>
	>
	>, std::tuple<
	texture_description<0, sampler_type::bilinear, kSampler_lightmap_tex>, // lightmap
	texture_description<1, sampler_type::anisotropic_repeat, kSampler_groundTex, pixel_format_target::texture_2d_array>, // ground
	texture_description<2, sampler_type::anisotropic_repeat, kSampler_groundNormal, pixel_format_target::texture_2d_array>, // ground normal
	texture_description<3, sampler_type::anisotropic_repeat, kSampler_groundSpecular, pixel_format_target::texture_2d_array>, // ground specular
	texture_description<4, sampler_type::anisotropic_repeat, kSampler_groundHeight, pixel_format_target::texture_2d_array>, // ground height
	texture_description<5, sampler_type::anisotropic, kSampler_decalTex, pixel_format_target::texture_2d_array>, // decal
	texture_description<6, sampler_type::anisotropic, kSampler_decalNormal, pixel_format_target::texture_2d_array>, // decal normal
	texture_description<7, sampler_type::anisotropic, kSampler_decalSpecular, pixel_format_target::texture_2d_array>, // decal specular
	texture_description<8, sampler_type::anisotropic, kSampler_decalHeight, pixel_format_target::texture_2d_array>,  // decal height
	texture_description<9, sampler_type::bilinear_border, kSampler_shadowMap, pixel_format_target::depth_map, border_color::opaque_white>,  // depth / shadow map
	tess_texture_description<10, sampler_type::bilinear, kSampler_terrainBakedHeight>, // baked terrain height
	tess_texture_description<11, sampler_type::bilinear, kSampler_terrainBakedOffset>, // baked terrain outline offset
	tess_texture_description<12, sampler_type::bilinear, kSampler_terrainBakedNormal>  // baked terrain normal
	>, shader>;

	using TerrainCombinedTess_Medium = TerrainCombinedTessTemplate<SHADER_TERRAIN_COMBINED_MEDIUM_TESS>;
	using TerrainCombinedTess_High = TerrainCombinedTessTemplate<SHADER_TERRAIN_COMBINED_HIGH_TESS>;

	template<>
	struct constant_buffer_type<SHADER_TERRAIN_DEPTHMAP_TESS>
	{
		glm::mat4 ModelViewProjectionMatrix; // this pass's MVP (the light's, for shadow cascades)
		glm::mat4 tessCameraMVP; // the MAIN camera's MVP: tessellation factors must match the color pass
		glm::vec4 paramX; // unused by this pass (retained to match the terrain depth shader interface)
		glm::vec4 paramY;
		glm::mat4 texture_matrix;
		glm::vec4 tessParams; // x = max tess level, y = viewport height (pixels)
	};
	using TerrainDepthMapTessUniforms = constant_buffer_type<SHADER_TERRAIN_DEPTHMAP_TESS>;

	using TerrainDepthOnlyForDepthMapTess = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 0, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::patch_list_4, index_type::u32,
	std::tuple<TerrainDepthMapTessUniforms>,
	std::tuple<
		TerrainPatchCornerVBODescription
	>, std::tuple<
		tess_texture_description<0, sampler_type::bilinear, kSampler_terrainBakedHeight>, // baked terrain height
		tess_texture_description<1, sampler_type::bilinear, kSampler_terrainBakedOffset>, // baked terrain outline offset
		tess_texture_description<2, sampler_type::bilinear, kSampler_terrainBakedNormal>  // baked terrain normal
	>, SHADER_TERRAIN_DEPTHMAP_TESS>;

	template<>
	struct constant_buffer_type<SHADER_TERRAIN_DEPTH_PREPASS_TESS>
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ViewMatrix;
		glm::mat4 tessCameraMVP; // tessellation factors must match the color pass
		glm::vec4 tessParams; // x = max tess level, y = viewport height (pixels)
	};

	using TerrainDepthPrepassTessUniforms = constant_buffer_type<SHADER_TERRAIN_DEPTH_PREPASS_TESS>;

	using TerrainDepthPrepassTess = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::enabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::patch_list_4, index_type::u32,
	std::tuple<TerrainDepthPrepassTessUniforms>,
	std::tuple<
		TerrainPatchCornerVBODescription
	>, std::tuple<
		tess_texture_description<0, sampler_type::bilinear, kSampler_terrainBakedHeight>, // baked terrain height
		tess_texture_description<1, sampler_type::bilinear, kSampler_terrainBakedOffset>, // baked terrain outline offset
		tess_texture_description<2, sampler_type::bilinear, kSampler_terrainBakedNormal>  // baked terrain normal
	>, SHADER_TERRAIN_DEPTH_PREPASS_TESS>;

	/// Tessellated terrain scene depth-only prepass (no color attachment - PrepassNeed::Normals absent)
	using TerrainDepthPrepassTessDepthOnly = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::enabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::patch_list_4, index_type::u32,
	std::tuple<TerrainDepthPrepassTessUniforms>,
	std::tuple<
		TerrainPatchCornerVBODescription
	>, std::tuple<
		tess_texture_description<0, sampler_type::bilinear, kSampler_terrainBakedHeight>, // baked terrain height
		tess_texture_description<1, sampler_type::bilinear, kSampler_terrainBakedOffset>, // baked terrain outline offset
		tess_texture_description<2, sampler_type::bilinear, kSampler_terrainBakedNormal>  // baked terrain normal
	>, SHADER_TERRAIN_DEPTH_PREPASS_TESS_DEPTHONLY>;

	template<>
	struct constant_buffer_type<SHADER_WATER>
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ModelUVLightmapMatrix;
		glm::mat4 ModelUV1Matrix;
		glm::mat4 ModelUV2Matrix;
		glm::vec4 cameraPos; // in modelSpace
		glm::vec4 sunPos; // in modelSpace
		glm::vec4 emissiveLight; // light colors/intensity
		glm::vec4 ambientLight;
		glm::vec4 diffuseLight;
		glm::vec4 specularLight;
		float timeSec;
		float mipLoadBias;
		float pad0 = 0.f;
		float pad1 = 0.f;
	};

	using WaterPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_MULTIPLICATIVE, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_WATER>>,
	std::tuple<
	vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float4, 0>> // WaterVertex, w is depth
	>, std::tuple<
		texture_description<0, sampler_type::anisotropic_repeat, kSampler_tex1>, // tex1
		texture_description<1, sampler_type::anisotropic_repeat, kSampler_tex2>, // tex2
		texture_description<2, sampler_type::bilinear, kSampler_lightmap_tex> // lightmap
	>, SHADER_WATER>;

	template<>
	struct constant_buffer_type<SHADER_WATER_HIGH>
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ViewMatrix;
		glm::mat4 ModelUVLightmapMatrix;
		glm::mat4 ShadowMapMVPMatrix[WZ_MAX_SHADOW_CASCADES];
		glm::vec4 cameraPos; // in modelSpace
		glm::vec4 sunPos; // in modelSpace
		glm::vec4 emissiveLight; // light colors/intensity
		glm::vec4 ambientLight;
		glm::vec4 diffuseLight;
		glm::vec4 specularLight;
		glm::vec4 ShadowMapCascadeSplits; // Can't use float[4] (because of std140 layout alignment rules, which don't match C/C++ and waste a lot of space)
		int ShadowMapSize;
		float timeSec;
		float mipLoadBias;
		float pad0 = 0.f;
		int viewportWidth;
		int viewportHeight;
		int bucketDimensionUsed;
		float pad1 = 0.f;
		// The bucket table is last because its length follows the grid dimension, which may become variable.
		// (Anything placed after it would shift whenever that changed.)
		std::array<glm::ivec4, max_bucket_dimension * max_bucket_dimension> bucketOffsetAndSize;
	};

	using WaterHighPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_WATER_HIGH>, PointLightsUniforms>,
	std::tuple<
	vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float4, 0>> // WaterVertex, w is depth
	>, std::tuple<
		texture_description<0, sampler_type::anisotropic_repeat, kSampler_tex, pixel_format_target::texture_2d_array>, // textures
		texture_description<1, sampler_type::anisotropic_repeat, kSampler_tex_nm, pixel_format_target::texture_2d_array>, // normal maps
		texture_description<2, sampler_type::anisotropic_repeat, kSampler_tex_sm, pixel_format_target::texture_2d_array>, // specular maps
		texture_description<3, sampler_type::bilinear, kSampler_lightmap_tex>, // lightmap
		texture_description<4, sampler_type::bilinear_border, kSampler_shadowMap, pixel_format_target::depth_map, border_color::opaque_white>  // depth / shadow map
	>, SHADER_WATER_HIGH>;

	template<>
	struct constant_buffer_type<SHADER_WATER_CLASSIC>
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ModelUVLightmapMatrix;
		glm::mat4 ShadowMapMVPMatrix;
		glm::mat4 ModelUV1Matrix;
		glm::mat4 ModelUV2Matrix;
		glm::vec4 cameraPos; // in modelSpace
		glm::vec4 sunPos; // in modelSpace
		float timeSec;
		float mipLoadBias;
		float pad0 = 0.f;
		float pad1 = 0.f;
	};

	using WaterClassicPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_WATER_CLASSIC>>,
	std::tuple<
	vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float4, 0>> // WaterVertex, w is depth
	>, std::tuple<
		texture_description<0, sampler_type::bilinear, kSampler_lightmap_tex>, // lightmap
		texture_description<1, sampler_type::anisotropic_repeat, kSampler_tex2, pixel_format_target::texture_2d> // water decal texture
	>, SHADER_WATER_CLASSIC>;

	using gfx_tc = vertex_buffer_description<8, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float2, 0>>;
	using gfx_colour = vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<color, gfx_api::vertex_attribute_type::u8x4_norm, 0>>;
	using gfx_vtx2 = vertex_buffer_description<8, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>>;
	using gfx_vtx3 = vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>;

	template<>
	struct constant_buffer_type<SHADER_GFX_TEXT>
	{
		glm::mat4 transform_matrix;
		glm::vec2 offset;
		glm::vec2 size;
		glm::vec4 color;
		int texture; // IGNORED
	};

	template<>
	struct constant_buffer_type<SHADER_SKYBOX>
	{
		glm::mat4 transform_matrix;
		glm::vec4 color;
		glm::vec4 fog_color;
		int fog_enabled;
	};

	template<>
	struct constant_buffer_type<SHADER_GFX_COLOUR>
	{
		glm::mat4 transform_matrix;
//		glm::vec2 offset;
//		glm::vec2 size;
//		glm::vec4 color;
//		int texture;
	};

	template<REND_MODE rm, DEPTH_MODE dm, primitive_type primitive, typename VTX, typename Second, SHADER_MODE shader, typename texture>
	using GFX = typename gfx_api::pipeline_state_helper<rasterizer_state<rm, dm, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive, index_type::u16, std::tuple<constant_buffer_type<shader>>, std::tuple<VTX, Second>, texture, shader>;
	using VideoPSO = GFX<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_tc, SHADER_GFX_TEXT, std::tuple<texture_description<0, gfx_api::sampler_type::bilinear, kSampler_Texture>>>;
	using BackDropPSO = GFX<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_tc, SHADER_GFX_TEXT, std::tuple<texture_description<0, gfx_api::sampler_type::nearest_clamped, kSampler_Texture>>>;
	using SkyboxPSO = typename gfx_api::pipeline_state_helper<
		rasterizer_state<REND_ALPHA, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>,
		primitive_type::triangles,
		index_type::u16,
		std::tuple<constant_buffer_type<SHADER_SKYBOX>>,
		std::tuple<
			gfx_vtx3, gfx_tc
		>,
		std::tuple<texture_description<0, gfx_api::sampler_type::bilinear_repeat, kSampler_theTexture>>,
		SHADER_SKYBOX
	>;


	using RadarPSO = GFX<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_tc, SHADER_GFX_TEXT, std::tuple<texture_description<0, gfx_api::sampler_type::nearest_clamped, kSampler_Texture>>>;
	using RadarViewInsideFillPSO = GFX<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_colour, SHADER_GFX_COLOUR, notexture>;
	using RadarViewOutlinePSO = GFX<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::line_strip, gfx_vtx2, gfx_colour, SHADER_GFX_COLOUR, notexture>;

	template<>
	struct constant_buffer_type<SHADER_TEXT>
	{
		glm::mat4 transform_matrix;
		glm::vec2 offset;
		glm::vec2 size;
		glm::vec4 color;
		int texture; // IGNORED
	};

	using DrawImageTextPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_TEXT, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_TEXT>>,
	std::tuple<
	vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>,
	std::tuple<texture_description<0, sampler_type::bilinear, kSampler_Texture>>, SHADER_TEXT>;

	template<>
	struct constant_buffer_type<SHADER_RECT>
	{
		glm::mat4 transform_matrix;
		glm::vec2 offset;
		glm::vec2 size;
		glm::vec4 colour;
	};

	using ShadowBox2DPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_RECT>>,
	std::tuple<
	vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, notexture, SHADER_RECT>;
	using UniTransBoxPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_RECT>>,
	std::tuple<
	vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, notexture, SHADER_RECT>;

	template<>
	struct constant_buffer_type<SHADER_TEXRECT>
	{
		glm::mat4 transform_matrix;
		glm::vec2 offset;
		glm::vec2 size;
		glm::vec4 color;
		int texture; // IGNORED
	};

	using DrawImagePSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_TEXRECT>>,
	std::tuple<
	vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, std::tuple<texture_description<0, sampler_type::bilinear, kSampler_Texture>>, SHADER_TEXRECT>;

	using DrawImageAnisotropicPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_TEXRECT>>,
	std::tuple<
	vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, std::tuple<texture_description<0, sampler_type::anisotropic, kSampler_Texture>>, SHADER_TEXRECT>;

	using BoxFillPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_RECT>>,
	std::tuple<
	vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, notexture, SHADER_RECT>;
	using BoxFillAlphaPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_shadow_quad, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_RECT>>,
	std::tuple<
	vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, notexture, SHADER_RECT>;

	template<>
	struct constant_buffer_type<SHADER_RECT_INSTANCED>
	{
		glm::mat4 ProjectionMatrix;
	};

	using BoxFillPSO_Instanced = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_RECT_INSTANCED>>,
	std::tuple<
	vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>,
	// instance data
	vertex_buffer_description<sizeof(MultiRectPerInstanceInterleavedData), gfx_api::vertex_attribute_input_rate::instance,
		vertex_attribute_description<instance_modelMatrix, gfx_api::vertex_attribute_type::float4, 0>,
		vertex_attribute_description<instance_modelMatrix + 1, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)>,
		vertex_attribute_description<instance_modelMatrix + 2, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)*2>,
		vertex_attribute_description<instance_modelMatrix + 3, gfx_api::vertex_attribute_type::float4, sizeof(glm::vec4)*3>,
		vertex_attribute_description<instance_packedValues, gfx_api::vertex_attribute_type::float4, offsetof(MultiRectPerInstanceInterleavedData, offset_scale)>,
		vertex_attribute_description<instance_Colour, gfx_api::vertex_attribute_type::u8x4_norm, offsetof(MultiRectPerInstanceInterleavedData, colour)>
		>
	>, notexture, SHADER_RECT_INSTANCED>;

	template<>
	struct constant_buffer_type<SHADER_LINE>
	{
		glm::mat4 mat;
		glm::vec2 p0;
		glm::vec2 p1;
		glm::vec4 colour;
	};

	using LinePSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::lines, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_LINE>>,
	std::tuple<
	vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, notexture, SHADER_LINE>;

	template<>
	struct constant_buffer_type<SHADER_DEBUG_TEXTURE2D_QUAD>
	{
		glm::mat4 transform_matrix;
		glm::mat4 uv_transform_matrix;
		glm::ivec4 swizzle;
		glm::vec4 color;
		int texture;
	};

	using DebugDrawTexture2DToQuad = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_TEXT, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_DEBUG_TEXTURE2D_QUAD>>,
	std::tuple<
		vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>,
	std::tuple<texture_description<0, sampler_type::bilinear, kSampler_theTexture>>, SHADER_DEBUG_TEXTURE2D_QUAD>;

	template<>
	struct constant_buffer_type<SHADER_DEBUG_TEXTURE2DARRAY_QUAD>
	{
		glm::mat4 transform_matrix;
		glm::mat4 uv_transform_matrix;
		glm::ivec4 swizzle;
		glm::vec4 color;
		int layer;
		int texture;
	};

	using DebugDrawTexture2DArrayToQuad = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_TEXT, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_DEBUG_TEXTURE2DARRAY_QUAD>>,
	std::tuple<
		vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>,
	std::tuple<texture_description<0, sampler_type::bilinear, kSampler_theTextureArray, pixel_format_target::texture_2d_array>>, SHADER_DEBUG_TEXTURE2DARRAY_QUAD>;

	template<>
	struct constant_buffer_type<SHADER_DEBUG_TESS_QUAD>
	{
		glm::mat4 transform_matrix;
		glm::vec4 color;
		float tessLevel;
	};

	// Dev-only tessellation smoke-test pipeline (only usable when context::supportsTessellationShaders())
	using DebugDrawTessQuad = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::patch_list_4, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_DEBUG_TESS_QUAD>>,
	std::tuple<
		vertex_buffer_description<sizeof(glm::vec2), gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>>
	>, notexture, SHADER_DEBUG_TESS_QUAD>;

	template<>
	struct constant_buffer_type<SHADER_WORLD_TO_SCREEN>
	{
		// xy = rendered sub-rect fraction of the source, zw = UV clamp inside its edge
		glm::vec4 uvScaleClamp;
	};

	using WorldToScreenPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_WORLD_TO_SCREEN>>,
	std::tuple<
		vertex_buffer_description<2 * sizeof(gfxFloat), gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>
		>
	>,
	std::tuple<texture_description<0, sampler_type::bilinear, kSampler_Texture, pixel_format_target::texture_2d>>, SHADER_WORLD_TO_SCREEN>;

	template<>
	struct constant_buffer_type<SHADER_FSR1_EASU>
	{
		// the standard FsrEasuCon constant vectors
		glm::vec4 con0;
		glm::vec4 con1;
		glm::vec4 con2;
		glm::vec4 con3;
		// UV clamps keeping taps inside the rendered input viewport
		// (xy for plain taps, zw for gather quad centers)
		glm::vec4 con4;
	};

	using Fsr1EasuPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_FSR1_EASU>>,
	std::tuple<
		vertex_buffer_description<2 * sizeof(gfxFloat), gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>
		>
	>,
	std::tuple<texture_description<0, sampler_type::bilinear, kSampler_Texture, pixel_format_target::texture_2d>>, SHADER_FSR1_EASU>;

	template<>
	struct constant_buffer_type<SHADER_FSR1_RCAS>
	{
		glm::vec4 con0; // x = exp2(-sharpness) per FsrRcasCon
		glm::vec4 con1; // xy = input texel size
	};

	using Fsr1RcasPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_FSR1_RCAS>>,
	std::tuple<
		vertex_buffer_description<2 * sizeof(gfxFloat), gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>
		>
	>,
	std::tuple<texture_description<0, sampler_type::bilinear, kSampler_Texture, pixel_format_target::texture_2d>>, SHADER_FSR1_RCAS>;

	template<>
	struct constant_buffer_type<SHADER_SMAA_EDGES>
	{
		// xy = 1 / input size, zw = input size
		glm::vec4 rtMetrics;
		// xy = rendered sub-rect fraction of the input, zw = UV clamp inside its edge
		glm::vec4 uvScaleClamp;
		// x = luma contrast threshold
		glm::vec4 params;
	};

	using SmaaEdgesPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_SMAA_EDGES>>,
	std::tuple<
		vertex_buffer_description<2 * sizeof(gfxFloat), gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>
		>
	>,
	std::tuple<texture_description<0, sampler_type::bilinear, kSampler_colorTex, pixel_format_target::texture_2d>>, SHADER_SMAA_EDGES>;

	template<>
	struct constant_buffer_type<SHADER_SMAA_WEIGHTS>
	{
		// xy = 1 / input size, zw = input size
		glm::vec4 rtMetrics;
		// xy = rendered sub-rect fraction of the input, zw = UV clamp inside its edge
		glm::vec4 uvScaleClamp;
		// x = max orthogonal search steps, y = max diagonal search steps (0 disables
		// diagonal processing), z = corner rounding [0..1] (1 disables corner processing)
		glm::vec4 params;
	};

	using SmaaWeightsPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_SMAA_WEIGHTS>>,
	std::tuple<
		vertex_buffer_description<2 * sizeof(gfxFloat), gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>
		>
	>,
	std::tuple<
		texture_description<0, sampler_type::bilinear, kSampler_edgesTex, pixel_format_target::texture_2d>,
		texture_description<1, sampler_type::bilinear, kSampler_areaTex, pixel_format_target::texture_2d>,
		texture_description<2, sampler_type::nearest_clamped, kSampler_searchTex, pixel_format_target::texture_2d>
	>, SHADER_SMAA_WEIGHTS>;

	template<>
	struct constant_buffer_type<SHADER_SMAA_BLEND>
	{
		// xy = 1 / input size, zw = input size
		glm::vec4 rtMetrics;
		// xy = rendered sub-rect fraction of the input, zw = UV clamp inside its edge
		glm::vec4 uvScaleClamp;
	};

	using SmaaBlendPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_SMAA_BLEND>>,
	std::tuple<
		vertex_buffer_description<2 * sizeof(gfxFloat), gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>
		>
	>,
	std::tuple<
		texture_description<0, sampler_type::bilinear, kSampler_colorTex, pixel_format_target::texture_2d>,
		texture_description<1, sampler_type::bilinear, kSampler_blendTex, pixel_format_target::texture_2d>
	>, SHADER_SMAA_BLEND>;

	template<>
	struct constant_buffer_type<SHADER_SSAO_GENERATE>
	{
		glm::mat4 invProjectionMatrix;
		glm::mat4 projectionMatrix;
		glm::vec4 params; // radius, biasFactor, minBias, rangeScale
		glm::vec2 noiseScale;
		float sampleCount;
		float padding;
		glm::vec4 kernel[SSAO_KERNEL_SIZE];
		glm::vec4 uvScaleClamp;
	};

	using SSAOGeneratePSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_SSAO_GENERATE>>,
	std::tuple<
		vertex_buffer_description<2 * sizeof(gfxFloat), gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>
		>
	>,
	std::tuple<
		texture_description<0, sampler_type::nearest_clamped, kSampler_depthTexture, pixel_format_target::texture_2d>, // depth
		texture_description<1, sampler_type::nearest_clamped, kSampler_normalsTexture, pixel_format_target::texture_2d>, // normals
		texture_description<2, sampler_type::bilinear_repeat, kSampler_noiseTexture, pixel_format_target::texture_2d> // noise
	>, SHADER_SSAO_GENERATE>;

	template<>
	struct constant_buffer_type<SHADER_SSAO_BLUR>
	{
		glm::vec2 blurDirection;
		float depthSigma;
		float tapPairs;
		glm::vec4 occlusionUvScaleClamp;
		glm::vec4 depthUvScaleClamp;
	};

	using SSAOBlurPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_SSAO_BLUR>>,
	std::tuple<
		vertex_buffer_description<2 * sizeof(gfxFloat), gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>
		>
	>,
	std::tuple<
		texture_description<0, sampler_type::bilinear, kSampler_occlusionTexture, pixel_format_target::texture_2d>, // occlusion
		texture_description<1, sampler_type::nearest_clamped, kSampler_depthTexture, pixel_format_target::texture_2d> // depth
	>, SHADER_SSAO_BLUR>;

	template<>
	struct constant_buffer_type<SHADER_SSAO_DOWNSAMPLE>
	{
		glm::vec4 uvScaleClamp;
	};

	using SSAODownsamplePSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_SSAO_DOWNSAMPLE>>,
	std::tuple<
		vertex_buffer_description<2 * sizeof(gfxFloat), gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>
		>
	>,
	std::tuple<
		texture_description<0, sampler_type::bilinear, kSampler_occlusionTexture, pixel_format_target::texture_2d> // occlusion
	>, SHADER_SSAO_DOWNSAMPLE>;

	template<>
	struct constant_buffer_type<SHADER_SCENE_COMPOSE_SSAO>
	{
		float ssaoIntensity;
		float padding0;
		float padding1;
		float padding2;
		glm::vec4 sceneUvScaleClamp;
		glm::vec4 aoUvScaleClamp;
	};

	using SceneComposeSSAOPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_SCENE_COMPOSE_SSAO>>,
	std::tuple<
		vertex_buffer_description<2 * sizeof(gfxFloat), gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>
		>
	>,
	std::tuple<
		texture_description<0, sampler_type::bilinear, kSampler_sceneTexture, pixel_format_target::texture_2d>, // scene
		texture_description<1, sampler_type::bilinear, kSampler_ssaoTexture, pixel_format_target::texture_2d>, // ao
		texture_description<2, sampler_type::bilinear, kSampler_prepassNormals, pixel_format_target::texture_2d>  // prepassNormals
	>, SHADER_SCENE_COMPOSE_SSAO>;

	template<>
	struct constant_buffer_type<SHADER_SCENE_FOG>
	{
		glm::vec4 fogColor;
		glm::mat4 invProjectionMatrix;
		glm::vec4 uvScaleClamp;
		float fogBegin;
		float fogEnd;
		float padding0;
		float padding1;
	};

	using SceneFogPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_SCENE_FOG>>,
	std::tuple<
		vertex_buffer_description<2 * sizeof(gfxFloat), gfx_api::vertex_attribute_input_rate::vertex,
			vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>
		>
	>,
	std::tuple<
		texture_description<0, sampler_type::bilinear, kSampler_sceneTexture, pixel_format_target::texture_2d>, // scene
		texture_description<1, sampler_type::nearest_clamped, kSampler_prepassDepth, pixel_format_target::texture_2d> // prepassDepth
	>, SHADER_SCENE_FOG>;

	template<>
	struct constant_buffer_type<SHADER_RANGE_RING_SDF>
	{
		glm::mat4 orthoViewProj;
		glm::vec4 mapOriginExtent; // xy origin.xz, zw size.xz (0 size = no clip)
		glm::vec4 sdfParams; // x = sdfBand in world units (8-bit encode range)
	};
	static_assert(sizeof(constant_buffer_type<SHADER_RANGE_RING_SDF>) == 96, "Range ring SDF cbuffer std140 size");

	// LEQ depth write: overlapping cones in one channel keep the nearer fragment
	// (cheap union). ColorMask selects R/G/B of the packed SDF target.
	template<uint8_t ColorMask>
	using RangeRingSdfMaskedPSO = typename gfx_api::pipeline_state_helper<
		rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, ColorMask,
			polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>,
		primitive_type::triangles, index_type::u16,
		std::tuple<constant_buffer_type<SHADER_RANGE_RING_SDF>>,
		std::tuple<
			vertex_buffer_description<sizeof(glm::vec4), vertex_attribute_input_rate::vertex,
				vertex_attribute_description<position, vertex_attribute_type::float4, 0>>,
			vertex_buffer_description<sizeof(RangeRingInstance), vertex_attribute_input_rate::instance,
				vertex_attribute_description<instance_packedValues, vertex_attribute_type::float4, 0>>
		>,
		notexture,
		SHADER_RANGE_RING_SDF>;

	using RangeRingSdfSensorPSO = RangeRingSdfMaskedPSO<0x01>;
	using RangeRingSdfWeaponPSO = RangeRingSdfMaskedPSO<0x02>;
	using RangeRingSdfMinPSO = RangeRingSdfMaskedPSO<0x04>;

	template<>
	struct constant_buffer_type<SHADER_RANGE_RING_COMPOSITE>
	{
		glm::mat4 invProjectionMatrix;
		glm::mat4 invViewMatrix;
		glm::vec4 uvScaleClamp;
		glm::vec4 sdfOriginExtent;
		glm::vec4 sdfUvScaleClamp;
		glm::vec4 sensorColor;
		glm::vec4 weaponColor;
		glm::vec4 minRangeColor;
		float fillAlpha;
		float sdfBand;
		float padding1;
		float padding2;
	};
	static_assert(sizeof(constant_buffer_type<SHADER_RANGE_RING_COMPOSITE>) == 240, "Range ring composite cbuffer std140 size");

	using RangeRingCompositePSO = typename gfx_api::pipeline_state_helper<
		rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255,
			polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>,
		primitive_type::triangles, index_type::u16,
		std::tuple<constant_buffer_type<SHADER_RANGE_RING_COMPOSITE>>,
		std::tuple<
			vertex_buffer_description<2 * sizeof(gfxFloat), vertex_attribute_input_rate::vertex,
				vertex_attribute_description<position, vertex_attribute_type::float2, 0>>
		>,
		std::tuple<
			texture_description<0, sampler_type::bilinear, kSampler_sceneTexture, pixel_format_target::texture_2d>, // scene
			texture_description<1, sampler_type::nearest_clamped, kSampler_prepassDepth, pixel_format_target::texture_2d>, // prepassDepth
			texture_description<2, sampler_type::bilinear, kSampler_rangeRingSdf, pixel_format_target::texture_2d> // rangeRingSdf
		>,
		SHADER_RANGE_RING_COMPOSITE>;

}
