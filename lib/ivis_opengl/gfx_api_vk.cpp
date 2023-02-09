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
#include "lib/framework/wzapp.h"
#include "lib/exceptionhandler/dumpinfo.h"

#include <algorithm>
#include <set>
#include <unordered_set>
#include <map>
#include <limits>
#include <chrono>
#include <thread>

// Fix #define MemoryBarrier coming from winnt.h
#undef MemoryBarrier

#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 9
#pragma GCC diagnostic ignored "-Wdeprecated-copy" // Ignore warnings caused by vulkan.hpp 148
#endif

const uint32_t minSupportedVulkanVersion = VK_API_VERSION_1_0;
#if defined(DEBUG)
// For debug builds, limit to the minimum that should be supported by this backend (which is Vulkan 1.0, see above)
const uint32_t maxRequestableInstanceVulkanVersion = VK_API_VERSION_1_0;
#else
// For regular builds, currently limit to: Vulkan 1.1
const uint32_t maxRequestableInstanceVulkanVersion = (uint32_t)VK_MAKE_VERSION(1, 1, 0);
#endif

const size_t MAX_FRAMES_IN_FLIGHT = 2;

// Vulkan version where extension is promoted to core; extension name
#define VK_NOT_PROMOTED_TO_CORE_YET 0
const std::vector<std::tuple<uint32_t, const char*>> optionalInstanceExtensions = {
	{ VK_MAKE_VERSION(1, 1, 0) , VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME },	// used for Vulkan info output
#if defined(VK_KHR_portability_enumeration)
	{ VK_NOT_PROMOTED_TO_CORE_YET , VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME}
#endif
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<const char*> optionalDeviceExtensions = {
	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
	VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
	"VK_KHR_portability_subset" // According to VUID-VkDeviceCreateInfo-pProperties-04451, if device supports this extension it *must* be enabled
};

const std::vector<vk::Format> supportedDepthFormats = {
	vk::Format::eD32SfloatS8Uint,
	vk::Format::eD24UnormS8Uint
};

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> debugAdditionalExtensions = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME // old, deprecated extension
};

// See: https://www.khronos.org/registry/vulkan/specs/1.1/html/vkspec.html#fundamentals-validusage-versions
#define VK_VERSION_GREATER_THAN_OR_EQUAL(a, b) \
	( ( VK_VERSION_MAJOR(a) > VK_VERSION_MAJOR(b) ) || ( ( VK_VERSION_MAJOR(a) == VK_VERSION_MAJOR(b) ) && ( VK_VERSION_MINOR(a) >= VK_VERSION_MINOR(b) ) ) )

const uint32_t minRequired_DescriptorSetUniformBuffers = 1;
const uint32_t minRequired_DescriptorSetUniformBuffersDynamic = 1;
const uint32_t minRequired_BoundDescriptorSets = 4;
const uint32_t minRequired_Viewports = 1;
const uint32_t minRequired_ColorAttachments = 1;

#if defined(WZ_OS_WIN)
#  define _vkl_env_text(x) L##x
#  define _vkl_env_text_type std::wstring
void _vk_setenv(const _vkl_env_text_type& name, const _vkl_env_text_type& value)
{
	if (SetEnvironmentVariableW(name.c_str(), value.c_str()) == 0)
	{
		// Failed to set environment variable
		DWORD lastError = GetLastError();
		debug(LOG_ERROR, "SetEnvironmentVariableW failed with error: %lu", lastError);
	}
}
#else
#  define _vkl_env_text(x) x
#  define _vkl_env_text_type std::string
void _vk_setenv(const _vkl_env_text_type& name, const _vkl_env_text_type& value)
{
#  if defined HAVE_SETENV
	setenv(name.c_str(), value.c_str(), 1);
#  else
#    warning "No supported method to set environment variables"
#  endif
}
#endif

const std::vector<std::pair<_vkl_env_text_type, _vkl_env_text_type>> vulkan_implicit_layer_environment_variables = {
	{_vkl_env_text("DISABLE_VK_LAYER_VALVE_steam_overlay_1"), _vkl_env_text("1")}
	, {_vkl_env_text("DISABLE_VK_LAYER_VALVE_steam_fossilize_1"), _vkl_env_text("1")}
	, {_vkl_env_text("DISABLE_FPSMON_LAYER"), _vkl_env_text("1")} // avoid crashes caused by this layer
};

#if defined(WZ_DEBUG_GFX_API_LEAKS)
static std::unordered_set<const VkTexture*> debugLiveTextures;
#endif


// MARK: General helper functions

void SetVKImplicitLayerEnvironmentVariables()
{
	for (const auto &it : vulkan_implicit_layer_environment_variables)
	{
		_vk_setenv(it.first, it.second);
	}
}

template <typename VKType, typename F, typename... Args>
std::vector<VKType> GetVectorFromVKFuncWithExplicitInit(F &&func, VKType init, Args &&... args)
{
	std::vector<VKType> results;
	uint32_t count = 0;
	VkResult status;
	do {
		status = func(args..., &count, nullptr);
		if ((status == VK_SUCCESS) && (count > 0))
		{
			results.resize(count, init);
			status = func(args..., &count, results.data());
		}
	} while (status == VK_INCOMPLETE);
	if (status == VK_SUCCESS)
	{
		ASSERT(count <= results.size(), "Unexpected result: count (%" PRIu32"); results.size (%zu)", count, results.size());
		results.resize(count);
	}
	return results;
}

template <typename VKType, typename F, typename... Args>
std::vector<VKType> GetVectorFromVKFunc(F &&func, Args &&... args)
{
	return GetVectorFromVKFuncWithExplicitInit(func, VKType(), args...);
}

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

[[noreturn]] static void handleUnrecoverableError(const vk::Result& reason)
{
	if (reason == vk::Result::eErrorDeviceLost)
	{
		// FUTURE TODO: Output a bunch more debugging info to the debug log?
	}
	// Display a message and prompt the user to try a different graphics backend next time
	wzPromptToChangeGfxBackendOnFailure("Failed with error: " + vk::to_string(reason));
	abort();
}

bool checkFormatSupport(const vk::PhysicalDevice& physicalDevice, vk::Format format, vk::ImageTiling tiling, vk::FormatFeatureFlags features, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	vk::FormatProperties props;
	physicalDevice.getFormatProperties(format, &props, vkDynLoader);

	if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
	{
		return true;
	}
	else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
	{
		return true;
	}
	return false;
}

vk::Format findSupportedFormat(const vk::PhysicalDevice& physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, const vk::DispatchLoaderDynamic& vkDynLoader) {
	for (vk::Format format : candidates)
	{
		if (checkFormatSupport(physicalDevice, format, tiling, features, vkDynLoader))
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface, const vk::DispatchLoaderDynamic &vkDynLoader)
{
	QueueFamilyIndices indices;

	const auto queueFamilies = device.getQueueFamilyProperties(vkDynLoader);

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
	try {
		const auto availableExtensions = device.enumerateDeviceExtensionProperties(nullptr, vkDynLoader);

		std::unordered_set<std::string> requiredExtensions(desiredExtensions.begin(), desiredExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}
	catch (const vk::SystemError& e)
	{
		debug(LOG_ERROR, "vkEnumerateDeviceExtensionProperties failed with error: %s", e.what());
	}
	return false;
}

bool VkRoot::getSupportedInstanceExtensions(std::vector<VkExtensionProperties> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	if (!supportedInstanceExtensionProperties.empty())
	{
		// use cached info
		output = supportedInstanceExtensionProperties;
		return true;
	}

	ASSERT_OR_RETURN(false, _vkGetInstanceProcAddr, "_vkGetInstanceProcAddr must be valid");
	PFN_vkEnumerateInstanceExtensionProperties _vkEnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties")));
	if (!_vkEnumerateInstanceExtensionProperties)
	{
		debug(LOG_ERROR, "Could not find symbol: vkEnumerateInstanceExtensionProperties\n");
		return false;
	}

	// cache the result in VkRoot instance, to minimize calls to vkEnumerateInstanceExtensionProperties
	supportedInstanceExtensionProperties = GetVectorFromVKFunc<VkExtensionProperties>(_vkEnumerateInstanceExtensionProperties, nullptr);

	output = supportedInstanceExtensionProperties;
	return true;
}

static bool getInstanceLayerProperties(std::vector<VkLayerProperties> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	ASSERT_OR_RETURN(false, _vkGetInstanceProcAddr, "_vkGetInstanceProcAddr must be valid");
	PFN_vkEnumerateInstanceLayerProperties _vkEnumerateInstanceLayerProperties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties")));
	if (!_vkEnumerateInstanceLayerProperties)
	{
		debug(LOG_ERROR, "Could not find symbol: vkEnumerateInstanceLayerProperties\n");
		return false;
	}

	output = GetVectorFromVKFunc<VkLayerProperties>(_vkEnumerateInstanceLayerProperties);
	return true;
}

vk::SampleCountFlagBits getMaxUsableSampleCount(const vk::PhysicalDeviceProperties &physicalDeviceProperties)
{
	VkSampleCountFlags counts = std::min({
		static_cast<VkSampleCountFlags>(physicalDeviceProperties.limits.framebufferColorSampleCounts),
		static_cast<VkSampleCountFlags>(physicalDeviceProperties.limits.framebufferDepthSampleCounts),
		static_cast<VkSampleCountFlags>(physicalDeviceProperties.limits.framebufferStencilSampleCounts)
	});
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return vk::SampleCountFlagBits::e64; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return vk::SampleCountFlagBits::e32; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return vk::SampleCountFlagBits::e16; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return vk::SampleCountFlagBits::e8; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return vk::SampleCountFlagBits::e4; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return vk::SampleCountFlagBits::e2; }

	return vk::SampleCountFlagBits::e1;
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

bool VkRoot::findSupportedInstanceExtensions(std::vector<const char*> extensionsToFind, std::vector<const char*> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
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
	auto bufferCreateInfo = vk::BufferCreateInfo()
		.setSize(newBlockSize)
		.setUsage(usageFlags);

	totalCapacity += newBlockSize;

	Block newBlock;
	newBlock.size = newBlockSize;
	vk::Result result = static_cast<vk::Result>(vmaCreateBuffer(allocator, reinterpret_cast<const VkBufferCreateInfo*>( &bufferCreateInfo ), &allocInfo, reinterpret_cast<VkBuffer*>( &newBlock.buffer ), &newBlock.allocation, nullptr));
	if (result != vk::Result::eSuccess)
	{
		// Failed to allocate memory!
		vk::throwResultException( result, "vmaCreateBuffer" );
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

constexpr uint32_t descriptorPoolMaxSetsDefault = 10000;
constexpr uint32_t descriptorPoolSizeDescriptorCountDefault = 10000;

perFrameResources_t::perFrameResources_t(vk::Device& _dev, const VmaAllocator& allocator, const uint32_t& graphicsQueueFamilyIndex, const vk::DispatchLoaderDynamic& vkDynLoader)
	: dev(_dev)
	, allocator(allocator)
	, stagingBufferAllocator(allocator, 1024 * 1024, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY)
	, streamedVertexBufferAllocator(allocator, 128 * 1024, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, true)
	, uniformBufferAllocator(allocator, 1024 * 1024, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, true)
	, pVkDynLoader(&vkDynLoader)
{
	combinedImageSamplerDescriptorPools.push_back(createNewDescriptorPool(vk::DescriptorType::eCombinedImageSampler, descriptorPoolMaxSetsDefault, descriptorPoolSizeDescriptorCountDefault));
	uniformDynamicDescriptorPools.push_back(createNewDescriptorPool(vk::DescriptorType::eUniformBufferDynamic, descriptorPoolMaxSetsDefault, descriptorPoolSizeDescriptorCountDefault));

	pool = dev.createCommandPool(
		vk::CommandPoolCreateInfo()
		.setQueueFamilyIndex(graphicsQueueFamilyIndex)
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		, nullptr, *pVkDynLoader
	);
	const auto buffer = dev.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo()
		.setCommandPool(pool)
		.setCommandBufferCount(2)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		, *pVkDynLoader
	);
	cmdDraw = buffer[0];
	cmdCopy = buffer[1];
	cmdCopy.begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit), vkDynLoader);
	previousSubmission = dev.createFence(
		vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled),
		nullptr, *pVkDynLoader
	);
}

perFrameResources_t::DescriptorPoolDetails perFrameResources_t::createNewDescriptorPool(vk::DescriptorType type, uint32_t maxSets, uint32_t descriptorCount)
{
	vk::DescriptorPoolSize poolSize(type, descriptorCount);

	return DescriptorPoolDetails(dev.createDescriptorPool(vk::DescriptorPoolCreateInfo()
			.setMaxSets(maxSets)
			.setPPoolSizes(&poolSize)
			.setPoolSizeCount(1)
			, nullptr, *pVkDynLoader
		), poolSize, maxSets);
}

vk::DescriptorPool perFrameResources_t::getDescriptorPool(uint32_t numSets, vk::DescriptorType descriptorType, uint32_t numDescriptors)
{
	// Take into account numSets, descriptorType and numDescriptors to return a descriptorPool (allocating a new one if needed)
	DescriptorPoolsContainer* pPools = nullptr;
	switch (descriptorType)
	{
		case vk::DescriptorType::eCombinedImageSampler:
			pPools = &combinedImageSamplerDescriptorPools;
			break;
		case vk::DescriptorType::eUniformBufferDynamic:
			pPools = &uniformDynamicDescriptorPools;
			break;
		default:
			debug(LOG_FATAL, "Invalid descriptor type: %s", vk::to_string(descriptorType).c_str());
			return vk::DescriptorPool();
	}

	DescriptorPoolDetails* pCurrPool = &pPools->current();
	if ((pCurrPool->maxSets - pCurrPool->requestedSets < numSets)
		|| (pCurrPool->size.descriptorCount - pCurrPool->requestedDescriptors < numDescriptors))
	{
		// not enough room in the current descriptor pool for this request
		if (!pPools->nextPool())
		{
			// No more existing pools - need to create a new one
			debug(LOG_INFO, "[%p] Creating new [%zu] descriptor pool of type: %s", (void*)this, pPools->currPool + 1, vk::to_string(descriptorType).c_str());
			pPools->push_back(createNewDescriptorPool(descriptorType, descriptorPoolMaxSetsDefault, descriptorPoolSizeDescriptorCountDefault));
			pPools->nextPool();
		}
		pCurrPool = &pPools->current();
	}

	pCurrPool->requestedSets += numSets;
	pCurrPool->requestedDescriptors += numDescriptors;

	return pCurrPool->poolHandle;
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
	for (const auto& descriptorPoolDetails : combinedImageSamplerDescriptorPools.pools)
	{
		dev.destroyDescriptorPool(descriptorPoolDetails.poolHandle, nullptr, *pVkDynLoader);
	}
	for (const auto& descriptorPoolDetails : uniformDynamicDescriptorPools.pools)
	{
		dev.destroyDescriptorPool(descriptorPoolDetails.poolHandle, nullptr, *pVkDynLoader);
	}
	dev.destroyFence(previousSubmission, nullptr, *pVkDynLoader);
	clean();
}

void perFrameResources_t::DescriptorPoolsContainer::reset(vk::Device dev, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	for (auto& descriptorPool : pools)
	{
		dev.resetDescriptorPool(descriptorPool.poolHandle, vk::DescriptorPoolResetFlags(), vkDynLoader);
		descriptorPool.requestedSets = 0;
		descriptorPool.requestedDescriptors = 0;
	}
	currPool = 0;
}

void perFrameResources_t::resetDescriptorPools()
{
	combinedImageSamplerDescriptorPools.reset(dev, *pVkDynLoader);
	uniformDynamicDescriptorPools.reset(dev, *pVkDynLoader);
}

perFrameResources_t& buffering_mechanism::get_current_resources()
{
	ASSERT(!perFrameResources.empty(), "perFrameResources are not initialized??");
	return *perFrameResources[currentFrame];
}

perSwapchainImageResources_t& buffering_mechanism::get_current_swapchain_resources()
{
	ASSERT(!perFrameResources.empty(), "perSwapchainImageResources are not initialized??");
	return *perSwapchainImageResources[currentSwapchainImageResourcesFrame];
}

bool buffering_mechanism::isInitialized()
{
	return !perFrameResources.empty();
}

perSwapchainImageResources_t::perSwapchainImageResources_t(vk::Device& _dev, const vk::DispatchLoaderDynamic& vkDynLoader)
	: dev(_dev)
	, pVkDynLoader(&vkDynLoader)
{
	imageAcquireSemaphore = dev.createSemaphore(vk::SemaphoreCreateInfo(), nullptr, *pVkDynLoader);
	renderFinishedSemaphore = dev.createSemaphore(vk::SemaphoreCreateInfo(), nullptr, *pVkDynLoader);
}

perSwapchainImageResources_t::~perSwapchainImageResources_t()
{
	dev.destroySemaphore(imageAcquireSemaphore, nullptr, *pVkDynLoader);
	dev.destroySemaphore(renderFinishedSemaphore, nullptr, *pVkDynLoader);
}

// MARK: buffering_mechanism

void buffering_mechanism::init(vk::Device dev, const VmaAllocator& allocator, size_t swapChainImageCount, const uint32_t& graphicsQueueFamilyIndex, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	currentFrame = 0;
	currentSwapchainImageResourcesFrame = 0;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		perFrameResources.emplace_back(new perFrameResources_t(dev, allocator, graphicsQueueFamilyIndex, vkDynLoader));
		perSwapchainImageResources.emplace_back(new perSwapchainImageResources_t(dev, vkDynLoader));
	}

	const auto fences = std::array<vk::Fence, 1> { buffering_mechanism::get_current_resources().previousSubmission };
	dev.resetFences(fences, vkDynLoader);
}

void buffering_mechanism::destroy(vk::Device dev, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	perFrameResources.clear();
	perSwapchainImageResources.clear();
	currentFrame = 0;
	currentSwapchainImageResourcesFrame = 0;
}

void buffering_mechanism::swap(vk::Device dev, const vk::DispatchLoaderDynamic& vkDynLoader, bool skipAcquireNewSwapchainImage)
{
	currentFrame = (currentFrame < (perFrameResources.size() - 1)) ? currentFrame + 1 : 0;
	if (!skipAcquireNewSwapchainImage)
	{
		currentSwapchainImageResourcesFrame = (currentSwapchainImageResourcesFrame < (perSwapchainImageResources.size() - 1)) ? currentSwapchainImageResourcesFrame + 1 : 0;
	}

	const auto fences = std::array<vk::Fence, 1> { buffering_mechanism::get_current_resources().previousSubmission };
	dev.waitForFences(fences, true, -1, vkDynLoader);
	dev.resetFences(fences, vkDynLoader);
	buffering_mechanism::get_current_resources().resetDescriptorPools();
	dev.resetCommandPool(buffering_mechanism::get_current_resources().pool, vk::CommandPoolResetFlagBits(), vkDynLoader);

	buffering_mechanism::get_current_resources().clean();
	buffering_mechanism::get_current_resources().numalloc = 0;
}

// MARK: Definitions of statics

std::vector<std::unique_ptr<perFrameResources_t>> buffering_mechanism::perFrameResources;
std::vector<std::unique_ptr<perSwapchainImageResources_t>> buffering_mechanism::perSwapchainImageResources;
size_t buffering_mechanism::currentFrame;
size_t buffering_mechanism::currentSwapchainImageResourcesFrame;

// MARK: Debug Callback

VKAPI_ATTR VkBool32 VKAPI_CALL WZDebugReportCallback(
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

VKAPI_ATTR VkBool32 VKAPI_CALL WZDebugUtilsCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
	void*                                            pUserData)
{
	std::stringstream buf;
	code_part part = LOG_3D;
	bool logFatal = false;
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		buf << "ERROR: ";
		part = LOG_ERROR;
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		buf << "WARNING: ";
		part = LOG_INFO; // make sure these are logged for now (since neither LOG_3D nor LOG_WARN are enabled by default)
	}

	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
		buf << "[PERF] ";
	}
	else if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
		buf << "[VALIDATION] ";
		logFatal = true;
	}

	if (pCallbackData)
	{
		if (pCallbackData->pMessageIdName != nullptr)
		{
			buf << "[" << pCallbackData->pMessageIdName << "] ";
		}
		buf << "Code " << pCallbackData->messageIdNumber << " : ";
		if (pCallbackData->pMessage != nullptr)
		{
			buf << pCallbackData->pMessage;
		}
		else
		{
			buf << "< NULL MESSAGE? >";
		}

		if (pCallbackData->objectCount > 0)
		{
			buf << std::endl;
			for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
			{
				const auto& objectInfo = pCallbackData->pObjects[i];
				buf << "\t - [" << i << "]: " << vk::to_string(static_cast<vk::ObjectType>(objectInfo.objectType)) << " 0x" << std::hex << objectInfo.objectHandle << std::dec;
				if (objectInfo.pObjectName)
				{
					buf << " : \"" << objectInfo.pObjectName << "\"";
				}
				buf << std::endl;
			}
		}
	}
	else
	{
		buf << "< no callback data? >";
	}

	debug((logFatal) ? LOG_FATAL : part, "%s", buf.str().c_str());

	return VK_FALSE;
}

// MARK: VkPSO

struct shader_infos
{
	std::string vertexSpv;
	std::string fragmentSpv;
	bool specializationConstant_0_mipLoadBias = false;
};

static const std::map<SHADER_MODE, shader_infos> spv_files
{
	std::make_pair(SHADER_COMPONENT, shader_infos{ "shaders/vk/tcmask.vert.spv", "shaders/vk/tcmask.frag.spv", true }),
	std::make_pair(SHADER_COMPONENT_INSTANCED, shader_infos{ "shaders/vk/tcmask_instanced.vert.spv", "shaders/vk/tcmask_instanced.frag.spv", true }),
	std::make_pair(SHADER_NOLIGHT, shader_infos{ "shaders/vk/nolight.vert.spv", "shaders/vk/nolight.frag.spv", true }),
	std::make_pair(SHADER_NOLIGHT_INSTANCED, shader_infos{ "shaders/vk/nolight_instanced.vert.spv", "shaders/vk/nolight_instanced.frag.spv", true }),
	std::make_pair(SHADER_TERRAIN, shader_infos{ "shaders/vk/terrain.vert.spv", "shaders/vk/terrain.frag.spv", true }),
	std::make_pair(SHADER_TERRAIN_DEPTH, shader_infos{ "shaders/vk/terrain_depth.vert.spv", "shaders/vk/terraindepth.frag.spv" }),
	std::make_pair(SHADER_DECALS, shader_infos{ "shaders/vk/decals.vert.spv", "shaders/vk/decals.frag.spv", true }),
	std::make_pair(SHADER_WATER, shader_infos{ "shaders/vk/terrain_water.vert.spv", "shaders/vk/water.frag.spv" }),
	std::make_pair(SHADER_RECT, shader_infos{ "shaders/vk/rect.vert.spv", "shaders/vk/rect.frag.spv" }),
	std::make_pair(SHADER_RECT_INSTANCED, shader_infos{ "shaders/vk/rect_instanced.vert.spv", "shaders/vk/rect_instanced.frag.spv" }),
	std::make_pair(SHADER_TEXRECT, shader_infos{ "shaders/vk/rect.vert.spv", "shaders/vk/texturedrect.frag.spv" }),
	std::make_pair(SHADER_GFX_COLOUR, shader_infos{ "shaders/vk/gfx_color.vert.spv", "shaders/vk/gfx.frag.spv" }),
	std::make_pair(SHADER_GFX_TEXT, shader_infos{ "shaders/vk/gfx_text.vert.spv", "shaders/vk/texturedrect.frag.spv" }),
	std::make_pair(SHADER_SKYBOX, shader_infos{ "shaders/vk/skybox.vert.spv", "shaders/vk/skybox.frag.spv" }),
	std::make_pair(SHADER_GENERIC_COLOR, shader_infos{ "shaders/vk/generic.vert.spv", "shaders/vk/rect.frag.spv" }),
	std::make_pair(SHADER_LINE, shader_infos{ "shaders/vk/line.vert.spv", "shaders/vk/rect.frag.spv" }),
	std::make_pair(SHADER_TEXT, shader_infos{ "shaders/vk/rect.vert.spv", "shaders/vk/text.frag.spv" })
};

std::vector<uint32_t> VkPSO::readShaderBuf(const std::string& name)
{
	auto fp = PHYSFS_openRead(name.c_str());
	debug(LOG_3D, "Reading...[directory: %s] %s", PHYSFS_getRealDir(name.c_str()), name.c_str());
	ASSERT_OR_RETURN({}, fp != nullptr, "Could not open %s", name.c_str());

	const auto filesize = PHYSFS_fileLength(fp);
	ASSERT_OR_RETURN(std::vector<uint32_t>(), filesize < static_cast<PHYSFS_sint64>(std::numeric_limits<PHYSFS_sint32>::max()), "\"%s\" filesize >= std::numeric_limits<PHYSFS_sint32>::max()", name.c_str());
	ASSERT_OR_RETURN(std::vector<uint32_t>(), static_cast<PHYSFS_uint64>(filesize) < static_cast<PHYSFS_uint64>(std::numeric_limits<size_t>::max()), "\"%s\" filesize >= std::numeric_limits<size_t>::max()", name.c_str());

	auto buffer = std::vector<uint32_t>(static_cast<size_t>(filesize / sizeof(uint32_t)));
	WZ_PHYSFS_readBytes(fp, buffer.data(), static_cast<PHYSFS_uint32>(filesize));
	PHYSFS_close(fp);

	return buffer;
}

vk::ShaderModule VkPSO::get_module(const std::string& name, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	const auto tmp = readShaderBuf(name);
	ASSERT_OR_RETURN(vk::ShaderModule(), tmp.size() > 0, "Failed to read shader: %s", name.c_str());
	return dev.createShaderModule(
		vk::ShaderModuleCreateInfo()
		.setCodeSize(tmp.size() * 4)
		.setPCode(tmp.data())
		, nullptr, vkDynLoader
	);
}

std::array<vk::PipelineShaderStageCreateInfo, 2> VkPSO::get_stages(const vk::ShaderModule& vertexModule, const vk::ShaderModule& fragmentModule)
{
	return std::array<vk::PipelineShaderStageCreateInfo, 2> {
		vk::PipelineShaderStageCreateInfo()
			.setModule(vertexModule)
			.setPName("main")
			.setStage(vk::ShaderStageFlagBits::eVertex),
			vk::PipelineShaderStageCreateInfo()
			.setModule(fragmentModule)
			.setPName("main")
			.setStage(vk::ShaderStageFlagBits::eFragment)
	};
}

std::array<vk::PipelineColorBlendAttachmentState, 1> VkPSO::to_vk(const REND_MODE& blend_state, const uint8_t& color_mask)
{
	const auto full_color_output = vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;
	const auto vk_color_mask = color_mask == 0 ? vk::ColorComponentFlags() : full_color_output;

	switch (blend_state)
	{
	case REND_ADDITIVE:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState()
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
			vk::PipelineColorBlendAttachmentState()
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
			vk::PipelineColorBlendAttachmentState()
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
			vk::PipelineColorBlendAttachmentState()
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
			vk::PipelineColorBlendAttachmentState()
				.setBlendEnable(false)
				.setColorWriteMask(vk_color_mask)
		};
	}
	case REND_TEXT:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState()
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
	return std::array<vk::PipelineColorBlendAttachmentState, 1>{vk::PipelineColorBlendAttachmentState()};
}

vk::PipelineDepthStencilStateCreateInfo VkPSO::to_vk(DEPTH_MODE depth_mode, const gfx_api::stencil_mode& stencil)
{
	auto state = vk::PipelineDepthStencilStateCreateInfo();
	switch (stencil)
	{
	case gfx_api::stencil_mode::stencil_disabled:
		state.setStencilTestEnable(false);
		break;
	case gfx_api::stencil_mode::stencil_shadow_quad:
	{
		state.setStencilTestEnable(true);
		const auto stencil_mode = vk::StencilOpState()
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
		state.setFront(vk::StencilOpState()
			.setDepthFailOp(vk::StencilOp::eKeep)
			.setFailOp(vk::StencilOp::eKeep)
			.setPassOp(vk::StencilOp::eIncrementAndWrap)
			.setCompareOp(vk::CompareOp::eAlways)
			.setReference(0)
			.setWriteMask(~0)
			.setCompareMask(~0));
		state.setBack(vk::StencilOpState()
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
	auto result = vk::PipelineRasterizationStateCreateInfo()
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
		return vk::SamplerCreateInfo()
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
		return vk::SamplerCreateInfo()
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
	{
		vk::SamplerCreateInfo result = vk::SamplerCreateInfo()
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setMaxAnisotropy(std::min<float>(16.f, root->physDeviceProps.limits.maxSamplerAnisotropy))
			.setAnisotropyEnable(true)
			.setMinLod(0.f)
			.setMaxLod(VK_LOD_CLAMP_NONE)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
		if (root->lodBiasMethod == VkRoot::LodBiasMethod::SamplerMipLodBias)
		{
			result.setMipLodBias(root->mipLodBias.value_or(0.f));
		}
		return result;
	}
	case gfx_api::sampler_type::anisotropic_repeat:
	{
		vk::SamplerCreateInfo result = vk::SamplerCreateInfo()
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setMaxAnisotropy(std::min<float>(16.f, root->physDeviceProps.limits.maxSamplerAnisotropy))
			.setAnisotropyEnable(true)
			.setMinLod(0.f)
			.setMaxLod(VK_LOD_CLAMP_NONE)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat);
		if (root->lodBiasMethod == VkRoot::LodBiasMethod::SamplerMipLodBias)
		{
			result.setMipLodBias(root->mipLodBias.value_or(0.f));
		}
		return result;
	}
	case gfx_api::sampler_type::nearest_clamped:
		return vk::SamplerCreateInfo()
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
	return vk::SamplerCreateInfo();
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
	return vk::PrimitiveTopology();
}

VkPSO::VkPSO(vk::Device _dev,
	const vk::PhysicalDeviceLimits& limits,
	const gfxapi_PipelineCreateInfo& createInfo,
	vk::RenderPass rp,
	const std::shared_ptr<VkhRenderPassCompat>& renderpass_compat,
	vk::SampleCountFlagBits rasterizationSamples,
	const vk::DispatchLoaderDynamic& _vkDynLoader,
	const VkRoot& _root
	) : dev(_dev), pVkDynLoader(&_vkDynLoader), renderpass_compat(renderpass_compat), root(&_root)
{
	const gfx_api::state_description& state_desc = createInfo.state_desc;
	const SHADER_MODE& shader_mode = createInfo.shader_mode;
	const gfx_api::primitive_type& primitive = createInfo.primitive;
	const std::vector<std::type_index>& uniform_blocks = createInfo.uniform_blocks;
	const std::vector<gfx_api::texture_input>& texture_desc = createInfo.texture_desc;
	const std::vector<gfx_api::vertex_buffer>& attribute_descriptions = createInfo.attribute_descriptions;

	auto layout_desc = std::vector<vk::DescriptorSetLayout>();

	for (size_t i = 0; i < uniform_blocks.size(); i++)
	{
		const auto cbuffer_layout_desc = std::array<vk::DescriptorSetLayoutBinding, 1>{
			vk::DescriptorSetLayoutBinding()
				.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
				.setStageFlags(vk::ShaderStageFlagBits::eAllGraphics)
		};
		auto set_layout = dev.createDescriptorSetLayout(
			vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(1)
				.setPBindings(cbuffer_layout_desc.data())
			, nullptr, *pVkDynLoader);
		cbuffer_set_layout.push_back(set_layout);
		layout_desc.push_back(set_layout);
	}

	auto textures_layout_desc = std::vector<vk::DescriptorSetLayoutBinding>();
	samplers.reserve(texture_desc.size());
	for (const auto& texture : texture_desc)
	{
		samplers.emplace_back(dev.createSampler(to_vk(texture.sampler), nullptr, *pVkDynLoader));

		textures_layout_desc.emplace_back(
			vk::DescriptorSetLayoutBinding()
				.setBinding(static_cast<uint32_t>(texture.id))
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setPImmutableSamplers(&samplers.back())
				.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		);
	}
	textures_set_layout = dev.createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(static_cast<uint32_t>(textures_layout_desc.size()))
			.setPBindings(textures_layout_desc.data())
		, nullptr, *pVkDynLoader);
	textures_first_set = static_cast<uint32_t>(layout_desc.size()); // Store descriptor set index for textures
	layout_desc.push_back(textures_set_layout);


	const auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
		.setPSetLayouts(layout_desc.data())
		.setSetLayoutCount(static_cast<uint32_t>(layout_desc.size()));
	ASSERT(layoutCreateInfo.setLayoutCount <= limits.maxBoundDescriptorSets, "Pipeline layout set count (%" PRIu32") exceeds limits.maxBoundDescriptorSets (%" PRIu32")", layoutCreateInfo.setLayoutCount, limits.maxBoundDescriptorSets);
	layout = dev.createPipelineLayout(layoutCreateInfo, nullptr, *pVkDynLoader);

	const auto dynamicStates = std::array<vk::DynamicState, 3>{vk::DynamicState::eScissor, vk::DynamicState::eViewport, vk::DynamicState::eDepthBias};
	const auto multisampleState = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(rasterizationSamples);
	const auto dynamicS = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size()))
		.setPDynamicStates(dynamicStates.data());
	const auto viewportState = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setScissorCount(1);
	ASSERT(viewportState.viewportCount <= limits.maxViewports, "viewportCount (%" PRIu32") exceeds limits.maxViewports (%" PRIu32")", viewportState.viewportCount, limits.maxViewports);

	const auto iassembly = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(to_vk(primitive));

	uint32_t buffer_id = 0;
	std::vector<vk::VertexInputBindingDescription> buffers;
	std::vector<vk::VertexInputAttributeDescription> attributes;
	for (const auto& buffer : attribute_descriptions)
	{
		ASSERT(buffer.stride <= limits.maxVertexInputBindingStride, "buffer.stride (%zu) exceeds limits.maxVertexInputBindingStride (%" PRIu32")", buffer.stride, limits.maxVertexInputBindingStride);
		buffers.emplace_back(
			vk::VertexInputBindingDescription()
			.setBinding(buffer_id)
			.setStride(static_cast<uint32_t>(buffer.stride))
			.setInputRate((buffer.rate == gfx_api::vertex_attribute_input_rate::instance) ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex)
		);
		for (const auto& attribute : buffer.attributes)
		{
			ASSERT(attribute.offset <= limits.maxVertexInputAttributeOffset, "attribute.offset (%zu) exceeds limits.maxVertexInputAttributeOffset (%" PRIu32")", attribute.offset, limits.maxVertexInputAttributeOffset);
			attributes.emplace_back(
				vk::VertexInputAttributeDescription()
				.setBinding(buffer_id)
				.setFormat(to_vk(attribute.type))
				.setOffset(static_cast<uint32_t>(attribute.offset))
				.setLocation(static_cast<uint32_t>(attribute.id))
			);
		}
		buffer_id++;
	}
	ASSERT(buffers.size() <= limits.maxVertexInputBindings, "vertexBindingDescriptionCount (%zu) exceeds limits.maxVertexInputBindings (%" PRIu32")", buffers.size(), limits.maxVertexInputBindings);
	ASSERT(attributes.size() <= limits.maxVertexInputAttributes, "vertexAttributeDescriptionCount (%zu) exceeds limits.maxVertexInputAttributes (%" PRIu32")", attributes.size(), limits.maxVertexInputAttributes);

	const auto vertex_desc = vk::PipelineVertexInputStateCreateInfo()
		.setPVertexBindingDescriptions(buffers.data())
		.setVertexBindingDescriptionCount(static_cast<uint32_t>(buffers.size()))
		.setPVertexAttributeDescriptions(attributes.data())
		.setVertexAttributeDescriptionCount(static_cast<uint32_t>(attributes.size()));

	const auto color_blend_attachments = to_vk(state_desc.blend_state, state_desc.output_mask);
	const auto color_blend_state = vk::PipelineColorBlendStateCreateInfo()
		.setAttachmentCount(static_cast<uint32_t>(color_blend_attachments.size()))
		.setPAttachments(color_blend_attachments.data());

	const auto depthStencilState = to_vk(state_desc.depth_mode, state_desc.stencil);
	const auto rasterizationState = to_vk(state_desc.offset, state_desc.cull);
	const auto& shaderInfo = spv_files.at(shader_mode);
	vertexShader = get_module(shaderInfo.vertexSpv, *pVkDynLoader);
	fragmentShader = get_module(shaderInfo.fragmentSpv, *pVkDynLoader);
	auto pipelineStages = get_stages(vertexShader, fragmentShader);

	float cpyMipLodBias = root->mipLodBias.value_or(0.f);
	vk::SpecializationMapEntry entry ( 0, 0, sizeof(float) );
	vk::SpecializationInfo spec_info = vk::SpecializationInfo()
		.setMapEntryCount(1)
		.setPMapEntries(&entry)
		.setDataSize(sizeof(float))
		.setPData(&cpyMipLodBias)
	;
	if (root->lodBiasMethod == VkRoot::LodBiasMethod::SpecializationConstant && shaderInfo.specializationConstant_0_mipLoadBias)
	{
		ASSERT(pipelineStages[1].pSpecializationInfo == nullptr, "get_stages unexpectedly set pSpecializationInfo - this will overwrite!");
		pipelineStages[1].setPSpecializationInfo(&spec_info);
	}

	const auto pso = vk::GraphicsPipelineCreateInfo()
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

	vk::ResultValue<vk::Pipeline> result = dev.createGraphicsPipeline(vk::PipelineCache(), pso, nullptr, *pVkDynLoader);
	switch (result.result)
	{
		case vk::Result::eSuccess:
			object = std::move(result.value);
			break;
		default:
			vk::throwResultException(result.result, "createGraphicsPipeline");
	}
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
	for (auto& descSetLayout : cbuffer_set_layout)
	{
		dev.destroyDescriptorSetLayout(descSetLayout, nullptr, *pVkDynLoader);
	}
	cbuffer_set_layout.clear();
}

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

	auto bufferCreateInfo = vk::BufferCreateInfo()
		.setSize(size)
		.setUsage(to_vk(usage) | vk::BufferUsageFlagBits::eTransferDst);

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vk::Result result = static_cast<vk::Result>(vmaCreateBuffer(root->allocator, reinterpret_cast<const VkBufferCreateInfo*>( &bufferCreateInfo ), &allocInfo, reinterpret_cast<VkBuffer*>( &object ), &allocation, nullptr));
	if (result != vk::Result::eSuccess)
	{
		// Failed to allocate memory!
		vk::throwResultException( result, "vmaCreateBuffer" );
	}

	buffer_size = size;
}

VkBuf::~VkBuf()
{
	// All buffers must be properly released before gfx_api::context::shutdown()
	if (buffering_mechanism::isInitialized())
	{
		buffering_mechanism::get_current_resources().buffer_to_delete.emplace_back(std::move(object));
		buffering_mechanism::get_current_resources().vmamemory_to_free.push_back(allocation);
	}
//	else
//	{
//		// ~VkBuf called too late! - probably after gfx_api::context::shutdown()
//	}
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
#if defined(DEBUG)
	ASSERT(flag == update_flag::non_overlapping_updates_promise || (lastUploaded_FrameNum != current_FrameNum), "Attempt to upload to buffer more than once per frame");
#endif
	lastUploaded_FrameNum = current_FrameNum;

	ASSERT(start < buffer_size, "Starting offset (%zu) is past end of buffer (length: %zu)", start, buffer_size);
	ASSERT(start + size <= buffer_size, "Attempt to write past end of buffer");
	if (size == 0)
	{
		debug(LOG_WARNING, "Attempt to update buffer with 0 bytes of new data");
		return;
	}

	auto& frameResources = buffering_mechanism::get_current_resources();

	const auto stagingMemory = frameResources.stagingBufferAllocator.alloc(static_cast<uint32_t>(size), 2);
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
		case gfx_api::pixel_format::FORMAT_RG8_UNORM:
			return 2;
		case gfx_api::pixel_format::FORMAT_R8_UNORM:
			return 1;
		// compressed formats
		case gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM:
			return 3;
		case gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM:
		case gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM:
			return 4;
		default:
			return 4;
	}
	return 4; // silence warning
}

size_t VkTexture::format_size(const vk::Format& format)
{
	switch (format)
	{
	case vk::Format::eR8Unorm: return sizeof(uint8_t);
	case vk::Format::eR8G8B8Unorm: return 3 * sizeof(uint8_t);
	case vk::Format::eB8G8R8A8Unorm:
	case vk::Format::eR8G8B8A8Unorm: return 4 * sizeof(uint8_t);
	default:
			debug(LOG_FATAL, "Unhandled format: %d", (int)format);
	}
	throw;
}

VkTexture::VkTexture(const VkRoot& root, const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const gfx_api::pixel_format& _internal_format, const std::string& filename)
	: dev(root.dev), internal_format(_internal_format), mipmap_levels(mipmap_count), root(&root)
{
	ASSERT(width > 0 && height > 0, "0 width/height textures are unsupported");
	ASSERT(width <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "width (%zu) exceeds uint32_t max", width);
	ASSERT(height <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "height (%zu) exceeds uint32_t max", height);
	ASSERT(mipmap_count <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "mipmap_count (%zu) exceeds uint32_t max", mipmap_count);
	ASSERT(mipmap_count > 0, "mipmap_count must be > 0");

#if defined(WZ_DEBUG_GFX_API_LEAKS)
	debugName = filename;
#endif

	vk::Format internal_vk_format = root.get_format(_internal_format);

	auto imageCreateInfo = vk::ImageCreateInfo()
	.setArrayLayers(1)
	.setExtent(vk::Extent3D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1))
	.setImageType(vk::ImageType::e2D)
	.setMipLevels(static_cast<uint32_t>(mipmap_count))
	.setTiling(vk::ImageTiling::eOptimal)
	.setFormat(internal_vk_format)
	.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
	.setInitialLayout(vk::ImageLayout::eUndefined)
	.setSamples(vk::SampleCountFlagBits::e1)
	.setSharingMode(vk::SharingMode::eExclusive);

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vk::Result result = static_cast<vk::Result>(vmaCreateImage(root.allocator, reinterpret_cast<const VkImageCreateInfo*>( &imageCreateInfo ), &allocInfo, reinterpret_cast<VkImage*>( &object ), &allocation, nullptr));
	if (result != vk::Result::eSuccess)
	{
		// Failed to allocate memory!
		vk::throwResultException( result, "vmaCreateImage" );
	}

	if (root.debugUtilsExtEnabled)
	{
		vk::DebugUtilsObjectNameInfoEXT objectNameInfo;
		objectNameInfo.setObjectType(vk::ObjectType::eImage);
		objectNameInfo.setObjectHandle(uint64_t(static_cast<VkImage>(object)));
		objectNameInfo.setPObjectName(filename.c_str());
		root.dev.setDebugUtilsObjectNameEXT(objectNameInfo, root.vkDynLoader);
	}

	const auto imageViewCreateInfo = vk::ImageViewCreateInfo()
		.setImage(object)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(internal_vk_format)
		.setComponents(vk::ComponentMapping())
		.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, static_cast<uint32_t>(mipmap_count), 0, 1));

	view = dev.createImageViewUnique(imageViewCreateInfo, nullptr, root.vkDynLoader);

#if defined(WZ_DEBUG_GFX_API_LEAKS)
	debugLiveTextures.insert(this);
#endif
}

VkTexture::~VkTexture()
{
	// All textures must be properly released before gfx_api::context::shutdown()
	if (buffering_mechanism::isInitialized())
	{
		auto& frameResources = buffering_mechanism::get_current_resources();
		frameResources.image_view_to_delete.emplace_back(std::move(view));
		frameResources.image_to_delete.emplace_back(std::move(object));
		frameResources.vmamemory_to_free.push_back(allocation);
	}
	else
	{
		// ~VkTexture called too late! - probably after gfx_api::context::shutdown()
		view.release(); // Can't properly destroy this VK object now, but call release() to ensure that vk::Device::destroy isn't called after the device is no longer available
	}

#if defined(WZ_DEBUG_GFX_API_LEAKS)
	debugLiveTextures.erase(this);
#endif
}

void VkTexture::bind() {}

bool VkTexture::upload_internal(const std::size_t& mip_level, const std::size_t& offset_x, const std::size_t& offset_y, const iV_BaseImage& image)
{
	ASSERT_OR_RETURN(false, image.data() != nullptr, "Attempt to upload image without data");
	ASSERT_OR_RETURN(false, image.pixel_format() == internal_format, "Uploading image to texture with different format");
	size_t width = image.width();
	size_t height = image.height();
	ASSERT(width > 0 && height > 0, "Attempt to upload texture with width or height of 0 (width: %zu, height: %zu)", width, height);

	ASSERT(mip_level <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "mip_level (%zu) exceeds uint32_t max", mip_level);
	ASSERT(offset_x <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "offset_x (%zu) exceeds uint32_t max", offset_x);
	ASSERT(offset_y <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "offset_y (%zu) exceeds uint32_t max", offset_y);
	ASSERT(width <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "width (%zu) exceeds uint32_t max", width);
	ASSERT(height <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "height (%zu) exceeds uint32_t max", height);

	size_t dynamicAlignment = std::max(0x4 * format_size(internal_format), static_cast<size_t>(root->physDeviceProps.limits.optimalBufferCopyOffsetAlignment));
	auto& frameResources = buffering_mechanism::get_current_resources();
	const size_t stagingBufferSize = image.data_size();
	ASSERT(stagingBufferSize <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "stagingBufferSize (%zu) exceeds uint32_t max", stagingBufferSize);
	const auto stagingMemory = frameResources.stagingBufferAllocator.alloc(static_cast<uint32_t>(stagingBufferSize), static_cast<uint32_t>(dynamicAlignment));

	auto* mappedMem = reinterpret_cast<uint8_t*>(frameResources.stagingBufferAllocator.mapMemory(stagingMemory));
	ASSERT(mappedMem != nullptr, "Failed to map memory");

	memcpy(mappedMem, image.data(), image.data_size());

	frameResources.stagingBufferAllocator.unmapMemory(stagingMemory);

	const auto& cmdBuffer = buffering_mechanism::get_current_resources().cmdCopy;
	const auto imageMemoryBarriers_BeforeCopy = std::array<vk::ImageMemoryBarrier, 1> {
		vk::ImageMemoryBarrier()
			.setImage(object)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, static_cast<uint32_t>(mip_level), 1, 0, 1))
			.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
			.setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
	};
	// TODO: Should this be eBottomOfPipe, eTopOfPipe, or something else? // FIXME
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlags(), nullptr, nullptr, imageMemoryBarriers_BeforeCopy, root->vkDynLoader);
	const auto bufferImageCopyRegions = std::array<vk::BufferImageCopy, 1> {
		vk::BufferImageCopy()
			.setBufferOffset(stagingMemory.offset)
			.setBufferImageHeight(static_cast<uint32_t>(image.bufferImageHeight()))
			.setBufferRowLength(static_cast<uint32_t>(image.bufferRowLength()))
			.setImageOffset(vk::Offset3D(static_cast<uint32_t>(offset_x), static_cast<uint32_t>(offset_y), 0))
			.setImageSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, static_cast<uint32_t>(mip_level), 0, 1))
			.setImageExtent(vk::Extent3D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1))
	};
	cmdBuffer.copyBufferToImage(stagingMemory.buffer, object, vk::ImageLayout::eTransferDstOptimal, bufferImageCopyRegions, root->vkDynLoader);
	const auto imageMemoryBarriers_AfterCopy = std::array<vk::ImageMemoryBarrier, 1> {
		vk::ImageMemoryBarrier()
			.setImage(object)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, static_cast<uint32_t>(mip_level), 1, 0, 1))
			.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
	};
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
		vk::DependencyFlags(), nullptr, nullptr, imageMemoryBarriers_AfterCopy, root->vkDynLoader);

	return true;
}

bool VkTexture::upload(const size_t& mip_level, const iV_BaseImage& image)
{
	return upload_internal(mip_level, 0, 0, image);
}

bool VkTexture::upload_sub(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const iV_Image& image)
{
	return upload_internal(mip_level, offset_x, offset_y, image);
}

unsigned VkTexture::id() { return 0; }

// MARK: VkRoot

VkRoot::VkRoot(bool _debug) : validationLayer(_debug)
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
	const std::vector<std::type_index>& uniform_blocks,
	const std::vector<gfx_api::texture_input>& texture_desc,
	const std::vector<gfx_api::vertex_buffer>& attribute_descriptions)
{
	// build a pipeline, return an indirect VkPSOId (to enable rebuilding pipelines if needed)
	const gfxapi_PipelineCreateInfo createInfo(state_desc, shader_mode, primitive, uniform_blocks, texture_desc, attribute_descriptions);
	auto pipeline = new VkPSO(dev, physDeviceProps.limits, createInfo, rp, rp_compat_info, msaaSamples, vkDynLoader, *this);
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
			pipeline.second = new VkPSO(dev, physDeviceProps.limits, pipeline.first, rp, rp_compat_info, msaaSamples, vkDynLoader, *this);
		}
	}
}

void VkRoot::createDefaultRenderpass(vk::Format swapchainFormat, vk::Format depthFormat)
{
	bool msaaEnabled = (msaaSamples != vk::SampleCountFlagBits::e1);

	auto attachments =
		std::vector<vk::AttachmentDescription>{
		vk::AttachmentDescription() // colorAttachment
			.setFormat(swapchainFormat)
			.setSamples(msaaSamples)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout((msaaEnabled) ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
//			.setStencilLoadOp(vk::AttachmentLoadOp::eClear) // ?
			.setStencilStoreOp(vk::AttachmentStoreOp::eStore),
		vk::AttachmentDescription() // depthAttachment
			.setFormat(depthFormat)
			.setSamples(msaaSamples)
			.setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	};
	if (msaaEnabled)
	{
		attachments.push_back(
			  vk::AttachmentDescription() // colorAttachmentResolve
			  .setFormat(swapchainFormat)
			  .setSamples(vk::SampleCountFlagBits::e1)
			  .setInitialLayout(vk::ImageLayout::eUndefined)
			  .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
			  .setLoadOp(vk::AttachmentLoadOp::eDontCare)
			  .setStoreOp(vk::AttachmentStoreOp::eStore)
			  .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			  .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		);
	}
	const size_t numColorAttachmentRef = 1;
	const auto colorAttachmentRef =
		std::array<vk::AttachmentReference, numColorAttachmentRef>{
		vk::AttachmentReference()
			.setAttachment(0)
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
	};
	static_assert(minRequired_ColorAttachments >= numColorAttachmentRef, "minRequired_ColorAttachments must be >= colorAttachmentRef.size()");
	const auto depthStencilAttachmentRef =
		vk::AttachmentReference()
		.setAttachment(1)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	const auto colorAttachmentResolveRef =
		vk::AttachmentReference()
		.setAttachment(2)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	const auto subpasses =
		std::array<vk::SubpassDescription, 1> {
		vk::SubpassDescription()
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachmentCount(static_cast<uint32_t>(colorAttachmentRef.size()))
			.setPColorAttachments(colorAttachmentRef.data())
			.setPDepthStencilAttachment(&depthStencilAttachmentRef)
			.setPResolveAttachments((msaaEnabled) ? &colorAttachmentResolveRef : nullptr)
	};

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;

	auto createInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
		.setPAttachments(attachments.data())
		.setSubpassCount(static_cast<uint32_t>(subpasses.size()))
		.setPSubpasses(subpasses.data())
		.setDependencyCount(1)
		.setPDependencies((vk::SubpassDependency *)&dependency);

	rp_compat_info = std::make_shared<VkhRenderPassCompat>(createInfo);
	rp = dev.createRenderPass(createInfo, nullptr, vkDynLoader);
}

bool VkRoot::setupDebugUtilsCallbacks(const std::vector<const char*>& extensions, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	// Verify that the requested extensions included VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	bool requested_debug_utils_extension = std::any_of(extensions.begin(), extensions.end(),
				 [](const char *extensionName) { return (strcmp(extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0);});
	if (!requested_debug_utils_extension)
	{
		return false;
	}

	CreateDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(static_cast<VkInstance>(inst), "vkCreateDebugUtilsMessengerEXT")));
	if (!CreateDebugUtilsMessenger)
	{
		// Failed to obtain vkCreateDebugReportCallbackEXT
		debug(LOG_INFO, "Could not find symbol: vkCreateDebugUtilsMessengerEXT\n");
		return false;
	}
	DestroyDebugUtilsMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(static_cast<VkInstance>(inst), "vkDestroyDebugUtilsMessengerEXT")));
	if (!DestroyDebugUtilsMessenger)
	{
		// Failed to obtain vkDestroyDebugReportCallbackEXT
		debug(LOG_INFO, "Could not find symbol: vkDestroyDebugUtilsMessengerEXT\n");
		return false;
	}

	VkDebugUtilsMessengerCreateInfoEXT dbgCreateInfo = {};
	dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	dbgCreateInfo.flags = 0;
	dbgCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	dbgCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	dbgCreateInfo.pfnUserCallback = WZDebugUtilsCallback;

	const VkResult err = CreateDebugUtilsMessenger(
												   static_cast<VkInstance>(inst),
												   &dbgCreateInfo,
												   nullptr,
												   &debugUtilsCallback);
	if (err != VK_SUCCESS)
	{
		debug(LOG_ERROR, "vkCreateDebugUtilsMessengerEXT failed with error: %d", (int)err);
		return false;
	}

	return true;
}

bool VkRoot::setupDebugReportCallbacks(const std::vector<const char*>& extensions, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	// Verify that the requested extensions included VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	bool requested_debug_report_extension = std::any_of(extensions.begin(), extensions.end(),
				 [](const char *extensionName) { return (strcmp(extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0);});
	if (!requested_debug_report_extension)
	{
		return false;
	}

	CreateDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(static_cast<VkInstance>(inst), "vkCreateDebugReportCallbackEXT")));
	if (!CreateDebugReportCallback)
	{
		// Failed to obtain vkCreateDebugReportCallbackEXT
		debug(LOG_INFO, "Could not find symbol: vkCreateDebugReportCallbackEXT\n");
		return false;
	}
	DestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(static_cast<VkInstance>(inst), "vkDestroyDebugReportCallbackEXT")));
	if (!DestroyDebugReportCallback)
	{
		// Failed to obtain vkDestroyDebugReportCallbackEXT
		debug(LOG_INFO, "Could not find symbol: vkDestroyDebugReportCallbackEXT\n");
		return false;
	}
	dbgBreakCallback = reinterpret_cast<PFN_vkDebugReportMessageEXT>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(static_cast<VkInstance>(inst), "vkDebugReportMessageEXT")));
	if (!dbgBreakCallback)
	{
		// Failed to obtain vkDebugReportMessageEXT
		debug(LOG_INFO, "Could not find symbol: vkDebugReportMessageEXT\n");
		return false;
	}

	VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
	dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	dbgCreateInfo.pfnCallback = WZDebugReportCallback;
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

// Attempts to create a Vulkan instance (vk::Instance) with the specified extensions and layers
// If successful, sets the following variable in VkRoot:
//	- inst (vk::Instance)
bool VkRoot::createVulkanInstance(uint32_t apiVersion, const std::vector<const char*>& extensions, const std::vector<const char*>& _layers, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	appInfo = vk::ApplicationInfo()
	.setPApplicationName("Warzone2100")
	.setApplicationVersion(1)
	.setPEngineName("Warzone2100")
	.setEngineVersion(1)
	.setApiVersion(apiVersion);

	// Now we can make the Vulkan instance
	instanceCreateInfo = vk::InstanceCreateInfo()
	  .setPpEnabledLayerNames(_layers.data())
	  .setEnabledLayerCount(static_cast<uint32_t>(_layers.size()))
	  .setPApplicationInfo(&appInfo)
	  .setPpEnabledExtensionNames(extensions.data())
	  .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size())
	);

	bool requesting_debug_utils_extension = std::any_of(extensions.begin(), extensions.end(),
				[](const char *extensionName) { return (strcmp(extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0);});
	if (requesting_debug_utils_extension)
	{
		debugUtilsExtEnabled = true;

		// From the VK_EXT_debug_utils docs:
		// "To capture events that occur while creating or destroying an instance an application can link a VkDebugUtilsMessengerCreateInfoEXT structure to the pNext element of the VkInstanceCreateInfo structure given to vkCreateInstance."
		instanceDebugUtilsCallbacksInfo = VkDebugUtilsMessengerCreateInfoEXT{};
		instanceDebugUtilsCallbacksInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		instanceDebugUtilsCallbacksInfo.flags = 0;
		instanceDebugUtilsCallbacksInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		instanceDebugUtilsCallbacksInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		instanceDebugUtilsCallbacksInfo.pfnUserCallback = WZDebugUtilsCallback;
		instanceCreateInfo.setPNext(&instanceDebugUtilsCallbacksInfo);
	}

#if defined(VK_KHR_portability_enumeration)
	bool requesting_portability_enumeration_extension = std::any_of(extensions.begin(), extensions.end(),
				 [](const char *extensionName) { return (strcmp(extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0);});
	if (requesting_portability_enumeration_extension)
	{
		// Must set VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR in flags
		instanceCreateInfo.setFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR);
	}
#endif

	PFN_vkCreateInstance _vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(nullptr, "vkCreateInstance")));
	if (!_vkCreateInstance)
	{
		// Failed to find vkCreateInstance
		debug(LOG_ERROR, "Could not find symbol: vkCreateInstance\n");
		return false;
	}

	debug(LOG_3D, "Using instance apiVersion: %s", VkhInfo::vulkan_apiversion_to_string(instanceCreateInfo.pApplicationInfo->apiVersion).c_str());
	std::string layersAsString;
	std::for_each(_layers.begin(), _layers.end(), [&layersAsString](const char *layer) {
		layersAsString += std::string(layer) + ",";
	});
	debug(LOG_3D, "Using layers: %s", layersAsString.c_str());
	std::string instanceExtensionsAsString;
	std::for_each(extensions.begin(), extensions.end(), [&instanceExtensionsAsString](const char *extension) {
		instanceExtensionsAsString += std::string(extension) + ",";
	});
	debug(LOG_3D, "Using instance extensions: %s", instanceExtensionsAsString.c_str());

	VkResult result = _vkCreateInstance(reinterpret_cast<const VkInstanceCreateInfo*>(&instanceCreateInfo), nullptr, reinterpret_cast<VkInstance*>(&inst));
	if (result != VK_SUCCESS)
	{
		// vkCreateInstance failed
		debug(LOG_ERROR, "vkCreateInstance failed with error: %s", to_string(vk::Result(result)).c_str());
		return false;
	}

	debugUtilsExtEnabled = requesting_debug_utils_extension;

	// Setup debug callbacks
	if (!debugCallbacksEnabled)
		return true;

	if (!setupDebugUtilsCallbacks(extensions, _vkGetInstanceProcAddr))
	{
		debug(LOG_3D, "Failed to initialize VK_EXT_debug_utils callbacks");
		if (!setupDebugReportCallbacks(extensions, _vkGetInstanceProcAddr))
		{
			debug(LOG_3D, "Failed to initialize VK_EXT_debug_report callbacks");
		}
	}

	return true;
}

// WZ-specific functions for rating / determining requirements
int rateDeviceSuitability(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface, const vk::DispatchLoaderDynamic &vkDynLoader)
{
	const auto deviceProperties = device.getProperties(vkDynLoader);
	vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures(vkDynLoader);

	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
	{
		score += 1000000; // picked a number greater than the max seen maxImageDimension2D value: https://vulkan.gpuinfo.org/displaydevicelimit.php?name=maxImageDimension2D
	}

	// Integrated GPUs should still be preferred over CPU types
	if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
	{
		score += 100000; // picked a number greater than the max seen maxImageDimension2D value: https://vulkan.gpuinfo.org/displaydevicelimit.php?name=maxImageDimension2D (but less than the dedicated gpu number above)
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Requires: deviceProperties.apiVersion >= minSupportedVulkanVersion
	if (!VK_VERSION_GREATER_THAN_OR_EQUAL(deviceProperties.apiVersion, minSupportedVulkanVersion))
	{
		return 0;
	}

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
	try {
		SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(device, surface, vkDynLoader);
		if (swapChainSupportDetails.formats.empty() || swapChainSupportDetails.presentModes.empty())
		{
			return 0;
		}
	}
	catch (const vk::SystemError&)
	{
		return 0;
	}

	if (deviceProperties.deviceType == vk::PhysicalDeviceType::eCpu && deviceProperties.vendorID == 0x10005)
	{
		if (strncmp("llvmpipe", deviceProperties.deviceName, strlen("llvmpipe")) == 0)
		{
			// allow llvmpipe, but prioritize it last
			score = std::min<int>(1, score);
		}
	}

	return score;
}

// General function that uses WZ-specific functions
vk::PhysicalDevice VkRoot::pickPhysicalDevice()
{
	ASSERT(inst, "Instance is null");
	ASSERT(surface, "Surface is null");

	const auto physicalDevices = inst.enumeratePhysicalDevices(vkDynLoader);
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
		// Theoretically, one could recreate swapchain here.
		// However this currently causes issues in practice / Vulkan validation layer errors in certain circumstances / (on certain menu screens?)
		// Relying on the DrawableSize versus swapchainSize check in flip() seems to work fine and doesn't have the same issues.
		return;
	}
}

std::pair<uint32_t, uint32_t> VkRoot::getDrawableDimensions()
{
	return {swapchainSize.width, swapchainSize.height};
}

bool VkRoot::shouldDraw()
{
	return swapchainSize.width > 1 && swapchainSize.height > 1; // check for > 1 here because we use 1,1 in place of a 0,0 swapchain size to avoid other issues
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

#if defined(WZ_DEBUG_GFX_API_LEAKS)
	if (!debugLiveTextures.empty())
	{
		// Some textures were not properly freed before Vulkan context shutdown
		for (auto texture : debugLiveTextures)
		{
			debug(LOG_ERROR, "Texture resource not cleaned up: %p (name: %s)", (const void*)texture, texture->debugName.c_str());
		}
		ASSERT(debugLiveTextures.empty(), "There are %zu textures that were not cleaned up.", debugLiveTextures.size());
	}
#endif

	// destroy instance
	if (inst)
	{
		// destroy debug callback
		if (debugUtilsCallback)
		{
			DestroyDebugUtilsMessenger(static_cast<VkInstance>(inst), debugUtilsCallback, nullptr);
			debugUtilsCallback = 0;
		}
		if (msgCallback)
		{
			DestroyDebugReportCallback(static_cast<VkInstance>(inst), msgCallback, nullptr);
			msgCallback = 0;
		}

		inst.destroy(nullptr, vkDynLoader);
		inst = vk::Instance();
	}
}

void VkRoot::destroySwapchainAndSwapchainSpecificStuff(bool doDestroySwapchain)
{
	if (!dev)
	{
		// Logical device is null - return
		return;
	}

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

	if (depthStencilView)
	{
		dev.destroyImageView(depthStencilView, nullptr, vkDynLoader);
		depthStencilView = vk::ImageView();
	}
	if (depthStencilMemory)
	{
		dev.freeMemory(depthStencilMemory, nullptr, vkDynLoader);
		depthStencilMemory = vk::DeviceMemory();
	}
	if (depthStencilImage)
	{
		dev.destroyImage(depthStencilImage, nullptr, vkDynLoader);
		depthStencilImage = vk::Image();
	}

	if (colorImageView)
	{
		dev.destroyImageView(colorImageView, nullptr, vkDynLoader);
		colorImageView = vk::ImageView();
	}
	if (colorImageMemory)
	{
		dev.freeMemory(colorImageMemory, nullptr, vkDynLoader);
		colorImageMemory = vk::DeviceMemory();
	}
	if (colorImage)
	{
		dev.destroyImage(colorImage, nullptr, vkDynLoader);
		colorImage = vk::Image();
	}

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

std::string to_string(gfx_api::context::swap_interval_mode swapMode)
{
	switch (swapMode)
	{
		case gfx_api::context::swap_interval_mode::immediate:
			return "immediate";
		case gfx_api::context::swap_interval_mode::vsync:
			return "vsync";
		case gfx_api::context::swap_interval_mode::adaptive_vsync:
			return "adaptive vsync";
	}
	return "n/a";
}

bool findBestAvailablePresentModeForSwapMode(const std::vector<vk::PresentModeKHR>& availablePresentModes, gfx_api::context::swap_interval_mode swapMode, vk::PresentModeKHR& outputPresentMode)
{
	std::vector<vk::PresentModeKHR> presentModes;
	switch (swapMode)
	{
		case gfx_api::context::swap_interval_mode::immediate:
			// If vsync is disabled, prefer VK_PRESENT_MODE_IMMEDIATE_KHR; fallback to VK_PRESENT_MODE_MAILBOX_KHR
			presentModes = { vk::PresentModeKHR::eImmediate, vk::PresentModeKHR::eMailbox };
			break;
		case gfx_api::context::swap_interval_mode::vsync:
			presentModes = { vk::PresentModeKHR::eFifo };
			break;
		case gfx_api::context::swap_interval_mode::adaptive_vsync:
			presentModes = { vk::PresentModeKHR::eFifoRelaxed };
			break;
	}

	for (const auto& presentMode : presentModes)
	{
		if(std::find(availablePresentModes.begin(), availablePresentModes.end(), presentMode) != availablePresentModes.end())
		{
			// presentMode was found - return it
			outputPresentMode = presentMode;
			return true;
		}
	}

	// VK_PRESENT_MODE_FIFO_KHR is *required* to be supported, so fall back to it
	if(std::find(availablePresentModes.begin(), availablePresentModes.end(), vk::PresentModeKHR::eFifo) == availablePresentModes.end())
	{
		// vk::PresentModeKHR::eFifo is unsupported??
		debug(LOG_FATAL, "vk::PresentModeKHR::eFifo does not appear to be supported?? Aborting.");
		abort();
	}
	outputPresentMode = vk::PresentModeKHR::eFifo;
	return false;
}

gfx_api::context::swap_interval_mode from_vk_presentmode(vk::PresentModeKHR presentMode)
{
	switch (presentMode)
	{
		case vk::PresentModeKHR::eImmediate:
		case vk::PresentModeKHR::eMailbox:
			return gfx_api::context::swap_interval_mode::immediate;
		case vk::PresentModeKHR::eFifo:
			return gfx_api::context::swap_interval_mode::vsync;
		case vk::PresentModeKHR::eFifoRelaxed:
			return gfx_api::context::swap_interval_mode::adaptive_vsync;
		default:
			debug(LOG_ERROR, "Unhandled vk::PresentModeKHR value: %s", to_string(presentMode).c_str());
	}
	return gfx_api::context::swap_interval_mode::vsync; // prevent warning
}

bool VkRoot::setSwapInterval(gfx_api::context::swap_interval_mode newSwapMode)
{
	ASSERT(physicalDevice, "Physical device is null");
	ASSERT(surface, "Surface is null");

	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface, vkDynLoader);
	vk::PresentModeKHR desiredPresentMode = vk::PresentModeKHR::eFifo;
	if (!findBestAvailablePresentModeForSwapMode(swapChainSupport.presentModes, newSwapMode, desiredPresentMode))
	{
		// unable to find a best available presentation mode for the desired swap mode, so return false
		debug(LOG_WARNING, "Unable to find best available presentation mode for swapmode: %s", to_string(newSwapMode).c_str());
		return false;
	}

	if (swapchain && (desiredPresentMode == presentMode))
	{
		// nothing to do - already in this mode
		debug(LOG_3D, "Ignoring request to set presentation mode to current presentation mode: %s", to_string(desiredPresentMode).c_str());
		return true;
	}

	// set swapMode
	swapMode = newSwapMode;

	// Destroy + recreate swapchain
	destroySwapchainAndSwapchainSpecificStuff(false);
	if (createSwapchain())
	{
		rebuildPipelinesIfNecessary();
		return true;
	}
	else
	{
		debug(LOG_ERROR, "createSwapchain() failed");
		return false;
	}
	return false;
}

gfx_api::context::swap_interval_mode VkRoot::getSwapInterval() const
{
	return from_vk_presentmode(presentMode);
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
	SwapChainSupportDetails swapChainSupport;
	try {
		swapChainSupport = querySwapChainSupport(physicalDevice, surface, vkDynLoader);
	}
	catch (const vk::SystemError &e)
	{
		debug(LOG_ERROR, "Querying swapchain support failed with error: %s", e.what());
		return false;
	}

	if (!findBestAvailablePresentModeForSwapMode(swapChainSupport.presentModes, swapMode, presentMode))
	{
		debug(LOG_WARNING, "Unable to find best available presentation mode for swapmode: %s", to_string(swapMode).c_str());

		// VK_PRESENT_MODE_FIFO_KHR is *required* to be supported, so fall back to it
		if(std::find(swapChainSupport.presentModes.begin(), swapChainSupport.presentModes.end(), vk::PresentModeKHR::eFifo) == swapChainSupport.presentModes.end())
		{
			// vk::PresentModeKHR::eFifo is unsupported??
			debug(LOG_FATAL, "vk::PresentModeKHR::eFifo does not appear to be supported?? Aborting.");
			return false;
		}
		presentMode = vk::PresentModeKHR::eFifo;
		swapMode = from_vk_presentmode(presentMode);
	}
	debug(LOG_3D, "Using present mode: %s", to_string(presentMode).c_str());

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
	// Some drivers may return 0 for swapchain min/maxImageExtent height/width in certain circumstances
	// (ex. some Nvidia drivers on Windows when minimizing the window)
	// but attempting to create a swapchain with extents of 0 is invalid
	if (swapchainSize.width == 0)
	{
		swapchainSize.width = std::max(drawableSize.width, static_cast<uint32_t>(1)); // ensure there's never a 0 swapchain width
	}
	if (swapchainSize.height == 0)
	{
		swapchainSize.height = std::max(drawableSize.height, static_cast<uint32_t>(1)); // ensure there's never a 0 swapchain height
	}
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
	debug(LOG_3D, "Using surface format: %s - %s", to_string(surfaceFormat.format).c_str(), to_string(surfaceFormat.colorSpace).c_str());

	vk::SwapchainCreateInfoKHR createSwapchainInfo = vk::SwapchainCreateInfoKHR()
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
		debug(LOG_3D, "Using separate graphics (%" PRIu32") and presentation (%" PRIu32") queue families", queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value());
		createSwapchainInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
		createSwapchainInfo.setQueueFamilyIndexCount(2);
		createSwapchainInfo.setPQueueFamilyIndices(queueFamilyIndicesU32);
	}

	try {
		swapchain = dev.createSwapchainKHR(createSwapchainInfo, nullptr, vkDynLoader);
	}
	catch (const vk::OutOfHostMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImageView(swapchainImages): OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImageView(swapchainImages): OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}
	catch (const vk::InitializationFailedError& e)
	{
		debug(LOG_ERROR, "vk::Device::createSwapchainKHR: InitializationFailedError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorInitializationFailed);
	}
	catch (const vk::SystemError &e)
	{
		debug(LOG_ERROR, "vk::Device::createSwapchainKHR: failed with error: %s", e.what());
		return false;
	}

	if (createSwapchainInfo.oldSwapchain)
	{
		dev.destroySwapchainKHR(createSwapchainInfo.oldSwapchain, nullptr, vkDynLoader);
	}

	// createSwapchainImageViews
	std::vector<vk::Image> swapchainImages = dev.getSwapchainImagesKHR(swapchain, vkDynLoader);
	debug(LOG_3D, "Requested swapchain minImageCount: %" PRIu32", received: %zu", swapchainDesiredImageCount, swapchainImages.size());
	try {
		std::transform(swapchainImages.begin(), swapchainImages.end(), std::back_inserter(swapchainImageView),
				   [&](const vk::Image& img) {
					   return dev.createImageView(
												  vk::ImageViewCreateInfo()
												  .setImage(img)
												  .setFormat(surfaceFormat.format)
												  .setViewType(vk::ImageViewType::e2D)
												  .setComponents(vk::ComponentMapping())
												  .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
												  , nullptr, vkDynLoader);
				   });
	}
	catch (const vk::OutOfHostMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImageView(swapchainImages): OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImageView(swapchainImages): OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}
	//

	try {
		buffering_mechanism::init(dev, allocator, MAX_FRAMES_IN_FLIGHT, queueFamilyIndices.graphicsFamily.value(), vkDynLoader);
	}
	catch (const vk::OutOfHostMemoryError& e) {
		debug(LOG_ERROR, "buffering_mechanism init: OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e) {
		debug(LOG_ERROR, "buffering_mechanism init: OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}

	// createColorResources
	vk::Format colorFormat = surfaceFormat.format;

	try {
		colorImage = dev.createImage(
			vk::ImageCreateInfo()
				.setFormat(colorFormat)
				.setArrayLayers(1)
				.setExtent(vk::Extent3D(swapchainSize.width, swapchainSize.height, 1))
				.setImageType(vk::ImageType::e2D)
				.setMipLevels(1)
				.setSamples(msaaSamples)
				.setTiling(vk::ImageTiling::eOptimal)
				.setUsage(vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment)
				.setSharingMode(vk::SharingMode::eExclusive)
			, nullptr, vkDynLoader
		);
	}
	catch (const vk::OutOfHostMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImage (colorImage): OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImage (colorImage): OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}

	const auto colorImage_memreq = dev.getImageMemoryRequirements(colorImage, vkDynLoader);
	ASSERT(colorImage_memreq.size > 0, "Attempting to allocate memory of size 0 will fail");
	try {
		colorImageMemory = dev.allocateMemory(
			vk::MemoryAllocateInfo()
				.setAllocationSize(colorImage_memreq.size)
				.setMemoryTypeIndex(findProperties(memprops, colorImage_memreq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
			, nullptr, vkDynLoader
		);

		dev.bindImageMemory(colorImage, colorImageMemory, 0, vkDynLoader);
	}
	catch (const vk::OutOfHostMemoryError& e) {
		debug(LOG_ERROR, "vkAllocateMemory / vkBindImageMemory (colorImage): OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e) {
		debug(LOG_ERROR, "vkAllocateMemory / vkBindImageMemory (colorImage): OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}

	try {
		colorImageView = dev.createImageView(
		   vk::ImageViewCreateInfo()
			   .setFormat(colorFormat)
			   .setImage(colorImage)
			   .setViewType(vk::ImageViewType::e2D)
			   .setComponents(vk::ComponentMapping())
			   .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
			, nullptr, vkDynLoader
		);
	}
	catch (const vk::OutOfHostMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImageView (colorImage): OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImageView (colorImage): OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}

	// createDepthStencilImage
	vk::Format depthFormat = findDepthFormat(physicalDevice, vkDynLoader);

	try {
		depthStencilImage = dev.createImage(
										vk::ImageCreateInfo()
										.setFormat(depthFormat)
										.setArrayLayers(1)
										.setExtent(vk::Extent3D(swapchainSize.width, swapchainSize.height, 1))
										.setImageType(vk::ImageType::e2D)
										.setMipLevels(1)
										.setSamples(msaaSamples)
										.setTiling(vk::ImageTiling::eOptimal)
										.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
										.setSharingMode(vk::SharingMode::eExclusive)
										, nullptr, vkDynLoader);
	}
	catch (const vk::OutOfHostMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImage (depthStencilImage): OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImage (depthStencilImage): OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}

	const auto memreq = dev.getImageMemoryRequirements(depthStencilImage, vkDynLoader);

	try {
		depthStencilMemory = dev.allocateMemory(
												vk::MemoryAllocateInfo()
												.setAllocationSize(memreq.size)
												.setMemoryTypeIndex(findProperties(memprops, memreq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
												, nullptr, vkDynLoader);
		dev.bindImageMemory(depthStencilImage, depthStencilMemory, 0, vkDynLoader);
	}
	catch (const vk::OutOfHostMemoryError& e) {
		debug(LOG_ERROR, "vkAllocateMemory / vkBindImageMemory (depthStencil): OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e) {
		debug(LOG_ERROR, "vkAllocateMemory / vkBindImageMemory (depthStencil): OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}

	try {
		depthStencilView = dev.createImageView(
											   vk::ImageViewCreateInfo()
											   .setFormat(depthFormat)
											   .setImage(depthStencilImage)
											   .setViewType(vk::ImageViewType::e2D)
											   .setComponents(vk::ComponentMapping())
											   .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1))
											   , nullptr, vkDynLoader);
	}
	catch (const vk::OutOfHostMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImageView (depthStencil): OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e) {
		debug(LOG_ERROR, "vkCreateImageView (depthStencil): OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}
	//

	setupSwapchainImages();

	try {
		createDefaultRenderpass(surfaceFormat.format, depthFormat);
	}
	catch (const vk::OutOfHostMemoryError& e) {
		debug(LOG_ERROR, "vkCreateRenderPass (default): OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e) {
		debug(LOG_ERROR, "vkCreateRenderPass (default): OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}

	// createFramebuffers()
	bool msaaEnabled = (msaaSamples != vk::SampleCountFlagBits::e1);
	try {
		std::transform(swapchainImageView.begin(), swapchainImageView.end(), std::back_inserter(fbo),
				   [&](const vk::ImageView& imageView) {
					   const auto attachments = (msaaEnabled) ? std::vector<vk::ImageView>{colorImageView, depthStencilView, imageView}
					   											: std::vector<vk::ImageView>{imageView, depthStencilView};
					   return dev.createFramebuffer(
													vk::FramebufferCreateInfo()
													.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
													.setPAttachments(attachments.data())
													.setLayers(1)
													.setWidth(swapchainSize.width)
													.setHeight(swapchainSize.height)
													.setRenderPass(rp)
													, nullptr, vkDynLoader);
				   });
	}
	catch (const vk::OutOfHostMemoryError& e) {
		debug(LOG_ERROR, "vkCreateFramebuffer: OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e) {
		debug(LOG_ERROR, "vkCreateFramebuffer: OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}

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
	pDefaultTexture = new VkTexture(*this, 1, defaultTexture_width, defaultTexture_height, gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8, "<default_texture>");

	iV_Image defaultTexture;
	defaultTexture.allocate(defaultTexture_width, defaultTexture_height, 4, true);
	if (!pDefaultTexture->upload(0, defaultTexture))
	{
		debug(LOG_ERROR, "Failed to upload default texture??");
	}

	startRenderPass();

	return true;
}

bool VkRoot::canUseVulkanInstanceAPI(uint32_t minVulkanAPICoreVersion) const
{
	ASSERT(inst, "Instance is null");
	return VK_VERSION_GREATER_THAN_OR_EQUAL(appInfo.apiVersion, minVulkanAPICoreVersion);
}

bool VkRoot::canUseVulkanDeviceAPI(uint32_t minVulkanAPICoreVersion) const
{
	ASSERT(physicalDevice, "Physical device is null");
	return VK_VERSION_GREATER_THAN_OR_EQUAL(appInfo.apiVersion, minVulkanAPICoreVersion) && VK_VERSION_GREATER_THAN_OR_EQUAL(physDeviceProps.apiVersion, minVulkanAPICoreVersion);
}

bool VkRoot::_initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode requestedSwapMode, optional<float> _mipLodBias)
{
	debug(LOG_3D, "VkRoot::initialize()");

	frameNum = 1;
	swapMode = requestedSwapMode;
	mipLodBias = _mipLodBias;

	SetVKImplicitLayerEnvironmentVariables();

	// obtain backend_Vulkan_Impl from impl
	backend_impl = impl.createVulkanBackendImpl();
	if (!backend_impl)
	{
		debug(LOG_ERROR, "Failed to get Vulkan backend implementation");
		return false;
	}

	PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr = backend_impl->getVkGetInstanceProcAddr();
	if(!_vkGetInstanceProcAddr)
	{
		debug(LOG_ERROR, "backend_impl->getVkGetInstanceProcAddr() failed\n");
		return false;
	}

	uint32_t instanceVersion;
	PFN_vkEnumerateInstanceVersion _vkEnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion")));
	if (_vkEnumerateInstanceVersion)
	{
		const VkResult enumResult = _vkEnumerateInstanceVersion(&instanceVersion);
		if (enumResult == VK_SUCCESS)
		{
			debug(LOG_3D, "vkEnumerateInstanceVersion: %s", VkhInfo::vulkan_apiversion_to_string(instanceVersion).c_str());
		}
		else
		{
			debug(LOG_3D, "vkEnumerateInstanceVersion failed with error: %s\n", to_string(vk::Result(enumResult)).c_str());
			instanceVersion = VK_API_VERSION_1_0;
		}
	}
	else
	{
		debug(LOG_3D, "Could not find symbol: vkEnumerateInstanceVersion\n");
		instanceVersion = VK_API_VERSION_1_0;
	}
	uint32_t requestedInstanceApiVersion = std::min(
			(uint32_t)VK_MAKE_VERSION(VK_VERSION_MAJOR(instanceVersion), VK_VERSION_MINOR(instanceVersion), 0),
			maxRequestableInstanceVulkanVersion
	); // cap requestedInstanceApiVersion to maxRequestableInstanceVulkanVersion

	try {
		debugInfo.Output_GlobalInstanceExtensions(_vkGetInstanceProcAddr);
	}
	catch (const vk::SystemError& e)
	{
		// catch any weird errors that debugInfo didn't handle
		debug(LOG_ERROR, "Output_GlobalInstanceExtensions failed: %s", e.what());
	}

	try {
		debugInfo.Output_InstanceLayerProperties(_vkGetInstanceProcAddr);
	}
	catch (const vk::SystemError& e)
	{
		// catch any weird errors that debugInfo didn't handle
		debug(LOG_ERROR, "Output_InstanceLayerProperties failed: %s", e.what());
	}

	instanceExtensions.clear();

	// Get required extensions (from backend implementation)
	if (!backend_impl->getRequiredInstanceExtensions(instanceExtensions))
	{
		debug(LOG_ERROR, "backend_impl->getRequiredInstanceExtensions failed");
		return false;
	}

	layers.clear();

// validationLayer handling
	if (validationLayer)
	{
		// determine if validation layers are available
		if (checkValidationLayerSupport(_vkGetInstanceProcAddr))
		{
			layers = validationLayers;
			debug(LOG_INFO, "Vulkan: Enabling validation layers (--gfxdebug) for development / bug-catching purposes (this will impact performance)");
		}
		else
		{
			debug(LOG_ERROR, "Vulkan: debug/validation layers requested, but not available - disabling");
			validationLayer = false;
		}
	}
// end validationLayer handling

	if (validationLayer || debugCallbacksEnabled)
	{
		// determine if desired debug extensions are available
		std::vector<const char*> supportedDebugExtensions;
		if (findSupportedInstanceExtensions(debugAdditionalExtensions, supportedDebugExtensions, _vkGetInstanceProcAddr) && !supportedDebugExtensions.empty())
		{
			instanceExtensions.insert(std::end(instanceExtensions), supportedDebugExtensions.begin(), supportedDebugExtensions.end());
		}
		else
		{
			debug(LOG_ERROR, "Failed to retrieve any supported debug extensions");
			validationLayer = false;
			debugCallbacksEnabled = false;
		}
	}

	// add other optional instance extensions
	std::vector<const char *> optionalInstanceExtensionsToCheck;
	for (const auto& optionalInstanceExtensionPair : optionalInstanceExtensions)
	{
		uint32_t minInstanceExtensionCoreVersion = std::get<0>(optionalInstanceExtensionPair);
		if (minInstanceExtensionCoreVersion != 0 && requestedInstanceApiVersion >= minInstanceExtensionCoreVersion)
		{
			// Vulkan instance version is sufficient for this extension to be promoted to core, so ignore
			// (the rest of the code should check and use the core version; see: canUseVulkanInstanceAPI / canUseVulkanDeviceAPI)
			const char * extensionName = std::get<1>(optionalInstanceExtensionPair);
			debug(LOG_3D, "Vulkan instance version is sufficient for this extension to be promoted to core: %s", extensionName);
		}
		else
		{
			// must check for extension availability
			optionalInstanceExtensionsToCheck.push_back(std::get<1>(optionalInstanceExtensionPair));
		}
	}
	if (!optionalInstanceExtensionsToCheck.empty())
	{
		std::string checkInstanceExtensionsAsString;
		std::for_each(optionalInstanceExtensionsToCheck.begin(), optionalInstanceExtensionsToCheck.end(), [&checkInstanceExtensionsAsString](const char* extension) {
			checkInstanceExtensionsAsString += std::string(extension) + ",";
			});
		debug(LOG_3D, "Checking for optional instance extensions: %s", checkInstanceExtensionsAsString.c_str());
		std::vector<const char*> supportedOptionalInstanceExtensions;
		if (!findSupportedInstanceExtensions(optionalInstanceExtensionsToCheck, supportedOptionalInstanceExtensions, _vkGetInstanceProcAddr))
		{
			debug(LOG_ERROR, "Failed to retrieve supported optional instance extensions");
			return false;
		}
		instanceExtensions.insert(std::end(instanceExtensions), supportedOptionalInstanceExtensions.begin(), supportedOptionalInstanceExtensions.end());
	}

	if (!createVulkanInstance(requestedInstanceApiVersion, instanceExtensions, layers, _vkGetInstanceProcAddr))
	{
		debug(LOG_ERROR, "createVulkanInstance failed");
		return false;
	}

	// Setup dynamic Vulkan loader
	vkDynLoader.init(static_cast<VkInstance>(inst), _vkGetInstanceProcAddr, VK_NULL_HANDLE, nullptr);

	// NOTE: From this point on, vkDynLoader *must* be initialized!
	ASSERT(vkDynLoader.vkGetInstanceProcAddr != nullptr, "vkDynLoader does not appear to be initialized");
	if (debugUtilsExtEnabled)
	{
		ASSERT(vkDynLoader.vkSetDebugUtilsObjectNameEXT != nullptr, "VK_EXT_debug_utils is enabled, but vkSetDebugUtilsObjectNameEXT is null??");
	}

	try {
		debugInfo.Output_PhysicalDevices(inst, appInfo, instanceExtensions, vkDynLoader);
	}
	catch (const vk::SystemError& e)
	{
		// catch any weird errors that debugInfo didn't handle
		debug(LOG_ERROR, "Output_PhysicalDevices failed: %s", e.what());
	}

	if (!createSurface())
	{
		debug(LOG_ERROR, "createSurface() failed");
		return false;
	}

	// pick physical device (and get properties)
	try
	{
		physicalDevice = pickPhysicalDevice();
	}
	catch (const std::exception& e)
	{
		// failed to find a physical device that supports Vulkan
		debug(LOG_ERROR, "pickPhysicalDevice failed: %s", e.what());
		return false;
	}
	physDeviceProps = physicalDevice.getProperties(vkDynLoader);
	debug(LOG_3D, "Picking device: %s", physDeviceProps.deviceName.data());
	debug(LOG_3D, "- apiVersion: %s", VkhInfo::vulkan_apiversion_to_string(physDeviceProps.apiVersion).c_str());
	debug(LOG_3D, "- driverVersion: %" PRIu32, physDeviceProps.driverVersion);
	debug(LOG_3D, "- vendorID: %" PRIu32, physDeviceProps.vendorID);
	debug(LOG_3D, "- deviceID: %" PRIu32, physDeviceProps.deviceID);
	debug(LOG_3D, "- deviceType: %s", to_string(physDeviceProps.deviceType).c_str());
	formattedRendererInfoString = calculateFormattedRendererInfoString(); // must be called after physDeviceProps is populated
	physDeviceFeatures = physicalDevice.getFeatures(vkDynLoader);
	memprops = physicalDevice.getMemoryProperties(vkDynLoader);

	hasPortabilitySubset = checkDeviceExtensionSupport(physicalDevice, {"VK_KHR_portability_subset"}, vkDynLoader);
#if 0 // this is not a "stable" extension yet
	if (hasPortabilitySubset && canUseVulkanDeviceAPI((uint32_t)VK_MAKE_VERSION(1, 1, 0))) // vkGetPhysicalDeviceFeatures2 is core Vulkan 1.1+
	{
		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDevicePortabilitySubsetFeaturesKHR> features2;
		physicalDevice.getFeatures2(&features2.get(), vkDynLoader);
		physDevicePortabilitySubsetFeatures = features2.get<vk::PhysicalDevicePortabilitySubsetFeaturesKHR>();
		debug(LOG_3D, "- samplerMipLodBias: %s", (physDevicePortabilitySubsetFeatures.samplerMipLodBias) ? "true" : "false");
	}
	else
	{
		physDevicePortabilitySubsetFeatures = vk::PhysicalDevicePortabilitySubsetFeaturesKHR();
	}
#endif

	if (!hasPortabilitySubset
#if 0 // this is not a "stable" extension yet
		|| physDevicePortabilitySubsetFeatures.samplerMipLodBias
#endif
		)
	{
		// Use setMipLodBias in vk::SamplerCreateInfo
		lodBiasMethod = LodBiasMethod::SamplerMipLodBias;
	}
	else
	{
		// Otherwise, use specialization constants to provide the bias to do it in-shader
		lodBiasMethod = LodBiasMethod::SpecializationConstant;
	}

	initPixelFormatsSupport();

	// convert antialiasing to vk::SampleCountFlagBits
	if (antialiasing >= 64)
	{
		msaaSamples = vk::SampleCountFlagBits::e64;
	}
	else if (antialiasing >= 32)
	{
		msaaSamples = vk::SampleCountFlagBits::e32;
	}
	else if (antialiasing >= 16)
	{
		msaaSamples = vk::SampleCountFlagBits::e16;
	}
	else if (antialiasing >= 8)
	{
		msaaSamples = vk::SampleCountFlagBits::e8;
	}
	else if (antialiasing >= 4)
	{
		msaaSamples = vk::SampleCountFlagBits::e4;
	}
	else if (antialiasing >= 2)
	{
		msaaSamples = vk::SampleCountFlagBits::e2;
	}
	else
	{
		msaaSamples = vk::SampleCountFlagBits::e1;
	}
	vk::SampleCountFlagBits maxUsableSampleCount = getMaxUsableSampleCount(physDeviceProps);
	if (static_cast<std::underlying_type<vk::SampleCountFlagBits>::type>(maxUsableSampleCount) < static_cast<std::underlying_type<vk::SampleCountFlagBits>::type>(msaaSamples))
	{
		msaaSamples = maxUsableSampleCount;
	}
	debug(LOG_3D, "Using sample count: %s", to_string(msaaSamples).c_str());

	try {
		debugInfo.Output_SurfaceInformation(physicalDevice, surface, vkDynLoader);
	}
	catch (const vk::SystemError& e)
	{
		// catch any weird errors that debugInfo didn't handle
		debug(LOG_ERROR, "Output_SurfaceInformation failed: %s", e.what());
	}

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

gfx_api::pixel_format_usage::flags VkRoot::getPixelFormatUsageSupport(gfx_api::pixel_format format) const
{
	gfx_api::pixel_format_usage::flags retVal = gfx_api::pixel_format_usage::none;
	ASSERT_OR_RETURN(retVal, physicalDevice, "Physical device is null");

	vk::Format vulkan_format = get_format(format);
	// TODO: Handle error getting format?

	bool checkTransferDstBit = canUseVulkanDeviceAPI((uint32_t)VK_MAKE_VERSION(1, 1, 0));

	// [sampled_image support]
	// NOTE:
	//	- For WZ, the following feature bits are required for sampled_image usage:
	//		- VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT (of course)
	//		- VK_FORMAT_FEATURE_BLIT_SRC_BIT (may not technically be needed yet? but is generally available when the former and latter ones are)
	//		- VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT (needed for many textures - we don't currently differentiate)
	//	- And if Vulkan >= 1.1, check VK_FORMAT_FEATURE_TRANSFER_DST_BIT to verify we can actually copy from the staging buffer to the image
	vk::FormatFeatureFlags sampledImageFormatFlags = vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eBlitSrc | vk::FormatFeatureFlagBits::eSampledImageFilterLinear;
	if (checkTransferDstBit)
	{
		sampledImageFormatFlags |= vk::FormatFeatureFlagBits::eTransferDst;
	}
	if (checkFormatSupport(physicalDevice, vulkan_format, vk::ImageTiling::eOptimal, sampledImageFormatFlags, vkDynLoader))
	{
		retVal |= gfx_api::pixel_format_usage::sampled_image;
	}

	// [storage_image support]
	//	- Following feature bits are required:
	//		- VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT (of course)
	//		- TODO: Will we need to check any other bits for WZ's use-cases?
	if (checkFormatSupport(physicalDevice, vulkan_format, vk::ImageTiling::eLinear /* ? */, vk::FormatFeatureFlagBits::eStorageImage, vkDynLoader))
	{
		retVal |= gfx_api::pixel_format_usage::storage_image;
	}

	// [depth_stencil_attachment support]
	//	- Following feature bits are required:
	//		- VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	if (checkFormatSupport(physicalDevice, vulkan_format, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment, vkDynLoader))
	{
		retVal |= gfx_api::pixel_format_usage::depth_stencil_attachment;
	}

	return retVal;
}

void VkRoot::initPixelFormatsSupport()
{
	ASSERT(backend_impl, "Backend implementation is null");
	ASSERT(inst, "Instance is null");
	ASSERT(physicalDevice, "Physical device is null");

	// set any existing entries to false
	for (size_t i = 0; i < texture2DFormatsSupport.size(); i++)
	{
		texture2DFormatsSupport[i] = gfx_api::pixel_format_usage::none;
	}
	texture2DFormatsSupport.resize(static_cast<size_t>(gfx_api::MAX_PIXEL_FORMAT) + 1, gfx_api::pixel_format_usage::none);

	#define PIXEL_FORMAT_SUPPORT_SET(x) \
	texture2DFormatsSupport[static_cast<size_t>(x)] = getPixelFormatUsageSupport(x);

	// these uncompressed formats should be mandatory
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8)
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8)
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG8_UNORM)
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R8_UNORM)

	// RGB8 is optional
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8)

	// S3TC
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM) // DXT1
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM) // DXT3
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM) // DXT5

	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R_BC4_UNORM)
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG_BC5_UNORM)

	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM)

	// ETC2
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB8_ETC2)
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC)
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R11_EAC)
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG11_EAC)

	// ASTC (LDR)
	PIXEL_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM)
}

bool VkRoot::createSurface()
{
	ASSERT(backend_impl, "Backend implementation is null");
	ASSERT(inst, "Instance is null");

	VkSurfaceKHR _surface = VK_NULL_HANDLE;
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
	ASSERT_OR_RETURN(, queueFamilyIndices.isComplete(), "Did not receive complete indices from findQueueFamilies");

	// check for optional features of queue family
	const auto queuesFamilies = physicalDevice.getQueueFamilyProperties(vkDynLoader);
	uint32_t graphicsFamilyIdx = queueFamilyIndices.graphicsFamily.value();
	queueSupportsTimestamps = false;
	ASSERT_OR_RETURN(, graphicsFamilyIdx < queuesFamilies.size(), "Failed to determine queue (%" PRIu32")'s timestampValidBits", graphicsFamilyIdx);
	queueSupportsTimestamps = (queuesFamilies[graphicsFamilyIdx].timestampValidBits > 0);
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
	debug(LOG_3D, "Using queue family indicies: <graphics: %" PRIu32">, <presentation: %" PRIu32">", queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value());
	std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};
	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		auto queueCreateInfo = vk::DeviceQueueCreateInfo()
			.setQueueFamilyIndex(queueFamily)
			.setQueueCount(1)
			.setPQueuePriorities(&queuePriority);

		queueCreateInfos.push_back(queueCreateInfo);
	}

	ASSERT(physDeviceFeatures.samplerAnisotropy, "samplerAnisotropy is required, but not available");
	const auto enabledFeatures = vk::PhysicalDeviceFeatures()
								.setSamplerAnisotropy(true)
								.setDepthBiasClamp(physDeviceFeatures.depthBiasClamp);
	debug(LOG_3D, "With features config: samplerAnisotropy(%d), depthBiasClamp(%d)", (int)enabledFeatures.samplerAnisotropy, (int)enabledFeatures.depthBiasClamp);

	std::string layersAsString;
	std::for_each(layers.begin(), layers.end(), [&layersAsString](const char *layer) {
		layersAsString += std::string(layer) + ",";
	});
	debug(LOG_3D, "Using layers: %s", layersAsString.c_str());
	std::string deviceExtensionsAsString;
	std::for_each(enabledDeviceExtensions.begin(), enabledDeviceExtensions.end(), [&deviceExtensionsAsString](const char *extension) {
		deviceExtensionsAsString += std::string(extension) + ",";
	});
	debug(LOG_3D, "Using device extensions: %s", deviceExtensionsAsString.c_str());

	const auto deviceCreateInfo = vk::DeviceCreateInfo()
		.setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
		.setPpEnabledLayerNames(layers.data())
		.setEnabledExtensionCount(static_cast<uint32_t>(enabledDeviceExtensions.size()))
		.setPpEnabledExtensionNames(enabledDeviceExtensions.data())
		.setPEnabledFeatures(&enabledFeatures)
		.setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
		.setPQueueCreateInfos(queueCreateInfos.data());

	try {
		dev = physicalDevice.createDevice(deviceCreateInfo, nullptr, vkDynLoader);
	}
	catch (const vk::SystemError& e)
	{
		debug(LOG_ERROR, "vk::PhysicalDevice::createDevice: failed with error: %s", e.what());
		// TODO: Any cleanup here?
		return false;
	}
	catch (std::exception& e)
	{
		debug(LOG_ERROR, "vk::PhysicalDevice::createDevice: failed with unexpected exception: %s", e.what());
		// TODO: Any cleanup here?
		return false;
	}

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
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
	vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkDynLoader.vkGetBufferMemoryRequirements2KHR;
	vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkDynLoader.vkGetImageMemoryRequirements2KHR;
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
	vulkanFunctions.vkBindBufferMemory2KHR = vkDynLoader.vkBindBufferMemory2KHR;
	vulkanFunctions.vkBindImageMemory2KHR = vkDynLoader.vkBindImageMemory2KHR;
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
	vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkDynLoader.vkGetPhysicalDeviceMemoryProperties2KHR;
#endif

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = dev;
	allocatorInfo.instance = inst;
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
	ASSERT(offset <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "offset (%zu) exceeds uint32_t max", offset);
	ASSERT(count <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "count (%zu) exceeds uint32_t max", count);
	buffering_mechanism::get_current_resources().cmdDraw.draw(static_cast<uint32_t>(count), 1, static_cast<uint32_t>(offset), 0, vkDynLoader);
}

void VkRoot::draw_instanced(const std::size_t& offset, const std::size_t &count, const gfx_api::primitive_type &primitive, std::size_t instance_count)
{
	ASSERT(offset <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "offset (%zu) exceeds uint32_t max", offset);
	ASSERT(count <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "count (%zu) exceeds uint32_t max", count);
	buffering_mechanism::get_current_resources().cmdDraw.draw(static_cast<uint32_t>(count), static_cast<uint32_t>(instance_count), static_cast<uint32_t>(offset), 0, vkDynLoader);
}

void VkRoot::draw_elements(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type&, const gfx_api::index_type&)
{
	ASSERT_OR_RETURN(, currentPSO != nullptr, "currentPSO == NULL");
	ASSERT(offset <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "offset (%zu) exceeds uint32_t max", offset);
	ASSERT(count <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "count (%zu) exceeds uint32_t max", count);
	buffering_mechanism::get_current_resources().cmdDraw.drawIndexed(static_cast<uint32_t>(count), 1, static_cast<uint32_t>(offset) >> 2, 0, 0, vkDynLoader);
}

void VkRoot::draw_elements_instanced(const std::size_t& offset, const std::size_t &count, const gfx_api::primitive_type &primitive, const gfx_api::index_type& index, std::size_t instance_count)
{
	ASSERT_OR_RETURN(, currentPSO != nullptr, "currentPSO == NULL");
	ASSERT(offset <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "offset (%zu) exceeds uint32_t max", offset);
	ASSERT(count <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "count (%zu) exceeds uint32_t max", count);
	buffering_mechanism::get_current_resources().cmdDraw.drawIndexed(static_cast<uint32_t>(count), static_cast<uint32_t>(instance_count), static_cast<uint32_t>(offset) >> 2, 0, 0, vkDynLoader);
}

void VkRoot::bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset)
{
	ASSERT_OR_RETURN(, currentPSO != nullptr, "currentPSO == NULL");
	std::vector<vk::Buffer> buffers;
	std::vector<VkDeviceSize> offsets;
	for (const auto &input : vertex_buffers_offset)
	{
		// null vertex buffers are not supported
		ASSERT(std::get<0>(input) != nullptr, "bind_vertex_buffers called with null vertex buffer");
		ASSERT(static_cast<const VkBuf *>(std::get<0>(input))->usage == gfx_api::buffer::usage::vertex_buffer, "bind_vertex_buffers called with non-vertex-buffer");
		buffers.push_back(static_cast<const VkBuf *>(std::get<0>(input))->object);
		offsets.push_back(std::get<1>(input));
	}
	ASSERT(first <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "first (%zu) exceeds uint32_t max", first);
	buffering_mechanism::get_current_resources().cmdDraw.bindVertexBuffers(static_cast<uint32_t>(first), buffers, offsets, vkDynLoader);
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
	ASSERT_OR_RETURN(, currentPSO != nullptr, "currentPSO == NULL");
	ASSERT(size > 0, "bind_streamed_vertex_buffers called with size 0");
	ASSERT(size <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "size (%zu) exceeds uint32_t max", size);
	auto& frameResources = buffering_mechanism::get_current_resources();
	const auto streamedMemory = frameResources.streamedVertexBufferAllocator.alloc(static_cast<uint32_t>(size), 16);
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
	const auto buffers = dev.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo()
		.setCommandPool(buffering_mechanism::perFrameResources[0]->pool)
		.setCommandBufferCount(1)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		, vkDynLoader
	);
	vk::CommandBuffer internalCommandBuffer = buffers[0];
	internalCommandBuffer.begin(
		vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		, vkDynLoader
	);

	//
	const auto imageMemoryBarriers_ColorImage = std::array<vk::ImageMemoryBarrier, 1> {
		vk::ImageMemoryBarrier()
			.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setImage(colorImage)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
	};
	internalCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
		vk::DependencyFlagBits(), nullptr, nullptr, imageMemoryBarriers_ColorImage, vkDynLoader);

	//
	const auto imageMemoryBarriers_DepthStencilImage = std::array<vk::ImageMemoryBarrier, 1> {
		vk::ImageMemoryBarrier()
			.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setImage(depthStencilImage)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1))
			.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
	};
	internalCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
		vk::DependencyFlagBits(), nullptr, nullptr, imageMemoryBarriers_DepthStencilImage, vkDynLoader);

	internalCommandBuffer.end(vkDynLoader);

	try {
		graphicsQueue.submit(
			vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&internalCommandBuffer),
			vk::Fence()
			, vkDynLoader);
	}
	catch (const vk::OutOfHostMemoryError& e)
	{
		debug(LOG_ERROR, "vk::Queue::submit: OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e)
	{
		debug(LOG_ERROR, "vk::Queue::submit: OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}
	catch (const vk::DeviceLostError& e)
	{
		debug(LOG_ERROR, "vk::Queue::submit: DeviceLostError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorDeviceLost);
	}
	catch (const vk::SystemError& e)
	{
		debug(LOG_FATAL, "vk::Queue::submit: unhandled error: %s", e.what());
	}

	try {
		graphicsQueue.waitIdle(vkDynLoader);
	}
	catch (const vk::OutOfHostMemoryError& e)
	{
		debug(LOG_ERROR, "vk::Queue::waitIdle: OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e)
	{
		debug(LOG_ERROR, "vk::Queue::waitIdle: OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}
	catch (const vk::DeviceLostError& e)
	{
		debug(LOG_ERROR, "vk::Queue::waitIdle: DeviceLostError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorDeviceLost);
	}
	catch (const vk::SystemError& e)
	{
		debug(LOG_FATAL, "vk::Queue::waitIdle: unhandled error: %s", e.what());
	}
}

vk::Format VkRoot::get_format(const gfx_api::pixel_format& format) const
{
	switch (format)
	{
	case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
		return vk::Format::eR8G8B8A8Unorm;
	case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
		return vk::Format::eB8G8R8A8Unorm;
	case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
		return vk::Format::eR8G8B8Unorm;
	case gfx_api::pixel_format::FORMAT_RG8_UNORM:
		return vk::Format::eR8G8Unorm;
	case gfx_api::pixel_format::FORMAT_R8_UNORM:
		return vk::Format::eR8Unorm;
	case gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM:
		return vk::Format::eBc1RgbUnormBlock;
	case gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM:
		return vk::Format::eBc2UnormBlock;
	case gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM:
		return vk::Format::eBc3UnormBlock;
	case gfx_api::pixel_format::FORMAT_R_BC4_UNORM:
		return vk::Format::eBc4UnormBlock;
	case gfx_api::pixel_format::FORMAT_RG_BC5_UNORM:
		return vk::Format::eBc5UnormBlock;
	case gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM:
		return vk::Format::eBc7UnormBlock;
	case gfx_api::pixel_format::FORMAT_RGB8_ETC2:
		return vk::Format::eEtc2R8G8B8UnormBlock;
	case gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC:
		return vk::Format::eEtc2R8G8B8A8UnormBlock;
	case gfx_api::pixel_format::FORMAT_R11_EAC:
		return vk::Format::eEacR11UnormBlock;
	case gfx_api::pixel_format::FORMAT_RG11_EAC:
		return vk::Format::eEacR11G11UnormBlock;
	case gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM:
		return vk::Format::eAstc4x4UnormBlock;
	case gfx_api::pixel_format::FORMAT_RGB8_ETC1:
		// Not supported!
	default:
		debug(LOG_FATAL, "Unsupported format: %d", (int)format);
	}
	throw;
}

bool VkRoot::textureFormatIsSupported(gfx_api::pixel_format_target target, gfx_api::pixel_format format, gfx_api::pixel_format_usage::flags usage)
{
	size_t formatIdx = static_cast<size_t>(format);
	ASSERT_OR_RETURN(false, formatIdx < texture2DFormatsSupport.size(), "Invalid format index: %zu", formatIdx);
	return (texture2DFormatsSupport[formatIdx] & usage) == usage;
}

bool VkRoot::supportsInstancedRendering()
{
	return true;
}

gfx_api::texture* VkRoot::create_texture(const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const gfx_api::pixel_format& internal_format, const std::string& filename)
{
	auto result = new VkTexture(*this, mipmap_count, width, height, internal_format, filename);
	return result;
}

gfx_api::buffer* VkRoot::create_buffer_object(const gfx_api::buffer::usage &usage, const buffer_storage_hint& hint /*= buffer_storage_hint::static_draw*/)
{
	return new VkBuf(dev, usage, *this);
}

std::vector<vk::DescriptorSet> VkRoot::allocateDescriptorSet(vk::DescriptorSetLayout arg, vk::DescriptorType descriptorType, uint32_t numDescriptors)
{
	const auto descriptorSet = std::array<vk::DescriptorSetLayout, 1>{ arg };
	buffering_mechanism::get_current_resources().numalloc++;
	return dev.allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(buffering_mechanism::get_current_resources().getDescriptorPool(1, descriptorType, numDescriptors))
			.setPSetLayouts(descriptorSet.data())
			.setDescriptorSetCount(static_cast<uint32_t>(descriptorSet.size()))
	, vkDynLoader);
}

std::vector<vk::DescriptorSet> VkRoot::allocateDescriptorSets(std::vector<vk::DescriptorSetLayout> args, vk::DescriptorType descriptorType, uint32_t numDescriptors)
{
	buffering_mechanism::get_current_resources().numalloc++;
	return dev.allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(buffering_mechanism::get_current_resources().getDescriptorPool(static_cast<uint32_t>(args.size()), descriptorType, numDescriptors))
			.setPSetLayouts(args.data())
			.setDescriptorSetCount(static_cast<uint32_t>(args.size()))
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
	ASSERT_OR_RETURN(, currentPSO != nullptr, "currentPSO == NULL");
	auto& casted_buf = static_cast<VkBuf&>(index_buffer);
	ASSERT(casted_buf.usage == gfx_api::buffer::usage::index_buffer, "Passed gfx_api::buffer is not an index buffer");
	buffering_mechanism::get_current_resources().cmdDraw.bindIndexBuffer(casted_buf.object, 0, to_vk(index), vkDynLoader);
}

void VkRoot::unbind_index_buffer(gfx_api::buffer&)
{
	// ?
}

void VkRoot::bind_textures(const std::vector<gfx_api::texture_input>& attribute_descriptions, const std::vector<gfx_api::abstract_texture*>& textures)
{
	ASSERT_OR_RETURN(, currentPSO != nullptr, "currentPSO == NULL");
	ASSERT(textures.size() <= attribute_descriptions.size(), "Received more textures than expected");
	ASSERT(textures.size() <= std::numeric_limits<uint32_t>::max(), "Too many textures: %zu", textures.size());

	const auto set = allocateDescriptorSet(currentPSO->textures_set_layout, vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(textures.size()));

	auto image_descriptor = std::vector<vk::DescriptorImageInfo>{};
	for (auto* texture : textures)
	{
		image_descriptor.emplace_back(vk::DescriptorImageInfo()
			.setImageView(texture != nullptr ? *static_cast<VkTexture*>(texture)->view : *pDefaultTexture->view)
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
	}
	uint32_t i = 0;
	auto write_info = std::vector<vk::WriteDescriptorSet>{};
	for (auto* texture : textures)
	{
		(void)texture; // silence unused variable warning
		write_info.emplace_back(
			vk::WriteDescriptorSet()
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDstSet(set[0])
				.setPImageInfo(&image_descriptor[i])
				.setDstBinding(i)
		);
		i++;
	}
	dev.updateDescriptorSets(write_info, nullptr, vkDynLoader);
	buffering_mechanism::get_current_resources().cmdDraw.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, currentPSO->layout, currentPSO->textures_first_set, set, nullptr, vkDynLoader);
}

void VkRoot::set_constants(const void* buffer, const std::size_t& size)
{
	set_uniforms_set(0, buffer, size);
}

void VkRoot::set_uniforms_set(const size_t& uniform_set, const void* buffer, size_t size)
{
	ASSERT_OR_RETURN(, currentPSO != nullptr, "currentPSO == NULL");
	ASSERT(size <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "size (%zu) exceeds uint32_t max", size);
	const auto stagingMemory = buffering_mechanism::get_current_resources().uniformBufferAllocator.alloc(static_cast<uint32_t>(size), physDeviceProps.limits.minUniformBufferOffsetAlignment);
	void * pDynamicUniformBufferMapped = buffering_mechanism::get_current_resources().uniformBufferAllocator.mapMemory(stagingMemory);
	memcpy(reinterpret_cast<uint8_t*>(pDynamicUniformBufferMapped), buffer, size);

	const auto bufferInfo = vk::DescriptorBufferInfo(stagingMemory.buffer, 0, size);

	vk::DescriptorSet descSet;
	auto perFrame_perPSO_dynamicUniformDescriptorSets = buffering_mechanism::get_current_resources().perPSO_dynamicUniformBufferDescriptorSets.find(currentPSO);
	if ((perFrame_perPSO_dynamicUniformDescriptorSets != buffering_mechanism::get_current_resources().perPSO_dynamicUniformBufferDescriptorSets.end())
		&& (perFrame_perPSO_dynamicUniformDescriptorSets->second.size() > uniform_set))
	{
		auto &uniformSetDescriptorSets = perFrame_perPSO_dynamicUniformDescriptorSets->second[uniform_set];
		if (uniformSetDescriptorSets.has_value() && uniformSetDescriptorSets.value().first == bufferInfo)
		{
			descSet = uniformSetDescriptorSets.value().second;
		}
	}

	if (!descSet)
	{
		auto sets = allocateDescriptorSet(currentPSO->cbuffer_set_layout[uniform_set], vk::DescriptorType::eUniformBufferDynamic, 1);
		descSet = sets[0];
		const auto descriptorWrite = std::array<vk::WriteDescriptorSet, 1>{
			vk::WriteDescriptorSet()
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
				.setDstBinding(0)
				.setPBufferInfo(&bufferInfo)
				.setDstSet(descSet)
		};
		dev.updateDescriptorSets(descriptorWrite, nullptr, vkDynLoader);
		if (perFrame_perPSO_dynamicUniformDescriptorSets == buffering_mechanism::get_current_resources().perPSO_dynamicUniformBufferDescriptorSets.end())
		{
			auto result = buffering_mechanism::get_current_resources().perPSO_dynamicUniformBufferDescriptorSets.insert(perFrameResources_t::PerPSODynamicUniformBufferDescriptorSets::value_type(currentPSO, std::vector<optional<perFrameResources_t::DynamicUniformBufferDescriptorSets>>()));
			perFrame_perPSO_dynamicUniformDescriptorSets = result.first;
		}
		perFrame_perPSO_dynamicUniformDescriptorSets->second.resize(currentPSO->cbuffer_set_layout.size());
		perFrame_perPSO_dynamicUniformDescriptorSets->second[uniform_set] = perFrameResources_t::DynamicUniformBufferDescriptorSets( bufferInfo, descSet);
	}
	const auto dynamicOffsets = std::array<uint32_t, 1> { stagingMemory.offset };
	buffering_mechanism::get_current_resources().cmdDraw.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, currentPSO->layout, static_cast<uint32_t>(uniform_set), descSet, dynamicOffsets, vkDynLoader);
}

void VkRoot::set_uniforms(const size_t& first, const std::vector<std::tuple<const void*, size_t>>& uniform_blocks)
{
	ASSERT_OR_RETURN(, currentPSO != nullptr, "currentPSO == NULL");
	for (size_t i = 0, e = uniform_blocks.size(); i < e && (first + i) < currentPSO->cbuffer_set_layout.size(); ++i)
	{
		auto* buffer = std::get<0>(uniform_blocks[i]);
		if (buffer == nullptr)
		{
			continue;
		}
		set_uniforms_set((first + i), buffer, std::get<1>(uniform_blocks[i]));
	}
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
		acquireNextImageResult = dev.acquireNextImageKHR(swapchain, -1, buffering_mechanism::get_current_swapchain_resources().imageAcquireSemaphore, vk::Fence(), vkDynLoader);
	}
	catch (const vk::OutOfDateKHRError&)
	{
		debug(LOG_3D, "vk::Device::acquireNextImageKHR: ErrorOutOfDateKHR - must recreate swapchain");
		if (createNewSwapchainAndSwapchainSpecificStuff(vk::Result::eErrorOutOfDateKHR))
		{
			return AcquireNextSwapchainImageResult::eRecoveredFromError;
		}
		return AcquireNextSwapchainImageResult::eUnhandledFailure;
	}
	catch (const vk::SurfaceLostKHRError&)
	{
		debug(LOG_3D, "vk::Device::acquireNextImageKHR: ErrorSurfaceLostKHR - must recreate surface + swapchain");
		// recreate surface + swapchain
		if (handleSurfaceLost())
		{
			return AcquireNextSwapchainImageResult::eRecoveredFromError;
		}
		return AcquireNextSwapchainImageResult::eUnhandledFailure;
	}
	catch (const vk::SystemError& e)
	{
		debug(LOG_ERROR, "vk::Device::acquireNextImageKHR: unhandled error: %s", e.what());
		return AcquireNextSwapchainImageResult::eUnhandledFailure;
	}
	if(acquireNextImageResult.result == vk::Result::eSuboptimalKHR)
	{
		debug(LOG_3D, "vk::Device::acquireNextImageKHR returned eSuboptimalKHR - should probably recreate swapchain (in the future)");
	}

	currentSwapchainIndex = acquireNextImageResult.value;
	return AcquireNextSwapchainImageResult::eSuccess;
}

void VkRoot::beginRenderPass()
{
	if (startedRenderPass)
	{
		return; // don't double-start the render pass
	}
	startRenderPass();
}

void VkRoot::endRenderPass()
{
	frameNum = std::max<size_t>(frameNum + 1, 1);

	currentPSO = nullptr;
	buffering_mechanism::get_current_resources().cmdDraw.endRenderPass(vkDynLoader);
	buffering_mechanism::get_current_resources().cmdDraw.end(vkDynLoader);

	startedRenderPass = false;

	// Add memory barrier at end of cmdCopy
	const auto memoryBarriers = std::array<vk::MemoryBarrier, 1> {
		vk::MemoryBarrier()
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eUniformRead )
	};

	buffering_mechanism::get_current_resources().cmdCopy.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
																		 vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eVertexShader,
																		 vk::DependencyFlagBits(), memoryBarriers, nullptr, nullptr, vkDynLoader);

	buffering_mechanism::get_current_resources().cmdCopy.end(vkDynLoader);

	buffering_mechanism::get_current_resources().uniformBufferAllocator.flushAutomappedMemory();
	buffering_mechanism::get_current_resources().uniformBufferAllocator.unmapAutomappedMemory();
	buffering_mechanism::get_current_resources().streamedVertexBufferAllocator.flushAutomappedMemory();
	buffering_mechanism::get_current_resources().streamedVertexBufferAllocator.unmapAutomappedMemory();

	bool mustSkipDrawing = !shouldDraw();
	bool mustRecreateSwapchain = false;
	int w, h;
	backend_impl->getDrawableSize(&w, &h);
	if (w != (int)swapchainSize.width || h != (int)swapchainSize.height)
	{
		// Ignore graphics instructions this time around (as there are issues on certain drivers like MoltenVK)
		// but *must* still submit the cmdCopy CommandBuffer
		mustSkipDrawing = true;

		if (w > 0 || h > 0 || swapchainSize.width > 1 || swapchainSize.height > 1)
		{
			// Must re-create swapchain
			debug(LOG_3D, "[1] Drawable size (%d x %d) does not match swapchainSize (%d x %d) - must re-create swapchain", w, h, (int)swapchainSize.width, (int)swapchainSize.height);
			mustRecreateSwapchain = true;
		}
	}

	const auto executableCmdBuffer = std::array<vk::CommandBuffer, 2>{buffering_mechanism::get_current_resources().cmdCopy, buffering_mechanism::get_current_resources().cmdDraw}; // copy before render
	const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput; //vk::PipelineStageFlagBits::eAllCommands;

	auto submitInfo = vk::SubmitInfo()
		// if mustSkipDrawing, only submit the cmdCopy buffer
		.setCommandBufferCount((!mustSkipDrawing) ? static_cast<uint32_t>(executableCmdBuffer.size()) : 1)
		.setPCommandBuffers(executableCmdBuffer.data());

	if (!mustSkipDrawing)
	{
		submitInfo
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&buffering_mechanism::get_current_swapchain_resources().imageAcquireSemaphore)
			.setPWaitDstStageMask(&waitStage);
	}

	auto presentInfo = vk::PresentInfoKHR()
		.setPSwapchains(&swapchain)
		.setSwapchainCount(1)
		.setPImageIndices(&currentSwapchainIndex);

	if ((graphicsQueue != presentQueue) && !mustSkipDrawing)
	{
		// for handling separate graphics and presentation queues
		submitInfo
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(&buffering_mechanism::get_current_swapchain_resources().renderFinishedSemaphore);

		// for handling separate graphics and presentation queues
		presentInfo
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&buffering_mechanism::get_current_swapchain_resources().renderFinishedSemaphore);
	}

	try {
		graphicsQueue.submit(submitInfo, buffering_mechanism::get_current_resources().previousSubmission, vkDynLoader);
	}
	catch (const vk::OutOfHostMemoryError& e)
	{
		debug(LOG_ERROR, "vk::Queue::submit: OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e)
	{
		debug(LOG_ERROR, "vk::Queue::submit: OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}
	catch (const vk::DeviceLostError& e)
	{
		debug(LOG_ERROR, "vk::Queue::submit: DeviceLostError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorDeviceLost);
	}
	catch (const vk::SystemError& e)
	{
		debug(LOG_FATAL, "vk::Queue::submit: unhandled error: %s", e.what());
		auto resultErr = static_cast<vk::Result>(e.code().value());
		handleUnrecoverableError(resultErr);
	}

	if (mustRecreateSwapchain)
	{
		createNewSwapchainAndSwapchainSpecificStuff(vk::Result::eErrorOutOfDateKHR);
		return; // end processing this flip
	}

	if (!mustSkipDrawing)
	{
		vk::Result presentResult;
		try {
			presentResult = presentQueue.presentKHR(presentInfo, vkDynLoader);
		}
		catch (const vk::OutOfDateKHRError&)
		{
			debug(LOG_3D, "vk::Queue::presentKHR: ErrorOutOfDateKHR - must recreate swapchain");
			createNewSwapchainAndSwapchainSpecificStuff(vk::Result::eErrorOutOfDateKHR);
			return; // end processing this flip
		}
		catch (const vk::SurfaceLostKHRError&)
		{
			debug(LOG_3D, "vk::Queue::presentKHR: ErrorSurfaceLostKHR - must recreate surface + swapchain");
			// recreate surface + swapchain
			handleSurfaceLost();
			return; // end processing this flip
		}
		catch (const vk::SystemError& e)
		{
			debug(LOG_FATAL, "vk::Queue::presentKHR: unhandled error: %s", e.what());
			presentResult = vk::Result::eErrorUnknown;
		}
		if(presentResult == vk::Result::eSuboptimalKHR)
		{
			debug(LOG_3D, "presentKHR returned eSuboptimalKHR (%d) - should probably recreate swapchain (in the future)", (int)presentResult);
		}
	}

	try {
		buffering_mechanism::swap(dev, vkDynLoader, mustSkipDrawing); // must be called *before* acquireNextSwapchainImage()
	}
	catch (const vk::OutOfHostMemoryError& e)
	{
		debug(LOG_ERROR, "buffering swap: OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfHostMemory);
	}
	catch (const vk::OutOfDeviceMemoryError& e)
	{
		debug(LOG_ERROR, "buffering swap: OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorOutOfDeviceMemory);
	}
	catch (const vk::DeviceLostError& e)
	{
		debug(LOG_ERROR, "buffering swap: DeviceLostError: %s", e.what());
		handleUnrecoverableError(vk::Result::eErrorDeviceLost);
	}
	catch (const vk::SystemError& e)
	{
		debug(LOG_FATAL, "buffering swap: unhandled error: %s", e.what());
		auto resultErr = static_cast<vk::Result>(e.code().value());
		handleUnrecoverableError(resultErr);
	}

	if (!mustSkipDrawing)
	{
		if (acquireNextSwapchainImage() != AcquireNextSwapchainImageResult::eSuccess)
		{
			return; // end processing this flip
		}

		backend_impl->getDrawableSize(&w, &h);
		if (w != (int)swapchainSize.width || h != (int)swapchainSize.height)
		{
			if (w > 0 || h > 0 || swapchainSize.width > 1 || swapchainSize.height > 1)
			{
				// Must re-create swapchain
				debug(LOG_3D, "[3] Drawable size (%d x %d) does not match swapchainSize (%d x %d) - re-create swapchain", w, h, (int)swapchainSize.width, (int)swapchainSize.height);
				createNewSwapchainAndSwapchainSpecificStuff(vk::Result::eErrorOutOfDateKHR);
				return; // end processing this flip
			}
		}
	}
	else
	{
		// since we skipped drawing, don't bother acquiring a new swapchain image
		// however, to avoid endless CPU drain, add a delay in here
		const uint32_t minFrameInterval = 1000 / 120; // limit to approx 120 FPS
		uint32_t renderPassEndTime = wzGetTicks();
		const uint32_t frameTime = renderPassEndTime - lastRenderPassEndTime;
		if (frameTime < minFrameInterval)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(minFrameInterval - frameTime));
			renderPassEndTime = wzGetTicks();
		}
		lastRenderPassEndTime = renderPassEndTime;
	}

	buffering_mechanism::get_current_resources().cmdCopy.begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit), vkDynLoader);
}

void VkRoot::startRenderPass()
{
	buffering_mechanism::get_current_resources().cmdDraw.begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit), vkDynLoader);

	const auto clearValue = std::array<vk::ClearValue, 2> {
		vk::ClearValue(), vk::ClearValue(vk::ClearDepthStencilValue(1.f, 0u))
	};
	buffering_mechanism::get_current_resources().cmdDraw.beginRenderPass(
		vk::RenderPassBeginInfo()
		.setFramebuffer(fbo[currentSwapchainIndex])
		.setClearValueCount(static_cast<uint32_t>(clearValue.size()))
		.setPClearValues(clearValue.data())
		.setRenderPass(rp)
		.setRenderArea(vk::Rect2D(vk::Offset2D(), swapchainSize)),
		vk::SubpassContents::eInline,
		vkDynLoader);
	const auto viewports = std::array<vk::Viewport, 1> {
		vk::Viewport().setHeight(swapchainSize.height).setWidth(swapchainSize.width).setMinDepth(0.f).setMaxDepth(1.f)
	};
	buffering_mechanism::get_current_resources().cmdDraw.setViewport(0, viewports, vkDynLoader);
	const auto scissors = std::array<vk::Rect2D, 1> {
		vk::Rect2D().setExtent(swapchainSize)
	};
	buffering_mechanism::get_current_resources().cmdDraw.setScissor(0, scissors, vkDynLoader);

	startedRenderPass = true;
}

void VkRoot::set_polygon_offset(const float& offset, const float& slope)
{
	buffering_mechanism::get_current_resources().cmdDraw.setDepthBias(offset, (physDeviceFeatures.depthBiasClamp) ? 1.0f : 0.f, slope, vkDynLoader);
}

void VkRoot::set_depth_range(const float& min, const float& max)
{
	const auto viewports = std::array<vk::Viewport, 1> {
		vk::Viewport().setHeight(swapchainSize.height).setWidth(swapchainSize.width).setMinDepth(min).setMaxDepth(max)
	};
	buffering_mechanism::get_current_resources().cmdDraw.setViewport(0, viewports, vkDynLoader);
}

int32_t VkRoot::get_context_value(const gfx_api::context::context_value property)
{
	switch(property)
	{
		case gfx_api::context::context_value::MAX_ELEMENTS_VERTICES:
			return 32000;
		case gfx_api::context::context_value::MAX_ELEMENTS_INDICES:
			return 32000;
		case gfx_api::context::context_value::MAX_TEXTURE_SIZE:
			return physDeviceProps.limits.maxImageDimension2D;
		case gfx_api::context::context_value::MAX_SAMPLES:
			// support MSAA: https://vulkan-tutorial.com/Multisampling
			return static_cast<std::underlying_type<vk::SampleCountFlagBits>::type>(getMaxUsableSampleCount(physDeviceProps));
		case gfx_api::context::context_value::MAX_ARRAY_TEXTURE_LAYERS:
			return physDeviceProps.limits.maxImageArrayLayers;
		case gfx_api::context::context_value::MAX_VERTEX_ATTRIBS:
			return physDeviceProps.limits.maxVertexInputAttributes;
		case gfx_api::context::context_value::MAX_VERTEX_OUTPUT_COMPONENTS:
			return std::min(physDeviceProps.limits.maxVertexOutputComponents, physDeviceProps.limits.maxFragmentInputComponents);
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
	backendGameInfo["vulkan_deviceName"] = static_cast<const char*>(physDeviceProps.deviceName);
	backendGameInfo["vulkan_apiversion"] = VkhInfo::vulkan_apiversion_to_string(physDeviceProps.apiVersion);
	backendGameInfo["vulkan_driverversion"] = std::to_string(physDeviceProps.driverVersion);
	return backendGameInfo;
}

const std::string& VkRoot::getFormattedRendererInfoString() const
{
	return formattedRendererInfoString;
}

std::string VkRoot::calculateFormattedRendererInfoString() const
{
	return std::string("Vulkan ") + VkhInfo::vulkan_apiversion_to_string(physDeviceProps.apiVersion) + " (" + static_cast<const char*>(physDeviceProps.deviceName) + ")";
}

bool VkRoot::getScreenshot(std::function<void (std::unique_ptr<iV_Image>)> callback)
{
	// TODO: Implement - save the callback, and trigger a screenshot save at the next opportunity
	// saveScreenshotCallback = callback;
	return false;
}

const size_t& VkRoot::current_FrameNum() const
{
	return frameNum;
}

bool VkRoot::supportsMipLodBias() const
{
	return lodBiasMethod != LodBiasMethod::Unsupported;
}

size_t VkRoot::maxFramesInFlight() const
{
	return MAX_FRAMES_IN_FLIGHT;
}

#endif // defined(WZ_VULKAN_ENABLED)
