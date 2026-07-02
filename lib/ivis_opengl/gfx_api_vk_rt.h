/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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

// Ray-traced (VK_KHR_ray_query) sun shadows backend for the Vulkan gfx_api implementation.
//
// The rasterized pipeline is untouched; this adds:
// - a BLAS cache for model meshes (keyed by (meshKey, deformation variant)) and the terrain
// - a per-frame TLAS built from the frame's shadow-casting instance list
// - a camera depth prepass (reusing the depth-only render pass + PSOs at the mask resolution)
// - a fullscreen fragment-shader ray-query pass writing an RG8 mask (R = sun visibility,
//   G = ambient occlusion)
// - two depth-aware separable blur passes
// The resulting mask texture is sampled by the existing lighting shaders (gated by the
// WZ_RAY_SHADOWS specialization constant) via gl_FragCoord.

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#include "gfx_api_vk.h"

#if defined(WZ_VK_RAY_QUERY_POSSIBLE)

#include <vector>
#include <map>
#include <memory>

struct VkRayShadowBackend
{
	VkRayShadowBackend(VkRoot& root);
	~VkRayShadowBackend();

	VkRayShadowBackend(const VkRayShadowBackend&) = delete;
	VkRayShadowBackend& operator=(const VkRayShadowBackend&) = delete;

	bool setMode(gfx_api::ray_shadow_mode newMode);
	gfx_api::ray_shadow_mode getMode() const { return mode; }

	void setTerrainGeometry(const float* vertices, size_t vertexCount, const uint32_t* indices, size_t indexCount);
	void updateTerrainVertices(size_t startVertex, const float* vertices, size_t vertexCount);
	void clearTerrainGeometry();

	bool hasMeshBLAS(const void* meshKey, int32_t variant) const;
	void registerMeshGeometry(const void* meshKey, int32_t variant, const float* vertices, size_t vertexCount, const uint32_t* indices, size_t indexCount);
	void invalidateMesh(const void* meshKey);

	void beginFrame();
	void addInstance(const void* meshKey, int32_t variant, const glm::mat4& worldTransform);
	void addTerrainInstance();
	void beginDepthPass();
	void traceAndFilter(const gfx_api::ray_shadow_frame_params& params);
	gfx_api::abstract_texture* getMaskTexture();

	// TLAS descriptor set for material shaders (ray-query pipeline variants); points at the
	// current frame's TLAS once traceAndFilter has run, else at an empty fallback TLAS
	vk::DescriptorSet getMaterialTlasDescSet();

	// Immediate destruction of all resources (for VkRoot::shutdown, after the buffering_mechanism is gone)
	void destroyNow();

private:
	struct RTBuffer
	{
		vk::Buffer buffer;
		VmaAllocation allocation = VK_NULL_HANDLE;
		vk::DeviceSize size = 0;
		void* pMapped = nullptr;
		vk::DeviceAddress address = 0;
	};

	struct BlasEntry
	{
		vk::AccelerationStructureKHR as;
		RTBuffer asBuffer;
		vk::DeviceAddress address = 0;
	};

	struct PendingBlasBuild
	{
		vk::AccelerationStructureKHR dst;
		RTBuffer geometryBuffer; // freed (deferred) after the build is recorded
		vk::DeviceAddress vertexAddress = 0;
		vk::DeviceAddress indexAddress = 0;
		uint32_t vertexCount = 0;
		uint32_t triangleCount = 0;
		vk::DeviceSize scratchSize = 0;
		bool keepGeometryBuffer = false; // terrain keeps its geometry buffer for later rebuilds
	};

	struct PerFrame
	{
		RTBuffer instanceBuffer; // host-visible, mapped
		uint32_t instanceCapacity = 0;
		vk::AccelerationStructureKHR tlas;
		RTBuffer tlasBuffer;
		RTBuffer tlasScratchBuffer;
		uint32_t tlasCapacity = 0;
		vk::DescriptorSet traceDescSet;
		vk::DescriptorSet blurDescSetA; // reads maskA (+depth)
		vk::DescriptorSet blurDescSetB; // reads maskB (+depth)
		bool descSetsAllocated = false;
		vk::DescriptorSet materialTlasDescSet; // TLAS set for material (ray-query variant) pipelines
		bool materialTlasDescSetStale = false; // set's TLAS was released; re-point at emptyTlas before next use
	};

private:
	bool createPipelines(); // returns false if e.g. shaders are unavailable
	void destroyPipelines();
	bool recreateImagesIfNeeded(); // (re)creates mask/depth images + FBOs at the current target extent
	void destroyImages();
	void releasePerFrameResources(PerFrame& pf);

	RTBuffer createRTBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage, bool mapped, const char* debugName);
	void destroyRTBufferNow(RTBuffer& buf);
	void destroyRTBufferDeferred(RTBuffer& buf);
	void destroyASDeferred(vk::AccelerationStructureKHR& as);

	BlasEntry createBlas(const float* vertices, size_t vertexCount, const uint32_t* indices, size_t indexCount, bool keepGeometryBuffer, const char* debugName);
	void recordPendingBuilds(vk::CommandBuffer cmdBuffer);
	void buildTlas(vk::CommandBuffer cmdBuffer, PerFrame& pf);
	void updateDescriptorSets(PerFrame& pf);
	PerFrame& currentPerFrame();
	bool createEmptyTlas(); // fallback TLAS for material shaders on frames without a trace
	void writeMaterialTlasDescSet(PerFrame& pf, vk::AccelerationStructureKHR tlas);

	vk::Extent2D targetMaskExtent() const;

private:
	VkRoot& root;
	gfx_api::ray_shadow_mode mode = gfx_api::ray_shadow_mode::Off;

	// BLAS cache
	std::map<std::pair<const void*, int32_t>, BlasEntry> meshBlas;
	std::vector<PendingBlasBuild> pendingBuilds;

	// terrain
	BlasEntry terrainBlas;
	RTBuffer terrainGeometryBuffer;
	uint32_t terrainVertexCount = 0;
	uint32_t terrainTriangleCount = 0;
	vk::DeviceSize terrainScratchSize = 0;
	bool terrainBlasDirty = false;
	bool haveTerrain = false;

	// per-frame instance list (CPU)
	std::vector<vk::AccelerationStructureInstanceKHR> frameInstances;

	// per-frame GPU resources
	std::vector<PerFrame> perFrame;

	// images / passes
	vk::Extent2D maskExtent = vk::Extent2D(0, 0);
	VkRenderedImage* pMaskImageA = nullptr; // final mask (exposed)
	VkRenderedImage* pMaskImageB = nullptr; // blur ping-pong
	vk::Image rtDepthImage;
	VmaAllocation rtDepthAllocation = VK_NULL_HANDLE;
	WZ_vk::UniqueImageView rtDepthView;
	vk::Framebuffer rtDepthFbo;
	vk::Framebuffer maskFboA;
	vk::Framebuffer maskFboB;

	// pipelines
	bool pipelinesCreated = false;
	vk::RenderPass maskRenderPass;
	vk::DescriptorSetLayout traceDescLayout;
	vk::DescriptorSetLayout blurDescLayout;
	vk::PipelineLayout tracePipelineLayout;
	vk::PipelineLayout blurPipelineLayout;
	vk::Pipeline tracePipeline;
	vk::Pipeline blurPipeline;
	vk::ShaderModule fullscreenVertShader;
	vk::ShaderModule traceFragShader;
	vk::ShaderModule blurFragShader;
	vk::Sampler depthSampler; // nearest, clamp
	vk::Sampler maskSampler; // linear, clamp
	vk::DescriptorPool descriptorPool;

	// empty fallback TLAS (0 instances) for material-shader ray queries outside traced frames
	vk::AccelerationStructureKHR emptyTlas;
	RTBuffer emptyTlasBuffer;

	// frame state
	bool depthPassActive = false;
	bool maskValid = false;
};

#else // !defined(WZ_VK_RAY_QUERY_POSSIBLE)

// Stub so that VkRoot's std::unique_ptr<VkRayShadowBackend> member has a complete type
struct VkRayShadowBackend
{
	void destroyNow() {}
	vk::DescriptorSet getMaterialTlasDescSet() { return vk::DescriptorSet(); }
};

#endif // defined(WZ_VK_RAY_QUERY_POSSIBLE)

#endif // defined(WZ_VULKAN_ENABLED)
