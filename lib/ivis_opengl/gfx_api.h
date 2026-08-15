// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2017-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <tuple>
#include <functional>
#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <array>

#include "lib/framework/frame.h"
#include "lib/framework/wzstring.h"
#include "lib/framework/loading_task_fwd.h"
#include "screen.h"
#include "pietypes.h"
#include "gfx_api_formats_def.h"
#include "render_graph/pipeline_surfaces.h"
#include "render_graph/render_pass.h"
#include "render_graph/compile.h"

#include <glm/glm.hpp>

#include "shadows.h"

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

class ResourceLoadingController;

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

	struct abstract_texture
	{
		virtual void bind() = 0;
		virtual bool isArray() const = 0;
		virtual void flush() {};
		virtual size_t backend_internal_value() const = 0; // used for backend-specific internal purposes
		virtual ~abstract_texture() {};
	};

	struct texture2dDimensions
	{
		size_t width = 0;
		size_t height = 0;
	};

	struct texture : abstract_texture
	{
		virtual bool upload(const size_t& mip_level, const iV_BaseImage& image) = 0;
		virtual bool upload_sub(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const iV_BaseImage& image) = 0;
		virtual unsigned id() = 0;
		virtual texture2dDimensions get_dimensions() const = 0;
		bool isArray() const { return false; }

		texture( const texture& other ) = delete; // non construction-copyable
		texture& operator=( const texture& ) = delete; // non copyable
		texture() {};
	};

	struct texture_array : abstract_texture
	{
		virtual bool upload_layer(const size_t& layer, const size_t& mip_level, const iV_BaseImage& image) = 0;
		virtual unsigned id() = 0;
		bool isArray() const { return true; }

		texture_array( const texture_array& other ) = delete; // non construction-copyable
		texture_array& operator=( const texture_array& ) = delete; // non copyable
		texture_array() {};
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

		virtual size_t current_buffer_size() = 0;

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
		patch_list_4, // 4-control-point patches (requires context::supportsTessellationShaders())
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
		front,
		shadow_mapping,
		none,
	};

	struct state_description
	{
		const REND_MODE blend_state;
		const DEPTH_MODE depth_mode;
		/// Color write mask: bit 0 = R, 1 = G, 2 = B, 3 = A. 0 writes nothing; 255 writes RGBA.
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
//		mat4,
		u8x4_norm,

		// not guaranteed to be supported on all systems:
		u8x4_uint,
		int1
	};

	enum class vertex_attribute_input_rate
	{
		vertex,
		instance
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
		const vertex_attribute_input_rate rate;
		const std::vector<vertex_buffer_input> attributes;
		vertex_buffer(std::size_t _stride, vertex_attribute_input_rate _rate, std::vector<vertex_buffer_input>&& _attributes)
		: stride(_stride), rate(_rate), attributes(std::forward<std::vector<vertex_buffer_input>>(_attributes))
		{}
	};

	enum class sampler_type
	{
		bilinear,
		bilinear_repeat,
		bilinear_border,
		anisotropic,
		nearest_clamped,
		nearest_border,
		anisotropic_repeat,
	};

	enum class border_color
	{
		none,
		transparent_black,
		opaque_black,
		opaque_white
	};

	/// Which stage samples a texture
	enum class shader_stage : uint8_t
	{
		fragment,
		tessellation_evaluation,
	};

	struct texture_input
	{
		const std::size_t id;
		const sampler_type sampler;
		const pixel_format_target target;
		const border_color border;
		const shader_stage stage;

		constexpr texture_input(std::size_t _id, sampler_type _sampler, pixel_format_target _target, border_color _border, shader_stage _stage = shader_stage::fragment)
		: id(_id), sampler(_sampler), target(_target), border(_border), stage(_stage)
		{}
	};

	struct pipeline_state_object
	{
		virtual ~pipeline_state_object() {}
		bool broken = false;
	};

	struct pipeline_create_info
	{
		gfx_api::state_description state_desc;
		SHADER_MODE shader_mode;
		gfx_api::primitive_type primitive;
		std::vector<std::type_index> uniform_blocks;
		std::vector<gfx_api::texture_input> texture_desc;
		std::vector<gfx_api::vertex_buffer> attribute_descriptions;

		pipeline_create_info(const gfx_api::state_description &state_desc, const SHADER_MODE& shader_mode, const gfx_api::primitive_type& primitive,
								  const std::vector<std::type_index>& uniform_blocks,
								  const std::vector<gfx_api::texture_input>& texture_desc,
								  const std::vector<gfx_api::vertex_buffer>& attribute_descriptions)
		: state_desc(state_desc)
		, shader_mode(shader_mode)
		, primitive(primitive)
		, uniform_blocks(uniform_blocks)
		, texture_desc(texture_desc)
		, attribute_descriptions(attribute_descriptions)
		{
			// Vulkan binds these by position and OpenGL by declared unit, which agree only while ids run 0, 1, 2 in order
			for (size_t i = 0; i < texture_desc.size(); ++i)
			{
				ASSERT(texture_desc[i].id == i, "Texture %zu declares unit %zu", i, texture_desc[i].id);
			}
		}
	};

	struct lighting_constants
	{
		uint32_t shadowMode = 1;
		uint32_t shadowFilterSize = 5;
		uint32_t shadowCascadesCount = WZ_MAX_SHADOW_CASCADES;
		bool isPointLightPerPixelEnabled = false;

		bool operator==(const lighting_constants& rhs) const
		{
			return shadowMode == rhs.shadowMode
			&& shadowFilterSize == rhs.shadowFilterSize
			&& shadowCascadesCount == rhs.shadowCascadesCount
			&& isPointLightPerPixelEnabled == rhs.isPointLightPerPixelEnabled;
		}
	};

	/// A constant block uploaded once for the current frame and bound by every pipeline that reads it
	struct frame_uniform_allocation
	{
		uint64_t handle = 0; // OpenGL: buffer name. Vulkan: 1-based index into the frame's uploaded blocks
		uint32_t offset = 0;
		uint32_t size = 0;
		uint32_t generation = 0;
		bool valid() const { return generation != 0; }
	};

	/// Typed handle to a frame uniform block - only context::upload_frame_uniform can produce a valid one
	/// (The type is what set_uniforms_at validates the pipeline slot against)
	template<typename T>
	class frame_uniform_block_ref
	{
	public:
		frame_uniform_block_ref() = default;
		bool valid() const { return allocation_.valid(); }
		const frame_uniform_allocation& allocation() const { return allocation_; }
	private:
		friend struct context;
		explicit frame_uniform_block_ref(const frame_uniform_allocation& allocation) : allocation_(allocation) {}
		frame_uniform_allocation allocation_;
	};

	/// How per frame point light data reaches the shaders
	enum class data_buffer_transport
	{
		uniform_block,   ///< std140 arrays in the point lights block, available everywhere
		texel_buffer,    ///< samplerBuffer read with texelFetch, desktop GL 3.1 and Vulkan
		storage_buffer,  ///< readonly std430 buffer block, desktop GL 4.3 and Vulkan
	};

	const char* to_string(data_buffer_transport transport);

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
			MAX_TEXTURE_SIZE,
			MAX_SAMPLES, // max antialiasing
			MAX_ARRAY_TEXTURE_LAYERS,
			MAX_VERTEX_ATTRIBS,
			MAX_VERTEX_OUTPUT_COMPONENTS,
			MAX_TESS_GEN_LEVEL, // 0 if supportsTessellationShaders() is false
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
		virtual texture_array* create_texture_array(const size_t& mipmap_count, const size_t& layer_count, const size_t& width, const size_t& height, const gfx_api::pixel_format& internal_format, const std::string& filename = "") = 0;
		virtual buffer* create_buffer_object(const buffer::usage&, const buffer_storage_hint& = buffer_storage_hint::static_draw, const std::string& debugName = "") = 0;
		virtual pipeline_state_object* build_pipeline(pipeline_state_object* existing_pso,
													  const pipeline_create_info& createInfo) = 0;
		virtual void bind_pipeline(pipeline_state_object*, bool notextures) = 0;
		virtual void bind_index_buffer(buffer&, const index_type&) = 0;
		virtual void unbind_index_buffer(buffer&) = 0;
		virtual void bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) = 0;
		virtual void unbind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) = 0;
		virtual void disable_all_vertex_buffers() = 0;
		virtual void bind_streamed_vertex_buffers(const void* data, const std::size_t size) = 0;
		virtual void bind_textures(const std::vector<texture_input>& attribute_descriptions, const std::vector<abstract_texture*>& textures) = 0;
		virtual void set_constants(const void* buffer, const std::size_t& size) = 0;
		virtual void set_uniforms(const size_t& first, const std::vector<std::tuple<const void*, size_t>>& uniform_blocks) = 0;
		/// Upload a constant block once for the current frame.
		/// The returned reference stays usable until the next frame begins, and may be bound by any number of pipelines.
		template<typename T>
		frame_uniform_block_ref<T> upload_frame_uniform(const T& data)
		{
			// Vulkan: many platforms have a maxUniformBufferRange of 64k
			// - see: https://vulkan.gpuinfo.org/displaydevicelimit.php?name=maxUniformBufferRange
			static_assert(sizeof(T) <= 65536, "Uniform block size exceeds 64k");
			return frame_uniform_block_ref<T>(upload_frame_uniform_raw(&data, sizeof(T)));
		}
		virtual frame_uniform_allocation upload_frame_uniform_raw(const void* data, size_t size) = 0;
		/// Bind a frame uniform to a slot of the bound pipeline.
		virtual void set_frame_uniform_at(size_t slot, const frame_uniform_allocation& allocation, std::type_index type) = 0;
		uint32_t frameUniformGeneration() const { return _frameUniformGeneration; }
		/// Which transport carries the point light arrays.
		virtual data_buffer_transport lightDataTransport() const { return data_buffer_transport::uniform_block; }
		/// Replace this frame's point light data.
		virtual void upload_light_data(const void* lights, size_t lightBytes, const void* indices, size_t indexBytes)
		{
			// Does nothing on the uniform block transport, where the same data travels in the block instead.
			(void)lights; (void)lightBytes; (void)indices; (void)indexBytes;
		}
		virtual void draw(const std::size_t& offset, const std::size_t&, const primitive_type&) = 0;
		virtual void draw_elements(const std::size_t& offset, const std::size_t&, const primitive_type&, const index_type&) = 0;
		// Depth-only polygon offset with glPolygonOffset(factor, units) semantics:
		// factor scales the primitive's max depth slope, units scales the minimum
		// resolvable depth difference. (Vulkan: slopeFactor / constantFactor.)
		virtual void set_polygon_offset(const float& factor, const float& units) = 0;
		virtual void set_depth_range(const float& min, const float& max) = 0;
		virtual int32_t get_context_value(const context_value property) = 0;
		virtual uint64_t get_estimated_vram_mb(bool dedicatedOnly) = 0;
		static context& get();
		static bool initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode mode, optional<float> mipLodBias, uint32_t depthMapResolution, gfx_api::backend_type backend);
		static bool isInitialized();
		virtual size_t numDepthPasses() { return 0; }
		virtual bool setDepthPassProperties(size_t numDepthPasses, size_t depthBufferResolution) { return false; }
		virtual void beginPass(const RenderPassDesc& pass, const CompiledPass* compiledPass = nullptr) = 0;
		virtual void endPass(const CompiledPass* compiledPass = nullptr) = 0;

		/// Screen-frame lifecycle (owned by piemode / main loop):
		///
		///   pie_ScreenFrameRenderBegin()
		///     -> wzProcessPendingWindowChanges() // apply queued window mode / resize / display scale
		///     -> reconcileSwapchainAtFrameOpen()  // Vulkan: recreate/surface-lost only; no acquire
		///                                         // (before beginScreenFrame; !_screenFrameOpen)
		///     -> beginScreenFrame()               // per-frame flag reset; releaseAll() on FBO pool;
		///                                         // Vulkan: open transfer (copy) command buffer
		///     -> consumeScreenGeometryDirty() -> screen_updateGeometry() when drawable changed
		///     ... game logic, uploads (TransferRecorder records copy work on Vulkan) ...
		///   pie_ScreenFrameRenderEnd()
		///     ... when !headlessOrSkipDrawing() ...
		///     -> acquireSwapchainForFrameDraw()   // Vulkan: acquire only; never recreate
		///     -> CachedRenderGraph::ensureBuilt(snapshot) + execute() when canRecordSwapchainDraws()
		///     -> finishScreenFrame()              // ALWAYS when gfx initialized: seal/submit/present,
		///                                         // ring advance; WSI recovery deferred to next Begin
		///
		/// Upload paths record into the current frame's copy command buffer throughout the
		/// screen frame. finishScreenFrame() must NOT be gated on graph execution or shouldDraw().
		/// Must pair beginScreenFrame() with finishScreenFrame().
		virtual void beginScreenFrame() = 0;
		virtual void finishScreenFrame() = 0;
		/// Vulkan: recreate swapchain / recover surface at frame open when needed.
		/// Must be called before beginScreenFrame() while no screen frame is open. Does not acquire.
		virtual void reconcileSwapchainAtFrameOpen() {}
		/// Vulkan: acquire a swapchain image immediately before render-graph record.
		/// Must not destroy/recreate the swapchain (defer WSI errors to next Begin reconcile).
		virtual void acquireSwapchainForFrameDraw() {}
		/// True when swapchain image is acquired and safe to record swapchain-targeting passes (Vulkan).
		virtual bool canRecordSwapchainDraws() const { return true; }
		/// Mark UI/backdrop geometry stale after a drawable resize (called from backends).
		/// Consumed at pie_ScreenFrameRenderBegin(); do not call screen_updateGeometry() directly on resize.
		void markScreenGeometryDirty() { _screenGeometryDirty = true; }
		/// Returns true once per dirty signal; used to refresh backdrop VBOs without per-frame work.
		bool consumeScreenGeometryDirty()
		{
			const bool dirty = _screenGeometryDirty;
			_screenGeometryDirty = false;
			return dirty;
		}

		virtual size_t getDepthPassDimensions(size_t idx) { return 0; }
		virtual gfx_api::abstract_texture* getPipelineSurface(PipelineSurfaceId id) { return nullptr; }
		virtual PipelineSurfaceUsage pipelineSurfaceUsage(PipelineSurfaceId id) const;
		virtual const ResolvedSurfaceSpec& resolvedPipelineSurface(PipelineSurfaceId id) const;
		bool isPipelineSurfaceDepthRole(PipelineSurfaceId id) const
		{
			return isDepthUsage(pipelineSurfaceUsage(id));
		}
		/// Dimensions from the live resolved surface spec (nullopt if disabled / zero extent).
		virtual optional<std::pair<uint32_t, uint32_t>> getPipelineSurfaceDimensions(PipelineSurfaceId id) const;
		/// Sync inputs with sceneW/H replaced by this frame's used scene size (dyn-res fraction).
		/// Drawable, shadow, enable flags, and SSAO divisors stay as last sync / allocation.
		PipelineSurfaceSyncInputs usedSceneSyncInputs();
		/// Catalog used extent for `id` (`MatchScene` / `MatchSceneDivided` track dyn-res).
		std::pair<uint32_t, uint32_t> usedPipelineSurfaceExtent(PipelineSurfaceId id);
		virtual bool isSceneMSAAEnabled() const { return false; }
		virtual bool isSwapchainMSAAEnabled() const { return false; }
		virtual bool isMultisampledColorAttachment(abstract_texture* texture) const { return false; }
		virtual pixel_format getDepthStencilFormat() const { return pixel_format::invalid; }

		/// Per-sync sizes / MSAA / cascades / present format from backend state.
		virtual PipelineSurfaceSyncInputs pipelineSurfaceSyncInputs() const { return {}; }
		/// Backend HW-negotiated formats for catalog format classes.
		virtual SurfaceCapabilityHints surfaceCapabilities() const { return {}; }
		/// Create / update / destroy pipeline surfaces from resolved specs (store + allocator).
		virtual bool ensurePipelineSurfaces(const ResolvedSurfaceTable& specs) = 0;
		/// Resolve catalog against sync inputs + capabilities, then ensure surfaces.
		bool syncPipelineSurfaces();

		/// Purge unused pooled framebuffers/FBOs after the frame accumulation window.
		/// Backends call releaseAll() at beginScreenFrame() and purgeFrameResources() at finish.
		virtual void purgeFrameResources() {}

		virtual optional<std::pair<uint32_t, uint32_t>> getRenderTargetDimensions(abstract_texture* texture) { return nullopt; }

		/// Dimensions of the scene area rendered this frame (the scene color target
		/// scaled by the dynamic render fraction), for the scene pass viewport and
		/// shader inputs that map gl_FragCoord within the scene pass. Falls back to
		/// the drawable size when no scene target exists.
		std::pair<uint32_t, uint32_t> getSceneRenderTargetDimensions();

		/// The mip LOD bias applied by material shaders when sampling textures
		/// (feeds the WZ_MIP_LOAD_BIAS uniform each frame)
		float getMipLodBias() const { return _mipLodBias.value_or(0.f); }
		void setMipLodBias(optional<float> bias) { _mipLodBias = bias; }

		static constexpr uint32_t minSceneRenderScalePercent = 10;
		static constexpr uint32_t maxSceneRenderScalePercent = 100;
		/// Percentage of the drawable size used for the scene render targets (100 = native)
		uint32_t getSceneRenderScalePercent() const { return _sceneRenderScalePercent; }
		/// Resize the scene render targets to a percentage of the drawable size.
		/// Backend overrides store the clamped value via this base implementation
		/// and then rebuild their scene targets to match.
		virtual bool setSceneRenderScale(uint32_t scalePercent)
		{
			_sceneRenderScalePercent = std::clamp(scalePercent, minSceneRenderScalePercent, maxSceneRenderScalePercent);
			return true;
		}
		static constexpr float minSceneRenderFraction = 0.5f;
		/// Dynamic resolution: the fraction of the scene render targets actually
		/// rendered to this frame. Changing it never reallocates targets and never
		/// rebuilds the render graph (the scene pass viewport, the scene shader
		/// dimensions, and the upscale constants all track it at execute time).
		float getSceneRenderFraction() const { return _sceneRenderFraction; }
		void setSceneRenderFraction(float fraction)
		{
			_sceneRenderFraction = std::clamp(fraction, minSceneRenderFraction, 1.f);
		}
		/// Marks dynamic resolution active. Backend overrides keep the FSR1 upscale
		/// chain and its intermediate surface alive even while the scene targets
		/// match the drawable size, so the pass topology stays stable per frame.
		virtual bool setSceneDynamicResolution(bool enabled)
		{
			_sceneDynamicResolution = enabled;
			return true;
		}
		bool sceneDynamicResolutionEnabled() const { return _sceneDynamicResolution; }

		/// The mip LOD bias for material shaders sampling in the scene pass, adding
		/// log2 of the effective scene render scale so texture detail matches the
		/// output size
		float getSceneMipLodBias() const
		{
			float bias = getMipLodBias();
			const float effectiveScale = (static_cast<float>(_sceneRenderScalePercent) / 100.f) * _sceneRenderFraction;
			if (effectiveScale < 1.f)
			{
				bias += std::log2(effectiveScale);
			}
			return bias;
		}

		/// A completed GPU frame timing measurement from timestamp queries
		struct GpuFrameTiming
		{
			/// The frame the measurement belongs to (current_FrameNum numbering)
			size_t frameNum = 0;
			/// GPU time between the frame's first and last rendering commands
			uint64_t durationNs = 0;
		};
		/// True when the backend can measure GPU frame time with timestamp queries
		virtual bool supportsGpuFrameTiming() const { return false; }
		/// Enable or disable per frame GPU timestamp queries.
		/// Results are read without stalling, so each measurement becomes
		/// available a few frames after the frame it measures.
		virtual bool setGpuFrameTimingEnabled(bool enabled) { return !enabled; }
		bool gpuFrameTimingEnabled() const { return _gpuFrameTimingEnabled; }
		/// The most recently completed measurement (compare frameNum to detect new samples)
		optional<GpuFrameTiming> getLastGpuFrameTiming() const { return _lastGpuFrameTiming; }

		/// How the reduced-resolution scene is upscaled to the drawable
		enum class scene_upscaling_mode : uint8_t
		{
			bilinear,
			fsr1,
		};
		scene_upscaling_mode getSceneUpscalingMode() const { return _sceneUpscalingMode; }
		/// Backend overrides store the value via this base implementation and rebuild
		/// their scene targets, creating or dropping the upscale intermediate surface.
		virtual bool setSceneUpscalingMode(scene_upscaling_mode mode)
		{
			_sceneUpscalingMode = mode;
			return true;
		}

		/// SMAA 1x post-process antialiasing over the scene color
		bool smaaEnabled() const { return _smaaEnabled; }
		/// Backend overrides store the value via this base implementation and rebuild
		/// their scene targets, creating or dropping the SMAA surfaces.
		virtual bool setSmaaEnabled(bool enabled)
		{
			_smaaEnabled = enabled;
			return true;
		}

		/// Commit SSAO/fog/range-ring catalog requests in one store. Backends rebuild surfaces.
		/// Scene prepass follows `prepassNeeds(cfg)` in pipelineSurfaceSyncInputs().
		virtual bool setSceneEffectSurfaces(SceneEffectSurfaces cfg)
		{
			storeSceneEffectSurfaces(cfg);
			return true;
		}
		/// Last committed catalog request (allocated surfaces, not raw gameplay config).
		SceneEffectSurfaces storedSceneEffectSurfaces() const { return _sceneEffectSurfaces; }

		/// Record draw commands for a compiled pass graph (beginPass / recordFunc / endPass).
		/// Does not submit, present, or advance the frame ring; piemode calls finishScreenFrame()
		/// afterward for GPU commit.
		virtual void executeCompiledRenderGraph(std::vector<RenderPassDesc>& passes,
			const PassGraphCompileResult& compileResult);
		/// Pre-warm backend pass resources after compile (e.g. Vulkan render-pass layout ids).
		virtual void warmCompiledRenderGraph(std::vector<RenderPassDesc>& passes,
			PassGraphCompileResult& compileResult);
		// Emit any out-of-graph pre-pass layout/sync barriers for a batch of passes (the passes
		// sharing one render pass). Called once before beginPass, outside any active render pass.
		virtual void emitPrePassBarriers(const ExecutionBatch& batch,
			const std::vector<CompiledPass>& compiledPasses) {}
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
		virtual std::pair<uint32_t, uint32_t> getDrawableDimensions() = 0;
		virtual bool isYAxisInverted() const = 0;
		virtual bool shouldDraw() = 0;
		virtual void shutdown() = 0;
		virtual const size_t& current_FrameNum() const = 0;
		typedef std::function<void()> SetSwapIntervalCompletionHandler;
		virtual bool setSwapInterval(swap_interval_mode mode, const SetSwapIntervalCompletionHandler& completionHandler) = 0;
		virtual swap_interval_mode getSwapInterval() const = 0;
		virtual bool textureFormatIsSupported(pixel_format_target target, pixel_format format, pixel_format_usage::flags usage) = 0;
		virtual bool supportsMipLodBias() const = 0;
		virtual bool supports2DTextureArrays() const = 0;
		virtual bool supportsIntVertexAttributes() const = 0;
		virtual size_t maxFramesInFlight() const = 0;
		virtual lighting_constants getShadowConstants() = 0;
		virtual bool setShadowConstants(lighting_constants values) = 0;

		/// Monotonic counter bumped when render-graph topology inputs change (resize, depth passes, swapchain recreate, etc.).
		uint64_t getRenderGraphEpoch() const { return _renderGraphEpoch; }
		// tessellation shader support (primitive_type::patch_list_4 + TCS/TES pipeline stages)
		virtual bool supportsTessellationShaders() const { return false; }
		// instanced rendering APIs
		virtual bool supportsInstancedRendering() = 0;
		virtual void draw_instanced(const std::size_t& offset, const std::size_t &count, const primitive_type &primitive, std::size_t instance_count) = 0;
		virtual void draw_elements_instanced(const std::size_t& offset, const std::size_t& count, const primitive_type& primitive, const index_type& index, std::size_t instance_count) = 0;
		// debug apis for recompiling pipelines
		virtual bool debugRecompileAllPipelines() = 0;
	public:
		// High-level API for getting a texture object from file / uncompressed bitmap
		gfx_api::texture* loadTextureFromFile(const char *filename, gfx_api::texture_type textureType, int maxWidth = -1, int maxHeight = -1, bool quiet = false);
		gfx_api::texture* loadTextureFromUncompressedImage(iV_Image&& image, gfx_api::texture_type textureType, const std::string& filename, int maxWidth = -1, int maxHeight = -1);
		typedef std::function<std::unique_ptr<iV_Image> (int width, int height, int channels)> GenerateDefaultTextureFunc;
		LoadingTask<gfx_api::texture_array*> loadTextureArrayFromFiles(ResourceLoadingController& controller, const std::vector<WzString>& filenames, gfx_api::texture_type textureType, int maxWidth = -1, int maxHeight = -1, const GenerateDefaultTextureFunc& defaultTextureGenerator = nullptr, const std::string& debugName = "");
		gfx_api::texture_array* loadTextureArrayFromFilesBlocking(const std::vector<WzString>& filenames, gfx_api::texture_type textureType, int maxWidth = -1, int maxHeight = -1, const GenerateDefaultTextureFunc& defaultTextureGenerator = nullptr, const std::string& debugName = "");

		bool loadTextureArrayLayerFromBaseImages(gfx_api::texture_array& array, size_t layer, const std::vector<std::unique_ptr<iV_BaseImage>>& images, const std::string& filename, int maxWidth = -1, int maxHeight = -1);

		optional<unsigned int> getClosestSupportedUncompressedImageFormatChannels(pixel_format_target target, unsigned int channels);
		gfx_api::pixel_format bestUncompressedPixelFormat(gfx_api::pixel_format_target target, gfx_api::texture_type textureType);

		gfx_api::texture* createTextureForCompatibleImageUploads(const size_t& mipmap_count, const iV_Image& bitmap, const std::string& filename);

	protected:
		// True while executeCompiledRenderGraph() is running. Draw/bind APIs must only be called from record callbacks.
		bool renderGraphExecuting() const { return _renderGraphExecuting; }
		void setRenderGraphExecuting(bool executing) { _renderGraphExecuting = executing; }
		void bumpRenderGraphEpoch() { ++_renderGraphEpoch; }

		static SceneEffectSurfaces normalizeSceneEffectSurfaces(SceneEffectSurfaces cfg)
		{
			cfg.ssaoGenerateDivisor = std::max(cfg.ssaoGenerateDivisor, 1u);
			cfg.ssaoBlurDivisor = std::max(cfg.ssaoBlurDivisor, 1u);
			return cfg;
		}
		void storeSceneEffectSurfaces(SceneEffectSurfaces cfg)
		{
			_sceneEffectSurfaces = normalizeSceneEffectSurfaces(cfg);
		}

		// GPU frame timing state maintained by the backends
		bool _gpuFrameTimingEnabled = false;
		optional<GpuFrameTiming> _lastGpuFrameTiming;

	protected:
		/// Called by backends where the per-frame uniform storage rotates
		/// (This is what makes every reference from the previous frame stale)
		void advanceFrameUniformGeneration()
		{
			if (++_frameUniformGeneration == 0) { _frameUniformGeneration = 1; } // zero means invalid
		}

	private:
		uint32_t _frameUniformGeneration = 0;
		bool _renderGraphExecuting = false;
		uint64_t _renderGraphEpoch = 1;
		bool _screenGeometryDirty = true;
		optional<float> _mipLodBias;
		uint32_t _sceneRenderScalePercent = 100;
		scene_upscaling_mode _sceneUpscalingMode = scene_upscaling_mode::bilinear;
		bool _smaaEnabled = false;
		SceneEffectSurfaces _sceneEffectSurfaces;
		float _sceneRenderFraction = 1.f;
		bool _sceneDynamicResolution = false;
		virtual bool _initialize(const backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode mode, optional<float> mipLodBias, uint32_t depthMapResolution) = 0;
	};

	// High-level API for getting an uncompressed image (iV_Image) from a file
	std::unique_ptr<iV_Image> loadUncompressedImageFromFile(const char *filename, gfx_api::pixel_format_target target, gfx_api::texture_type textureType, int maxWidth = -1, int maxHeight = -1, bool forceRGBA8 = false);

	WzString imageLoadFilenameFromInputFilename(const WzString& filename);
	bool checkImageFilesWouldLoadFromSameParentMountPath(const std::vector<WzString>& filenames, bool ignoreNotFound);

	// Per-texture compression overrides
	bool loadTextureCompressionOverrides();
	optional<max_texture_compression_level> getMaxTextureCompressionLevelOverride(const std::string& filename);

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
	template<std::size_t stride, vertex_attribute_input_rate rate, typename... input_description>
	struct vertex_buffer_description
	{
		static vertex_buffer get_desc()
		{
			return { stride, rate, { input_description::get_desc()...} };
		}
	};

	template<std::size_t texture_unit, sampler_type sampler, pixel_format_target target = pixel_format_target::texture_2d, border_color border = border_color::none, shader_stage stage = shader_stage::fragment>
	struct texture_description
	{
		static texture_input get_desc()
		{
			return texture_input{ texture_unit, sampler, target, border, stage };
		}
	};

	/// A texture sampled by the tessellation evaluation stage
	template<std::size_t texture_unit, sampler_type sampler, pixel_format_target target = pixel_format_target::texture_2d, border_color border = border_color::none>
	using tess_texture_description = texture_description<texture_unit, sampler, target, border, shader_stage::tessellation_evaluation>;

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
			if (this->nextpso != nullptr && !this->nextpso->broken)
			{
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
			gfx_api::context::get().bind_textures(texture_descriptions(), { args... });
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

		/// Set one block by value
		template<size_t slot, typename T>
		void set_uniforms_at(const T& data)
		{
			static_assert(slot < std::tuple_size<uniform_inputs>::value, "Uniform slot is out of range for this pipeline");
			static_assert(std::is_same<typename std::tuple_element<slot, uniform_inputs>::type, T>::value, "Wrong uniform block type for this slot");
			gfx_api::context::get().set_uniforms(slot, { std::make_tuple(static_cast<const void*>(&data), sizeof(T)) });
		}

		/// Bind a block already uploaded for this frame
		template<size_t slot, typename T>
		void set_uniforms_at(const frame_uniform_block_ref<T>& ref)
		{
			static_assert(slot < std::tuple_size<uniform_inputs>::value, "Uniform slot is out of range for this pipeline");
			static_assert(std::is_same<typename std::tuple_element<slot, uniform_inputs>::type, T>::value, "Wrong uniform block type for this slot");
			gfx_api::context::get().set_frame_uniform_at(slot, ref.allocation(), std::type_index(typeid(T)));
		}

		void draw(const std::size_t& count, const std::size_t& offset)
		{
			context::get().draw(offset, count, primitive);
		}

		void draw_elements(const std::size_t& count, const std::size_t& offset)
		{
			context::get().draw_elements(offset, count, primitive, index);
		}

		void draw_instanced(const std::size_t& count, const std::size_t& offset, const std::size_t& instance_count)
		{
			context::get().draw_instanced(offset, count, primitive, instance_count);
		}

		void draw_elements_instanced(const std::size_t& count, const std::size_t& offset, const std::size_t& instance_count)
		{
			context::get().draw_elements_instanced(offset, count, primitive, index, instance_count);
		}

		bool recompile()
		{
			nextpso = gfx_api::context::get().build_pipeline(pso, pipeline_create_info(rasterizer::get(), shader, primitive, untuple_typeinfo(uniform_inputs{}), texture_descriptions(), untuple<vertex_buffer>(vertex_buffer_inputs{})));
			return nextpso != nullptr && !nextpso->broken;
		}

		bool isBroken()
		{
			return nextpso ? nextpso->broken : pso->broken;
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
		static std::vector<Output> untuple(const std::tuple<Args...>&)
		{
			return std::vector<Output>({ Args::get_desc()... });
		}

		// The texture descriptors are built purely from texture_inputs' template arguments
		// (texture_description::get_desc() is static and returns its own non-type parameters), so they
		// are identical for every draw through this pipeline. Build the list once per instantiation
		// rather than allocating a fresh vector on every bind_textures() call.
		static const std::vector<texture_input>& texture_descriptions()
		{
			static const std::vector<texture_input> descriptions = untuple<texture_input>(texture_inputs{});
			return descriptions;
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

	// for new terrain shader
	constexpr std::size_t terrain_tileNo = 5;
	constexpr std::size_t terrain_grounds = 6;
	constexpr std::size_t terrain_groundWeights = 7;

	// for instanced model rendering
	constexpr std::size_t instance_modelMatrix = 5;
	constexpr std::size_t instance_packedValues = 9;
	constexpr std::size_t instance_Colour = 10;
	constexpr std::size_t instance_TeamColour = 11;

	using notexture = std::tuple<>;

	// NOTE: Be very careful changing these constant_buffer_type structs;
	//		 they must match std140 layout rules (see: the Vulkan shaders)

	// Only change once per frame
	struct Draw3DShapeGlobalUniforms
	{
		glm::mat4 ProjectionMatrix;
		glm::mat4 ViewMatrix;
		glm::mat4 ShadowMapMVPMatrix;
		glm::vec4 cameraPos; // in world space
		glm::vec4 sunPos; // in world space
		glm::vec4 sceneColor;
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		float timeState; // graphicsCycle
		float mipLoadBias;
		float pad0 = 0.f;
		float pad1 = 0.f;
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
		glm::mat4 ModelMatrix;
		glm::mat4 NormalMatrix;
		glm::vec4 colour;
		glm::vec4 teamcolour;
		float shaderStretch;
		float animFrameNumber;
		int ecmState;
		int alphaTest;
	};

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
	texture_description<0, sampler_type::anisotropic>, // diffuse
	texture_description<1, sampler_type::bilinear>, // team color mask
	texture_description<2, sampler_type::anisotropic>, // normal map
	texture_description<3, sampler_type::anisotropic> // specular map
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

	constexpr size_t max_lights = 128;
	constexpr size_t max_indexed_lights = 512;
	constexpr size_t bucket_dimension = 8;

	// Only change once per frame
	// The bucket table leads so that it forms a prefix of the block.
	// A shader that fetches the light arrays from elsewhere can then declare and bind just that prefix.
	// std140 aligns the arrays that follow bucketDimensionUsed to 16 while C++ does not, hence the explicit padding.
	struct PointLightsUniforms
	{
		std::array<glm::ivec4, bucket_dimension * bucket_dimension> bucketOffsetAndSize;
		int bucketDimensionUsed;
		int pad[3];
		std::array<glm::vec4, max_lights> PointLightsPosition;
		std::array<glm::vec4, max_lights> PointLightsColorAndEnergy;
		std::array<glm::ivec4, max_indexed_lights> indexed_lights;
	};

	// std140 puts every one of these arrays on a 16 byte boundary and rounds the block up to
	// a multiple of 16, neither of which C++ does on its own here.
	static_assert(offsetof(PointLightsUniforms, bucketOffsetAndSize) % 16 == 0, "std140 requires 16 byte alignment");
	static_assert(offsetof(PointLightsUniforms, PointLightsPosition) % 16 == 0, "std140 requires 16 byte alignment");
	static_assert(offsetof(PointLightsUniforms, PointLightsColorAndEnergy) % 16 == 0, "std140 requires 16 byte alignment");
	static_assert(offsetof(PointLightsUniforms, indexed_lights) % 16 == 0, "std140 requires 16 byte alignment");
	static_assert(sizeof(PointLightsUniforms) % 16 == 0, "std140 rounds the block up to a multiple of 16");

	// Only change once per frame
	struct Draw3DShapeInstancedGlobalUniforms
	{
		glm::mat4 ProjectionMatrix;
		glm::mat4 ViewMatrix;
		glm::mat4 ModelUVLightmapMatrix;
		glm::mat4 ShadowMapMVPMatrix[WZ_MAX_SHADOW_CASCADES];
		glm::vec4 cameraPos; // in modelSpace
		glm::vec4 sunPos; // in world space
		glm::vec4 sceneColor;
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		glm::vec4 ShadowMapCascadeSplits; // Can't use float[4] (because of std140 layout alignment rules, which don't match C/C++ and waste a lot of space)
		int ShadowMapSize;
		float timeState; // graphicsCycle
		int viewportWidth;
		int viewportheight;
		float mipLoadBias;
		float pad0 = 0.f;
		float pad1 = 0.f;
		float pad2 = 0.f;
	};

	// Only change per mesh
	struct Draw3DShapeInstancedPerMeshUniforms
	{
		int tcmask;
		int normalMap;
		int specularMap;
		int hasTangents;
		int shieldEffect;
	};

	// interleaved vertex data
	struct Draw3DShapePerInstanceInterleavedData
	{
		glm::mat4 ModelViewMatrix; // 16 bytes * 4 = 64 bytes
		// glm::mat4 NormalMatrix; // can be calculated in the shader
		glm::vec4 shaderStretch_ecmState_alphaTest_animFrameNumber; // 16 bytes
		uint32_t colour; // 4 bytes (stored as u8x4_norm)
		uint32_t teamcolour; // 4 bytes (stored as u8x4_norm)
		// (+ 8 bytes padding for overall struct)
		uint32_t _unused_padding[2] = {};
	};
	static_assert(sizeof(Draw3DShapePerInstanceInterleavedData) % 16 == 0, "Size must be a multiple of 16");
	static_assert(offsetof(Draw3DShapePerInstanceInterleavedData, shaderStretch_ecmState_alphaTest_animFrameNumber) == 64, "Unexpected offset");
	static_assert(offsetof(Draw3DShapePerInstanceInterleavedData, colour) == 80, "Unexpected offset");
	static_assert(offsetof(Draw3DShapePerInstanceInterleavedData, teamcolour) == 84, "Unexpected offset");

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
	texture_description<0, sampler_type::anisotropic>, // diffuse
	texture_description<1, sampler_type::bilinear>, // team color mask
	texture_description<2, sampler_type::anisotropic>, // normal map
	texture_description<3, sampler_type::anisotropic>, // specular map
	texture_description<4, sampler_type::bilinear_border, pixel_format_target::depth_map, border_color::opaque_white>,  // depth / shadow map
	texture_description<5, sampler_type::bilinear> // lightmap
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

	struct Draw3DShapeInstancedDepthOnlyGlobalUniforms
	{
		glm::mat4 ProjectionMatrix;
		glm::mat4 ViewMatrix;
	};

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
	>, notexture, SHADER_COMPONENT_DEPTH_PREPASS_INSTANCED>;

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
	vertex_buffer_description<12, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>>
	>, notexture, SHADER_GENERIC_COLOR>;
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

	/// Terrain geometry vertex used by the depth-normal prepass.
	struct TerrainGeometryVertex
	{
		Vector3f pos = Vector3f(0.f, 0.f, 0.f);
		Vector3f normal = Vector3f(0.f, 1.f, 0.f);
	};

	/// Position-only view of TerrainGeometryVertex used by depth-only PSOs.
	using TerrainVertexVBODescription = vertex_buffer_description<sizeof(TerrainGeometryVertex), gfx_api::vertex_attribute_input_rate::vertex,
		vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, offsetof(TerrainGeometryVertex, pos)>
	>;

	using TerrainDepth = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 0, polygon_offset::enabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_TERRAIN_DEPTH>>,
	std::tuple<
		TerrainVertexVBODescription
	>, std::tuple<texture_description<0, sampler_type::bilinear_repeat>>, SHADER_TERRAIN_DEPTH>;

	template<>
	struct constant_buffer_type<SHADER_TERRAIN_DEPTHMAP>
	{
		glm::mat4 transform_matrix;
	};

	using TerrainDepthOnlyForDepthMap = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 0, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_TERRAIN_DEPTHMAP>>,
	std::tuple<
		TerrainVertexVBODescription
	>, std::tuple<texture_description<0, sampler_type::bilinear_repeat>>, SHADER_TERRAIN_DEPTHMAP>;

	template<>
	struct constant_buffer_type<SHADER_TERRAIN_DEPTH_PREPASS>
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ViewMatrix;
	};

	using TerrainDepthPrepassVertexVBODescription = vertex_buffer_description<sizeof(TerrainGeometryVertex), gfx_api::vertex_attribute_input_rate::vertex,
		vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, offsetof(TerrainGeometryVertex, pos)>,
		vertex_attribute_description<normal, gfx_api::vertex_attribute_type::float3, offsetof(TerrainGeometryVertex, normal)>
	>;

	using TerrainDepthPrepass = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::enabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_TERRAIN_DEPTH_PREPASS>>,
	std::tuple<
		TerrainDepthPrepassVertexVBODescription
	>, notexture, SHADER_TERRAIN_DEPTH_PREPASS>;

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


	struct TerrainDecalVertex
	{
		Vector3f pos = Vector3f(0.f, 0.f, 0.f);
		Vector2f decalUv = Vector2f(0.f, 0.f);
		Vector3f normal = Vector3f(0.f, 1.f, 0.f);
		glm::vec4 decalTangent = glm::vec4(1.f, 0.f, 0.f, 1.f); // applicable only to decal texture!
		int32_t decalNo = 0; // tile #
		uint32_t grounds; // ground texture indexes for splatting. encoded as rgba bytes.
		PIELIGHT groundWeights; // weights of corresponding ground textures. encoded as rgba floats
	};

	static_assert(WZ_MAX_SHADOW_CASCADES <= 4, "Packing the ShadowMapCascadeSplits into a vec4 won't work...");


	struct TerrainCombinedUniforms
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::mat4 ViewMatrix;
		glm::mat4 ModelUVLightmapMatrix;
		glm::mat4 ShadowMapMVPMatrix[WZ_MAX_SHADOW_CASCADES];
		glm::mat4 groundScale; // array of scales for ground textures, encoded in mat4. scale_i = groundScale[i/4][i%4]
		glm::vec4 cameraPos; // in modelSpace
		glm::vec4 sunPos; // in modelSpace
		glm::vec4 emissiveLight; // light colors/intensity
		glm::vec4 ambientLight;
		glm::vec4 diffuseLight;
		glm::vec4 specularLight;
		glm::vec4 ShadowMapCascadeSplits; // Can't use float[4] (because of std140 layout alignment rules, which don't match C/C++ and waste a lot of space)
		int ShadowMapSize;
		int quality;
		int viewportWidth;
		int viewportheight;
		float tessMaxLevel; // hardware-tessellated terrain only
		float mipLoadBias;
		float pad0 = 0.f;
		float pad1 = 0.f;
	};

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
	texture_description<0, sampler_type::bilinear>, // lightmap
	texture_description<1, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d_array>, // ground
	texture_description<2, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d_array>, // ground normal
	texture_description<3, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d_array>, // ground specular
	texture_description<4, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d_array>, // ground height
	texture_description<5, sampler_type::anisotropic, pixel_format_target::texture_2d_array>, // decal
	texture_description<6, sampler_type::anisotropic, pixel_format_target::texture_2d_array>, // decal normal
	texture_description<7, sampler_type::anisotropic, pixel_format_target::texture_2d_array>, // decal specular
	texture_description<8, sampler_type::anisotropic, pixel_format_target::texture_2d_array>,  // decal height
	texture_description<9, sampler_type::bilinear_border, pixel_format_target::depth_map, border_color::opaque_white>  // depth / shadow map
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
	texture_description<0, sampler_type::bilinear>, // lightmap
	texture_description<1, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d_array>, // ground
	texture_description<2, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d_array>, // ground normal
	texture_description<3, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d_array>, // ground specular
	texture_description<4, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d_array>, // ground height
	texture_description<5, sampler_type::anisotropic, pixel_format_target::texture_2d_array>, // decal
	texture_description<6, sampler_type::anisotropic, pixel_format_target::texture_2d_array>, // decal normal
	texture_description<7, sampler_type::anisotropic, pixel_format_target::texture_2d_array>, // decal specular
	texture_description<8, sampler_type::anisotropic, pixel_format_target::texture_2d_array>,  // decal height
	texture_description<9, sampler_type::bilinear_border, pixel_format_target::depth_map, border_color::opaque_white>,  // depth / shadow map
	tess_texture_description<10, sampler_type::bilinear>, // baked terrain height
	tess_texture_description<11, sampler_type::bilinear>, // baked terrain outline offset
	tess_texture_description<12, sampler_type::bilinear>  // baked terrain normal
	>, shader>;

	using TerrainCombinedTess_Medium = TerrainCombinedTessTemplate<SHADER_TERRAIN_COMBINED_MEDIUM_TESS>;
	using TerrainCombinedTess_High = TerrainCombinedTessTemplate<SHADER_TERRAIN_COMBINED_HIGH_TESS>;

	// position-only view of the TerrainDecalVertex buffer, for the tessellated shadow/depth map pass
	using TerrainPatchCornerVBODescription = vertex_buffer_description<sizeof(TerrainDecalVertex), gfx_api::vertex_attribute_input_rate::vertex,
		vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0>
	>;

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
		tess_texture_description<0, sampler_type::bilinear>, // baked terrain height
		tess_texture_description<1, sampler_type::bilinear>, // baked terrain outline offset
		tess_texture_description<2, sampler_type::bilinear>  // baked terrain normal
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
		tess_texture_description<0, sampler_type::bilinear>, // baked terrain height
		tess_texture_description<1, sampler_type::bilinear>, // baked terrain outline offset
		tess_texture_description<2, sampler_type::bilinear>  // baked terrain normal
	>, SHADER_TERRAIN_DEPTH_PREPASS_TESS>;

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
		texture_description<0, sampler_type::anisotropic_repeat>, // tex1
		texture_description<1, sampler_type::anisotropic_repeat>, // tex2
		texture_description<2, sampler_type::bilinear> // lightmap
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
	};

	using WaterHighPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
	std::tuple<constant_buffer_type<SHADER_WATER_HIGH>>,
	std::tuple<
	vertex_buffer_description<16, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::float4, 0>> // WaterVertex, w is depth
	>, std::tuple<
		texture_description<0, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d_array>, // textures
		texture_description<1, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d_array>, // normal maps
		texture_description<2, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d_array>, // specular maps
		texture_description<3, sampler_type::bilinear>, // lightmap
		texture_description<4, sampler_type::bilinear_border, pixel_format_target::depth_map, border_color::opaque_white>  // depth / shadow map
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
		texture_description<0, sampler_type::bilinear>, // lightmap
		texture_description<1, sampler_type::anisotropic_repeat, pixel_format_target::texture_2d> // water decal texture
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
		int texture; // IGNORED
	};

	using DrawImageTextPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_TEXT, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_TEXT>>,
	std::tuple<
	vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
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
	>, std::tuple<texture_description<0, sampler_type::bilinear>>, SHADER_TEXRECT>;

	using DrawImageAnisotropicPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
	std::tuple<constant_buffer_type<SHADER_TEXRECT>>,
	std::tuple<
	vertex_buffer_description<4, gfx_api::vertex_attribute_input_rate::vertex, vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0>>
	>, std::tuple<texture_description<0, sampler_type::anisotropic>>, SHADER_TEXRECT>;

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

	// interleaved vertex data
	struct MultiRectPerInstanceInterleavedData
	{
		MultiRectPerInstanceInterleavedData(const glm::mat4 &TransformationMatrix, const glm::vec4 &offset_scale, const uint32_t &colour)
		: TransformationMatrix(TransformationMatrix), offset_scale(offset_scale), colour(colour)
		{ }

		glm::mat4 TransformationMatrix; // 16 bytes * 4 = 64 bytes
		glm::vec4 offset_scale; // 16 bytes (2 vec2)
		uint32_t colour; // 4 bytes (stored as u8x4_norm)
		// (+ 12 bytes padding for overall struct)
		uint32_t _unused_padding[3] = {};
	};
	static_assert(sizeof(MultiRectPerInstanceInterleavedData) % 16 == 0, "Size must be a multiple of 16");
	static_assert(offsetof(MultiRectPerInstanceInterleavedData, offset_scale) == 64, "Unexpected offset");
	static_assert(offsetof(MultiRectPerInstanceInterleavedData, colour) == 80, "Unexpected offset");

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
	std::tuple<texture_description<0, sampler_type::bilinear>>, SHADER_DEBUG_TEXTURE2D_QUAD>;

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
	std::tuple<texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d_array>>, SHADER_DEBUG_TEXTURE2DARRAY_QUAD>;

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
	std::tuple<texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d>>, SHADER_WORLD_TO_SCREEN>;

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
	std::tuple<texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d>>, SHADER_FSR1_EASU>;

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
	std::tuple<texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d>>, SHADER_FSR1_RCAS>;

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
	std::tuple<texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d>>, SHADER_SMAA_EDGES>;

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
		texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d>,
		texture_description<1, sampler_type::bilinear, pixel_format_target::texture_2d>,
		texture_description<2, sampler_type::nearest_clamped, pixel_format_target::texture_2d>
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
		texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d>,
		texture_description<1, sampler_type::bilinear, pixel_format_target::texture_2d>
	>, SHADER_SMAA_BLEND>;

	static constexpr size_t SSAO_KERNEL_SIZE = 16;

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
		texture_description<0, sampler_type::nearest_clamped, pixel_format_target::texture_2d>, // depth
		texture_description<1, sampler_type::nearest_clamped, pixel_format_target::texture_2d>, // normals
		texture_description<2, sampler_type::bilinear_repeat, pixel_format_target::texture_2d> // noise
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
		texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d>, // occlusion
		texture_description<1, sampler_type::nearest_clamped, pixel_format_target::texture_2d> // depth
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
		texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d> // occlusion
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
		texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d>, // scene
		texture_description<1, sampler_type::bilinear, pixel_format_target::texture_2d>, // ao
		texture_description<2, sampler_type::bilinear, pixel_format_target::texture_2d>  // prepassNormals
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
		texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d>, // scene
		texture_description<1, sampler_type::nearest_clamped, pixel_format_target::texture_2d> // prepassDepth
	>, SHADER_SCENE_FOG>;

	template<>
	struct constant_buffer_type<SHADER_RANGE_RING_SDF>
	{
		glm::mat4 orthoViewProj;
		glm::vec4 mapOriginExtent; // xy origin.xz, zw size.xz (0 size = no clip)
		glm::vec4 sdfParams; // x = sdfBand in world units (8-bit encode range)
	};
	static_assert(sizeof(constant_buffer_type<SHADER_RANGE_RING_SDF>) == 96, "Range ring SDF cbuffer std140 size");

	struct RangeRingInstance
	{
		glm::vec4 centerRadius; // x, z, R, pad
	};
	static_assert(sizeof(RangeRingInstance) == 16, "RangeRingInstance stride");

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
			texture_description<0, sampler_type::bilinear, pixel_format_target::texture_2d>, // scene
			texture_description<1, sampler_type::nearest_clamped, pixel_format_target::texture_2d>, // prepassDepth
			texture_description<2, sampler_type::bilinear, pixel_format_target::texture_2d> // rangeRingSdf
		>,
		SHADER_RANGE_RING_COMPOSITE>;
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
