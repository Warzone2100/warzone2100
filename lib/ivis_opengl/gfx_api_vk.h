/*
	This file is part of Warzone 2100.
	Copyright (C) 2017-2022  Warzone 2100 Project

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

#if defined(WZ_VULKAN_ENABLED)

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4191 ) // warning C4191: '<function-style-cast>': unsafe conversion from 'PFN_vkVoidFunction' to 'PFN_vk<...>'
#endif
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wnewline-eof"
#endif
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy" // Ignore warnings caused by vulkan.hpp 148
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wcast-align"
#endif
#define VULKAN_HPP_TYPESAFE_CONVERSION 1
#include <vulkan/vulkan.hpp>
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 9
#pragma GCC diagnostic pop
#endif
#if defined( _MSC_VER )
#pragma warning( pop )
#endif

#include "lib/framework/frame.h"

#include "gfx_api.h"
#include <algorithm>
#include <sstream>
#include <map>
#include <vector>
#include <unordered_map>
#include <typeindex>

#include <nonstd/optional.hpp>
using nonstd::optional;

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4191 ) // warning C4191: '<function-style-cast>': unsafe conversion from 'PFN_vkVoidFunction' to 'PFN_vk<...>'
#pragma warning( disable : 4324 ) // warning C4324: 'struct_name' : structure was padded due to alignment specifier
#endif
#include "3rdparty/vkh_renderpasscompat.hpp"
#include "3rdparty/vkh_info.hpp"
#if defined(__clang__)
# pragma clang diagnostic push
#  if defined(__has_warning)
#    if __has_warning("-Wnullability-extension")
#      pragma clang diagnostic ignored "-Wnullability-extension"
#    endif
#    if __has_warning("-Wnullability-completeness")
#      pragma clang diagnostic ignored "-Wnullability-completeness"
#    endif
#  endif
#endif
#include "3rdparty/vk_mem_alloc.h"
#if defined(__clang__)
# pragma clang diagnostic pop
#endif
#if defined( _MSC_VER )
#pragma warning( pop )
#endif

namespace gfx_api
{
	class backend_Vulkan_Impl
	{
	public:
		backend_Vulkan_Impl() {};
		virtual ~backend_Vulkan_Impl() {};

		virtual PFN_vkGetInstanceProcAddr getVkGetInstanceProcAddr() = 0;
		virtual bool getRequiredInstanceExtensions(std::vector<const char*> &output) = 0;
		virtual bool createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) = 0;

		// Use this function to get the size of the window's underlying drawable dimensions in pixels. This is used for setting viewport sizes, scissor rectangles, and other places where the a VkExtent might show up in relation to the window.
		virtual void getDrawableSize(int* w, int* h) = 0;

		virtual bool allowImplicitLayers() const = 0;
	};
}

namespace WZ_vk {
#if VK_HEADER_VERSION >= 301
	using DispatchLoaderDynamic = vk::detail::DispatchLoaderDynamic;
#else
	using DispatchLoaderDynamic = vk::DispatchLoaderDynamic;
#endif
	using UniqueBuffer = vk::UniqueHandle<vk::Buffer, WZ_vk::DispatchLoaderDynamic>;
	using UniqueDeviceMemory = vk::UniqueHandle<vk::DeviceMemory, WZ_vk::DispatchLoaderDynamic>;
	using UniqueImage = vk::UniqueHandle<vk::Image, WZ_vk::DispatchLoaderDynamic>;
	using UniqueImageView = vk::UniqueHandle<vk::ImageView, WZ_vk::DispatchLoaderDynamic>;
	using UniqueSemaphore = vk::UniqueHandle<vk::Semaphore, WZ_vk::DispatchLoaderDynamic>;
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
namespace std {
	template <>
	struct hash<vk::DescriptorBufferInfo>
	{
		std::size_t operator()(const vk::DescriptorBufferInfo& k) const
		{
			std::size_t h = 0;
			hash_combine(h, static_cast<VkBuffer>(k.buffer), k.offset, k.range);
			return h;
		}
	};
}

struct BlockBufferAllocator
{
	BlockBufferAllocator(VmaAllocator allocator, uint32_t minimumBlockSize, const vk::BufferUsageFlags& usageFlags, const VmaAllocationCreateInfo& allocInfo, bool autoMap = false);
	BlockBufferAllocator(VmaAllocator allocator, uint32_t minimumBlockSize, const vk::BufferUsageFlags& usageFlags, const VmaMemoryUsage usage, bool autoMap = false);
	~BlockBufferAllocator();

private:
	static std::tuple<uint32_t, uint32_t> getWritePosAndNewWriteLocation(uint32_t currentWritePos, uint32_t amount, uint32_t totalSize, uint32_t align);

	void allocateNewBlock(uint32_t minimumSize);

	struct Block
	{
		vk::Buffer buffer;
		VmaAllocation allocation = VK_NULL_HANDLE;
		uint32_t size = 0;
		void *pMappedMemory = nullptr;
	};

public:
	struct AllocationResult
	{
		vk::Buffer buffer;
		uint32_t offset;

		AllocationResult(const Block& block, const uint32_t offset)
		: buffer(block.buffer)
		, offset(offset)
		, block(block)
		{ }

		const Block& block;
	};
	AllocationResult alloc(uint32_t amount, uint32_t align);
	void * mapMemory(AllocationResult memoryAllocation);
	void unmapMemory(AllocationResult memoryAllocation);
	void unmapAutomappedMemory();
	void flushAutomappedMemory();
	void clean();

private:
	VmaAllocator allocator;
	const uint32_t minimumBlockSize;
	const vk::BufferUsageFlags usageFlags;
	VmaAllocationCreateInfo allocInfo;

	std::vector<Block> blocks;
	uint64_t totalCapacity = 0;
	uint32_t currentWritePosInLastBlock = 0;

	uint32_t minimumFirstBlockSize = 0;

	const bool autoMap = false;
};

struct VkRoot; // forward-declare
struct VkPSO; // forward-declare
struct buffering_mechanism;

struct perFrameResources_t
{
	vk::Device dev;
	VmaAllocator allocator;
	struct DescriptorPoolDetails
	{
		vk::DescriptorPool poolHandle;
		vk::DescriptorPoolSize size;
		size_t maxSets = 0;
		size_t requestedDescriptors = 0;
		size_t requestedSets = 0;

		DescriptorPoolDetails(vk::DescriptorPool poolHandle,
							  vk::DescriptorPoolSize size,
							  size_t maxSets)
		: poolHandle(poolHandle)
		, size(size)
		, maxSets(maxSets)
		{ }
	};
	struct DescriptorPoolsContainer
	{
	public:
		inline void push_back(const DescriptorPoolDetails& pool)
		{
			pools.push_back(pool);
		}
		void reset(vk::Device dev, const WZ_vk::DispatchLoaderDynamic& vkDynLoader);
		inline DescriptorPoolDetails& current() { return pools.at(currPool); }
		bool nextPool() { if (!pools.empty() && (currPool < (pools.size() - 1))) { ++currPool; return true; } else { return false; } }
	public:
		std::vector<DescriptorPoolDetails> pools;
		size_t currPool = 0;
	};
	DescriptorPoolsContainer combinedImageSamplerDescriptorPools;
	DescriptorPoolsContainer uniformDynamicDescriptorPools;
	uint32_t numalloc = 0;
	vk::CommandPool pool;
	vk::Fence previousSubmission;
	std::vector</*WZ_vk::UniqueBuffer*/vk::Buffer> buffer_to_delete;
	std::vector</*WZ_vk::UniqueImage*/vk::Image> image_to_delete;
	std::vector<WZ_vk::UniqueImageView> image_view_to_delete;
	std::vector<VmaAllocation> vmamemory_to_free;
	std::vector<VkPSO*> pso_to_delete;
	std::vector<vk::Framebuffer> fbo_to_delete;

	BlockBufferAllocator stagingBufferAllocator;
	BlockBufferAllocator streamedVertexBufferAllocator;
	BlockBufferAllocator uniformBufferAllocator;

	typedef std::pair<vk::DescriptorBufferInfo, vk::DescriptorSet> DynamicUniformBufferDescriptorSets;
	typedef std::unordered_map<VkPSO *, std::vector<optional<DynamicUniformBufferDescriptorSets>>> PerPSODynamicUniformBufferDescriptorSets;
	PerPSODynamicUniformBufferDescriptorSets perPSO_dynamicUniformBufferDescriptorSets;

	perFrameResources_t( const perFrameResources_t& other ) = delete; // non construction-copyable
	perFrameResources_t& operator=( const perFrameResources_t& ) = delete; // non copyable

	perFrameResources_t(vk::Device& _dev, const VmaAllocator& allocator, const uint32_t& graphicsQueueIndex, const WZ_vk::DispatchLoaderDynamic& vkDynLoader);
	~perFrameResources_t();

public:
	void beginDepthPass();
	void endCurrentDepthPass();

	void beginScenePass();
	void endScenePass();

	vk::CommandBuffer* currentCopyCmdBuffer();
	vk::CommandBuffer* currentDrawCmdBuffer();

	vk::CommandBuffer copyCmdBuffer();
	vk::CommandBuffer depthPassDrawCmdBuffer();
	vk::CommandBuffer scenePassDrawCmdBuffer();
	vk::CommandBuffer renderPassDrawCmdBuffer();

protected:
	friend struct buffering_mechanism;

	// main command buffer for copying command
	vk::CommandBuffer cmdCopy;

	// drawing command buffer for depth pass(es)
	vk::CommandBuffer cmdDrawDepth;

	// drawing command buffer for scene pass
	vk::CommandBuffer cmdDrawScene;

	// main command buffer for drawing (default render pass)
	vk::CommandBuffer cmdDraw;

	void resetDescriptorPools();
	void clean();

public:
	vk::DescriptorPool getDescriptorPool(uint32_t numSets, vk::DescriptorType descriptorType, uint32_t numDescriptors);

private:
	DescriptorPoolDetails createNewDescriptorPool(vk::DescriptorType type, uint32_t maxSets, uint32_t descriptorCount);

private:
	const WZ_vk::DispatchLoaderDynamic *pVkDynLoader;
	vk::CommandBuffer *pCurrentDrawCmdBuffer = nullptr;
};

struct perSwapchainImageResources_t
{
	vk::Device dev;
	vk::Semaphore imageAcquireSemaphore;
	vk::Semaphore renderFinishedSemaphore;

	perSwapchainImageResources_t( const perSwapchainImageResources_t& other ) = delete; // non construction-copyable
	perSwapchainImageResources_t& operator=( const perSwapchainImageResources_t& ) = delete; // non copyable

	perSwapchainImageResources_t(vk::Device& _dev, const WZ_vk::DispatchLoaderDynamic& vkDynLoader);
	~perSwapchainImageResources_t();

protected:
	friend struct buffering_mechanism;
	void clean();

private:
	const WZ_vk::DispatchLoaderDynamic *pVkDynLoader;
};

struct buffering_mechanism
{
	static std::vector<std::unique_ptr<perFrameResources_t>> perFrameResources;
	static std::vector<std::unique_ptr<perSwapchainImageResources_t>> perSwapchainImageResources;

	static size_t currentFrame;
	static size_t currentSwapchainImageResourcesFrame;

	static perFrameResources_t& get_current_resources();
	static perSwapchainImageResources_t& get_current_swapchain_resources();
	static void init(vk::Device dev, const VmaAllocator& allocator, size_t swapchainImageCount, const uint32_t& graphicsQueueFamilyIndex, const WZ_vk::DispatchLoaderDynamic& vkDynLoader);
	static void destroy(vk::Device dev, const WZ_vk::DispatchLoaderDynamic& vkDynLoader);
	static void swap(vk::Device dev, const WZ_vk::DispatchLoaderDynamic& vkDynLoader, bool skipAcquireNewSwapchainImage);
	static bool isInitialized();
	static size_t get_current_frame_num();
	static size_t numFrames();
};

VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(
						 VkDebugReportFlagsEXT flags,
						 VkDebugReportObjectTypeEXT objType,
						 uint64_t srcObject,
						 std::size_t location,
						 int32_t msgCode,
						 const char* pLayerPrefix,
						 const char* pMsg,
						 void* pUserData);

struct VkPSOId final : public gfx_api::pipeline_state_object
{
public:
	size_t psoID = 0;
public:
	VkPSOId(size_t psoID, bool _isbroken) : psoID(psoID)
	{
		broken = _isbroken;
	}
	~VkPSOId() {}
};

struct VkPSO final
{
	vk::Pipeline object;
	std::vector<vk::DescriptorSetLayout> cbuffer_set_layout;
	uint32_t textures_first_set = 0;
	vk::DescriptorSetLayout textures_set_layout;
	vk::PipelineLayout layout;
	vk::ShaderModule vertexShader;
	vk::ShaderModule fragmentShader;
	vk::Device dev;
	const WZ_vk::DispatchLoaderDynamic* pVkDynLoader;
	std::vector<vk::Sampler> samplers;

	std::shared_ptr<VkhRenderPassCompat> renderpass_compat;
	bool hasSpecializationConstant_ShadowConstants = false;
	bool hasSpecializationConstant_PointLightConstants = false;

private:
	// Read shader into text buffer
	static std::vector<uint32_t> readShaderBuf(const std::string& name);

	vk::ShaderModule get_module(const std::string& name, const WZ_vk::DispatchLoaderDynamic& vkDynLoader);

	static std::array<vk::PipelineShaderStageCreateInfo, 2> get_stages(const vk::ShaderModule& vertexModule, const vk::ShaderModule& fragmentModule);

	static std::array<vk::PipelineColorBlendAttachmentState, 1> to_vk(const REND_MODE& blend_state, const uint8_t& color_mask);

	static vk::PipelineDepthStencilStateCreateInfo to_vk(DEPTH_MODE depth_mode, const gfx_api::stencil_mode& stencil);

	static vk::PipelineRasterizationStateCreateInfo to_vk(const bool& offset, const gfx_api::cull_mode& cull);

	static vk::Format to_vk(const gfx_api::vertex_attribute_type& type);

	vk::SamplerCreateInfo to_vk(const gfx_api::sampler_type& type, const gfx_api::pixel_format_target& target, gfx_api::border_color border);

	static vk::PrimitiveTopology to_vk(const gfx_api::primitive_type& primitive);

	static vk::BorderColor to_vk(gfx_api::border_color border);

public:
	VkPSO(vk::Device _dev,
		  const vk::PhysicalDeviceLimits& limits,
		  const gfx_api::pipeline_create_info& createInfo,
		  vk::RenderPass rp,
		  const std::shared_ptr<VkhRenderPassCompat>& renderpass_compat,
		  vk::SampleCountFlagBits rasterizationSamples,
		  const WZ_vk::DispatchLoaderDynamic& _vkDynLoader,
		  const VkRoot& root
		  );

	~VkPSO();

	VkPSO(const VkPSO&) = default;
	VkPSO(VkPSO&&) = default;
	VkPSO& operator=(VkPSO&&) = default;

private:
	const VkRoot* root;
};

struct VkBuf final : public gfx_api::buffer
{
	vk::Device dev;
	gfx_api::buffer::usage usage;
//	WZ_vk::UniqueBuffer object;
//	WZ_vk::UniqueDeviceMemory memory;
	vk::Buffer object;
	VmaAllocation allocation = VK_NULL_HANDLE;
	size_t buffer_size = 0;
	size_t lastUploaded_FrameNum = 0;

	std::string debugName;

	VkBuf(vk::Device _dev, const gfx_api::buffer::usage&, const VkRoot& root, const std::string& debugName);

	virtual ~VkBuf() override;

	virtual void upload(const size_t & size, const void * data) override;
	virtual void update(const size_t & start, const size_t & size, const void * data, const update_flag flag = update_flag::none) override;
	virtual size_t current_buffer_size() override;

	virtual void bind() override;

private:
	void allocateBufferObject(const std::size_t& size);

private:
	const VkRoot* root;
};

struct VkTexture final : public gfx_api::texture
{
	vk::Device dev;
//	WZ_vk::UniqueImage object;
	vk::Image object;
	WZ_vk::UniqueImageView view;
//	WZ_vk::UniqueDeviceMemory memory;
	VmaAllocation allocation = VK_NULL_HANDLE;
	gfx_api::pixel_format internal_format = gfx_api::pixel_format::invalid;
	size_t mipmap_levels = 0;
	size_t tex_width = 0;
	size_t tex_height = 0;

#if defined(WZ_DEBUG_GFX_API_LEAKS)
	std::string debugName;
#endif


	static size_t format_size(const gfx_api::pixel_format& format);

	static size_t format_size(const vk::Format& format);

	VkTexture(const VkRoot& root, const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const gfx_api::pixel_format& internal_pixel_format, const std::string& filename);

	virtual ~VkTexture() override;

	virtual void bind() override;

	virtual bool upload(const size_t& mip_level, const iV_BaseImage& image) override;
	virtual bool upload_sub(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const iV_Image& image) override;
	virtual unsigned id() override;
	virtual gfx_api::texture2dDimensions get_dimensions() const override;
	virtual size_t backend_internal_value() const override;

	VkTexture( const VkTexture& other ) = delete; // non construction-copyable
	VkTexture& operator=( const VkTexture& ) = delete; // non copyable

private:
	bool upload_internal(const std::size_t& mip_level, const std::size_t& offset_x, const std::size_t& offset_y, const iV_BaseImage& image);

private:
	const VkRoot* root;
};

struct VkTextureArray final : public gfx_api::texture_array
{
	const vk::Device dev;
	vk::Image object;
	WZ_vk::UniqueImageView view;
	VmaAllocation allocation = VK_NULL_HANDLE;
	gfx_api::pixel_format internal_format = gfx_api::pixel_format::invalid;
	size_t mipmap_levels;
	size_t layer_count;
	size_t texWidth;
	size_t texHeight;
	bool transitionedToTransferDstFormat = false;

#if defined(WZ_DEBUG_GFX_API_LEAKS)
	std::string debugName;
#endif

	VkTextureArray(const VkRoot& root, size_t mipmap_count, size_t layer_count, size_t width, size_t height, gfx_api::pixel_format internal_pixel_format, const std::string& filename);

	virtual ~VkTextureArray() override;

	virtual void bind() override {};
	virtual unsigned id() override { return 0; }
	virtual size_t backend_internal_value() const override;

	virtual bool upload_layer(const size_t& layer, const size_t& mip_level, const iV_BaseImage& image) override;

	virtual void flush() override;

	VkTextureArray( const VkTextureArray& other ) = delete; // non construction-copyable
	VkTextureArray& operator=( const VkTextureArray& ) = delete; // non copyable

private:
	const VkRoot* root;
};

struct VkDepthMapImage final : public gfx_api::abstract_texture
{
	vk::Device dev;
//	WZ_vk::UniqueImage object;
	vk::Image object;
	WZ_vk::UniqueImageView view;
//	WZ_vk::UniqueDeviceMemory memory;
	VmaAllocation allocation = VK_NULL_HANDLE;

	size_t layer_count;

#if defined(WZ_DEBUG_GFX_API_LEAKS)
	std::string debugName;
#endif

	VkDepthMapImage(const VkRoot& root, const std::size_t& layer_count, const std::size_t& size, vk::Format depthMapFormat, const std::string& filename);

	virtual ~VkDepthMapImage() override;

	virtual void bind() override;
	virtual bool isArray() const override;
	virtual size_t backend_internal_value() const override;

	void destroy(vk::Device dev, const VmaAllocator& allocator, const WZ_vk::DispatchLoaderDynamic& vkDynLoader);

	VkDepthMapImage( const VkDepthMapImage& other ) = delete; // non construction-copyable
	VkDepthMapImage& operator=( const VkDepthMapImage& ) = delete; // non copyable
};

struct VkRenderedImage final : public gfx_api::abstract_texture
{
	vk::Device dev;
//	WZ_vk::UniqueImage object;
	vk::Image object;
	WZ_vk::UniqueImageView view;
//	WZ_vk::UniqueDeviceMemory memory;
	VmaAllocation allocation = VK_NULL_HANDLE;

#if defined(WZ_DEBUG_GFX_API_LEAKS)
	std::string debugName;
#endif

	VkRenderedImage(const VkRoot& root, size_t width, size_t height, vk::Format imageFormat, const std::string& filename);

	virtual ~VkRenderedImage() override;

	virtual void bind() override;
	virtual bool isArray() const override;
	virtual size_t backend_internal_value() const override;

	void destroy(vk::Device dev, const VmaAllocator& allocator, const WZ_vk::DispatchLoaderDynamic& vkDynLoader);

	VkRenderedImage( const VkRenderedImage& other ) = delete; // non construction-copyable
	VkRenderedImage& operator=( const VkRenderedImage& ) = delete; // non copyable
};

struct QueueFamilyIndices
{
	optional<uint32_t> graphicsFamily;
	optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

struct VkRoot final : gfx_api::context
{
	std::unique_ptr<gfx_api::backend_Vulkan_Impl> backend_impl;
	VkhInfo debugInfo;
	gfx_api::context::swap_interval_mode swapMode = gfx_api::context::swap_interval_mode::vsync;
	optional<float> mipLodBias;
	gfx_api::lighting_constants shadowConstants;

	std::vector<VkExtensionProperties> supportedInstanceExtensionProperties;
	std::vector<const char*> instanceExtensions;
	vk::ApplicationInfo appInfo;
	vk::InstanceCreateInfo instanceCreateInfo;
	vk::Instance inst;
	std::vector<const char*> layers;
	WZ_vk::DispatchLoaderDynamic vkDynLoader;

	// physical device (and info)
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties physDeviceProps;
	vk::PhysicalDeviceFeatures physDeviceFeatures;
	vk::PhysicalDeviceMemoryProperties memprops;
#if 0 // this is not a "stable" extension yet
	vk::PhysicalDevicePortabilitySubsetFeaturesKHR physDevicePortabilitySubsetFeatures;
#endif
	bool hasPortabilitySubset = false;

	enum class LodBiasMethod
	{
		Unsupported,
		SpecializationConstant,
		SamplerMipLodBias
	};
	LodBiasMethod lodBiasMethod = LodBiasMethod::Unsupported;

	QueueFamilyIndices queueFamilyIndices;
	std::vector<const char*> enabledDeviceExtensions;
	vk::Device dev;
	vk::SurfaceKHR surface;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	bool queueSupportsTimestamps = false;

	// allocator
	VmaAllocator allocator = VK_NULL_HANDLE;

	// swapchain
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
	vk::Extent2D swapchainSize;
	vk::SurfaceFormatKHR surfaceFormat;
	vk::SwapchainKHR swapchain;
	uint32_t currentSwapchainIndex = 0;
	std::vector<vk::ImageView> swapchainImageView;

	vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1; // msaaSamples used for scene
	vk::SampleCountFlagBits msaaSamplesSwapchain = vk::SampleCountFlagBits::e1; // msaaSamples used for swapchain assets (should generally be 1)
	vk::Image colorImage;
	vk::DeviceMemory colorImageMemory;
	vk::ImageView colorImageView;

	vk::Image depthStencilImage;
	vk::DeviceMemory depthStencilMemory;
	vk::ImageView depthStencilView;

	// render passes
	const size_t DEFAULT_RENDER_PASS_ID = 0;
	const size_t DEPTH_RENDER_PASS_ID = 1;
	const size_t SCENE_RENDER_PASS_ID = 2;
	const size_t NUM_RENDERPASS_IDS = 3;

	struct RenderPassDetails
	{
		vk::RenderPass rp;
		std::shared_ptr<VkhRenderPassCompat> rp_compat_info;
		std::vector<vk::Framebuffer> fbo;
		vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
		size_t identifier;

		RenderPassDetails(size_t _identifier)
		: identifier(_identifier)
		{ }
	};

	// render passes
	std::vector<RenderPassDetails> renderPasses;

	// depth render passes
	vk::Format depthBufferFormat = vk::Format::eUndefined;
	uint32_t depthMapSize = 4096;
	VkDepthMapImage* pDepthMapImage = nullptr;
	std::vector<WZ_vk::UniqueImageView> depthMapCascadeView;

	// scene render pass
	vk::Format sceneImageFormat = vk::Format::eUndefined;
	VkRenderedImage* pSceneImage = nullptr;
	vk::Image sceneMSAAImage;
	vk::DeviceMemory sceneMSAAMemory;
	vk::ImageView sceneMSAAView;
	vk::Image sceneDepthStencilImage;
	vk::DeviceMemory sceneDepthStencilMemory;
	vk::ImageView sceneDepthStencilView;

	// default textures
	VkTexture* pDefaultTexture = nullptr;
	VkTextureArray* pDefaultArrayTexture = nullptr;
	VkDepthMapImage* pDefaultDepthMapTexture = nullptr;

	// Debug_Utils
	PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessenger = nullptr;
	PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessenger = nullptr;
	VkDebugUtilsMessengerEXT debugUtilsCallback = 0;
	VkDebugUtilsMessengerCreateInfoEXT instanceDebugUtilsCallbacksInfo = {};

	// Debug_Report (old)
	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = nullptr;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = nullptr;
	PFN_vkDebugReportMessageEXT dbgBreakCallback = nullptr;
	VkDebugReportCallbackEXT msgCallback = 0;

	struct BuiltPipelineRegistry
	{
		const gfx_api::pipeline_create_info createInfo;
		std::vector<VkPSO *> renderPassPSO;

		BuiltPipelineRegistry(const gfx_api::pipeline_create_info& _createInfo, size_t numRenderPasses)
		: createInfo(_createInfo)
		{
			renderPassPSO.resize(numRenderPasses, nullptr);
		}
	};

	std::vector<BuiltPipelineRegistry> createdPipelines;
	VkPSO* currentPSO = nullptr;

	bool validationLayer = false;
	bool debugCallbacksEnabled = true;
	bool debugUtilsExtEnabled = false;

	bool startedRenderPass = false;
	const size_t maxErrorHandlingDepth = 10;
	std::vector<vk::Result> errorHandlingDepth;

	size_t frameNum = 0;

public:
	VkRoot(bool _debug);
	~VkRoot();

	virtual gfx_api::pipeline_state_object * build_pipeline(gfx_api::pipeline_state_object *existing_pso, const gfx_api::pipeline_create_info& createInfo) override;

	virtual void draw(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type&) override;
	virtual void draw_elements(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type&, const gfx_api::index_type&) override;
	virtual void bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) override;
	virtual void unbind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) override;
	virtual void disable_all_vertex_buffers() override;
	virtual void bind_streamed_vertex_buffers(const void* data, const std::size_t size) override;

	virtual gfx_api::texture* create_texture(const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const gfx_api::pixel_format& internal_format, const std::string& filename = "") override;
	virtual gfx_api::texture_array* create_texture_array(const std::size_t& mipmap_count, const std::size_t& layer_count, const std::size_t& width, const std::size_t& height, const gfx_api::pixel_format& internal_format, const std::string& filename = "") override;

	virtual gfx_api::buffer * create_buffer_object(const gfx_api::buffer::usage &usage, const buffer_storage_hint& hint = buffer_storage_hint::static_draw, const std::string& debugName = "") override;

private:

	std::vector<vk::DescriptorSet> allocateDescriptorSet(vk::DescriptorSetLayout arg, vk::DescriptorType descriptorType, uint32_t numDescriptors);
	std::vector<vk::DescriptorSet> allocateDescriptorSets(std::vector<vk::DescriptorSetLayout> args, vk::DescriptorType descriptorType, uint32_t numDescriptors);

	bool getSupportedInstanceExtensions(std::vector<VkExtensionProperties> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr);
	bool findSupportedInstanceExtensions(std::vector<const char*> extensionsToFind, std::vector<const char*> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr);
#if defined(VK_EXT_layer_settings)
	std::vector<vk::LayerSettingEXT> initLayerSettings();
#endif
	bool createVulkanInstance(uint32_t apiVersion, const std::vector<const char*>& extensions, const std::vector<const char*>& layers, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr);
	bool setupDebugUtilsCallbacks(const std::vector<const char*>& extensions, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr);
	bool setupDebugReportCallbacks(const std::vector<const char*>& extensions, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr);
	vk::PhysicalDevice pickPhysicalDevice();

	bool createSurface();
	bool canUseVulkanInstanceAPI(uint32_t minVulkanAPICoreVersion) const;
	bool canUseVulkanDeviceAPI(uint32_t minVulkanAPICoreVersion) const;

	// pickPhysicalDevice();
	void getQueueFamiliesInfo();
	bool createLogicalDevice();
	bool createAllocator();
	void getQueues();
	bool createSwapchain(bool allowHandleSurfaceLost = true);
	void rebuildPipelinesIfNecessary();

	void createDefaultRenderpass(vk::Format swapchainFormat, vk::Format depthFormat);
	void createDepthPasses(vk::Format depthFormat);
	void createDepthPassImagesAndFBOs(vk::Format depthFormat);
	void createSceneRenderpass(vk::Format sceneFormat, vk::Format depthFormat);
	void destroySceneRenderpass();
	void setupSwapchainImages();

public:
	vk::Format get_format(const gfx_api::pixel_format& format) const;

private:
	static vk::IndexType to_vk(const gfx_api::index_type& index);

public:
	virtual void bind_index_buffer(gfx_api::buffer& index_buffer, const gfx_api::index_type& index) override;
	virtual void unbind_index_buffer(gfx_api::buffer&) override;

	virtual void bind_textures(const std::vector<gfx_api::texture_input>& attribute_descriptions, const std::vector<gfx_api::abstract_texture*>& textures) override;

public:
	virtual void set_constants(const void* buffer, const std::size_t& size) override;
	virtual void set_uniforms(const size_t& first, const std::vector<std::tuple<const void*, size_t>>& uniform_blocks) override;

	virtual void bind_pipeline(gfx_api::pipeline_state_object* pso, bool notextures) override;

	virtual size_t numDepthPasses() override;
	virtual bool setDepthPassProperties(size_t numDepthPasses, size_t depthBufferResolution) override;
	virtual void beginDepthPass(size_t idx) override;
	virtual size_t getDepthPassDimensions(size_t idx) override;
	virtual void endCurrentDepthPass() override;
	virtual gfx_api::abstract_texture* getDepthTexture() override;

	virtual void beginSceneRenderPass() override;
	virtual void endSceneRenderPass() override;
	virtual gfx_api::abstract_texture* getSceneTexture() override;

	virtual void beginRenderPass() override;
	virtual void endRenderPass() override;
	virtual void set_polygon_offset(const float& offset, const float& slope) override;
	virtual void set_depth_range(const float& min, const float& max) override;
private:
	void startRenderPass();

	enum AcquireNextSwapchainImageResult
	{
		eSuccess,
		eRecoveredFromError,
		eUnhandledFailure
	};
	AcquireNextSwapchainImageResult acquireNextSwapchainImage();

	bool handleSurfaceLost();
	void waitForAllIdle();
	void destroySwapchainAndSwapchainSpecificStuff(bool doDestroySwapchain);
	bool createNewSwapchainAndSwapchainSpecificStuff(const vk::Result& reason);

public:
	virtual int32_t get_context_value(const gfx_api::context::context_value property) override;
	virtual uint64_t get_estimated_vram_mb(bool dedicatedOnly) override;
	virtual void debugStringMarker(const char *str) override;
	virtual void debugSceneBegin(const char *descr) override;
	virtual void debugSceneEnd(const char *descr) override;
	virtual bool debugPerfAvailable() override;
	virtual bool debugPerfStart(size_t sample) override;
	virtual void debugPerfStop() override;
	virtual void debugPerfBegin(PERF_POINT pp, const char *descr) override;
	virtual void debugPerfEnd(PERF_POINT pp) override;
	virtual uint64_t debugGetPerfValue(PERF_POINT pp) override;
	virtual std::map<std::string, std::string> getBackendGameInfo() override;
	virtual const std::string& getFormattedRendererInfoString() const override;
	virtual bool getScreenshot(std::function<void (std::unique_ptr<iV_Image>)> callback) override;
	virtual void handleWindowSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight) override;
	virtual std::pair<uint32_t, uint32_t> getDrawableDimensions() override;
	bool isYAxisInverted() const override { return true; }
	virtual bool shouldDraw() override;
	virtual void shutdown() override;
	virtual const size_t& current_FrameNum() const override;
	virtual bool setSwapInterval(gfx_api::context::swap_interval_mode mode) override;
	virtual gfx_api::context::swap_interval_mode getSwapInterval() const override;
	virtual bool textureFormatIsSupported(gfx_api::pixel_format_target target, gfx_api::pixel_format format, gfx_api::pixel_format_usage::flags usage) override;
	virtual bool supportsMipLodBias() const override;
	virtual bool supports2DTextureArrays() const override;
	virtual bool supportsIntVertexAttributes() const override;
	virtual size_t maxFramesInFlight() const override;
	virtual gfx_api::lighting_constants getShadowConstants() override;
	virtual bool setShadowConstants(gfx_api::lighting_constants values) override;
	// instanced rendering APIs
	virtual bool supportsInstancedRendering() override;
	virtual void draw_instanced(const std::size_t& offset, const std::size_t &count, const gfx_api::primitive_type &primitive, std::size_t instance_count) override;
	virtual void draw_elements_instanced(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type& primitive, const gfx_api::index_type& index, std::size_t instance_count) override;
	// debug apis for recompiling pipelines
	virtual bool debugRecompileAllPipelines() override;
private:
	virtual bool _initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode mode, optional<float> mipLodBias, uint32_t depthMapResolution) override;
	void initPixelFormatsSupport();
	gfx_api::pixel_format_usage::flags getPixelFormatUsageSupport(gfx_api::pixel_format format) const;
	std::string calculateFormattedRendererInfoString() const;
	void set_uniforms_set(const size_t& set_idx, const void* buffer, size_t bufferSize);
	const RenderPassDetails& currentRenderPass();
	RenderPassDetails& defaultRenderpass() { return renderPasses[DEFAULT_RENDER_PASS_ID]; }
private:
	size_t depthPassCount = WZ_MAX_SHADOW_CASCADES;
	std::string formattedRendererInfoString;
	std::vector<gfx_api::pixel_format_usage::flags> texture2DFormatsSupport;
	uint32_t lastRenderPassEndTime = 0;
	size_t currentRenderPassId = DEFAULT_RENDER_PASS_ID;
};

#endif // defined(WZ_VULKAN_ENABLED)
