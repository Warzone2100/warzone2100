/*
	This file is part of Warzone 2100.
	Copyright (C) 2017-2020  Warzone 2100 Project

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

#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <tuple>
#include <functional>
#include <typeinfo>
#include <typeindex>

#include "lib/framework/frame.h"
#include "screen.h"
#include "pietypes.h"

#include <glm/glm.hpp>

namespace gfx_api
{
	// Must be implemented by backend (ex. SDL)
	class backend_Null_Impl;   // see: gfx_api_null.h
	class backend_OpenGL_Impl; // see: gfx_api_gl.h
#if defined(WZ_VULKAN_ENABLED)
	class backend_Vulkan_Impl; // see: gfx_api_vk.h
#endif
	class backend_Impl_Factory
	{
	public:
		virtual ~backend_Impl_Factory() {};
		virtual std::unique_ptr<backend_Null_Impl> createNullBackendImpl() const = 0;
		virtual std::unique_ptr<backend_OpenGL_Impl> createOpenGLBackendImpl() const = 0;
#if defined(WZ_VULKAN_ENABLED)
		virtual std::unique_ptr<backend_Vulkan_Impl> createVulkanBackendImpl() const = 0;
#endif
	};
	//
}

namespace gfx_api
{
	enum class backend_type
	{
		null_backend,
		opengl_backend,
		vulkan_backend
	};
}

namespace gfx_api
{
#ifdef GL_ONLY
	using gfxFloat = GLfloat;
	using gfxByte = GLbyte;
	using gfxUByte = GLubyte;
#else
	using gfxFloat = float;
	using gfxByte = char;
	using gfxUByte = unsigned char;
#endif

	enum class pixel_format
	{
		invalid,
		FORMAT_RGBA8_UNORM_PACK8,
		FORMAT_BGRA8_UNORM_PACK8,
		FORMAT_RGB8_UNORM_PACK8,
	};

	struct texture
	{
		virtual ~texture() {};
		virtual void bind() = 0;
		virtual void upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t& width, const size_t& height, const pixel_format& buffer_format, const void* data) = 0;
		virtual void upload_and_generate_mipmaps(const size_t& offset_x, const size_t& offset_y, const size_t& width, const size_t& height, const pixel_format& buffer_format, const void* data) = 0;
		virtual unsigned id() = 0;

		texture( const texture& other ) = delete; // non construction-copyable
		texture& operator=( const texture& ) = delete; // non copyable
		texture() {};
	};

	// An abstract base that manages a single gfx buffer
	struct buffer
	{
		enum class usage
		{
			 vertex_buffer,
			 index_buffer,
		};
		static const size_t USAGE_COUNT = 2;

		// Create a new data store for the buffer. Any existing data store will be deleted.
		// The new data store is created with the specified `size` in bytes.
		// If `data` is not NULL, the data store is initialized with `size` bytes of data from the pointer.
		// - If data is NULL, a data store of the specified `size` is still created, but its contents may be uninitialized (and thus undefined).
		//
		// NOTE: A single buffer instance should only be uploaded / updated *once per frame*.
		//       Buffer uploads / updates may be re-ordered to occur before the processing of *any* draw calls in a frame,
		//       hence re-using a single buffer instance for different data may lead to anything using that buffer (in that frame)
		//       seeing only the last-written data in the buffer.
		//       (i.e. Don't re-use a buffer instance for different data in the same frame - use separate buffer instances.)
		virtual void upload(const size_t& size, const void* data) = 0;

		enum class update_flag {
			// Default behavior
			none,

			// This flag disables asserts caused by multiple updates to a single buffer instance in a single frame
			// *ONLY* use if you are certain the updates are non-overlapping
			non_overlapping_updates_promise
		};

		// Update `size` bytes in the existing data store, from the `start` offset, using the data at `data`.
		// - The `start` offset must be within the range of the existing data store (0 to buffer_size - 1, inclusive).
		// - The `size` of the data specified (+ the offset) must not attempt to write past the end of the existing data store.
		// - A data store must be allocated before it can be updated - call `upload` on a `buffer` instance before `update`.
		//
		// NOTE: A single buffer instance should only be uploaded / updated *once per frame*.
		//       Buffer uploads / updates may be re-ordered to occur before the processing of *any* draw calls in a frame,
		//       hence re-using a single buffer instance for different data may lead to anything using that buffer (in that frame)
		//       seeing only the last-written data in the buffer.
		//       (i.e. Don't re-use a buffer instance for different data in the same frame - use separate buffer instances.)
		virtual void update(const size_t& start, const size_t& size, const void* data, const update_flag flag = update_flag::none) = 0;

		virtual void bind() = 0;

		virtual ~buffer() {};

		buffer( const buffer& other ) = delete; // non construction-copyable
		buffer& operator=( const buffer& ) = delete; // non copyable
		buffer() {};
	};

	enum class primitive_type
	{
		lines,
		line_strip,
		triangles,
		triangle_strip,
		// NOTE: Do *NOT* support triangle_fan, for portability reasons
	};

	enum class index_type
	{
		u16,
		u32,
	};

	enum class polygon_offset
	{
		enabled,
		disabled,
	};

	enum class stencil_mode
	{
		stencil_shadow_silhouette,
		stencil_shadow_quad,
		stencil_disabled,
	};

	enum class cull_mode
	{
		back,
		none,
	};

	struct state_description
	{
		const REND_MODE blend_state;
		const DEPTH_MODE depth_mode;
		const uint8_t output_mask;
		const bool offset;
		const stencil_mode stencil;
		const cull_mode cull;

		constexpr state_description(REND_MODE _blend_state, DEPTH_MODE _depth_mode, uint8_t _output_mask, polygon_offset _polygon_offset, stencil_mode _stencil, cull_mode _cull) :
		blend_state(_blend_state), depth_mode(_depth_mode), output_mask(_output_mask), offset(_polygon_offset == polygon_offset::enabled), stencil(_stencil), cull(_cull) {}
	};

	enum class vertex_attribute_type
	{
		float2,
		float3,
		float4,
		u8x4_norm,
		int1,
	};

	struct vertex_buffer_input
	{
		const std::size_t id;
		const vertex_attribute_type type;
		const std::size_t offset;

		constexpr vertex_buffer_input(std::size_t _id, vertex_attribute_type _type, std::size_t _offset)
		: id(_id), type(_type), offset(_offset)
		{}
	};

	struct vertex_buffer
	{
		const std::size_t stride;
		const std::vector<vertex_buffer_input> attributes;
		vertex_buffer(std::size_t _stride, std::vector<vertex_buffer_input>&& _attributes)
		: stride(_stride), attributes(std::forward<std::vector<vertex_buffer_input>>(_attributes))
		{}
	};

	enum class sampler_type
	{
		bilinear,
		bilinear_repeat,
		anisotropic,
		nearest_clamped,
		anisotropic_repeat,
	};

	struct texture_input
	{
		const std::size_t id;
		const sampler_type sampler;

		constexpr texture_input(std::size_t _id, sampler_type _sampler)
		: id(_id), sampler(_sampler)
		{}
	};

	struct pipeline_state_object
	{
		virtual ~pipeline_state_object() {}
	};

	struct context
	{
		enum class buffer_storage_hint
		{
				static_draw,
				stream_draw,
				dynamic_draw,
		};

		enum class context_value
		{
			MAX_ELEMENTS_VERTICES,
			MAX_ELEMENTS_INDICES,
			MAX_TEXTURE_SIZE,
			MAX_SAMPLES, // max antialiasing
		};

		enum class swap_interval_mode
		{
			adaptive_vsync = -1,
			immediate = 0,
			vsync = 1,
		};
		static const swap_interval_mode min_swap_interval_mode = swap_interval_mode::adaptive_vsync;
		static const swap_interval_mode max_swap_interval_mode = swap_interval_mode::vsync;

		virtual ~context() {};
		virtual texture* create_texture(const size_t& mipmap_count, const size_t& width, const size_t& height, const pixel_format& internal_format, const std::string& filename = "") = 0;
		virtual buffer* create_buffer_object(const buffer::usage&, const buffer_storage_hint& = buffer_storage_hint::static_draw) = 0;
		virtual pipeline_state_object* build_pipeline(const state_description&,
													  const SHADER_MODE&,
													  const gfx_api::primitive_type& primitive,
													  const std::vector<std::type_index>& uniform_blocks,
													  const std::vector<gfx_api::texture_input>& texture_desc,
													  const std::vector<vertex_buffer>& attribute_descriptions) = 0;
		virtual void bind_pipeline(pipeline_state_object*, bool notextures) = 0;
		virtual void bind_index_buffer(buffer&, const index_type&) = 0;
		virtual void unbind_index_buffer(buffer&) = 0;
		virtual void bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) = 0;
		virtual void unbind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) = 0;
		virtual void disable_all_vertex_buffers() = 0;
		virtual void bind_streamed_vertex_buffers(const void* data, const std::size_t size) = 0;
		virtual void bind_textures(const std::vector<texture_input>& attribute_descriptions, const std::vector<texture*>& textures) = 0;
		virtual void set_constants(const void* buffer, const std::size_t& size) = 0;
		virtual void set_uniforms(const size_t& first, const std::vector<std::tuple<const void*, size_t>>& uniform_blocks) = 0;
		virtual void draw(const std::size_t& offset, const std::size_t&, const primitive_type&) = 0;
		virtual void draw_elements(const std::size_t& offset, const std::size_t&, const primitive_type&, const index_type&) = 0;
		virtual void set_polygon_offset(const float& offset, const float& slope) = 0;
		virtual void set_depth_range(const float& min, const float& max) = 0;
		virtual int32_t get_context_value(const context_value property) = 0;
		static context& get();
		static bool initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode mode, gfx_api::backend_type backend);
		virtual void flip(int clearMode) = 0;
		virtual void debugStringMarker(const char *str) = 0;
		virtual void debugSceneBegin(const char *descr) = 0;
		virtual void debugSceneEnd(const char *descr) = 0;
		virtual bool debugPerfAvailable() = 0;
		virtual bool debugPerfStart(size_t sample) = 0;
		virtual void debugPerfStop() = 0;
		virtual void debugPerfBegin(PERF_POINT pp, const char *descr) = 0;
		virtual void debugPerfEnd(PERF_POINT pp) = 0;
		virtual uint64_t debugGetPerfValue(PERF_POINT pp) = 0;
		virtual std::map<std::string, std::string> getBackendGameInfo() = 0;
		virtual const std::string& getFormattedRendererInfoString() const = 0;
		virtual bool getScreenshot(std::function<void (std::unique_ptr<iV_Image>)> callback) = 0;
		virtual void handleWindowSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight) = 0;
		virtual void shutdown() = 0;
		virtual const size_t& current_FrameNum() const = 0;
		virtual bool setSwapInterval(swap_interval_mode mode) = 0;
		virtual swap_interval_mode getSwapInterval() const = 0;
	private:
		virtual bool _initialize(const backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode mode) = 0;
	};

	template<std::size_t id, vertex_attribute_type type, std::size_t offset>
	struct vertex_attribute_description
	{
		static vertex_buffer_input get_desc()
		{
			return vertex_buffer_input{id, type, offset};
		}
	};

	/**
	 * A struct templated by a tuple.
	 * Describes a buffer input.
	 * input_description describes the various vertex attributes fetched from this buffer.
	 */
	template<std::size_t stride, typename... input_description>
	struct vertex_buffer_description
	{
		static vertex_buffer get_desc()
		{
			return { stride, { input_description::get_desc()...} };
		}
	};

	template<std::size_t texture_unit, sampler_type sampler>
	struct texture_description
	{
		static texture_input get_desc()
		{
			return texture_input{ texture_unit, sampler };
		}
	};

	template<REND_MODE render_mode, DEPTH_MODE depth_mode, uint8_t output_mask, polygon_offset offset, stencil_mode stencil, cull_mode cull>
	struct rasterizer_state
	{
		static state_description get()
		{
			return state_description{ render_mode, depth_mode, output_mask, offset, stencil, cull };
		}
	};

	template<SHADER_MODE T>
	struct constant_buffer_type {};

	template<typename rasterizer, primitive_type primitive, index_type index, typename uniform_inputs, typename vertex_buffer_inputs, typename texture_inputs, SHADER_MODE shader>
	struct pipeline_state_helper
	{
		using texture_tuple = texture_inputs;

		static pipeline_state_helper<rasterizer, primitive, index, uniform_inputs, vertex_buffer_inputs, texture_inputs, shader>& get()
		{
			static pipeline_state_helper < rasterizer, primitive, index, uniform_inputs, vertex_buffer_inputs, texture_inputs, shader> object;
			return object;
		}

		void bind()
		{
			if (this->nextpso != nullptr) {
				if (this->pso != nullptr) delete this->pso;
				this->pso = this->nextpso;
				this->nextpso = nullptr;
			}
			gfx_api::context::get().bind_pipeline(pso, std::tuple_size<texture_inputs>::value == 0);
		}

		template<typename... Args>
		void bind_vertex_buffers(Args&&... args)
		{
			static_assert(sizeof...(args) == std::tuple_size<vertex_buffer_inputs>::value, "Wrong number of vertex buffer");
			gfx_api::context::get().bind_vertex_buffers(0, { std::make_tuple(args, 0)... });
		}

		template<typename... Args>
		void unbind_vertex_buffers(Args&&... args)
		{
			static_assert(sizeof...(args) == std::tuple_size<vertex_buffer_inputs>::value, "Wrong number of vertex buffer");
			gfx_api::context::get().unbind_vertex_buffers(0, { std::make_tuple(args, 0)... });
		}

		template<typename...Args>
		void bind_textures(Args&&... args)
		{
			static_assert(sizeof...(args) == std::tuple_size<texture_inputs>::value, "Wrong number of textures");
			gfx_api::context::get().bind_textures(untuple<texture_input>(texture_inputs{}), { args... });
		}

		void bind_constants(const constant_buffer_type<shader>& data)
		{
			// Vulkan: Many platforms have a maxUniformBufferRange of 64k
			// - see: https://vulkan.gpuinfo.org/displaydevicelimit.php?name=maxUniformBufferRange
			static_assert(sizeof(constant_buffer_type<shader>) <= 65536, "Constant buffer size exceeds 64k");

			gfx_api::context::get().set_constants(static_cast<const void*>(&data), sizeof(constant_buffer_type<shader>));
		}

		template<typename...Args>
		void set_uniforms(Args&&... args)
		{
			static_assert(sizeof...(args) == std::tuple_size<uniform_inputs>::value, "Wrong number of uniform inputs");
			gfx_api::context::get().set_uniforms(0, { std::make_tuple(static_cast<const void*>(&args), sizeof(args))... });
		}

		template<typename...Args>
		void set_uniforms_at(const size_t& first, Args&&... args)
		{
			gfx_api::context::get().set_uniforms(first, { std::make_tuple(static_cast<const void*>(&args), sizeof(args))... });
		}

		void draw(const std::size_t& count, const std::size_t& offset)
		{
			context::get().draw(offset, count, primitive);
		}

		void draw_elements(const std::size_t& count, const std::size_t& offset)
		{
			context::get().draw_elements(offset, count, primitive, index);
		}

		void recompile()
		{
			this->nextpso = gfx_api::context::get().build_pipeline(rasterizer::get(), shader, primitive, untuple_typeinfo(uniform_inputs{}), untuple<texture_input>(texture_inputs{}), untuple<vertex_buffer>(vertex_buffer_inputs{}));
		}
	private:
		pipeline_state_object* pso = nullptr;
		pipeline_state_object* nextpso = nullptr;
		pipeline_state_helper()
		{
			recompile();
		}

//		// Requires C++14 (+)
//		template<typename...Args>
//		auto untuple(const std::tuple<Args...>&)
//		{
//			auto type_holder = { Args::get_desc()... };
//			using output_type = decltype(type_holder);
//			return std::vector<typename output_type::value_type>(type_holder);
//		}
//
//		std::vector<gfx_api::texture_input> untuple(const std::tuple<>&)
//		{
//			return std::vector<gfx_api::texture_input>{};
//		}

		// C++11, but requires specifying Output type
		template<typename Output, typename...Args>
		std::vector<Output> untuple(const std::tuple<Args...>&)
		{
			return std::vector<Output>({ Args::get_desc()... });
		}
		template<typename...Args>
		std::vector<std::type_index> untuple_typeinfo(const std::tuple<Args...>&) const
		{
			return std::vector<std::type_index>({ std::type_index(typeid(Args))... });
		}
	};

	constexpr std::size_t position = 0;
	constexpr std::size_t texcoord = 1;
	constexpr std::size_t color = 2;
	constexpr std::size_t normal = 3;
	constexpr std::size_t tangent = 4;
	constexpr std::size_t tileNo = 5;

	using notexture = std::tuple<>;

	// NOTE: Be very careful changing these constant_buffer_type structs;
	//		 they must match std140 layout rules (see: the Vulkan shaders)
	template<>
	struct constant_buffer_type<SHADER_BUTTON>
	{
		glm::vec4 colour;
		glm::vec4 teamcolour;
		float shaderStretch;
		int tcmask;
		int fogEnabled;
		int normalMap;
		int specularMap;
		int ecmState;
		int alphaTest;
		float timeState;
		glm::mat4 ModelViewMatrix;
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 NormalMatrix;
		glm::vec4 sunPos;
		glm::vec4 sceneColor;
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		glm::vec4 fogColour;
		float fogEnd;
		float fogBegin;
		int hasTangents;
	};

	template<>
	struct constant_buffer_type<SHADER_COMPONENT>
	{
		glm::vec4 colour;
		glm::vec4 teamcolour;
		float shaderStretch;
		int tcmask;
		int fogEnabled;
		int normalMap;
		int specularMap;
		int ecmState;
		int alphaTest;
		float timeState;
		glm::mat4 ModelViewMatrix;
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 NormalMatrix;
		glm::vec4 sunPos;
		glm::vec4 sceneColor;
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		glm::vec4 fogColour;
		float fogEnd;
		float fogBegin;
		int hasTangents;
	};

	template<>
	struct constant_buffer_type<SHADER_NOLIGHT>
	{
		glm::vec4 colour;
		glm::vec4 teamcolour;
		float shaderStretch;
		int tcmask;
		int fogEnabled;
		int normalMap;
		int specularMap;
		int ecmState;
		int alphaTest;
		float timeState;
		glm::mat4 ModelViewMatrix;
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 NormalMatrix;
		glm::vec4 sunPos;
		glm::vec4 sceneColor;
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		glm::vec4 fogColour;
		float fogEnd;
		float fogBegin;
		int hasTangents;
	};

	// Only change once per frame
	struct Draw3DShapeGlobalUniforms
	{
		glm::mat4 ProjectionMatrix;
		glm::vec4 sunPos;
		glm::vec4 sceneColor;
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		glm::vec4 fogColour;
		float fogEnd;
		float fogBegin;
		float timeState;
		int fogEnabled;
	};

	// Only change per mesh
	struct Draw3DShapePerMeshUniforms
	{
		int tcmask;
		int normalMap;
		int specularMap;
		int hasTangents;
	};

	// Change per instance of mesh
	struct Draw3DShapePerInstanceUniforms
	{
		glm::mat4 ModelViewMatrix;
		glm::mat4 NormalMatrix; // can be calculated in the shader
		glm::vec4 colour;
		glm::vec4 teamcolour;
		float shaderStretch;
		int ecmState;
		int alphaTest;
	};

	template<REND_MODE render_mode, SHADER_MODE shader>
	using Draw3DShape = typename gfx_api::pipeline_state_helper<rasterizer_state<render_mode, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u16,
	std::tuple<
	Draw3DShapeGlobalUniforms,
	Draw3DShapePerMeshUniforms,
	Draw3DShapePerInstanceUniforms
	>,
	std::tuple<
	vertex_buffer_description<12, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<12, vertex_attribute_description<normal, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<8, vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float2, 0>>,
	vertex_buffer_description<16, vertex_attribute_description<tangent, gfx_api::vertex_attribute_type::float4, 0>>
	>,
	std::tuple<
	texture_description<0, sampler_type::anisotropic>, // diffuse
	texture_description<1, sampler_type::bilinear>, // team color mask
	texture_description<2, sampler_type::anisotropic>, // normal map
	texture_description<3, sampler_type::anisotropic> // specular map
	>, shader>;

	using Draw3DButtonPSO = Draw3DShape<REND_OPAQUE, SHADER_BUTTON>;
	using Draw3DShapeOpaque = Draw3DShape<REND_OPAQUE, SHADER_COMPONENT>;
	using Draw3DShapeAlpha = Draw3DShape<REND_ALPHA, SHADER_COMPONENT>;
	using Draw3DShapePremul = Draw3DShape<REND_PREMULTIPLIED, SHADER_COMPONENT>;
	using Draw3DShapeAdditive = Draw3DShape<REND_ADDITIVE, SHADER_COMPONENT>;
	using Draw3DShapeNoLightOpaque = Draw3DShape<REND_OPAQUE, SHADER_NOLIGHT>;
	using Draw3DShapeNoLightAlpha = Draw3DShape<REND_ALPHA, SHADER_NOLIGHT>;
	using Draw3DShapeNoLightPremul = Draw3DShape<REND_PREMULTIPLIED, SHADER_NOLIGHT>;
	using Draw3DShapeNoLightAdditive = Draw3DShape<REND_ADDITIVE, SHADER_NOLIGHT>;

	template<>
	struct constant_buffer_type<SHADER_GENERIC_COLOR>
	{
		glm::mat4 transform_matrix;
		glm::vec2 unused;
		glm::vec2 unused2;
		glm::vec4 colour;
	};

	using TransColouredTrianglePSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ADDITIVE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_GENERIC_COLOR>>,
	std::tuple<
	vertex_buffer_description<12, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>
	>, notexture, SHADER_GENERIC_COLOR>;
	using DrawStencilShadow = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_OFF, 0, polygon_offset::disabled, stencil_mode::stencil_shadow_silhouette, cull_mode::none>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_GENERIC_COLOR>>,
	std::tuple<
	vertex_buffer_description<12, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>
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
		glm::vec4 fog_colour;
		int fog_enabled;
		float fog_begin;
		float fog_end;
		int texture0;
		int texture1;
	};

	using TerrainDepth = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 0, polygon_offset::enabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_TERRAIN_DEPTH>>,
	std::tuple<
	vertex_buffer_description<12, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>
	>, std::tuple<texture_description<0, sampler_type::bilinear_repeat>>, SHADER_TERRAIN_DEPTH>;

	template<>
	struct constant_buffer_type<SHADER_TERRAIN>
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ModelUVMatrix;
		glm::mat4 ModelUVLightMatrix;
		glm::vec4 cameraPos; // in modelSpace
		glm::vec4 sunPos; // in modelSpace
		glm::vec4 emissiveLight; // light colors/intensity
		glm::vec4 ambientLight;
		glm::vec4 diffuseLight;
		glm::vec4 specularLight;
		glm::vec4 fog_colour;
		int fog_enabled;
		float fog_begin;
		float fog_end;
		int quality;
		int hasNormalmap;
		int hasSpecularmap;
		int hasHeightmap;
	};

	using TerrainLayer = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ADDITIVE, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_TERRAIN>>,
	std::tuple<
	vertex_buffer_description<12, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>,
	vertex_buffer_description<4, vertex_attribute_description<color, gfx_api::vertex_attribute_type::u8x4_norm, 0>>,
	vertex_buffer_description<12, vertex_attribute_description<normal, gfx_api::vertex_attribute_type::float3, 0>>
	>, std::tuple<
	texture_description<0, sampler_type::anisotropic_repeat>,
	texture_description<1, sampler_type::bilinear>,
	texture_description<2, sampler_type::anisotropic_repeat>, // normal map
	texture_description<3, sampler_type::anisotropic_repeat>, // specular map
	texture_description<4, sampler_type::anisotropic_repeat> // height map
	>, SHADER_TERRAIN>;

	template<>
	struct constant_buffer_type<SHADER_DECALS>
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ModelUVLightmapMatrix;
		glm::vec4 cameraPos; // in modelSpace
		glm::vec4 sunPos; // in modelSpace
		glm::vec4 emissiveLight; // light colors/intensity
		glm::vec4 ambientLight;
		glm::vec4 diffuseLight;
		glm::vec4 specularLight;
		glm::vec4 fog_colour;
		int fog_enabled;
		float fog_begin;
		float fog_end;
		int quality;
	};

	using TerrainDecals = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_DECALS>>,
	std::tuple<
	vertex_buffer_description<2*sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec4) + sizeof(int32_t), // DecalVertex struct
	vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>,
	vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float2, sizeof(glm::vec3)>,
	vertex_attribute_description<normal,   gfx_api::vertex_attribute_type::float3, sizeof(glm::vec3) + sizeof(glm::vec2)>,
	vertex_attribute_description<tangent,  gfx_api::vertex_attribute_type::float4, 2*sizeof(glm::vec3) + sizeof(glm::vec2)>,
	vertex_attribute_description<tileNo,   gfx_api::vertex_attribute_type::int1,   2*sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec4)>
	>
	>, std::tuple<
	texture_description<0, sampler_type::anisotropic>,
	texture_description<1, sampler_type::bilinear>,
	texture_description<2, sampler_type::anisotropic>, // normal map
	texture_description<3, sampler_type::anisotropic>, // specular map
	texture_description<4, sampler_type::anisotropic>  // height map
	>, SHADER_DECALS>;

	template<>
	struct constant_buffer_type<SHADER_WATER>
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ModelUV1Matrix;
		glm::mat4 ModelUV2Matrix;
		glm::vec4 cameraPos; // in modelSpace
		glm::vec4 sunPos; // in modelSpace
		glm::vec4 emissiveLight; // light colors/intensity
		glm::vec4 ambientLight;
		glm::vec4 diffuseLight;
		glm::vec4 specularLight;
		glm::vec4 fog_colour;
		int fog_enabled;
		float fog_begin;
		float fog_end;
		float timeSec;
		int quality;
	};

	using WaterPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_MULTIPLICATIVE, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_WATER>>,
	std::tuple<
	vertex_buffer_description<sizeof(glm::vec4), vertex_attribute_description<position, gfx_api::vertex_attribute_type::float4, 0>> // WaterVertex, w is depth
	>, std::tuple<
	texture_description<0, sampler_type::anisotropic_repeat>, // tex1
	texture_description<1, sampler_type::anisotropic_repeat>, // tex2
	texture_description<2, sampler_type::anisotropic_repeat>, // normal map1
	texture_description<3, sampler_type::anisotropic_repeat>, // normal map2
	texture_description<4, sampler_type::anisotropic_repeat>, // specular map1
	texture_description<5, sampler_type::anisotropic_repeat>, // specular map2
	texture_description<6, sampler_type::anisotropic_repeat>, // height map1
	texture_description<7, sampler_type::anisotropic_repeat>  // height map2
	>, SHADER_WATER>;

	using gfx_tc = vertex_buffer_description<8, vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float2, 0>>;
	using gfx_colour = vertex_buffer_description<4, vertex_attribute_description<color, gfx_api::vertex_attribute_type::u8x4_norm, 0>>;
	using gfx_vtx2 = vertex_buffer_description<8, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0>>;
	using gfx_vtx3 = vertex_buffer_description<12, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>;

	template<>
	struct constant_buffer_type<SHADER_GFX_TEXT>
	{
		glm::mat4 transform_matrix;
		glm::vec2 offset;
		glm::vec2 size;
		glm::vec4 color;
		int texture;
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
	using VideoPSO = GFX<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_tc, SHADER_GFX_TEXT, std::tuple<texture_description<0, gfx_api::sampler_type::bilinear>>>;
	using BackDropPSO = GFX<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_tc, SHADER_GFX_TEXT, std::tuple<texture_description<0, gfx_api::sampler_type::nearest_clamped>>>;
	using SkyboxPSO = typename gfx_api::pipeline_state_helper<
		rasterizer_state<REND_ALPHA, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>,
		primitive_type::triangles,
		index_type::u16,
		std::tuple<constant_buffer_type<SHADER_SKYBOX>>,
		std::tuple<
			gfx_vtx3, gfx_tc
		>,
		std::tuple<texture_description<0, gfx_api::sampler_type::bilinear_repeat>>,
		SHADER_SKYBOX
	>;



	using RadarPSO = GFX<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_tc, SHADER_GFX_TEXT, std::tuple<texture_description<0, gfx_api::sampler_type::nearest_clamped>>>;
	using RadarViewInsideFillPSO = GFX<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_colour, SHADER_GFX_COLOUR, notexture>;
	using RadarViewOutlinePSO = GFX<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::line_strip, gfx_vtx2, gfx_colour, SHADER_GFX_COLOUR, notexture>;

	template<>
	struct constant_buffer_type<SHADER_TEXT>
	{
		glm::mat4 transform_matrix;
		glm::vec2 offset;
		glm::vec2 size;
		glm::vec4 color;
		int texture;
	};

	using DrawImageTextPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_TEXT, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_TEXT>>,
	std::tuple<
	vertex_buffer_description<4, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>,
	std::tuple<texture_description<0, sampler_type::bilinear>>, SHADER_TEXT>;

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
	vertex_buffer_description<4, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, notexture, SHADER_RECT>;
	using UniTransBoxPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_RECT>>,
	std::tuple<
	vertex_buffer_description<4, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, notexture, SHADER_RECT>;

	template<>
	struct constant_buffer_type<SHADER_TEXRECT>
	{
		glm::mat4 transform_matrix;
		glm::vec2 offset;
		glm::vec2 size;
		glm::vec4 color;
		int texture;
	};

	using DrawImagePSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_TEXRECT>>,
	std::tuple<
	vertex_buffer_description<4, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, std::tuple<texture_description<0, sampler_type::bilinear>>, SHADER_TEXRECT>;

	using DrawImageAnisotropicPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_TEXRECT>>,
	std::tuple<
	vertex_buffer_description<4, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, std::tuple<texture_description<0, sampler_type::anisotropic>>, SHADER_TEXRECT>;

	using BoxFillPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_RECT>>,
	std::tuple<
	vertex_buffer_description<4, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, notexture, SHADER_RECT>;
	using BoxFillAlphaPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_shadow_quad, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_RECT>>,
	std::tuple<
	vertex_buffer_description<4, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, notexture, SHADER_RECT>;

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
	vertex_buffer_description<4, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, notexture, SHADER_LINE>;
}

static inline int to_int(gfx_api::context::swap_interval_mode mode)
{
	return static_cast<int>(mode);
}

static inline gfx_api::context::swap_interval_mode to_swap_mode(int value)
{
	switch (value) {
		case static_cast<int>(gfx_api::context::swap_interval_mode::immediate):
			return gfx_api::context::swap_interval_mode::immediate;
		case static_cast<int>(gfx_api::context::swap_interval_mode::vsync):
			return gfx_api::context::swap_interval_mode::vsync;
		case static_cast<int>(gfx_api::context::swap_interval_mode::adaptive_vsync):
			return gfx_api::context::swap_interval_mode::adaptive_vsync;
		default:
			debug(LOG_WARNING, "Invalid vsync value (%d); defaulting to vsync ON", value);
	}
	return gfx_api::context::swap_interval_mode::vsync;
}
