/*
	This file is part of Warzone 2100.
	Copyright (C) 2017-2019  Warzone 2100 Project

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

#if defined(WZ_VULKAN_ENABLED)

// General notes for developers:
//
// To maintain compatibility with as many systems as possible:
// 1.) Ensure Vulkan 1.0 compatibility
// 2.) Avoid *requiring* anything outside of the scope of the "Vulkan Portable Subset"
// 3.) All calls to Vulkan APIs should use dynamic dispatch (see the uses of vk::DispatchLoaderDynamic in this file)
// 4.) Test with the Vulkan validation layers enabled (run WZ with --gfxdebug)
//
// #2 means the following things are currently best avoided:
//   - Triangle fans
//   - Separate stencil reference masks
//   - Vulkan Event functionality
//   - Texture-specific swizzles
//   - Allocation callbacks in object creation functions
//
// When in doubt, consult both:
//   - the Vulkan Portability Initiative: https://www.khronos.org/vulkan/portability-initiative
//   - MoltenVK limitations: https://github.com/KhronosGroup/MoltenVK/blob/master/Docs/MoltenVK_Runtime_UserGuide.md#known-moltenvk-limitations
//

#include "gfx_api_vk.h"
#include "lib/framework/physfs_ext.h"
#include "lib/exceptionhandler/dumpinfo.h"

#include <algorithm>
#include <set>
#include <unordered_set>
#include <map>

const size_t MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> optionalInstanceExtensions = {
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME	// used for Vulkan info output
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<const char*> optionalDeviceExtensions = {
	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
	VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME
};

const std::vector<vk::Format> supportedDepthFormats = {
	vk::Format::eD32SfloatS8Uint,
	vk::Format::eD24UnormS8Uint
};

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> debugAdditionalExtensions = {
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};

const uint32_t minRequired_DescriptorSetUniformBuffers = 1;
const uint32_t minRequired_DescriptorSetUniformBuffersDynamic = 1;
const uint32_t minRequired_BoundDescriptorSets = 2;
const uint32_t minRequired_Viewports = 1;
const uint32_t minRequired_ColorAttachments = 1;

// MARK: General helper functions

static uint32_t findProperties(const vk::PhysicalDeviceMemoryProperties& memprops, const uint32_t& memoryTypeBits, const vk::MemoryPropertyFlagBits& properties)
{
	for (uint32_t i = 0; i < memprops.memoryTypeCount; ++i)
	{
		if ((memoryTypeBits & (1 << i)) &&
			((memprops.memoryTypes[i].propertyFlags & properties) == properties))
		{
			return i;
		}
	}
	return -1;
}

vk::Format findSupportedFormat(const vk::PhysicalDevice& physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, const vk::DispatchLoaderDynamic& vkDynLoader) {
	for (vk::Format format : candidates)
	{
		vk::FormatProperties props;
		physicalDevice.getFormatProperties(format, &props, vkDynLoader);

		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface, const vk::DispatchLoaderDynamic &vkDynLoader)
{
	QueueFamilyIndices indices;

	const auto & queueFamilies = device.getQueueFamilyProperties(vkDynLoader);

	uint32_t i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
		{
			indices.graphicsFamily = i;
		}

		vk::Bool32 presentSupport = false;
		try
		{
			presentSupport = device.getSurfaceSupportKHR(i, surface, vkDynLoader);
		}
		catch (const vk::SystemError& e)
		{
			// getSurfaceSupportKHR failed
			debug(LOG_ERROR, "getSurfaceSupportKHR failed: %s", e.what());
		}

		if (queueFamily.queueCount > 0 && presentSupport)
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface, const vk::DispatchLoaderDynamic &vkDynLoader)
{
	SwapChainSupportDetails details;

	details.capabilities = device.getSurfaceCapabilitiesKHR(surface, vkDynLoader);
	details.formats = device.getSurfaceFormatsKHR(surface, vkDynLoader);
	details.presentModes = device.getSurfacePresentModesKHR(surface, vkDynLoader);

	return details;
}

std::vector<const char*> findSupportedDeviceExtensions(const vk::PhysicalDevice &device, const std::vector<const char*> &desiredExtensions, const vk::DispatchLoaderDynamic &vkDynLoader)
{
	const auto availableExtensions = device.enumerateDeviceExtensionProperties(nullptr, vkDynLoader); // TODO: handle thrown error?
	std::unordered_set<std::string> supportedExtensionNames;
	for (auto & extension : availableExtensions)
	{
		supportedExtensionNames.insert(extension.extensionName);
	}

	std::vector<const char*> foundExtensions;
	for (const char* extensionName : desiredExtensions)
	{
		if(supportedExtensionNames.find(extensionName) != supportedExtensionNames.end())
		{
			foundExtensions.push_back(extensionName);
		}
		else
		{
			debug(LOG_3D, "Vulkan: Did not find device extension: %s", extensionName);
		}
	}

	return foundExtensions;
}

bool checkDeviceExtensionSupport(const vk::PhysicalDevice &device, const std::vector<const char*> &desiredExtensions, const vk::DispatchLoaderDynamic &vkDynLoader)
{
	const auto availableExtensions = device.enumerateDeviceExtensionProperties(nullptr, vkDynLoader); // TODO: handle thrown error?

	std::unordered_set<std::string> requiredExtensions(desiredExtensions.begin(), desiredExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

static bool getSupportedInstanceExtensions(std::vector<VkExtensionProperties> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	ASSERT(_vkGetInstanceProcAddr, "_vkGetInstanceProcAddr must be valid");
	PFN_vkEnumerateInstanceExtensionProperties _vkEnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties")));
	if (!_vkEnumerateInstanceExtensionProperties)
	{
		debug(LOG_ERROR, "Could not find symbol: vkEnumerateInstanceExtensionProperties\n");
		return false;
	}
	uint32_t extensionCount;
	_vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); //get number of extensions

	std::vector<VkExtensionProperties> _extensions(extensionCount);
	_vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, _extensions.data()); //populate buffer

	output = _extensions;
	return true;
}

static bool getInstanceLayerProperties(std::vector<VkLayerProperties> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	ASSERT(_vkGetInstanceProcAddr, "_vkGetInstanceProcAddr must be valid");
	PFN_vkEnumerateInstanceLayerProperties _vkEnumerateInstanceLayerProperties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties")));
	if (!_vkEnumerateInstanceLayerProperties)
	{
		debug(LOG_ERROR, "Could not find symbol: vkEnumerateInstanceLayerProperties\n");
		return false;
	}
	uint32_t layerCount;
	_vkEnumerateInstanceLayerProperties(&layerCount, nullptr); //get number of layers

	std::vector<VkLayerProperties> availableLayers(layerCount);
	_vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); //populate buffer

	output = availableLayers;
	return true;
}

VkSampleCountFlagBits getMaxUsableSampleCount(const vk::PhysicalDeviceProperties &physicalDeviceProperties)
{
	VkSampleCountFlags counts = std::min({
		static_cast<VkSampleCountFlags>(physicalDeviceProperties.limits.framebufferColorSampleCounts),
		static_cast<VkSampleCountFlags>(physicalDeviceProperties.limits.framebufferDepthSampleCounts),
		static_cast<VkSampleCountFlags>(physicalDeviceProperties.limits.framebufferStencilSampleCounts)
	});
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

vk::Format findDepthFormat(const vk::PhysicalDevice& physicalDevice, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	return findSupportedFormat(
		physicalDevice,
		supportedDepthFormats,
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlags{vk::FormatFeatureFlagBits::eDepthStencilAttachment},
		vkDynLoader
	);
}

bool checkValidationLayerSupport(PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	std::vector<VkLayerProperties> availableLayers;
	if (!getInstanceLayerProperties(availableLayers, _vkGetInstanceProcAddr))
	{
		// getInstanceLayerProperties failed
		return false;
	}

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

bool findSupportedInstanceExtensions(std::vector<const char*> extensionsToFind, std::vector<const char*> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	std::vector<VkExtensionProperties> supportedExtensions;
	if (!getSupportedInstanceExtensions(supportedExtensions, _vkGetInstanceProcAddr))
	{
		// Failed to get supported extensions
		return false;
	}
	std::unordered_set<std::string> supportedExtensionNames;
	for (auto & extension : supportedExtensions)
	{
		supportedExtensionNames.insert(extension.extensionName);
	}

	std::vector<const char*> foundExtensions;
	for (const char* extensionName : extensionsToFind)
	{

		if(supportedExtensionNames.find(extensionName) != supportedExtensionNames.end())
		{
			foundExtensions.push_back(extensionName);
		}
		else
		{
			debug(LOG_3D, "Vulkan: Did not find extension: %s", extensionName);
		}
	}

	output = foundExtensions;
	return true;
}

// MARK: BlockBufferAllocator

BlockBufferAllocator::BlockBufferAllocator(VmaAllocator allocator, uint32_t minimumBlockSize, const vk::BufferUsageFlags& usageFlags, const VmaAllocationCreateInfo& allocInfo, bool autoMap /* = false */)
: allocator(allocator), minimumBlockSize(minimumBlockSize), usageFlags(usageFlags), allocInfo(allocInfo), autoMap(autoMap)
{ }

BlockBufferAllocator::BlockBufferAllocator(VmaAllocator allocator, uint32_t minimumBlockSize, const vk::BufferUsageFlags& usageFlags, const VmaMemoryUsage usage, bool autoMap /* = false */)
: allocator(allocator), minimumBlockSize(minimumBlockSize), usageFlags(usageFlags), autoMap(autoMap)
{
	VmaAllocationCreateInfo tmpAllocInfo = { };
	tmpAllocInfo.usage = usage;
	allocInfo = tmpAllocInfo;
}

BlockBufferAllocator::~BlockBufferAllocator()
{
	if (autoMap)
	{
		unmapAutomappedMemory();
	}
	for (auto& block : blocks)
	{
		ASSERT(block.pMappedMemory == nullptr, "Likely missing a call to unmapAutomappedMemory");
		vmaDestroyBuffer(allocator, block.buffer, block.allocation);
	}
	blocks.clear();
}

std::tuple<uint32_t, uint32_t> BlockBufferAllocator::getWritePosAndNewWriteLocation(uint32_t currentWritePos, uint32_t amount, uint32_t totalSize, uint32_t align)
{
	assert(amount < totalSize);
	currentWritePos = ((currentWritePos + align - 1) / align) * align;
	if (currentWritePos + amount < totalSize)
	{
		return std::make_tuple(currentWritePos, currentWritePos + amount);
	}
	return std::make_tuple(0u, amount);
}

void BlockBufferAllocator::allocateNewBlock(uint32_t minimumSize)
{
	uint32_t newBlockSize = std::max({minimumSize, (blocks.empty() ? minimumFirstBlockSize : 0), minimumBlockSize, (totalCapacity < static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())) ? static_cast<uint32_t>(totalCapacity) : std::numeric_limits<uint32_t>::max()});
	auto bufferCreateInfo = vk::BufferCreateInfo{}
		.setSize(newBlockSize)
		.setUsage(usageFlags);

	totalCapacity += newBlockSize;

	Block newBlock;
	newBlock.size = newBlockSize;
	vk::Result result = static_cast<vk::Result>(vmaCreateBuffer(allocator, reinterpret_cast<const VkBufferCreateInfo*>( &bufferCreateInfo ), &allocInfo, reinterpret_cast<VkBuffer*>( &newBlock.buffer ), &newBlock.allocation, nullptr));
	if (result != vk::Result::eSuccess)
	{
		// Failed to allocate memory!
		throwResultException( result, "vmaCreateBuffer" );
	}

	if (autoMap)
	{
		vmaMapMemory(allocator, newBlock.allocation, &newBlock.pMappedMemory);
	}

	blocks.push_back(newBlock);
	currentWritePosInLastBlock = 0;
}

BlockBufferAllocator::AllocationResult BlockBufferAllocator::alloc(uint32_t amount, uint32_t align)
{
	if (!blocks.empty())
	{
		const uint32_t lastBlockSize = blocks.back().size;
		if (amount < lastBlockSize)
		{
			// attempt to see if this request fits in the remaining size in the last block
			uint32_t newWritePos = ((currentWritePosInLastBlock + align - 1) / align) * align;
			if (newWritePos + amount < lastBlockSize)
			{
				currentWritePosInLastBlock = newWritePos + amount;
				return BlockBufferAllocator::AllocationResult(blocks.back(), newWritePos);
			}
		}
	}

	// otherwise, allocate a new block
	allocateNewBlock(amount);
	uint32_t newWritePos = 0;
	ASSERT(newWritePos + amount <= blocks.back().size, "Failed to allocate new block");
	currentWritePosInLastBlock = newWritePos + amount;
	return BlockBufferAllocator::AllocationResult(blocks.back(), newWritePos);
}

void * BlockBufferAllocator::mapMemory(AllocationResult memoryAllocation)
{
	void* mappedData = nullptr;
	if (memoryAllocation.block.pMappedMemory)
	{
		mappedData = memoryAllocation.block.pMappedMemory;
	}
	else
	{
		vmaMapMemory(allocator, memoryAllocation.block.allocation, &mappedData);
	}
	if (memoryAllocation.offset > 0)
	{
		return reinterpret_cast<uint8_t*>(mappedData) + memoryAllocation.offset;
	}
	else
	{
		return mappedData;
	}
}

void BlockBufferAllocator::unmapMemory(AllocationResult memoryAllocation)
{
	if (memoryAllocation.block.pMappedMemory)
	{
		// no-op - call unmapAutomappedMemory instead
		return;
	}
	vmaUnmapMemory(allocator, memoryAllocation.block.allocation);
}

void BlockBufferAllocator::unmapAutomappedMemory()
{
	ASSERT(autoMap, "Only useful when autoMap == true");
	for (auto& block : blocks)
	{
		if (block.pMappedMemory)
		{
			vmaUnmapMemory(allocator, block.allocation);
			block.pMappedMemory = nullptr;
		}
	}
}

void BlockBufferAllocator::flushAutomappedMemory()
{
	ASSERT(autoMap, "Only useful when autoMap == true");
	for (auto& block : blocks)
	{
		ASSERT(block.pMappedMemory != nullptr, "Block must still be (auto-)mapped");
		vmaFlushAllocation(allocator, block.allocation, 0, VK_WHOLE_SIZE);
	}
}

void BlockBufferAllocator::clean()
{
	uint64_t totalMemoryAllocated = 0;
	uint64_t totalMemoryUsed = 0;
	for (auto& block : blocks)
	{
		totalMemoryAllocated += static_cast<uint64_t>(block.size);
	}
	if (!blocks.empty())
	{
		totalMemoryUsed = totalMemoryAllocated - (blocks.back().size - currentWritePosInLastBlock);
	}

	uint32_t old_minimumFirstBlockSize = minimumFirstBlockSize;
	if (blocks.size() > 1)
	{
		minimumFirstBlockSize = totalMemoryAllocated;
	}
	else if (totalMemoryUsed < (minimumFirstBlockSize / 4))
	{
		minimumFirstBlockSize = minimumFirstBlockSize / 2;
	}

	if ((old_minimumFirstBlockSize != minimumFirstBlockSize) && (minimumFirstBlockSize > minimumBlockSize))
	{
		// free all existing blocks
		for (auto& block : blocks)
		{
			ASSERT(block.pMappedMemory == nullptr, "Likely missing a call to unmapAutomappedMemory");
			vmaDestroyBuffer(allocator, block.buffer, block.allocation);
		}
		blocks.clear();
	}

	ASSERT(blocks.size() <= 1, "Should either be 0 or 1 retained block");

	if (autoMap)
	{
		// re-map blocks that were retained
		for (auto& block : blocks)
		{
			if (block.pMappedMemory == nullptr)
			{
				vmaMapMemory(allocator, block.allocation, &block.pMappedMemory);
			}
		}
	}

	currentWritePosInLastBlock = 0;
	totalCapacity = (!blocks.empty()) ? blocks.back().size : 0;
}

// MARK: perFrameResources_t

perFrameResources_t::perFrameResources_t(vk::Device& _dev, const VmaAllocator& allocator, const uint32_t& graphicsQueueFamilyIndex, const vk::DispatchLoaderDynamic& vkDynLoader)
	: dev(_dev)
	, allocator(allocator)
	, stagingBufferAllocator(allocator, 1024 * 1024, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY)
	, streamedVertexBufferAllocator(allocator, 128 * 1024, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, true)
	, uniformBufferAllocator(allocator, 1024 * 1024, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, true)
	, pVkDynLoader(&vkDynLoader)
{
	const auto& descriptorSize =
		std::array<vk::DescriptorPoolSize, 2> {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 10000),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 10000)
	};
	descriptorPool = dev.createDescriptorPool(
		vk::DescriptorPoolCreateInfo{}
		.setMaxSets(10000)
		.setPPoolSizes(descriptorSize.data())
		.setPoolSizeCount(descriptorSize.size())
		, nullptr, *pVkDynLoader
	);
	pool = dev.createCommandPool(
		vk::CommandPoolCreateInfo{}
		.setQueueFamilyIndex(graphicsQueueFamilyIndex)
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		, nullptr, *pVkDynLoader
	);
	const auto& buffer = dev.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo{}
		.setCommandPool(pool)
		.setCommandBufferCount(2)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		, *pVkDynLoader
	);
	cmdDraw = buffer[0];
	cmdCopy = buffer[1];
	cmdCopy.begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit), vkDynLoader);
	previousSubmission = dev.createFence(
		vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled),
		nullptr, *pVkDynLoader
	);

	imageAcquireSemaphore = dev.createSemaphore(vk::SemaphoreCreateInfo{}, nullptr, *pVkDynLoader);
	renderFinishedSemaphore = dev.createSemaphore(vk::SemaphoreCreateInfo{}, nullptr, *pVkDynLoader);
}

void perFrameResources_t::clean()
{
	stagingBufferAllocator.clean();
	streamedVertexBufferAllocator.clean();
	uniformBufferAllocator.clean();

	for (auto buffer : buffer_to_delete)
	{
		dev.destroyBuffer(buffer, nullptr, *pVkDynLoader);
	}
	buffer_to_delete.clear();
	image_view_to_delete.clear();
	for (auto image : image_to_delete)
	{
		dev.destroyImage(image, nullptr, *pVkDynLoader);
	}
	image_to_delete.clear();
	perPSO_dynamicUniformBufferDescriptorSets.clear();
	for (auto allocation : vmamemory_to_free)
	{
		vmaFreeMemory(allocator, allocation);
	}
	vmamemory_to_free.clear();
}

perFrameResources_t::~perFrameResources_t()
{
	dev.destroyCommandPool(pool, nullptr, *pVkDynLoader);
	dev.destroyDescriptorPool(descriptorPool, nullptr, *pVkDynLoader);
	dev.destroyFence(previousSubmission, nullptr, *pVkDynLoader);
	dev.destroySemaphore(imageAcquireSemaphore, nullptr, *pVkDynLoader);
	dev.destroySemaphore(renderFinishedSemaphore, nullptr, *pVkDynLoader);
	clean();
}

perFrameResources_t& buffering_mechanism::get_current_resources()
{
	return *perFrameResources[currentFrame];
}

// MARK: buffering_mechanism

void buffering_mechanism::init(vk::Device dev, const VmaAllocator& allocator, size_t swapChainImageCount, const uint32_t& graphicsQueueFamilyIndex, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	currentFrame = 0;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		perFrameResources.emplace_back(new perFrameResources_t(dev, allocator, graphicsQueueFamilyIndex, vkDynLoader));
	}

	const auto fences = std::array<vk::Fence, 1> { buffering_mechanism::get_current_resources().previousSubmission };
	dev.resetFences(fences, vkDynLoader);
}

void buffering_mechanism::destroy(vk::Device dev, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	perFrameResources.clear();
	currentFrame = 0;
}

void buffering_mechanism::swap(vk::Device dev, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	currentFrame = (currentFrame < (perFrameResources.size() - 1)) ? currentFrame + 1 : 0;

	const auto fences = std::array<vk::Fence, 1> { buffering_mechanism::get_current_resources().previousSubmission };
	dev.waitForFences(fences, true, -1, vkDynLoader);
	dev.resetFences(fences, vkDynLoader);
	dev.resetDescriptorPool(buffering_mechanism::get_current_resources().descriptorPool, vk::DescriptorPoolResetFlags(), vkDynLoader);
	dev.resetCommandPool(buffering_mechanism::get_current_resources().pool, vk::CommandPoolResetFlagBits{}, vkDynLoader);

	buffering_mechanism::get_current_resources().clean();
	buffering_mechanism::get_current_resources().numalloc = 0;
}

// MARK: Definitions of statics

std::vector<std::unique_ptr<perFrameResources_t>> buffering_mechanism::perFrameResources;
size_t buffering_mechanism::currentFrame;

// MARK: Debug Callback

VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t srcObject,
	std::size_t location,
	int32_t msgCode,
	const char* pLayerPrefix,
	const char* pMsg,
	void* pUserData)
{
	std::stringstream buf;
	bool logFatal = false;
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		buf << "ERROR: ";
		logFatal = true;
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		buf << "WARNING: ";
		logFatal = true;
	}
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		buf << "PERF: ";
		logFatal = false;
	}
	else {
		logFatal = false;
	}
	buf << "[" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg;
	debug((logFatal) ? LOG_FATAL : LOG_3D, "%s", buf.str().c_str());
	return false;
}

// MARK: VkPSO

struct shader_infos
{
	std::string vertex;
	std::string fragment;
};

static const std::map<SHADER_MODE, shader_infos> spv_files
{
	std::make_pair(SHADER_COMPONENT, shader_infos{ "shaders/vk/tcmask.vert.spv", "shaders/vk/tcmask.frag.spv" }),
	std::make_pair(SHADER_BUTTON, shader_infos{ "shaders/vk/button.vert.spv", "shaders/vk/button.frag.spv" }),
	std::make_pair(SHADER_NOLIGHT, shader_infos{ "shaders/vk/nolight.vert.spv", "shaders/vk/nolight.frag.spv" }),
	std::make_pair(SHADER_TERRAIN, shader_infos{ "shaders/vk/terrain.vert.spv", "shaders/vk/terrain.frag.spv" }),
	std::make_pair(SHADER_TERRAIN_DEPTH, shader_infos{ "shaders/vk/terrain_depth.vert.spv", "shaders/vk/terraindepth.frag.spv" }),
	std::make_pair(SHADER_DECALS, shader_infos{ "shaders/vk/decals.vert.spv", "shaders/vk/decals.frag.spv" }),
	std::make_pair(SHADER_WATER, shader_infos{ "shaders/vk/terrain_water.vert.spv", "shaders/vk/water.frag.spv" }),
	std::make_pair(SHADER_RECT, shader_infos{ "shaders/vk/rect.vert.spv", "shaders/vk/rect.frag.spv" }),
	std::make_pair(SHADER_TEXRECT, shader_infos{ "shaders/vk/rect.vert.spv", "shaders/vk/texturedrect.frag.spv" }),
	std::make_pair(SHADER_GFX_COLOUR, shader_infos{ "shaders/vk/gfx_color.vert.spv", "shaders/vk/gfx.frag.spv" }),
	std::make_pair(SHADER_GFX_TEXT, shader_infos{ "shaders/vk/gfx_text.vert.spv", "shaders/vk/texturedrect.frag.spv" }),
	std::make_pair(SHADER_GENERIC_COLOR, shader_infos{ "shaders/vk/generic.vert.spv", "shaders/vk/rect.frag.spv" }),
	std::make_pair(SHADER_LINE, shader_infos{ "shaders/vk/line.vert.spv", "shaders/vk/rect.frag.spv" }),
	std::make_pair(SHADER_TEXT, shader_infos{ "shaders/vk/rect.vert.spv", "shaders/vk/text.frag.spv" })
};

std::vector<uint32_t> VkPSO::readShaderBuf(const std::string& name)
{
	auto fp = PHYSFS_openRead(name.c_str());
	debug(LOG_3D, "Reading...[directory: %s] %s", PHYSFS_getRealDir(name.c_str()), name.c_str());
	assert(fp != nullptr);

	const auto& filesize = PHYSFS_fileLength(fp);
	ASSERT_OR_RETURN(std::vector<uint32_t>(), filesize < static_cast<PHYSFS_sint64>(std::numeric_limits<PHYSFS_sint32>::max()), "\"%s\" filesize >= std::numeric_limits<PHYSFS_sint32>::max()", name.c_str());
	ASSERT_OR_RETURN(std::vector<uint32_t>(), static_cast<PHYSFS_uint64>(filesize) < static_cast<PHYSFS_uint64>(std::numeric_limits<size_t>::max()), "\"%s\" filesize >= std::numeric_limits<size_t>::max()", name.c_str());

	auto buffer = std::vector<uint32_t>(static_cast<size_t>(filesize / sizeof(uint32_t)));
	WZ_PHYSFS_readBytes(fp, buffer.data(), static_cast<PHYSFS_uint32>(filesize));
	PHYSFS_close(fp);

	return buffer;
}

vk::ShaderModule VkPSO::get_module(const std::string& name, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	const auto& tmp = readShaderBuf(name);
	return dev.createShaderModule(
		vk::ShaderModuleCreateInfo{}
		.setCodeSize(tmp.size() * 4)
		.setPCode(tmp.data())
		, nullptr, vkDynLoader
	);
}

std::array<vk::PipelineShaderStageCreateInfo, 2> VkPSO::get_stages(const vk::ShaderModule& vertexModule, const vk::ShaderModule& fragmentModule)
{
	return std::array<vk::PipelineShaderStageCreateInfo, 2> {
		vk::PipelineShaderStageCreateInfo{}
			.setModule(vertexModule)
			.setPName("main")
			.setStage(vk::ShaderStageFlagBits::eVertex),
			vk::PipelineShaderStageCreateInfo{}
			.setModule(fragmentModule)
			.setPName("main")
			.setStage(vk::ShaderStageFlagBits::eFragment)
	};
}

std::array<vk::PipelineColorBlendAttachmentState, 1> VkPSO::to_vk(const REND_MODE& blend_state, const uint8_t& color_mask)
{
	const auto& full_color_output = vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;
	const auto vk_color_mask = color_mask == 0 ? vk::ColorComponentFlags{} : full_color_output;

	switch (blend_state)
	{
	case REND_ADDITIVE:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(true)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
				.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
				.setDstColorBlendFactor(vk::BlendFactor::eOne)
				.setDstAlphaBlendFactor(vk::BlendFactor::eOne)
				.setColorWriteMask(vk_color_mask)
		};
	}
	case REND_ALPHA:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(true)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
				.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
				.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setColorWriteMask(vk_color_mask)
		};
	}
	case REND_PREMULTIPLIED:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(true)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
				.setSrcColorBlendFactor(vk::BlendFactor::eOne)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
				.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setColorWriteMask(vk_color_mask)
		};
	}
	case REND_MULTIPLICATIVE:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(true)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
				.setSrcColorBlendFactor(vk::BlendFactor::eZero)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eZero)
				.setDstColorBlendFactor(vk::BlendFactor::eSrcColor)
				.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setColorWriteMask(vk_color_mask)
		};
	}
	case REND_OPAQUE:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(false)
				.setColorWriteMask(vk_color_mask)
		};
	}
	case REND_TEXT:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(true)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
				.setSrcColorBlendFactor(vk::BlendFactor::eOne)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
				.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setColorWriteMask(vk_color_mask)
		};
	}
	default:
		debug(LOG_FATAL, "Wrong alpha state");
	}
	debug(LOG_FATAL, "Unsupported blend_state");
	return std::array<vk::PipelineColorBlendAttachmentState, 1>{vk::PipelineColorBlendAttachmentState{}};
}

vk::PipelineDepthStencilStateCreateInfo VkPSO::to_vk(DEPTH_MODE depth_mode, const gfx_api::stencil_mode& stencil)
{
	auto state = vk::PipelineDepthStencilStateCreateInfo{};
	switch (stencil)
	{
	case gfx_api::stencil_mode::stencil_disabled:
		state.setStencilTestEnable(false);
		break;
	case gfx_api::stencil_mode::stencil_shadow_quad:
	{
		state.setStencilTestEnable(true);
		const auto stencil_mode = vk::StencilOpState{}
			.setDepthFailOp(vk::StencilOp::eKeep)
			.setFailOp(vk::StencilOp::eKeep)
			.setPassOp(vk::StencilOp::eKeep)
			.setCompareOp(vk::CompareOp::eLess)
			.setReference(0)
			.setWriteMask(~0)
			.setCompareMask(~0);
		state.setFront(stencil_mode);
		state.setBack(stencil_mode);
		break;
	}
	case gfx_api::stencil_mode::stencil_shadow_silhouette:
	{
		state.setStencilTestEnable(true);
		state.setFront(vk::StencilOpState{}
			.setDepthFailOp(vk::StencilOp::eKeep)
			.setFailOp(vk::StencilOp::eKeep)
			.setPassOp(vk::StencilOp::eIncrementAndWrap)
			.setCompareOp(vk::CompareOp::eAlways)
			.setReference(0)
			.setWriteMask(~0)
			.setCompareMask(~0));
		state.setBack(vk::StencilOpState{}
			.setDepthFailOp(vk::StencilOp::eKeep)
			.setFailOp(vk::StencilOp::eKeep)
			.setPassOp(vk::StencilOp::eDecrementAndWrap)
			.setCompareOp(vk::CompareOp::eAlways)
			.setReference(0)
			.setWriteMask(~0)
			.setCompareMask(~0));
		break;
	}
	}

	switch (depth_mode)
	{
	case DEPTH_CMP_LEQ_WRT_ON:
	{
		return state
			.setDepthTestEnable(true)
			.setDepthWriteEnable(true)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual);
	}
	case DEPTH_CMP_LEQ_WRT_OFF:
	{
		return state
			.setDepthTestEnable(true)
			.setDepthWriteEnable(false)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual);
	}
	case DEPTH_CMP_ALWAYS_WRT_ON:
		return state
			.setDepthTestEnable(true)
			.setDepthWriteEnable(true)
			.setDepthCompareOp(vk::CompareOp::eAlways);
	case DEPTH_CMP_ALWAYS_WRT_OFF:
	{
		return state
			.setDepthTestEnable(false)
			.setDepthWriteEnable(false);
	}
	default:
		debug(LOG_FATAL, "Wrong depth mode");
	}
	// silence warning
	debug(LOG_FATAL, "Reached end of function");
	return state;
}

vk::PipelineRasterizationStateCreateInfo VkPSO::to_vk(const bool& offset, const gfx_api::cull_mode& cull)
{
	auto result = vk::PipelineRasterizationStateCreateInfo{}
		.setLineWidth(1.f)
		.setPolygonMode(vk::PolygonMode::eFill);
	if (offset)
	{
		result = result.setDepthBiasEnable(true);
	}
	switch (cull)
	{
	case gfx_api::cull_mode::back:
		result = result.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eClockwise);
		break;
	case gfx_api::cull_mode::none:
		result = result.setCullMode(vk::CullModeFlagBits::eNone)
			.setFrontFace(vk::FrontFace::eClockwise);
		break;
	default:
		break;
	}
	return result;
}

vk::Format VkPSO::to_vk(const gfx_api::vertex_attribute_type& type)
{
	switch (type)
	{
	case gfx_api::vertex_attribute_type::float4:
		return vk::Format::eR32G32B32A32Sfloat;
	case gfx_api::vertex_attribute_type::float3:
		return vk::Format::eR32G32B32Sfloat;
	case gfx_api::vertex_attribute_type::float2:
		return vk::Format::eR32G32Sfloat;
	case gfx_api::vertex_attribute_type::u8x4_norm:
		return vk::Format::eR8G8B8A8Unorm;
	}
	debug(LOG_FATAL, "Unsupported vertex_attribute_type");
	return vk::Format::eUndefined;
}

vk::SamplerCreateInfo VkPSO::to_vk(const gfx_api::sampler_type& type)
{
	switch (type)
	{
	case gfx_api::sampler_type::bilinear:
		return vk::SamplerCreateInfo{}
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setMaxAnisotropy(1.f)
			.setMinLod(0.f)
			.setMaxLod(0.f)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
	case gfx_api::sampler_type::bilinear_repeat:
		return vk::SamplerCreateInfo{}
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setMaxAnisotropy(1.f)
			.setMinLod(0.f)
			.setMaxLod(0.f)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat);
	case gfx_api::sampler_type::anisotropic:
		return vk::SamplerCreateInfo{}
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setMaxAnisotropy(16.f)
			.setAnisotropyEnable(true)
			.setMinLod(0.f)
			.setMaxLod(10.f)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
	case gfx_api::sampler_type::anisotropic_repeat:
		return vk::SamplerCreateInfo{}
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setMaxAnisotropy(16.f)
			.setAnisotropyEnable(true)
			.setMinLod(0.f)
			.setMaxLod(10.f)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat);
	case gfx_api::sampler_type::nearest_clamped:
		return vk::SamplerCreateInfo{}
			.setMinFilter(vk::Filter::eNearest)
			.setMagFilter(vk::Filter::eNearest)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setMaxAnisotropy(1.f)
			.setMinLod(0.f)
			.setMaxLod(0.f)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
	}
	debug(LOG_FATAL, "Unsupported sampler_type");
	return vk::SamplerCreateInfo{};
}

vk::PrimitiveTopology VkPSO::to_vk(const gfx_api::primitive_type& primitive)
{
	switch (primitive)
	{
	case gfx_api::primitive_type::lines:
		return vk::PrimitiveTopology::eLineList;
	case gfx_api::primitive_type::line_strip:
		return vk::PrimitiveTopology::eLineStrip;
	case gfx_api::primitive_type::triangles:
		return vk::PrimitiveTopology::eTriangleList;
	case gfx_api::primitive_type::triangle_strip:
		return vk::PrimitiveTopology::eTriangleStrip;
	// NOTE: triangle_fan is explicitly *NOT* supported, as it is not part of the Vulkan Portable Subset
	//       (And is not supported on portability layers, like Vulkan -> DX, or Vulkan -> Metal)
	//       See: https://www.khronos.org/vulkan/portability-initiative
	}
	debug(LOG_FATAL, "Unsupported primitive_type");
	return vk::PrimitiveTopology{};
}

VkPSO::VkPSO(vk::Device _dev,
	const vk::PhysicalDeviceLimits& limits,
	const gfxapi_PipelineCreateInfo& createInfo,
	vk::RenderPass rp,
	const std::shared_ptr<VkhRenderPassCompat>& renderpass_compat,
	vk::SampleCountFlagBits rasterizationSamples,
	const vk::DispatchLoaderDynamic& _vkDynLoader
	) : dev(_dev), pVkDynLoader(&_vkDynLoader), renderpass_compat(renderpass_compat)
{
	const gfx_api::state_description& state_desc = createInfo.state_desc;
	const SHADER_MODE& shader_mode = createInfo.shader_mode;
	const gfx_api::primitive_type& primitive = createInfo.primitive;
	const std::vector<gfx_api::texture_input>& texture_desc = createInfo.texture_desc;
	const std::vector<gfx_api::vertex_buffer>& attribute_descriptions = createInfo.attribute_descriptions;

	const auto cbuffer_layout_desc = std::array<vk::DescriptorSetLayoutBinding, 1>{
		vk::DescriptorSetLayoutBinding{}
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
			.setStageFlags(vk::ShaderStageFlagBits::eAllGraphics)
	};
	cbuffer_set_layout = dev.createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo{}
			.setBindingCount(1)
			.setPBindings(cbuffer_layout_desc.data())
		, nullptr, *pVkDynLoader);


	auto textures_layout_desc = std::vector<vk::DescriptorSetLayoutBinding>{};
	samplers.reserve(texture_desc.size());
	for (const auto& texture : texture_desc)
	{
		samplers.emplace_back(dev.createSampler(to_vk(texture.sampler), nullptr, *pVkDynLoader));

		textures_layout_desc.emplace_back(
			vk::DescriptorSetLayoutBinding{}
				.setBinding(texture.id)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setPImmutableSamplers(&samplers.back())
				.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		);
	}
	textures_set_layout = dev.createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo{}
			.setBindingCount(textures_layout_desc.size())
			.setPBindings(textures_layout_desc.data())
		, nullptr, *pVkDynLoader);

	const auto layout_desc = std::array<vk::DescriptorSetLayout, 2>{cbuffer_set_layout, textures_set_layout};
	static_assert(minRequired_BoundDescriptorSets >= layout_desc.size(), "minRequired_BoundDescriptorSets must be >= layout_desc.size()");

	layout = dev.createPipelineLayout(vk::PipelineLayoutCreateInfo{}
		.setPSetLayouts(layout_desc.data())
		.setSetLayoutCount(layout_desc.size())
		, nullptr, *pVkDynLoader);

	const auto dynamicStates = std::array<vk::DynamicState, 4>{vk::DynamicState::eScissor, vk::DynamicState::eViewport, vk::DynamicState::eDepthBias};
	const auto multisampleState = vk::PipelineMultisampleStateCreateInfo{}
		.setRasterizationSamples(rasterizationSamples);
	const auto dynamicS = vk::PipelineDynamicStateCreateInfo{}
		.setDynamicStateCount(dynamicStates.size())
		.setPDynamicStates(dynamicStates.data());
	const auto viewportState = vk::PipelineViewportStateCreateInfo{}
		.setViewportCount(1)
		.setScissorCount(1);
	ASSERT(viewportState.viewportCount <= limits.maxViewports, "viewportCount (%" PRIu32") exceeds limits.maxViewports (%" PRIu32")", viewportState.viewportCount, limits.maxViewports);

	const auto iassembly = vk::PipelineInputAssemblyStateCreateInfo{}
		.setTopology(to_vk(primitive));

	std::size_t buffer_id = 0;
	std::vector<vk::VertexInputBindingDescription> buffers{};
	std::vector<vk::VertexInputAttributeDescription> attributes{};
	for (const auto& buffer : attribute_descriptions)
	{
		ASSERT(buffer.stride <= limits.maxVertexInputBindingStride, "buffer.stride (%zu) exceeds limits.maxVertexInputBindingStride (%" PRIu32")", buffer.stride, limits.maxVertexInputBindingStride);
		buffers.emplace_back(
			vk::VertexInputBindingDescription{}
			.setBinding(buffer_id)
			.setStride(buffer.stride)
			.setInputRate(vk::VertexInputRate::eVertex)
		);
		for (const auto& attribute : buffer.attributes)
		{
			ASSERT(attribute.offset <= limits.maxVertexInputAttributeOffset, "attribute.offset (%zu) exceeds limits.maxVertexInputAttributeOffset (%" PRIu32")", attribute.offset, limits.maxVertexInputAttributeOffset);
			attributes.emplace_back(
				vk::VertexInputAttributeDescription{}
				.setBinding(buffer_id)
				.setFormat(to_vk(attribute.type))
				.setOffset(attribute.offset)
				.setLocation(attribute.id)
			);
		}
		buffer_id++;
	}
	ASSERT(buffers.size() <= limits.maxVertexInputBindings, "vertexBindingDescriptionCount (%zu) exceeds limits.maxVertexInputBindings (%" PRIu32")", buffers.size(), limits.maxVertexInputBindings);
	ASSERT(attributes.size() <= limits.maxVertexInputAttributes, "vertexAttributeDescriptionCount (%zu) exceeds limits.maxVertexInputAttributes (%" PRIu32")", attributes.size(), limits.maxVertexInputAttributes);

	const auto vertex_desc = vk::PipelineVertexInputStateCreateInfo{}
		.setPVertexBindingDescriptions(buffers.data())
		.setVertexBindingDescriptionCount(buffers.size())
		.setPVertexAttributeDescriptions(attributes.data())
		.setVertexAttributeDescriptionCount(attributes.size());

	const auto color_blend_attachments = to_vk(state_desc.blend_state, state_desc.output_mask);
	const auto color_blend_state = vk::PipelineColorBlendStateCreateInfo{}
		.setAttachmentCount(color_blend_attachments.size())
		.setPAttachments(color_blend_attachments.data());

	const auto depthStencilState = to_vk(state_desc.depth_mode, state_desc.stencil);
	const auto rasterizationState = to_vk(state_desc.offset, state_desc.cull);
	vertexShader = get_module(spv_files.at(shader_mode).vertex, *pVkDynLoader);
	fragmentShader = get_module(spv_files.at(shader_mode).fragment, *pVkDynLoader);
	const auto pipelineStages = get_stages(vertexShader, fragmentShader);

	const auto pso = vk::GraphicsPipelineCreateInfo{}
		.setPColorBlendState(&color_blend_state)
		.setPDepthStencilState(&depthStencilState)
		.setPRasterizationState(&rasterizationState)
		.setPDynamicState(&dynamicS)
		.setPViewportState(&viewportState)
		.setLayout(layout)
		.setPStages(pipelineStages.data())
		.setStageCount(2)
		.setSubpass(0)
		.setPInputAssemblyState(&iassembly)
		.setPVertexInputState(&vertex_desc)
		.setPMultisampleState(&multisampleState)
		.setRenderPass(rp);

	object = dev.createGraphicsPipeline(vk::PipelineCache(), pso, nullptr, *pVkDynLoader); // may throw
}

VkPSO::~VkPSO()
{
	dev.destroyPipeline(object, nullptr, *pVkDynLoader);
	dev.destroyShaderModule(vertexShader, nullptr, *pVkDynLoader);
	dev.destroyShaderModule(fragmentShader, nullptr, *pVkDynLoader);
	dev.destroyPipelineLayout(layout, nullptr, *pVkDynLoader);
	dev.destroyDescriptorSetLayout(textures_set_layout, nullptr, *pVkDynLoader);
	for (auto & sampler : samplers)
	{
		dev.destroySampler(sampler, nullptr, *pVkDynLoader);
	}
	samplers.clear();
	dev.destroyDescriptorSetLayout(cbuffer_set_layout, nullptr, *pVkDynLoader);
}

VkPSO::VkPSO(vk::Pipeline&& p) : object(p) {}

// MARK: VkBuf

VkBuf::VkBuf(vk::Device _dev, const gfx_api::buffer::usage& usage, const VkRoot& root)
: dev(_dev), usage(usage), buffer_size(0), root(&root)
{
	// no-op
}

inline vk::BufferUsageFlags to_vk(const gfx_api::buffer::usage& usage)
{
	switch (usage)
	{
		case gfx_api::buffer::usage::vertex_buffer:
			return vk::BufferUsageFlagBits::eVertexBuffer;
		case gfx_api::buffer::usage::index_buffer:
			return vk::BufferUsageFlagBits::eIndexBuffer;
	}
	return vk::BufferUsageFlags(); // silence warning
}

void VkBuf::allocateBufferObject(const std::size_t& size)
{
	if (buffer_size == size)
	{
		return;
	}

	buffering_mechanism::get_current_resources().buffer_to_delete.emplace_back(std::move(object));
	buffering_mechanism::get_current_resources().vmamemory_to_free.push_back(allocation);

	auto bufferCreateInfo = vk::BufferCreateInfo{}
		.setSize(size)
		.setUsage(to_vk(usage) | vk::BufferUsageFlagBits::eTransferDst);

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vk::Result result = static_cast<vk::Result>(vmaCreateBuffer(root->allocator, reinterpret_cast<const VkBufferCreateInfo*>( &bufferCreateInfo ), &allocInfo, reinterpret_cast<VkBuffer*>( &object ), &allocation, nullptr));
	if (result != vk::Result::eSuccess)
	{
		// Failed to allocate memory!
		throwResultException( result, "vmaCreateBuffer" );
	}

	buffer_size = size;
}

VkBuf::~VkBuf()
{
	buffering_mechanism::get_current_resources().buffer_to_delete.emplace_back(std::move(object));
	buffering_mechanism::get_current_resources().vmamemory_to_free.push_back(allocation);
}

void VkBuf::upload(const size_t & size, const void * data)
{
	ASSERT(size > 0, "Attempt to upload buffer of size 0");
	allocateBufferObject(size);
	update(0, size, data);
}

void VkBuf::update(const size_t & start, const size_t & size, const void * data, const update_flag flag)
{
	size_t current_FrameNum = gfx_api::context::get().current_FrameNum();
	ASSERT(flag == update_flag::non_overlapping_updates_promise || (lastUploaded_FrameNum != current_FrameNum), "Attempt to upload to buffer more than once per frame");
	lastUploaded_FrameNum = current_FrameNum;

	ASSERT(start < buffer_size, "Starting offset (%zu) is past end of buffer (length: %zu)", start, buffer_size);
	ASSERT(start + size <= buffer_size, "Attempt to write past end of buffer");
	if (size == 0)
	{
		debug(LOG_WARNING, "Attempt to update buffer with 0 bytes of new data");
		return;
	}

	auto& frameResources = buffering_mechanism::get_current_resources();

	const auto stagingMemory = frameResources.stagingBufferAllocator.alloc(size, 2);
	const auto mappedMem = frameResources.stagingBufferAllocator.mapMemory(stagingMemory);
	ASSERT(mappedMem != nullptr, "Failed to map memory");
	memcpy(mappedMem, data, size);
	frameResources.stagingBufferAllocator.unmapMemory(stagingMemory);
	const auto& cmdBuffer = buffering_mechanism::get_current_resources().cmdCopy;
	const auto copyRegions = std::array<vk::BufferCopy, 1> { vk::BufferCopy(stagingMemory.offset, start, size) };
	cmdBuffer.copyBuffer(stagingMemory.buffer, object, copyRegions, root->vkDynLoader);
}

void VkBuf::bind() {}

// MARK: VkTexture

size_t VkTexture::format_size(const gfx_api::pixel_format& format)
{
	switch (format)
	{
		case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
			return 4;
		case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
			return 4;
		case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
			return 3;
		default:
			debug(LOG_FATAL, "Unrecognized pixel format");
	}
	return 0; // silence warning
}

size_t VkTexture::format_size(const vk::Format& format)
{
	switch (format)
	{
	case vk::Format::eR8G8B8Unorm: return 3 * sizeof(uint8_t);
	case vk::Format::eB8G8R8A8Unorm:
	case vk::Format::eR8G8B8A8Unorm: return 4 * sizeof(uint8_t);
	default:
			debug(LOG_FATAL, "Unhandled format: %d", (int)format);
	}
	throw;
}

VkTexture::VkTexture(const VkRoot& root, const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const vk::Format& _internal_format, const std::string& filename)
	: dev(root.dev), internal_format(_internal_format), mipmap_levels(mipmap_count), root(&root)
{
	ASSERT(width > 0 && height > 0, "0 width/height textures are unsupported");

	auto imageCreateInfo = vk::ImageCreateInfo{}
	.setArrayLayers(1)
	.setExtent(vk::Extent3D(width, height, 1))
	.setImageType(vk::ImageType::e2D)
	.setMipLevels(mipmap_count)
	.setTiling(vk::ImageTiling::eOptimal)
	.setFormat(internal_format)
	.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
	.setInitialLayout(vk::ImageLayout::eUndefined)
	.setSamples(vk::SampleCountFlagBits::e1);

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vk::Result result = static_cast<vk::Result>(vmaCreateImage(root.allocator, reinterpret_cast<const VkImageCreateInfo*>( &imageCreateInfo ), &allocInfo, reinterpret_cast<VkImage*>( &object ), &allocation, nullptr));
	if (result != vk::Result::eSuccess)
	{
		// Failed to allocate memory!
		throwResultException( result, "vmaCreateImage" );
	}

	view = dev.createImageViewUnique(
		vk::ImageViewCreateInfo{}
		.setImage(object)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(internal_format)
		.setComponents(vk::ComponentMapping())
		.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipmap_count, 0, 1))
		, nullptr, root.vkDynLoader
	);
}

VkTexture::~VkTexture()
{
	// All textures must be properly released before gfx_api::context::shutdown()
	auto& frameResources = buffering_mechanism::get_current_resources();
	frameResources.image_view_to_delete.emplace_back(std::move(view));
	frameResources.image_to_delete.emplace_back(std::move(object));
	frameResources.vmamemory_to_free.push_back(allocation);
}

void VkTexture::bind() {}

void VkTexture::upload(const std::size_t& mip_level, const std::size_t& offset_x, const std::size_t& offset_y, const std::size_t& width, const std::size_t& height, const gfx_api::pixel_format& buffer_format, const void* data)
{
	ASSERT(width > 0 && height > 0, "Attempt to upload texture with width or height of 0 (width: %zu, height: %zu)", width, height);
	size_t dynamicAlignment = std::max(0x4 * format_size(internal_format), static_cast<size_t>(root->physDeviceProps.limits.optimalBufferCopyOffsetAlignment));
	auto& frameResources = buffering_mechanism::get_current_resources();
	const size_t stagingBufferSize = width * height * format_size(internal_format);
	const auto stagingMemory = frameResources.stagingBufferAllocator.alloc(stagingBufferSize, dynamicAlignment);

	auto* mappedMem = reinterpret_cast<uint8_t*>(frameResources.stagingBufferAllocator.mapMemory(stagingMemory));
	ASSERT(mappedMem != nullptr, "Failed to map memory");
	auto* srcMem = reinterpret_cast<const uint8_t*>(data);

	if (format_size(buffer_format) == format_size(internal_format))
	{
		// fast-path
		memcpy(mappedMem, data, (width * height * format_size(buffer_format)));
	}
	else
	{
		for (unsigned row = 0; row < height; row++)
		{
			for (unsigned col = 0; col < width; col++)
			{
				unsigned byte = 0;
				for (; byte < format_size(buffer_format); byte++)
				{
					const auto& texel = srcMem[(row * width + col) * format_size(buffer_format) + byte];
					mappedMem[(row * width + col) * format_size(internal_format) + byte] = texel;
				}
				for (; byte < format_size(internal_format); byte++)
				{
					mappedMem[(row * width + col) * format_size(internal_format) + byte] = 255;
				}
			}
		}
	}

	frameResources.stagingBufferAllocator.unmapMemory(stagingMemory);

	const auto& cmdBuffer = buffering_mechanism::get_current_resources().cmdCopy;
	const auto imageMemoryBarriers_BeforeCopy = std::array<vk::ImageMemoryBarrier, 1> {
		vk::ImageMemoryBarrier{}
			.setImage(object)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, mip_level, 1, 0, 1))
			.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
			.setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
	};
	// TODO: Should this be eBottomOfPipe, eTopOfPipe, or something else? // FIXME
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlags{}, nullptr, nullptr, imageMemoryBarriers_BeforeCopy, root->vkDynLoader);
	const auto bufferImageCopyRegions = std::array<vk::BufferImageCopy, 1> {
		vk::BufferImageCopy{}
			.setBufferOffset(stagingMemory.offset)
			.setBufferImageHeight(height)
			.setBufferRowLength(width)
			.setImageOffset(vk::Offset3D(offset_x, offset_y, 0))
			.setImageSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mip_level, 0, 1))
			.setImageExtent(vk::Extent3D(width, height, 1))
	};
	cmdBuffer.copyBufferToImage(stagingMemory.buffer, object, vk::ImageLayout::eTransferDstOptimal, bufferImageCopyRegions, root->vkDynLoader);
	const auto imageMemoryBarriers_AfterCopy = std::array<vk::ImageMemoryBarrier, 1> {
		vk::ImageMemoryBarrier{}
			.setImage(object)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, mip_level, 1, 0, 1))
			.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
	};
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
		vk::DependencyFlags{}, nullptr, nullptr, imageMemoryBarriers_AfterCopy, root->vkDynLoader);
}

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wcast-align"
#endif
#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "3rdparty/stb_image_resize.h"

#if defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

void VkTexture::upload_and_generate_mipmaps(const size_t& offset_x, const size_t& offset_y, const size_t& width, const size_t& height, const gfx_api::pixel_format& buffer_format, const void* data)
{
	// upload initial (full) level
	upload(0, offset_x, offset_y, width, height, buffer_format, data);

	// generate and upload mipmaps
	const unsigned char * input_pixels = (const unsigned char*)data;
	void * prev_input_pixels_malloc = nullptr;
	size_t components = format_size(buffer_format);
	size_t input_w = width;
	size_t input_h = height;
	for (size_t i = 1; i < mipmap_levels; i++)
	{
		int output_w = std::max((size_t)1, input_w >> 1);
		int output_h = std::max((size_t)1, input_h >> 1);

		unsigned char *output_pixels = (unsigned char *)malloc(output_w * output_h * components);
		stbir_resize_uint8(input_pixels, input_w, input_h, 0,
						   output_pixels, output_w, output_h, 0,
						   components);
//		stbir_resize_uint8_generic(input_pixels, input_w, input_h, 0,
//								   output_pixels, output_w, output_h, 0,
//								   components, components == 4 ? 3 : STBIR_ALPHA_CHANNEL_NONE, STBIR_FLAG_ALPHA_PREMULTIPLIED,
//								   STBIR_EDGE_CLAMP,
//								   STBIR_FILTER_MITCHELL,
//								   STBIR_COLORSPACE_LINEAR,
//								   nullptr);

		upload(i, offset_x, offset_y, output_w, output_h, buffer_format, (const void*)output_pixels);

		if (prev_input_pixels_malloc)
		{
			free(prev_input_pixels_malloc);
		}
		input_pixels = output_pixels;
		prev_input_pixels_malloc = (void *)output_pixels;

		input_w = output_w;
		input_h = output_h;
	}
	if (prev_input_pixels_malloc)
	{
		free(prev_input_pixels_malloc);
	}
}

unsigned VkTexture::id() { return 0; }

// MARK: VkRoot

VkRoot::VkRoot(bool _debug) : debugLayer(_debug)
{
	debugInfo.setOutputHandler([&](const std::string& output) {
		addDumpInfo(output.c_str());
		if (enabled_debug[LOG_3D])
		{
			_debug_multiline(0, LOG_3D, "vk", output);
		}
	});
}

VkRoot::~VkRoot()
{
	// nothing, currently
}

gfx_api::pipeline_state_object * VkRoot::build_pipeline(const gfx_api::state_description &state_desc, const SHADER_MODE& shader_mode, const gfx_api::primitive_type& primitive,
	const std::vector<gfx_api::texture_input>& texture_desc,
	const std::vector<gfx_api::vertex_buffer>& attribute_descriptions)
{
	// build a pipeline, return an indirect VkPSOId (to enable rebuilding pipelines if needed)
	const gfxapi_PipelineCreateInfo createInfo(state_desc, shader_mode, primitive, texture_desc, attribute_descriptions);
	auto pipeline = new VkPSO(dev, physDeviceProps.limits, createInfo, rp, rp_compat_info, vk::SampleCountFlagBits::e1, vkDynLoader);
	createdPipelines.emplace_back(createInfo, pipeline);
	return new VkPSOId(createdPipelines.size() - 1);
}

void VkRoot::rebuildPipelinesIfNecessary()
{
	ASSERT(rp_compat_info, "Called before rendering pass is set up");
	// rebuild existing pipelines
	for (auto& pipeline : createdPipelines)
	{
		ASSERT(pipeline.second->renderpass_compat, "Pipeline has no associated renderpass compat structure");
		if (!rp_compat_info->isCompatibleWith(*pipeline.second->renderpass_compat))
		{
			delete pipeline.second;
			pipeline.second = new VkPSO(dev, physDeviceProps.limits, pipeline.first, rp, rp_compat_info, vk::SampleCountFlagBits::e1, vkDynLoader);
		}
	}
}

void VkRoot::createDefaultRenderpass(vk::Format swapchainFormat, vk::Format depthFormat)
{
	const auto attachments =
		std::array<vk::AttachmentDescription, 2>{
		vk::AttachmentDescription{} // colorAttachment
			.setFormat(swapchainFormat)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
//			.setStencilLoadOp(vk::AttachmentLoadOp::eClear) // ?
			.setStencilStoreOp(vk::AttachmentStoreOp::eStore),
		vk::AttachmentDescription{} // depthAttachment
			.setFormat(depthFormat)
			.setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	};
	const size_t numColorAttachmentRef = 1;
	const auto colorAttachmentRef =
		std::array<vk::AttachmentReference, numColorAttachmentRef>{
		vk::AttachmentReference{}
			.setAttachment(0)
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
	};
	static_assert(minRequired_ColorAttachments >= numColorAttachmentRef, "minRequired_ColorAttachments must be >= colorAttachmentRef.size()");
	const auto depthStencilAttachmentRef =
		vk::AttachmentReference{}
		.setAttachment(1)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	const auto subpasses =
		std::array<vk::SubpassDescription, 1> {
		vk::SubpassDescription{}
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachmentCount(colorAttachmentRef.size())
			.setPColorAttachments(colorAttachmentRef.data())
			.setPDepthStencilAttachment(&depthStencilAttachmentRef)
	};

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	auto createInfo = vk::RenderPassCreateInfo{}
		.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
		.setPAttachments(attachments.data())
		.setSubpassCount(static_cast<uint32_t>(subpasses.size()))
		.setPSubpasses(subpasses.data())
		.setDependencyCount(1)
		.setPDependencies((vk::SubpassDependency *)&dependency);

	rp_compat_info = std::make_shared<VkhRenderPassCompat>(createInfo);
	rp = dev.createRenderPass(createInfo, nullptr, vkDynLoader);
}

// Attempts to create a Vulkan instance (vk::Instance) with the specified extensions and layers
// If successful, sets the following variable in VkRoot:
//	- inst (vk::Instance)
bool VkRoot::createVulkanInstance(std::vector<const char*> extensions, std::vector<const char*> _layers, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	const auto appInfo = vk::ApplicationInfo()
	.setPApplicationName("Warzone2100")
	.setApplicationVersion(1)
	.setPEngineName("Warzone2100")
	.setEngineVersion(1)
	.setApiVersion(VK_API_VERSION_1_0);

	// Now we can make the Vulkan instance
	vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
	  .setPpEnabledLayerNames(_layers.data())
	  .setEnabledLayerCount(_layers.size())
	  .setPApplicationInfo(&appInfo)
	  .setPpEnabledExtensionNames(extensions.data())
	  .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size())
	);

	PFN_vkCreateInstance _vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(nullptr, "vkCreateInstance")));
	if (!_vkCreateInstance)
	{
		// Failed to find vkCreateInstance
		debug(LOG_ERROR, "Could not find symbol: vkCreateInstance\n");
		return false;
	}
	VkResult result = _vkCreateInstance(reinterpret_cast<const VkInstanceCreateInfo*>(&createInfo), nullptr, reinterpret_cast<VkInstance*>(&inst));
	if (result != VK_SUCCESS)
	{
		// vkCreateInstance failed
		debug(LOG_ERROR, "vkCreateInstance failed with error: %d", (int)result);
		return false;
	}

	// Setup debug layer
	if (!debugLayer)
		return true;

	// Verify that the requested extensions included VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	bool requested_debug_report_extension = std::find_if(extensions.begin(), extensions.end(),
				 [](const char *extensionName) { return (strcmp(extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0);}) != extensions.end();
	if (!requested_debug_report_extension)
	{
		debug(LOG_ERROR, "debugLayer is enabled, but did not request VK_EXT_debug_report - disabling debug report callbacks");
		return true;
	}

	CreateDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(static_cast<VkInstance>(inst), "vkCreateDebugReportCallbackEXT")));
	if (!CreateDebugReportCallback)
	{
		// Failed to obtain vkCreateDebugReportCallbackEXT
		debug(LOG_WARNING, "Could not find symbol: vkCreateDebugReportCallbackEXT\n");
		return true;
	}
	DestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(static_cast<VkInstance>(inst), "vkDestroyDebugReportCallbackEXT")));
	if (!DestroyDebugReportCallback)
	{
		// Failed to obtain vkDestroyDebugReportCallbackEXT
		debug(LOG_WARNING, "Could not find symbol: vkDestroyDebugReportCallbackEXT\n");
		return true;
	}
	dbgBreakCallback = reinterpret_cast<PFN_vkDebugReportMessageEXT>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(static_cast<VkInstance>(inst), "vkDebugReportMessageEXT")));
	if (!dbgBreakCallback)
	{
		// Failed to obtain vkDebugReportMessageEXT
		debug(LOG_WARNING, "Could not find symbol: vkDebugReportMessageEXT\n");
		return true;
	}

	VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
	dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	dbgCreateInfo.pfnCallback = messageCallback;
	dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
	VK_DEBUG_REPORT_WARNING_BIT_EXT; // |
	//VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

	const VkResult err = CreateDebugReportCallback(
												   static_cast<VkInstance>(inst),
												   &dbgCreateInfo,
												   nullptr,
												   &msgCallback);
	if (err != VK_SUCCESS)
	{
		debug(LOG_ERROR, "vkCreateDebugReportCallbackEXT failed with error: %d", (int)err);
		return false;
	}

	return true;
}

// WZ-specific functions for rating / determining requirements
int rateDeviceSuitability(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface, const vk::DispatchLoaderDynamic &vkDynLoader)
{
	const auto deviceProperties = device.getProperties(vkDynLoader);
	const auto deviceFeatures = device.getFeatures(vkDynLoader);

	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
	{
		score += 100000; // picked a number greater than the max seen maxImageDimension2D value: https://vulkan.gpuinfo.org/displaydevicelimit.php?name=maxImageDimension2D
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Requires: limits.maxDescriptorSetUniformBuffers >= minRequired_DescriptorSetUniformBuffers
	if (deviceProperties.limits.maxDescriptorSetUniformBuffers < minRequired_DescriptorSetUniformBuffers)
	{
		return 0;
	}

	// Requires: limits.maxDescriptorSetUniformBuffersDynamic >= minRequired_DescriptorSetUniformBuffersDynamic
	if (deviceProperties.limits.maxDescriptorSetUniformBuffersDynamic < minRequired_DescriptorSetUniformBuffersDynamic)
	{
		return 0;
	}

	// Requires: limits.maxBoundDescriptorSets >= minRequired_BoundDescriptorSets
	if (deviceProperties.limits.maxBoundDescriptorSets < minRequired_BoundDescriptorSets)
	{
		return 0;
	}

	// Requires: limits.maxViewports >= minRequired_Viewports
	if (deviceProperties.limits.maxViewports < minRequired_Viewports)
	{
		return 0;
	}

	// Requires: limits.maxColorAttachments >= minRequired_ColorAttachments
	if (deviceProperties.limits.maxColorAttachments < minRequired_ColorAttachments)
	{
		return 0;
	}

	// Requires: samplerAnisotropy
	if (!deviceFeatures.samplerAnisotropy)
	{
		return 0;
	}

	// depthBiasClamp - desired, but not required
	if (deviceFeatures.depthBiasClamp)
	{
		score += 10;
	}

	// Ensure device has required queue families
	QueueFamilyIndices indices = findQueueFamilies(device, surface, vkDynLoader);
	if (!indices.isComplete())
	{
		return 0;
	}
	if (indices.graphicsFamily.value() == indices.presentFamily.value())
	{
		// For performance reasons: prefer a physical device that supports drawing and presentation in the same queue
		score += 100;
	}

	// Check that device supports `deviceExtensions`
	if (!checkDeviceExtensionSupport(device, deviceExtensions, vkDynLoader))
	{
		return 0;
	}

	// Check that swapchain is suitable
	SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(device, surface, vkDynLoader);
	if (swapChainSupportDetails.formats.empty() || swapChainSupportDetails.presentModes.empty())
	{
		return 0;
	}

	return score;
}

// General function that uses WZ-specific functions
vk::PhysicalDevice VkRoot::pickPhysicalDevice()
{
	ASSERT(inst, "Instance is null");
	ASSERT(surface, "Surface is null");

	const auto& physicalDevices = inst.enumeratePhysicalDevices(vkDynLoader);
	if (physicalDevices.empty())
	{
		throw std::runtime_error("No physical devices with Vulkan support detected");
	}

	// Use an ordered map to automatically sort candidates by increasing score
	std::multimap<int, vk::PhysicalDevice> candidates;

	for (const auto& device : physicalDevices)
	{
		int score = rateDeviceSuitability(device, surface, vkDynLoader);
		candidates.insert(std::make_pair(score, device));
	}

	// Check if the best candidate is suitable at all
	if (candidates.rbegin()->first > 0)
	{
		return candidates.rbegin()->second;
	}
	else
	{
		throw std::runtime_error("Failed to find a suitable GPU!");
	}
}

void VkRoot::handleWindowSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	ASSERT(!swapchainImageView.empty(), "Swapchain does not appear to be set up yet?");
	int w, h;
	backend_impl->getDrawableSize(&w, &h);
	if (w != (int)swapchainSize.width || h != (int)swapchainSize.height)
	{
		// Must re-create swapchain
		debug(LOG_3D, "[2] Drawable size (%d x %d) does not match swapchainSize (%d x %d) - re-create swapchain", w, h, (int)swapchainSize.width, (int)swapchainSize.height);
		createNewSwapchainAndSwapchainSpecificStuff(vk::Result::eErrorOutOfDateKHR);
		return;
	}
}

void VkRoot::shutdown()
{
	destroySwapchainAndSwapchainSpecificStuff(true);

	for (auto& pipeline : createdPipelines)
	{
		delete pipeline.second;
	}
	createdPipelines.clear();

	// destroy allocator
	if (allocator != VK_NULL_HANDLE)
	{
		vmaDestroyAllocator(allocator);
	}

	// destroy logical device
	if (dev)
	{
		dev.destroy(nullptr, vkDynLoader);
		dev = vk::Device();
	}

	// destroy surface
	if (surface)
	{
		inst.destroySurfaceKHR(surface, nullptr, vkDynLoader);
		surface = vk::SurfaceKHR();
	}

	// destroy debug callback
	if(msgCallback)
	{
		DestroyDebugReportCallback(static_cast<VkInstance>(inst), msgCallback, nullptr);
	}

	// destroy instance
	inst.destroy(nullptr, vkDynLoader);
	inst = vk::Instance();
}

void VkRoot::destroySwapchainAndSwapchainSpecificStuff(bool doDestroySwapchain)
{
	if (graphicsQueue)
	{
		graphicsQueue.waitIdle(vkDynLoader);
	}
	if (presentQueue && presentQueue != graphicsQueue)
	{
		presentQueue.waitIdle(vkDynLoader);
	}
	dev.waitIdle(vkDynLoader);

	if (pDefaultTexture)
	{
		delete pDefaultTexture;
		pDefaultTexture = nullptr;
	}
	buffering_mechanism::destroy(dev, vkDynLoader);

	for (auto f : fbo)
	{
		dev.destroyFramebuffer(f, nullptr, vkDynLoader);
	}
	fbo.clear();

	dev.destroyImageView(depthStencilView, nullptr, vkDynLoader);
	depthStencilView = vk::ImageView();
	dev.freeMemory(depthStencilMemory, nullptr, vkDynLoader);
	depthStencilMemory = vk::DeviceMemory();
	dev.destroyImage(depthStencilImage, nullptr, vkDynLoader);
	depthStencilImage = vk::Image();

	if (rp)
	{
		dev.destroyRenderPass(rp, nullptr, vkDynLoader);
		rp = vk::RenderPass();
	}

	for (auto& imgview : swapchainImageView)
	{
		dev.destroyImageView(imgview, nullptr, vkDynLoader);
	}
	swapchainImageView.clear();

	if(doDestroySwapchain && swapchain)
	{
		dev.destroySwapchainKHR(swapchain, nullptr, vkDynLoader);
		swapchain = vk::SwapchainKHR();
	}
}

// recreate surface + swapchain
bool VkRoot::handleSurfaceLost()
{
	debug(LOG_3D, "handleSurfaceLost()");

	destroySwapchainAndSwapchainSpecificStuff(true);

	// destroy surface
	if (surface)
	{
		inst.destroySurfaceKHR(surface, nullptr, vkDynLoader);
		surface = vk::SurfaceKHR();
	}

	if (!createSurface())
	{
		debug(LOG_FATAL, "createSurface() failed");
		return false;
	}

	debugInfo.Output_SurfaceInformation(physicalDevice, surface, vkDynLoader);

	auto newSurfaceQueueFamilyIndices = findQueueFamilies(physicalDevice, surface, vkDynLoader);
	if (!newSurfaceQueueFamilyIndices.isComplete())
	{
		debug(LOG_FATAL, "Failed to get new graphics and presentation queue indices");
		return false;
	}
	if ((newSurfaceQueueFamilyIndices.graphicsFamily.value() != queueFamilyIndices.graphicsFamily.value())
		|| (newSurfaceQueueFamilyIndices.presentFamily.value() != queueFamilyIndices.presentFamily.value()))
	{
		// If the graphicsFamily or presentFamily queue indicies do not match what was previously used
		// to create the current logical device, we probably(?) can't reuse it - fail out for now
		debug(LOG_FATAL, "New graphics and presentation queue indices do not match those used to create the logical device");
		return false;
	}

	bool result = false;
	if (createSwapchain())
	{
		rebuildPipelinesIfNecessary();
		result = true;
	}
	else
	{
		debug(LOG_ERROR, "createSwapchain() failed");
		result = false;
	}

	return result;
}

bool VkRoot::createNewSwapchainAndSwapchainSpecificStuff(const vk::Result& reason)
{
	bool result = false;

	// ensure we don't end up in endless recursion because of failures
	if (errorHandlingDepth.size() > maxErrorHandlingDepth)
	{
		std::stringstream reasonsStr;
		for (const auto& _reason : errorHandlingDepth)
		{
			reasonsStr << to_string(_reason) << ";";
		}
		debug(LOG_FATAL, "createNewSwapchainAndSwapchainSpecificStuff failed with recursive depth: %zu; [%s]", errorHandlingDepth.size(), reasonsStr.str().c_str());
		return false;
	}
	errorHandlingDepth.push_back(reason);

	destroySwapchainAndSwapchainSpecificStuff(false);

	if (createSwapchain())
	{
		rebuildPipelinesIfNecessary();
		result = true;
	}
	else
	{
		debug(LOG_ERROR, "createSwapchain() failed");
		result = false;
	}

	errorHandlingDepth.pop_back();
	return result;
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
	const auto desiredFormats = std::array<vk::SurfaceFormatKHR, 2> {
		vk::SurfaceFormatKHR{ vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear },
		vk::SurfaceFormatKHR{ vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear }
	};

	if(availableFormats.size() == 1
	   && availableFormats[0].format == vk::Format::eUndefined)
	{
		// don't appear to be any preferred formats, so create one
		vk::SurfaceFormatKHR format;
		format.colorSpace = vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
		format.format = vk::Format::eB8G8R8A8Unorm;
		return format;
	}
	else
	{

		for (const auto& desiredFormat : desiredFormats)
		{
			for (const auto& availableFormat : availableFormats)
			{
				if(availableFormat.format == desiredFormat.format && availableFormat.colorSpace == desiredFormat.colorSpace)
				{
					return availableFormat;
				}
			}
		}


		for (const auto& desiredFormat : desiredFormats)
		{
			for (const auto& availableFormat : availableFormats)
			{
				if(availableFormat.format == desiredFormat.format)
				{
					debug(LOG_3D, "Desired format + colorSpace is not supported. Selecting desired format (%s) with colorSpace: %s", to_string(desiredFormat.format).c_str(), to_string(availableFormat.colorSpace).c_str());
					return availableFormat;
				}
			}
		}
	}

	debug(LOG_3D, "Desired format is not supported. Selecting format: %s - %s", to_string(availableFormats[0].format).c_str(), to_string(availableFormats[0].colorSpace).c_str());
	return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

	// For now, just support VK_PRESENT_MODE_FIFO_KHR
	if(std::find(availablePresentModes.begin(), availablePresentModes.end(), presentMode) == availablePresentModes.end())
	{
		// vk::PresentModeKHR::eFifo is unsupported??
		debug(LOG_FATAL, "vk::PresentModeKHR::eFifo does not appear to be supported?? Aborting.");
		abort();
	}

	return presentMode;
}

template <typename T>
T clamp(const T& n, const T& lower, const T& upper) {
	return std::max(lower, std::min(n, upper));
}

bool VkRoot::createSwapchain()
{
	ASSERT(backend_impl, "Backend implementation is null");
	ASSERT(physicalDevice, "Physical device is null");
	ASSERT(surface, "Surface is null");
	ASSERT(dev, "Logical device is null");
	ASSERT(allocator != VK_NULL_HANDLE, "Allocator is null");
	ASSERT(graphicsQueue, "Graphics queue not initialized");
	ASSERT(presentQueue, "Presentation queue not initialized");
	ASSERT(queueFamilyIndices.isComplete(), "Did not receive complete indices from findQueueFamilies");

	debug(LOG_3D, "createSwapchain()");

	currentSwapchainIndex = 0;
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface, vkDynLoader);

	vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);

	int w, h;
	backend_impl->getDrawableSize(&w, &h);
	ASSERT(w > 0 && h > 0, "getDrawableSize returned: %d x %d", w, h);
	vk::Extent2D drawableSize;
	drawableSize.width = static_cast<uint32_t>(w);
	drawableSize.height = static_cast<uint32_t>(h);
	// clamp drawableSize to VkSurfaceCapabilitiesKHR minImageExtent / maxImageExtent
	// see: https://bugzilla.libsdl.org/show_bug.cgi?id=4671
	swapchainSize.width = clamp(drawableSize.width, swapChainSupport.capabilities.minImageExtent.width, swapChainSupport.capabilities.maxImageExtent.width);
	swapchainSize.height = clamp(drawableSize.height, swapChainSupport.capabilities.minImageExtent.height, swapChainSupport.capabilities.maxImageExtent.height);
	ASSERT(swapchainSize.width > 0 && swapchainSize.height > 0, "swapchain dimensions: %" PRIu32" x %" PRIu32"", swapchainSize.width, swapchainSize.height);
	if (drawableSize != swapchainSize)
	{
		debug(LOG_3D, "Clamped drawableSize (%" PRIu32" x %" PRIu32") to minImageExtent (%" PRIu32" x %" PRIu32") & maxImageExtent (%" PRIu32" x %" PRIu32"): swapchainSize (%" PRIu32" x %" PRIu32")",
			  drawableSize.width, drawableSize.height,
			  swapChainSupport.capabilities.minImageExtent.width, swapChainSupport.capabilities.minImageExtent.height,
			  swapChainSupport.capabilities.maxImageExtent.width, swapChainSupport.capabilities.maxImageExtent.height,
			  swapchainSize.width, swapchainSize.height);
	}

	// pick swapchain image count
	uint32_t swapchainDesiredImageCount = swapChainSupport.capabilities.minImageCount + 1;
	if(swapchainDesiredImageCount > swapChainSupport.capabilities.maxImageCount
	   && swapChainSupport.capabilities.maxImageCount > 0)
	{
		swapchainDesiredImageCount = swapChainSupport.capabilities.maxImageCount;
	}

	// pick surface format
	surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);

	vk::SwapchainCreateInfoKHR createSwapchainInfo = vk::SwapchainCreateInfoKHR{}
		.setSurface(surface)
		.setMinImageCount(swapchainDesiredImageCount)
		.setPresentMode(presentMode)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setImageArrayLayers(1)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setClipped(true)
		.setImageExtent(swapchainSize)
		.setImageFormat(surfaceFormat.format)
		.setImageColorSpace(surfaceFormat.colorSpace)
		.setImageSharingMode(vk::SharingMode::eExclusive)
		.setOldSwapchain(swapchain);

	// Handle separate graphicsFamily and presentFamily indices (i.e. separate graphics and presentation queues)
	uint32_t queueFamilyIndicesU32[] = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};
	if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily)
	{
		createSwapchainInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
		createSwapchainInfo.setQueueFamilyIndexCount(2);
		createSwapchainInfo.setPQueueFamilyIndices(queueFamilyIndicesU32);
	}

	try {
		swapchain = dev.createSwapchainKHR(createSwapchainInfo, nullptr, vkDynLoader);
	}
	catch (vk::SystemError &e)
	{
		debug(LOG_ERROR, "vk::Device::createSwapchainKHR: failed with error: %s", e.what());
		return false;
	}

	if (createSwapchainInfo.oldSwapchain)
	{
		dev.destroySwapchainKHR(createSwapchainInfo.oldSwapchain, nullptr, vkDynLoader);
	}

	// TODO: Debug output swapchainDesiredImageCount vs actual count?

	// createSwapchainImageViews
	std::vector<vk::Image> swapchainImages = dev.getSwapchainImagesKHR(swapchain, vkDynLoader);
	std::transform(swapchainImages.begin(), swapchainImages.end(), std::back_inserter(swapchainImageView),
				   [&](const vk::Image& img) {
					   return dev.createImageView(
												  vk::ImageViewCreateInfo{}
												  .setImage(img)
												  .setFormat(surfaceFormat.format)
												  .setViewType(vk::ImageViewType::e2D)
												  .setComponents(vk::ComponentMapping())
												  .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
												  , nullptr, vkDynLoader);
				   });
	//

	buffering_mechanism::init(dev, allocator, MAX_FRAMES_IN_FLIGHT, queueFamilyIndices.graphicsFamily.value(), vkDynLoader);

	// createDepthStencilImage
	vk::Format depthFormat = findDepthFormat(physicalDevice, vkDynLoader);

	depthStencilImage = dev.createImage(
										vk::ImageCreateInfo{}
										.setFormat(depthFormat)
										.setArrayLayers(1)
										.setExtent(vk::Extent3D(swapchainSize.width, swapchainSize.height, 1))
										.setImageType(vk::ImageType::e2D)
										.setMipLevels(1)
										.setSamples(vk::SampleCountFlagBits::e1)
										.setTiling(vk::ImageTiling::eOptimal)
										.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
										, nullptr, vkDynLoader);

	const auto& memreq = dev.getImageMemoryRequirements(depthStencilImage, vkDynLoader);
	depthStencilMemory = dev.allocateMemory(
											vk::MemoryAllocateInfo{}
											.setAllocationSize(memreq.size)
											.setMemoryTypeIndex(findProperties(memprops, memreq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
											, nullptr, vkDynLoader);
	dev.bindImageMemory(depthStencilImage, depthStencilMemory, 0, vkDynLoader);

	depthStencilView = dev.createImageView(
										   vk::ImageViewCreateInfo{}
										   .setFormat(depthFormat)
										   .setImage(depthStencilImage)
										   .setViewType(vk::ImageViewType::e2D)
										   .setComponents(vk::ComponentMapping())
										   .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1))
										   , nullptr, vkDynLoader);
	//

	setupSwapchainImages();

	createDefaultRenderpass(surfaceFormat.format, depthFormat);

	// createFramebuffers()
	std::transform(swapchainImageView.begin(), swapchainImageView.end(), std::back_inserter(fbo),
				   [&](const vk::ImageView& imageView) {
					   const auto &attachments = std::array<vk::ImageView, 2>{imageView, depthStencilView};
					   return dev.createFramebuffer(
													vk::FramebufferCreateInfo{}
													.setAttachmentCount(attachments.size())
													.setPAttachments(attachments.data())
													.setLayers(1)
													.setWidth(swapchainSize.width)
													.setHeight(swapchainSize.height)
													.setRenderPass(rp)
													, nullptr, vkDynLoader);
				   });

	auto acquireNextResult = acquireNextSwapchainImage();
	switch (acquireNextResult)
	{
		case AcquireNextSwapchainImageResult::eSuccess:
			// continue on with processing
			break;
		case AcquireNextSwapchainImageResult::eRecoveredFromError:
			// acquireNextSwapchainImage recovered from an error - that means it succeeded at re-setting things up
			// return true immediately (this iteration is no longer responsible for creating the swapchain)
			return true;
			break;
		case AcquireNextSwapchainImageResult::eUnhandledFailure:
			// acquireNextSwapchainImage failed, and couldn't recover
			return false;
			break;
	}

	// create defaultTexture (2x2, all initialized to 0)
	const size_t defaultTexture_width = 2;
	const size_t defaultTexture_height = 2;
	pDefaultTexture = new VkTexture(*this, 1, defaultTexture_width, defaultTexture_height, get_format(gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8), "<default_texture>");

	const size_t defaultTexture_dataSize = defaultTexture_width * defaultTexture_height * VkTexture::format_size(gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8);
	std::vector<uint8_t> defaultTexture_zeroData(defaultTexture_dataSize, 0);
	pDefaultTexture->upload(0, 0, 0, defaultTexture_width, defaultTexture_height, gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8, defaultTexture_zeroData.data());

	startRenderPass();

	return true;
}

bool VkRoot::initialize(const gfx_api::backend_Impl_Factory& impl)
{
	debug(LOG_3D, "VkRoot::initialize()");

	frameNum = 1;

	// obtain backend_Vulkan_Impl from impl
	backend_impl = impl.createVulkanBackendImpl();
	if (!backend_impl)
	{
		debug(LOG_ERROR, "Failed to get Vulkan backend implementation");
		return false;
	}

	PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(backend_impl->getVkGetInstanceProcAddr());
	if(!_vkGetInstanceProcAddr)
	{
		debug(LOG_ERROR, "backend_impl->getVkGetInstanceProcAddr() failed\n");
		return false;
	}

	debugInfo.Output_InstanceExtensions(_vkGetInstanceProcAddr);

	std::vector<const char*> extensions;

	// Get required extensions (from backend implementation)
	if (!backend_impl->getRequiredInstanceExtensions(extensions))
	{
		debug(LOG_ERROR, "backend_impl->getRequiredInstanceExtensions failed");
		return false;
	}

	layers = std::vector<const char*>{};

// debugLayer handling
	if (debugLayer)
	{
		// determine if debug layers are available
		if (checkValidationLayerSupport(_vkGetInstanceProcAddr))
		{
			layers = validationLayers;
			debug(LOG_INFO, "Vulkan: Enabling validation layers (--gfxdebug) for development / bug-catching purposes (this will impact performance)");
		}
		else
		{
			debug(LOG_ERROR, "Vulkan: debug/validation layers requested, but not available - disabling");
			debugLayer = false;
		}

		if (debugLayer)
		{
			// determine if desired debug extensions are available
			std::vector<const char*> supportedDebugExtensions;
			if (!findSupportedInstanceExtensions(debugAdditionalExtensions, supportedDebugExtensions, _vkGetInstanceProcAddr))
			{
				debug(LOG_ERROR, "Failed to retrieve supported debug extensions");
				return false;
			}
			extensions.insert(std::end(extensions), supportedDebugExtensions.begin(), supportedDebugExtensions.end());
		}
	}
// end debugLayer handling

	// add other optional instance extensions
	std::vector<const char*> supportedOptionalInstanceExtensions;
	if (!findSupportedInstanceExtensions(optionalInstanceExtensions, supportedOptionalInstanceExtensions, _vkGetInstanceProcAddr))
	{
		debug(LOG_ERROR, "Failed to retrieve supported optional instance extensions");
		return false;
	}
	extensions.insert(std::end(extensions), supportedOptionalInstanceExtensions.begin(), supportedOptionalInstanceExtensions.end());

	if (!createVulkanInstance(extensions, layers, _vkGetInstanceProcAddr))
	{
		debug(LOG_ERROR, "createVulkanInstance failed");
		return false;
	}

	// Setup dynamic Vulkan loader
	vkDynLoader.init(static_cast<VkInstance>(inst), _vkGetInstanceProcAddr, static_cast<VkDevice>(vk::Device()), nullptr);

	// NOTE: From this point on, vkDynLoader *must* be initialized!
	ASSERT(vkDynLoader.vkGetInstanceProcAddr != nullptr, "vkDynLoader does not appear to be initialized");

	bool getProperties2Available = std::find_if(extensions.begin(), extensions.end(),
												[](const char *extensionName) { return (strcmp(extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0);}) != extensions.end();
	debugInfo.Output_PhysicalDevices(inst, getProperties2Available, vkDynLoader);

	if (!createSurface())
	{
		debug(LOG_ERROR, "createSurface() failed");
		return false;
	}

	// pick physical device (and get properties)
	physicalDevice = pickPhysicalDevice();
	physDeviceProps = physicalDevice.getProperties(vkDynLoader);
	physDeviceFeatures = physicalDevice.getFeatures(vkDynLoader);
	memprops = physicalDevice.getMemoryProperties(vkDynLoader);
	const auto& formatSupport = physicalDevice.getFormatProperties(vk::Format::eR8G8B8Unorm, vkDynLoader);
	supports_rgb = bool(formatSupport.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);

	debugInfo.Output_SurfaceInformation(physicalDevice, surface, vkDynLoader);

	getQueueFamiliesInfo();

	if (!createLogicalDevice())
	{
		debug(LOG_ERROR, "createLogicalDevice() failed");
		return false;
	}

	if (!createAllocator())
	{
		debug(LOG_ERROR, "createAllocator() failed");
		return false;
	}

	getQueues();

	if (!createSwapchain())
	{
		debug(LOG_ERROR, "createSwapchain() failed");
		return false;
	}

	return true;
}

bool VkRoot::createSurface()
{
	ASSERT(backend_impl, "Backend implementation is null");
	ASSERT(inst, "Instance is null");

	VkSurfaceKHR _surface{};
	if (!backend_impl->createWindowSurface(static_cast<VkInstance>(inst), &_surface))
	{
		debug(LOG_ERROR, "backend_impl->createWindowSurface() failed");
		return false;
	}
	surface = vk::SurfaceKHR{ _surface };
	return true;
}

void VkRoot::getQueueFamiliesInfo()
{
	ASSERT(physicalDevice, "Physical device is null");
	ASSERT(surface, "Surface is null");

	queueFamilyIndices = findQueueFamilies(physicalDevice, surface, vkDynLoader);
	ASSERT(queueFamilyIndices.isComplete(), "Did not receive complete indices from findQueueFamilies");

	// check for optional features of queue family
	const auto& queuesFamilies = physicalDevice.getQueueFamilyProperties(vkDynLoader);
	queueSupportsTimestamps = (queuesFamilies[queueFamilyIndices.graphicsFamily.value()].timestampValidBits > 0);
}

bool VkRoot::createLogicalDevice()
{
	ASSERT(physicalDevice, "Physical device is null");
	ASSERT(surface, "Surface is null");

	ASSERT(queueFamilyIndices.isComplete(), "Did not receive complete indices from findQueueFamilies");

	// determine extensions to use
	enabledDeviceExtensions = deviceExtensions;
	auto supportedOptionalExtensions = findSupportedDeviceExtensions(physicalDevice, optionalDeviceExtensions, vkDynLoader);
	enabledDeviceExtensions.insert(std::end(enabledDeviceExtensions), supportedOptionalExtensions.begin(), supportedOptionalExtensions.end());

	// create logical device
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};
	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		auto queueCreateInfo = vk::DeviceQueueCreateInfo{}
			.setQueueFamilyIndex(queueFamily)
			.setQueueCount(1)
			.setPQueuePriorities(&queuePriority);

		queueCreateInfos.push_back(queueCreateInfo);
	}

	ASSERT(physDeviceFeatures.samplerAnisotropy, "samplerAnisotropy is required, but not available");
	const auto enabledFeatures = vk::PhysicalDeviceFeatures{}
								.setSamplerAnisotropy(true)
								.setDepthBiasClamp(physDeviceFeatures.depthBiasClamp);

	dev = physicalDevice.createDevice(
		vk::DeviceCreateInfo{}
		.setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
		.setPpEnabledLayerNames(layers.data())
		.setEnabledExtensionCount(static_cast<uint32_t>(enabledDeviceExtensions.size()))
		.setPpEnabledExtensionNames(enabledDeviceExtensions.data())
		.setPEnabledFeatures(&enabledFeatures)
		.setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
		.setPQueueCreateInfos(queueCreateInfos.data())
		, nullptr, vkDynLoader
	);

	if(!dev)
	{
		debug(LOG_ERROR, "Empty device!");
		// TODO: Any cleanup here?
		return false;
	}

	// Now that the vk::Device is created, re-init the dynamic loader with the device
	// This uses vkGetDeviceProcAddr to potentially obtain device-specific function pointers
	// that bypass dynamic dispatch logic (for increased performance)
	// See: https://stackoverflow.com/a/35504844
	vkDynLoader.init(static_cast<VkInstance>(inst), vkDynLoader.vkGetInstanceProcAddr, static_cast<VkDevice>(dev), vkDynLoader.vkGetDeviceProcAddr);

	return true;
}

bool VkRoot::createAllocator()
{
	ASSERT(physicalDevice, "Physical device is null");
	ASSERT(dev, "Logical device is null");

	VmaVulkanFunctions vulkanFunctions;
	vulkanFunctions.vkGetPhysicalDeviceProperties = vkDynLoader.vkGetPhysicalDeviceProperties;
	vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkDynLoader.vkGetPhysicalDeviceMemoryProperties;
	vulkanFunctions.vkAllocateMemory = vkDynLoader.vkAllocateMemory;
	vulkanFunctions.vkFreeMemory = vkDynLoader.vkFreeMemory;
	vulkanFunctions.vkMapMemory = vkDynLoader.vkMapMemory;
	vulkanFunctions.vkUnmapMemory = vkDynLoader.vkUnmapMemory;
	vulkanFunctions.vkFlushMappedMemoryRanges = vkDynLoader.vkFlushMappedMemoryRanges;
	vulkanFunctions.vkInvalidateMappedMemoryRanges = vkDynLoader.vkInvalidateMappedMemoryRanges;
	vulkanFunctions.vkBindBufferMemory = vkDynLoader.vkBindBufferMemory;
	vulkanFunctions.vkBindImageMemory = vkDynLoader.vkBindImageMemory;
	vulkanFunctions.vkGetBufferMemoryRequirements = vkDynLoader.vkGetBufferMemoryRequirements;
	vulkanFunctions.vkGetImageMemoryRequirements = vkDynLoader.vkGetImageMemoryRequirements;
	vulkanFunctions.vkCreateBuffer = vkDynLoader.vkCreateBuffer;
	vulkanFunctions.vkDestroyBuffer = vkDynLoader.vkDestroyBuffer;
	vulkanFunctions.vkCreateImage = vkDynLoader.vkCreateImage;
	vulkanFunctions.vkDestroyImage = vkDynLoader.vkDestroyImage;
	vulkanFunctions.vkCmdCopyBuffer = vkDynLoader.vkCmdCopyBuffer;
#if VMA_DEDICATED_ALLOCATION
	vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkDynLoader.vkGetBufferMemoryRequirements2KHR;
	vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkDynLoader.vkGetImageMemoryRequirements2KHR;
#endif

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = dev;
	allocatorInfo.pVulkanFunctions = &vulkanFunctions;
	allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;

	bool enabled_VK_KHR_get_memory_requirements2 = std::find_if(enabledDeviceExtensions.begin(), enabledDeviceExtensions.end(),
														 [](const char *extensionName) { return (strcmp(extensionName, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) == 0);}) != enabledDeviceExtensions.end();
	bool enabled_VK_KHR_dedicated_allocation = std::find_if(enabledDeviceExtensions.begin(), enabledDeviceExtensions.end(),
																[](const char *extensionName) { return (strcmp(extensionName, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0);}) != enabledDeviceExtensions.end();
	if (enabled_VK_KHR_get_memory_requirements2 && enabled_VK_KHR_dedicated_allocation)
	{
		allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
	}

	VkResult result = vmaCreateAllocator(&allocatorInfo, &allocator);
	if (result != VK_SUCCESS)
	{
		debug(LOG_ERROR, "vmaCreateAllocator failed with error: %s", to_string(static_cast<vk::Result>(result)).c_str());
	}
	return result == VK_SUCCESS;
}

void VkRoot::getQueues()
{
	ASSERT(queueFamilyIndices.isComplete(), "Did not receive complete indices from findQueueFamilies");
	ASSERT(dev, "Logical device is null");
	graphicsQueue = dev.getQueue(queueFamilyIndices.graphicsFamily.value(), 0, vkDynLoader);
	presentQueue = dev.getQueue(queueFamilyIndices.presentFamily.value(), 0, vkDynLoader);
}

void VkRoot::draw(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type&)
{
	buffering_mechanism::get_current_resources().cmdDraw.draw(count, 1, offset, 0, vkDynLoader);
}

void VkRoot::draw_elements(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type&, const gfx_api::index_type&)
{
	buffering_mechanism::get_current_resources().cmdDraw.drawIndexed(count, 1, offset >> 2, 0, 0, vkDynLoader);
}

void VkRoot::bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset)
{
	using vertex_buffers_offset_type = const std::tuple<gfx_api::buffer*, std::size_t>;
	auto&& temp = std::vector<vk::Buffer>{};
	std::transform(vertex_buffers_offset.begin(), vertex_buffers_offset.end(), std::back_inserter(temp),
		[](vertex_buffers_offset_type& input) {
			ASSERT(static_cast<const VkBuf *>(std::get<0>(input))->usage == gfx_api::buffer::usage::vertex_buffer, "bind_vertex_buffers called with non-vertex-buffer");
			return static_cast<const VkBuf *>(std::get<0>(input))->object;
		});
	auto&& offsets = std::vector<VkDeviceSize>{};
	std::transform(vertex_buffers_offset.begin(), vertex_buffers_offset.end(), std::back_inserter(offsets),
		[](vertex_buffers_offset_type& input) { return std::get<1>(input); });
	buffering_mechanism::get_current_resources().cmdDraw.bindVertexBuffers(first, temp, offsets, vkDynLoader);
}

void VkRoot::unbind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset)
{
	// ?
}

void VkRoot::disable_all_vertex_buffers()
{
	// ?
}

void VkRoot::bind_streamed_vertex_buffers(const void* data, const std::size_t size)
{
	ASSERT(size > 0, "bind_streamed_vertex_buffers called with size 0");
	auto& frameResources = buffering_mechanism::get_current_resources();
	const auto streamedMemory = frameResources.streamedVertexBufferAllocator.alloc(size, 16);
	const auto mappedPtr = frameResources.streamedVertexBufferAllocator.mapMemory(streamedMemory);
	ASSERT(mappedPtr != nullptr, "Failed to map memory");
	memcpy(mappedPtr, data, size);
	frameResources.streamedVertexBufferAllocator.unmapMemory(streamedMemory);
	const auto buffers = std::array<vk::Buffer, 1> { streamedMemory.buffer };
	const auto offsets = std::array<vk::DeviceSize, 1> { streamedMemory.offset };
	buffering_mechanism::get_current_resources().cmdDraw.bindVertexBuffers(0, buffers, offsets, vkDynLoader);
}

void VkRoot::setupSwapchainImages()
{
	const auto& buffers = dev.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo{}
		.setCommandPool(buffering_mechanism::perFrameResources[0]->pool)
		.setCommandBufferCount(1)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		, vkDynLoader
	);
	vk::CommandBuffer internalCommandBuffer = buffers[0];
	internalCommandBuffer.begin(
		vk::CommandBufferBeginInfo{}
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		, vkDynLoader
	);
	const auto imageMemoryBarriers_DepthStencilImage = std::array<vk::ImageMemoryBarrier, 1> {
		vk::ImageMemoryBarrier{}
			.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setImage(depthStencilImage)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1))
			.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
	};
	internalCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
		vk::DependencyFlagBits{}, nullptr, nullptr, imageMemoryBarriers_DepthStencilImage, vkDynLoader);

	internalCommandBuffer.end(vkDynLoader);
	graphicsQueue.submit(
		vk::SubmitInfo{}
		.setCommandBufferCount(1)
		.setPCommandBuffers(&internalCommandBuffer),
		vk::Fence()
		, vkDynLoader);
	graphicsQueue.waitIdle(vkDynLoader);
}

vk::Format VkRoot::get_format(const gfx_api::pixel_format& format)
{
	switch (format)
	{
	case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
		return supports_rgb ? vk::Format::eR8G8B8Unorm : vk::Format::eR8G8B8A8Unorm;
	case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
		return vk::Format::eR8G8B8A8Unorm;
	case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
		return vk::Format::eB8G8R8A8Unorm;
	default:
		debug(LOG_FATAL, "Unsupported format: %d", (int)format);
	}
	throw;
}

gfx_api::texture* VkRoot::create_texture(const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const gfx_api::pixel_format& internal_format, const std::string& filename)
{
	auto result = new VkTexture(*this, mipmap_count, width, height, get_format(internal_format), filename);
	return result;
}

gfx_api::buffer* VkRoot::create_buffer_object(const gfx_api::buffer::usage &usage, const buffer_storage_hint& hint /*= buffer_storage_hint::static_draw*/)
{
	return new VkBuf(dev, usage, *this);
}

std::vector<vk::DescriptorSet> VkRoot::allocateDescriptorSets(vk::DescriptorSetLayout arg)
{
	const auto descriptorSet = std::array<vk::DescriptorSetLayout, 1>{ arg };
	buffering_mechanism::get_current_resources().numalloc++;
	return dev.allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo{}
			.setDescriptorPool(buffering_mechanism::get_current_resources().descriptorPool)
			.setPSetLayouts(descriptorSet.data())
			.setDescriptorSetCount(descriptorSet.size())
	, vkDynLoader);
}

vk::IndexType VkRoot::to_vk(const gfx_api::index_type& index)
{
	switch (index)
	{
	case gfx_api::index_type::u16:
		return vk::IndexType::eUint16;
	case gfx_api::index_type::u32:
		return vk::IndexType::eUint32;
	}
	debug(LOG_FATAL, "Unsupported index_type");
	return vk::IndexType::eNoneNV;
}

void VkRoot::bind_index_buffer(gfx_api::buffer& index_buffer, const gfx_api::index_type& index)
{
	auto& casted_buf = static_cast<VkBuf&>(index_buffer);
	ASSERT(casted_buf.usage == gfx_api::buffer::usage::index_buffer, "Passed gfx_api::buffer is not an index buffer");
	buffering_mechanism::get_current_resources().cmdDraw.bindIndexBuffer(casted_buf.object, 0, to_vk(index), vkDynLoader);
}

void VkRoot::unbind_index_buffer(gfx_api::buffer&)
{
	// ?
}

void VkRoot::bind_textures(const std::vector<gfx_api::texture_input>& attribute_descriptions, const std::vector<gfx_api::texture*>& textures)
{
	ASSERT(textures.size() <= attribute_descriptions.size(), "Received more textures than expected");

	const auto set = allocateDescriptorSets(currentPSO->textures_set_layout);

	auto image_descriptor = std::vector<vk::DescriptorImageInfo>{};
	for (auto* texture : textures)
	{
		image_descriptor.emplace_back(vk::DescriptorImageInfo{}
			.setImageView(texture != nullptr ? *static_cast<VkTexture*>(texture)->view : *pDefaultTexture->view)
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
	}
	std::size_t i = 0;
	auto write_info = std::vector<vk::WriteDescriptorSet>{};
	for (auto* texture : textures)
	{
		(void)texture; // silence unused variable warning
		write_info.emplace_back(
			vk::WriteDescriptorSet{}
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDstSet(set[0])
				.setPImageInfo(&image_descriptor[i])
				.setDstBinding(i)
		);
		i++;
	}
	dev.updateDescriptorSets(write_info, nullptr, vkDynLoader);
	buffering_mechanism::get_current_resources().cmdDraw.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, currentPSO->layout, 1, set, nullptr, vkDynLoader);
}

void VkRoot::set_constants(const void* buffer, const std::size_t& size)
{
	const auto stagingMemory = buffering_mechanism::get_current_resources().uniformBufferAllocator.alloc(size, physDeviceProps.limits.minUniformBufferOffsetAlignment);
	void * pDynamicUniformBufferMapped = buffering_mechanism::get_current_resources().uniformBufferAllocator.mapMemory(stagingMemory);
	memcpy(reinterpret_cast<uint8_t*>(pDynamicUniformBufferMapped), buffer, size);

	const auto bufferInfo = vk::DescriptorBufferInfo(stagingMemory.buffer, 0, size);

	std::vector<vk::DescriptorSet> sets;
	auto perFrame_perPSO_dynamicUniformDescriptorSets = buffering_mechanism::get_current_resources().perPSO_dynamicUniformBufferDescriptorSets.find(currentPSO);
	if (perFrame_perPSO_dynamicUniformDescriptorSets != buffering_mechanism::get_current_resources().perPSO_dynamicUniformBufferDescriptorSets.end())
	{
		auto perFrame_perPSO_dynamicUniformDescriptorSet = perFrame_perPSO_dynamicUniformDescriptorSets->second.find(bufferInfo);
		if (perFrame_perPSO_dynamicUniformDescriptorSet != perFrame_perPSO_dynamicUniformDescriptorSets->second.end())
		{
			sets.push_back(perFrame_perPSO_dynamicUniformDescriptorSet->second);
		}
	}

	if (sets.empty())
	{
		sets = allocateDescriptorSets(currentPSO->cbuffer_set_layout);
		const auto descriptorWrite = std::array<vk::WriteDescriptorSet, 1>{
			vk::WriteDescriptorSet{}
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
				.setDstBinding(0)
				.setPBufferInfo(&bufferInfo)
				.setDstSet(sets[0])
		};
		dev.updateDescriptorSets(descriptorWrite, nullptr, vkDynLoader);
		buffering_mechanism::get_current_resources().perPSO_dynamicUniformBufferDescriptorSets[currentPSO] = perFrameResources_t::DynamicUniformBufferDescriptorSets({{ bufferInfo, sets[0] }});
	}
	const auto dynamicOffsets = std::array<uint32_t, 1> { stagingMemory.offset };
	buffering_mechanism::get_current_resources().cmdDraw.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, currentPSO->layout, 0, sets, dynamicOffsets, vkDynLoader);
}

void VkRoot::bind_pipeline(gfx_api::pipeline_state_object* pso, bool /*notextures*/)
{
	VkPSOId* newPSOId = static_cast<VkPSOId*>(pso);
	// lookup PSO
	VkPSO* newPSO = createdPipelines[newPSOId->psoID].second;
	if (currentPSO != newPSO)
	{
		currentPSO = newPSO;
		buffering_mechanism::get_current_resources().cmdDraw.bindPipeline(vk::PipelineBindPoint::eGraphics, currentPSO->object, vkDynLoader);
	}
}

VkRoot::AcquireNextSwapchainImageResult VkRoot::acquireNextSwapchainImage()
{
	vk::ResultValue<uint32_t> acquireNextImageResult = vk::ResultValue<uint32_t>(vk::Result::eNotReady, 0);
	try {
		acquireNextImageResult = dev.acquireNextImageKHR(swapchain, -1, buffering_mechanism::get_current_resources().imageAcquireSemaphore, vk::Fence{}, vkDynLoader);
	}
	catch (vk::OutOfDateKHRError&)
	{
		debug(LOG_3D, "vk::Device::acquireNextImageKHR: ErrorOutOfDateKHR - must recreate swapchain");
		if (createNewSwapchainAndSwapchainSpecificStuff(vk::Result::eErrorOutOfDateKHR))
		{
			return AcquireNextSwapchainImageResult::eRecoveredFromError;
		}
		return AcquireNextSwapchainImageResult::eUnhandledFailure;
	}
	catch (vk::SurfaceLostKHRError&)
	{
		debug(LOG_3D, "vk::Device::acquireNextImageKHR: ErrorSurfaceLostKHR - must recreate surface + swapchain");
		// recreate surface + swapchain
		if (handleSurfaceLost())
		{
			return AcquireNextSwapchainImageResult::eRecoveredFromError;
		}
		return AcquireNextSwapchainImageResult::eUnhandledFailure;
	}
	catch (vk::SystemError& e)
	{
		debug(LOG_ERROR, "vk::Device::acquireNextImageKHR: unhandled error: %s", e.what());
		return AcquireNextSwapchainImageResult::eUnhandledFailure;
	}
	if(acquireNextImageResult.result == vk::Result::eSuboptimalKHR)
	{
		debug(LOG_3D, "vk::Device::acquireNextImageKHR returned eSuboptimalKHR - recreate swapchain");
		if (createNewSwapchainAndSwapchainSpecificStuff(acquireNextImageResult.result))
		{
			return AcquireNextSwapchainImageResult::eRecoveredFromError;
		}
		return AcquireNextSwapchainImageResult::eUnhandledFailure;
	}

	currentSwapchainIndex = acquireNextImageResult.value;
	return AcquireNextSwapchainImageResult::eSuccess;
}

void VkRoot::flip(int clearMode)
{
	frameNum = std::max<size_t>(frameNum + 1, 1);

	currentPSO = nullptr;
	buffering_mechanism::get_current_resources().cmdDraw.endRenderPass(vkDynLoader);
	buffering_mechanism::get_current_resources().cmdDraw.end(vkDynLoader);

	// Add memory barrier at end of cmdCopy
	const auto memoryBarriers = std::array<vk::MemoryBarrier, 1> {
		vk::MemoryBarrier{}
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eUniformRead )
	};

	buffering_mechanism::get_current_resources().cmdCopy.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
																		 vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eVertexShader,
																		 vk::DependencyFlagBits{}, memoryBarriers, nullptr, nullptr, vkDynLoader);

	buffering_mechanism::get_current_resources().cmdCopy.end(vkDynLoader);

	buffering_mechanism::get_current_resources().uniformBufferAllocator.flushAutomappedMemory();
	buffering_mechanism::get_current_resources().uniformBufferAllocator.unmapAutomappedMemory();
	buffering_mechanism::get_current_resources().streamedVertexBufferAllocator.flushAutomappedMemory();
	buffering_mechanism::get_current_resources().streamedVertexBufferAllocator.unmapAutomappedMemory();

	const auto executableCmdBuffer = std::array<vk::CommandBuffer, 2>{buffering_mechanism::get_current_resources().cmdCopy, buffering_mechanism::get_current_resources().cmdDraw}; // copy before render
	const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput; //vk::PipelineStageFlagBits::eAllCommands;

	auto submitInfo = vk::SubmitInfo{}
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&buffering_mechanism::get_current_resources().imageAcquireSemaphore)
		.setPWaitDstStageMask(&waitStage)
		.setCommandBufferCount(executableCmdBuffer.size())
		.setPCommandBuffers(executableCmdBuffer.data());

	auto presentInfo = vk::PresentInfoKHR{}
		.setPSwapchains(&swapchain)
		.setSwapchainCount(1)
		.setPImageIndices(&currentSwapchainIndex);

	if (graphicsQueue != presentQueue)
	{
		// for handling separate graphics and presentation queues
		submitInfo
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(&buffering_mechanism::get_current_resources().renderFinishedSemaphore);

		// for handling separate graphics and presentation queues
		presentInfo
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&buffering_mechanism::get_current_resources().renderFinishedSemaphore);
	}

	graphicsQueue.submit(submitInfo, buffering_mechanism::get_current_resources().previousSubmission, vkDynLoader);

	vk::Result presentResult;
	try {
		presentResult = presentQueue.presentKHR(presentInfo, vkDynLoader);
	}
	catch (vk::OutOfDateKHRError&)
	{
		debug(LOG_3D, "vk::Queue::presentKHR: ErrorOutOfDateKHR - must recreate swapchain");
		createNewSwapchainAndSwapchainSpecificStuff(vk::Result::eErrorOutOfDateKHR);
		return; // end processing this flip
	}
	catch (vk::SurfaceLostKHRError&)
	{
		debug(LOG_3D, "vk::Queue::presentKHR: ErrorSurfaceLostKHR - must recreate surface + swapchain");
		// recreate surface + swapchain
		handleSurfaceLost();
		return; // end processing this flip
	}
	catch (vk::SystemError& e)
	{
		debug(LOG_ERROR, "vk::Queue::presentKHR: unhandled error: %s", e.what());
	}
	if(presentResult == vk::Result::eSuboptimalKHR)
	{
		debug(LOG_3D, "presentKHR returned eSuboptimalKHR (%d) - recreate swapchain", (int)presentResult);
		createNewSwapchainAndSwapchainSpecificStuff(presentResult);
		return; // end processing this flip
	}

	buffering_mechanism::swap(dev, vkDynLoader); // must be called *before* acquireNextSwapchainImage()
	if (acquireNextSwapchainImage() != AcquireNextSwapchainImageResult::eSuccess)
	{
		return; // end processing this flip
	}

	int w, h;
	backend_impl->getDrawableSize(&w, &h);
	if (w != (int)swapchainSize.width || h != (int)swapchainSize.height)
	{
		// Must re-create swapchain
		debug(LOG_3D, "Drawable size (%d x %d) does not match swapchainSize (%d x %d) - re-create swapchain", w, h, (int)swapchainSize.width, (int)swapchainSize.height);
		createNewSwapchainAndSwapchainSpecificStuff(vk::Result::eErrorOutOfDateKHR);
		return; // end processing this flip
	}
	startRenderPass();
	buffering_mechanism::get_current_resources().cmdCopy.begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit), vkDynLoader);
}

void VkRoot::startRenderPass()
{
	buffering_mechanism::get_current_resources().cmdDraw.begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit), vkDynLoader);

	const auto clearValue = std::array<vk::ClearValue, 2> {
		vk::ClearValue(), vk::ClearValue(vk::ClearDepthStencilValue(1.f, 0u))
	};
	buffering_mechanism::get_current_resources().cmdDraw.beginRenderPass(
		vk::RenderPassBeginInfo{}
		.setFramebuffer(fbo[currentSwapchainIndex])
		.setClearValueCount(clearValue.size())
		.setPClearValues(clearValue.data())
		.setRenderPass(rp)
		.setRenderArea(vk::Rect2D(vk::Offset2D(), swapchainSize)),
		vk::SubpassContents::eInline,
		vkDynLoader);
	const auto viewports = std::array<vk::Viewport, 1> {
		vk::Viewport{}.setHeight(swapchainSize.height).setWidth(swapchainSize.width).setMinDepth(0.f).setMaxDepth(1.f)
	};
	buffering_mechanism::get_current_resources().cmdDraw.setViewport(0, viewports, vkDynLoader);
	const auto scissors = std::array<vk::Rect2D, 1> {
		vk::Rect2D{}.setExtent(swapchainSize)
	};
	buffering_mechanism::get_current_resources().cmdDraw.setScissor(0, scissors, vkDynLoader);
}

void VkRoot::set_polygon_offset(const float& offset, const float& slope)
{
	buffering_mechanism::get_current_resources().cmdDraw.setDepthBias(offset, (physDeviceFeatures.depthBiasClamp) ? 1.0f : 0.f, slope, vkDynLoader);
}

void VkRoot::set_depth_range(const float& min, const float& max)
{
	const auto viewports = std::array<vk::Viewport, 1> {
		vk::Viewport{}.setHeight(swapchainSize.height).setWidth(swapchainSize.width).setMinDepth(min).setMaxDepth(max)
	};
	buffering_mechanism::get_current_resources().cmdDraw.setViewport(0, viewports, vkDynLoader);
}

int32_t VkRoot::get_context_value(const context_value property)
{
	switch(property)
	{
		case MAX_ELEMENTS_VERTICES:
			return 32000;
		case MAX_ELEMENTS_INDICES:
			return 32000;
		case MAX_TEXTURE_SIZE:
			return physDeviceProps.limits.maxImageDimension2D;
		case MAX_SAMPLES:
			// TODO: support MSAA?: https://vulkan-tutorial.com/Multisampling
			// return getMaxUsableSampleCount(physDeviceProps);
			return 0;
	}
	debug(LOG_FATAL, "Unsupported property");
	return 0;
}

// DEBUG-handling

void VkRoot::debugStringMarker(const char *str)
{
	// TODO: Implement
}

void VkRoot::debugSceneBegin(const char *descr)
{
	// TODO: Implement
}

void VkRoot::debugSceneEnd(const char *descr)
{
	// TODO: Implement
}

bool VkRoot::debugPerfAvailable()
{
	// TODO: Implement
	return false;
}

struct perfQueryDetails
{
	uint32_t start_copy = 0;
	uint32_t start_draw = 0;

	uint32_t end_copy = 0;
	uint32_t end_draw = 0;
};

bool perfStarted = false;

bool VkRoot::debugPerfStart(size_t sample)
{
	// TODO: Implement
	return false;
}

void VkRoot::debugPerfStop()
{
	// TODO: Implement
}

void VkRoot::debugPerfBegin(PERF_POINT pp, const char *descr)
{
	if (!perfStarted) return;
	// TODO: Implement
}

void VkRoot::debugPerfEnd(PERF_POINT pp)
{
	if (!perfStarted) return;
	// TODO: Implement
}

uint64_t VkRoot::debugGetPerfValue(PERF_POINT pp)
{
	// TODO: Implement
	return 0;
}

std::map<std::string, std::string> VkRoot::getBackendGameInfo()
{
	std::map<std::string, std::string> backendGameInfo;
	backendGameInfo["vulkan_vendor"] = std::to_string(physDeviceProps.vendorID);
	backendGameInfo["vulkan_deviceID"] = std::to_string(physDeviceProps.deviceID);
	backendGameInfo["vulkan_deviceType"] = to_string(physDeviceProps.deviceType);
	backendGameInfo["vulkan_deviceName"] = physDeviceProps.deviceName;
	backendGameInfo["vulkan_apiversion"] = VkhInfo::vulkan_apiversion_to_string(physDeviceProps.apiVersion);
	backendGameInfo["vulkan_driverversion"] = std::to_string(physDeviceProps.driverVersion);
	return backendGameInfo;
}

bool VkRoot::getScreenshot(iV_Image &output)
{
	// TODO: Implement
	return false;
}

const size_t& VkRoot::current_FrameNum() const
{
	return frameNum;
}

#endif // defined(WZ_VULKAN_ENABLED)
