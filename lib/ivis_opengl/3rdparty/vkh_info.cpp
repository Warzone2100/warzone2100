//
// VkhInfo
// Version: 1.2
//
// Copyright (c) 2019-2020 past-due
//
// https://github.com/past-due/vulkan-helpers
//
// Distributed under the MIT License.
// See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT
//

#define VULKAN_HPP_TYPESAFE_CONVERSION 1
#include "vkh_info.hpp"

#include <sstream>

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
		// ASSERT(count <= results.size(), "Unexpected result: count (%" PRIu32"); results.size (%zu)", count, results.size());
		results.resize(count);
	}
	return results;
}

template <typename VKType, typename F, typename... Args>
std::vector<VKType> GetVectorFromVKFunc(F &&func, Args &&... args)
{
	return GetVectorFromVKFuncWithExplicitInit(func, VKType(), args...);
}

VkhInfo::VkhInfo(const outputHandlerFuncType& _outputHandler)
: outputHandler(_outputHandler)
{ }

void VkhInfo::setOutputHandler(const outputHandlerFuncType& _outputHandler)
{
	outputHandler = _outputHandler;
}

bool VkhInfo::getInstanceLayerProperties(std::vector<VkLayerProperties> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	if (!_vkGetInstanceProcAddr) return false;

	PFN_vkEnumerateInstanceLayerProperties _vkEnumerateInstanceLayerProperties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties")));
	if (!_vkEnumerateInstanceLayerProperties)
	{
		// Could not find symbol: vkEnumerateInstanceLayerProperties
		return false;
	}

	output = GetVectorFromVKFunc<VkLayerProperties>(_vkEnumerateInstanceLayerProperties);
	return true;
}

bool VkhInfo::getInstanceExtensions(const char * layerName, std::vector<VkExtensionProperties> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	if (!_vkGetInstanceProcAddr) return false;

	PFN_vkEnumerateInstanceExtensionProperties _vkEnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties")));
	if (!_vkEnumerateInstanceExtensionProperties)
	{
		// Could not find symbol: vkEnumerateInstanceExtensionProperties
		return false;
	}

	output = GetVectorFromVKFunc<VkExtensionProperties>(_vkEnumerateInstanceExtensionProperties, layerName);
	return true;
}

bool VkhInfo::supportsInstanceExtension(const char * layerName, const char * extensionName, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	std::vector<VkExtensionProperties> extensions;
	if (!VkhInfo::getInstanceExtensions(layerName, extensions, _vkGetInstanceProcAddr))
	{
		return false;
	}

	return std::find_if(extensions.begin(), extensions.end(),
						[extensionName](const VkExtensionProperties& props) {
							return (strcmp(props.extensionName, extensionName) == 0);
						}) != extensions.end();
}

void VkhInfo::Output_InstanceLayerProperties(PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	std::stringstream buf;

	// Supported (global) instance extensions
	std::vector<VkExtensionProperties> supportedGlobalInstanceExtensions;
	if (VkhInfo::getInstanceExtensions(nullptr, supportedGlobalInstanceExtensions, _vkGetInstanceProcAddr))
	{
		buf << "Global Instance Extensions:\n";
		buf << "===========================\n";
		buf << "Count: " << supportedGlobalInstanceExtensions.size() << "\n";
		for (auto & extension : supportedGlobalInstanceExtensions)
		{
			buf << "  - " << extension.extensionName << " (version: " << extension.specVersion << ")\n";
		}
		buf << "\n";
	}
	else
	{
		// Failure to request supported extensions
		buf << "Failed to retrieve supported instance extensions\n";
	}

	// Instance layer properties
	std::vector<VkLayerProperties> layerProperties;
	if (VkhInfo::getInstanceLayerProperties(layerProperties, _vkGetInstanceProcAddr))
	{
		// Layer properties
		buf << "Instance Layers:\n";
		buf << "====================\n";
		buf << "Count: " << layerProperties.size() << "\n";

		for (size_t idx = 0; idx < layerProperties.size(); ++idx)
		{
			auto & layer = layerProperties[idx];

			buf << "[Layer " << idx << "]\n";
			buf << "- layerName: " << layer.layerName << "\n";
			buf << "- description: " << layer.description << "\n";
			buf << "- specVersion: " << VkhInfo::vulkan_apiversion_to_string(layer.specVersion) << "\n";
			buf << "- implementationVersion: " << layer.implementationVersion << "\n";

			// Supported instance extensions
			std::vector<VkExtensionProperties> supportedInstanceExtensions;
			if (VkhInfo::getInstanceExtensions(layer.layerName, supportedInstanceExtensions, _vkGetInstanceProcAddr))
			{
				buf << "- Instance Extensions:\n";
				buf << "  --------------------\n";
				buf << "  Count: " << supportedInstanceExtensions.size() << "\n";
				for (auto & extension : supportedInstanceExtensions)
				{
					buf << "  - " << extension.extensionName << " (version: " << extension.specVersion << ")\n";
				}
				buf << "\n";
			}
			else
			{
				// Failure to request supported extensions
				buf << "  Failed to retrieve supported layer instance extensions\n";
			}
		}
	}
	else
	{
		buf << "Failed to retrieve instance layer properties\n";
	}

	if (outputHandler)
	{
		outputHandler(buf.str());
	}
}

void VkhInfo::Output_SurfaceInformation(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	std::stringstream buf;

	buf << "Surface Information:\n";
	buf << "====================\n";

	// VkSurfaceCapabilitiesKHR
	const auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface, vkDynLoader);

	buf << "VkSurfaceCapabilitiesKHR:\n";
	buf << "-------------------------\n";
	buf << "- minImageCount: " << surfaceCapabilities.minImageCount << "\n";
	buf << "- maxImageCount: " << surfaceCapabilities.maxImageCount << "\n";
	buf << "- currentExtent: " << surfaceCapabilities.currentExtent.width << " x " << surfaceCapabilities.currentExtent.height << "\n";
	buf << "- minImageExtent: " << surfaceCapabilities.minImageExtent.width << " x " << surfaceCapabilities.minImageExtent.height << "\n";
	buf << "- maxImageExtent: " << surfaceCapabilities.maxImageExtent.width << " x " << surfaceCapabilities.maxImageExtent.height << "\n";
	buf << "- maxImageArrayLayers: " << surfaceCapabilities.maxImageArrayLayers << "\n";
	buf << "- supportedTransforms: " << to_string(surfaceCapabilities.supportedTransforms) << "\n";
	buf << "- currentTransform: " << to_string(surfaceCapabilities.currentTransform) << "\n";
	buf << "- supportedCompositeAlpha: " << to_string(surfaceCapabilities.supportedCompositeAlpha) << "\n";
	buf << "- supportedUsageFlags: " << to_string(surfaceCapabilities.supportedUsageFlags) << "\n";
	buf << "\n";

	// Surface Formats
	const auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface, vkDynLoader);

	buf << "Surface Formats:\n";
	buf << "-------------------------\n";
	size_t idx = 0;
	for (const auto& format : surfaceFormats)
	{
		buf << " - [" << idx << "]: " << to_string(format.format) << " - " << to_string(format.colorSpace) << "\n";
		++idx;
	}
	buf << "\n";

	// Present Modes
	const auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface, vkDynLoader);

	buf << "Surface Present Modes:\n";
	buf << "-------------------------\n";
	idx = 0;
	for (const auto& presentMode : presentModes)
	{
		buf << " - [" << idx << "]: " << to_string(presentMode) << "\n";
		++idx;
	}
	buf << "\n";

	if (outputHandler)
	{
		outputHandler(buf.str());
	}
}

// If `getProperties2` is true, the instance `inst` *must* have been created with the "VK_KHR_get_physical_device_properties2" extension enabled
void VkhInfo::Output_PhysicalDevices(const vk::Instance& inst, bool getProperties2, const vk::DispatchLoaderDynamic& vkDynLoader)
{
	std::stringstream buf;

	// - Physical Devices

	std::vector<vk::PhysicalDevice> physicalDevices;
	try
	{
		physicalDevices = inst.enumeratePhysicalDevices(vkDynLoader);
	}
	catch (const vk::SystemError& e)
	{
		// enumeratePhysicalDevices failed
		buf << "enumeratePhysicalDevices failed: " << e.what() << "\n";
	}

	if (physicalDevices.empty())
	{
		buf << "No physical devices with Vulkan support detected\n";
	}

	for (size_t idx = 0; idx < physicalDevices.size(); ++idx)
	{
		auto & physicalDevice = physicalDevices[idx];

		buf << "Device Properties and Extensions:\n";
		buf << "=================================\n";
		buf << "[Device " << idx << "]\n";

		// VkPhysicalDeviceProperties
		const auto deviceProperties = physicalDevice.getProperties(vkDynLoader);

		buf << "VkPhysicalDeviceProperties:\n";
		buf << "---------------------------\n";
		buf << "- apiVersion: " << VkhInfo::vulkan_apiversion_to_string(deviceProperties.apiVersion) << "\n";
		buf << "- driverVersion: " << std::to_string(deviceProperties.driverVersion) << "\n";
		buf << "- vendorID: " << std::to_string(deviceProperties.vendorID) << "\n";
		buf << "- deviceID: " << std::to_string(deviceProperties.deviceID) << "\n";
		buf << "- deviceType: " << to_string(deviceProperties.deviceType) << "\n";
		buf << "- deviceName: " << deviceProperties.deviceName << "\n";

		// VkPhysicalDeviceLimits
		const auto & deviceLimits = deviceProperties.limits;

		buf << "  VkPhysicalDeviceLimits:\n";
		buf << "  -----------------------\n";
		buf << "  - maxImageDimension1D: " << deviceLimits.maxImageDimension1D << "\n";
		buf << "  - maxImageDimension2D: " << deviceLimits.maxImageDimension2D << "\n";
		buf << "  - maxImageDimension3D: " << deviceLimits.maxImageDimension3D << "\n";
		buf << "  - maxImageDimensionCube: " << deviceLimits.maxImageDimensionCube << "\n";
		buf << "  - maxImageArrayLayers: " << deviceLimits.maxImageArrayLayers << "\n";
		buf << "  - maxTexelBufferElements: " << deviceLimits.maxTexelBufferElements << "\n";
		buf << "  - maxUniformBufferRange: " << deviceLimits.maxUniformBufferRange << "\n";
		buf << "  - maxStorageBufferRange: " << deviceLimits.maxStorageBufferRange << "\n";
		buf << "  - maxPushConstantsSize: " << deviceLimits.maxPushConstantsSize << "\n";
		buf << "  - maxMemoryAllocationCount: " << deviceLimits.maxMemoryAllocationCount << "\n";
		buf << "  - maxSamplerAllocationCount: " << deviceLimits.maxSamplerAllocationCount << "\n";
		buf << "  - bufferImageGranularity: " << deviceLimits.bufferImageGranularity << "\n";
		buf << "  - sparseAddressSpaceSize: " << deviceLimits.sparseAddressSpaceSize << "\n";
		buf << "  - maxBoundDescriptorSets: " << deviceLimits.maxBoundDescriptorSets << "\n";
		buf << "  - maxPerStageDescriptorSamplers: " << deviceLimits.maxPerStageDescriptorSamplers << "\n";
		buf << "  - maxPerStageDescriptorUniformBuffers: " << deviceLimits.maxPerStageDescriptorUniformBuffers << "\n";
		buf << "  - maxPerStageDescriptorStorageBuffers: " << deviceLimits.maxPerStageDescriptorStorageBuffers << "\n";
		buf << "  - maxPerStageDescriptorSampledImages: " << deviceLimits.maxPerStageDescriptorSampledImages << "\n";
		buf << "  - maxPerStageDescriptorStorageImages: " << deviceLimits.maxPerStageDescriptorStorageImages << "\n";
		buf << "  - maxPerStageDescriptorInputAttachments: " << deviceLimits.maxPerStageDescriptorInputAttachments << "\n";
		buf << "  - maxPerStageResources: " << deviceLimits.maxPerStageResources << "\n";
		buf << "  - maxDescriptorSetSamplers: " << deviceLimits.maxDescriptorSetSamplers << "\n";
		buf << "  - maxDescriptorSetUniformBuffers: " << deviceLimits.maxDescriptorSetUniformBuffers << "\n";
		buf << "  - maxDescriptorSetUniformBuffersDynamic: " << deviceLimits.maxDescriptorSetUniformBuffersDynamic << "\n";
		buf << "  - maxDescriptorSetStorageBuffers: " << deviceLimits.maxDescriptorSetStorageBuffers << "\n";
		buf << "  - maxDescriptorSetStorageBuffersDynamic: " << deviceLimits.maxDescriptorSetStorageBuffersDynamic << "\n";
		buf << "  - maxDescriptorSetSampledImages: " << deviceLimits.maxDescriptorSetSampledImages << "\n";
		buf << "  - maxDescriptorSetStorageImages: " << deviceLimits.maxDescriptorSetStorageImages << "\n";
		buf << "  - maxDescriptorSetInputAttachments: " << deviceLimits.maxDescriptorSetInputAttachments << "\n";
		buf << "  - maxVertexInputAttributes: " << deviceLimits.maxVertexInputAttributes << "\n";
		buf << "  - maxVertexInputBindings: " << deviceLimits.maxVertexInputBindings << "\n";
		buf << "  - maxVertexInputAttributeOffset: " << deviceLimits.maxVertexInputAttributeOffset << "\n";
		buf << "  - maxVertexInputBindingStride: " << deviceLimits.maxVertexInputBindingStride << "\n";
		buf << "  - maxVertexOutputComponents: " << deviceLimits.maxVertexOutputComponents << "\n";
		buf << "  - maxTessellationGenerationLevel: " << deviceLimits.maxTessellationGenerationLevel << "\n";
		buf << "  - maxTessellationPatchSize: " << deviceLimits.maxTessellationPatchSize << "\n";
		buf << "  - maxTessellationControlPerVertexInputComponents: " << deviceLimits.maxTessellationControlPerVertexInputComponents << "\n";
		buf << "  - maxTessellationControlPerVertexOutputComponents: " << deviceLimits.maxTessellationControlPerVertexOutputComponents << "\n";
		buf << "  - maxTessellationControlPerPatchOutputComponents: " << deviceLimits.maxTessellationControlPerPatchOutputComponents << "\n";
		buf << "  - maxTessellationControlTotalOutputComponents: " << deviceLimits.maxTessellationControlTotalOutputComponents << "\n";
		buf << "  - maxTessellationEvaluationInputComponents: " << deviceLimits.maxTessellationEvaluationInputComponents << "\n";
		buf << "  - maxTessellationEvaluationOutputComponents: " << deviceLimits.maxTessellationEvaluationOutputComponents << "\n";
		buf << "  - maxGeometryShaderInvocations: " << deviceLimits.maxGeometryShaderInvocations << "\n";
		buf << "  - maxGeometryInputComponents: " << deviceLimits.maxGeometryInputComponents << "\n";
		buf << "  - maxGeometryOutputComponents: " << deviceLimits.maxGeometryOutputComponents << "\n";
		buf << "  - maxGeometryOutputVertices: " << deviceLimits.maxGeometryOutputVertices << "\n";
		buf << "  - maxGeometryTotalOutputComponents: " << deviceLimits.maxGeometryTotalOutputComponents << "\n";
		buf << "  - maxFragmentInputComponents: " << deviceLimits.maxFragmentInputComponents << "\n";
		buf << "  - maxFragmentOutputAttachments: " << deviceLimits.maxFragmentOutputAttachments << "\n";
		buf << "  - maxFragmentDualSrcAttachments: " << deviceLimits.maxFragmentDualSrcAttachments << "\n";
		buf << "  - maxFragmentCombinedOutputResources: " << deviceLimits.maxFragmentCombinedOutputResources << "\n";
		buf << "  - maxComputeSharedMemorySize: " << deviceLimits.maxComputeSharedMemorySize << "\n";
		buf << "  - maxComputeWorkGroupCount[0]: " << deviceLimits.maxComputeWorkGroupCount[0] << "\n";
		buf << "  - maxComputeWorkGroupCount[1]: " << deviceLimits.maxComputeWorkGroupCount[1] << "\n";
		buf << "  - maxComputeWorkGroupCount[2]: " << deviceLimits.maxComputeWorkGroupCount[2] << "\n";
		buf << "  - maxComputeWorkGroupInvocations: " << deviceLimits.maxComputeWorkGroupInvocations << "\n";
		buf << "  - maxComputeWorkGroupSize[0]: " << deviceLimits.maxComputeWorkGroupSize[0] << "\n";
		buf << "  - maxComputeWorkGroupSize[1]: " << deviceLimits.maxComputeWorkGroupSize[1] << "\n";
		buf << "  - maxComputeWorkGroupSize[2]: " << deviceLimits.maxComputeWorkGroupSize[2] << "\n";
		buf << "  - subPixelPrecisionBits: " << deviceLimits.subPixelPrecisionBits << "\n";
		buf << "  - subTexelPrecisionBits: " << deviceLimits.subTexelPrecisionBits << "\n";
		buf << "  - mipmapPrecisionBits: " << deviceLimits.mipmapPrecisionBits << "\n";
		buf << "  - maxDrawIndexedIndexValue: " << deviceLimits.maxDrawIndexedIndexValue << "\n";
		buf << "  - maxDrawIndirectCount: " << deviceLimits.maxDrawIndirectCount << "\n";
		buf << "  - maxSamplerLodBias: " << deviceLimits.maxSamplerLodBias << "\n";
		buf << "  - maxSamplerAnisotropy: " << deviceLimits.maxSamplerAnisotropy << "\n";
		buf << "  - maxViewports: " << deviceLimits.maxViewports << "\n";
		buf << "  - maxViewportDimensions[0]: " << deviceLimits.maxViewportDimensions[0] << "\n";
		buf << "  - maxViewportDimensions[1]: " << deviceLimits.maxViewportDimensions[1] << "\n";
		buf << "  - viewportBoundsRange[0]: " << deviceLimits.viewportBoundsRange[0] << "\n";
		buf << "  - viewportBoundsRange[1]: " << deviceLimits.viewportBoundsRange[1] << "\n";
		buf << "  - viewportSubPixelBits: " << deviceLimits.viewportSubPixelBits << "\n";
		buf << "  - viewportSubPixelBits: " << deviceLimits.minMemoryMapAlignment << "\n";
		buf << "  - minTexelBufferOffsetAlignment: " << deviceLimits.minTexelBufferOffsetAlignment << "\n";
		buf << "  - minUniformBufferOffsetAlignment: " << deviceLimits.minUniformBufferOffsetAlignment << "\n";
		buf << "  - minStorageBufferOffsetAlignment: " << deviceLimits.minStorageBufferOffsetAlignment << "\n";
		buf << "  - minTexelOffset: " << deviceLimits.minTexelOffset << "\n";
		buf << "  - maxTexelOffset: " << deviceLimits.maxTexelOffset << "\n";
		buf << "  - minTexelGatherOffset: " << deviceLimits.minTexelGatherOffset << "\n";
		buf << "  - maxTexelGatherOffset: " << deviceLimits.maxTexelGatherOffset << "\n";
		buf << "  - minInterpolationOffset: " << deviceLimits.minInterpolationOffset << "\n";
		buf << "  - maxInterpolationOffset: " << deviceLimits.maxInterpolationOffset << "\n";
		buf << "  - subPixelInterpolationOffsetBits: " << deviceLimits.subPixelInterpolationOffsetBits << "\n";
		buf << "  - maxFramebufferWidth: " << deviceLimits.maxFramebufferWidth << "\n";
		buf << "  - maxFramebufferHeight: " << deviceLimits.maxFramebufferHeight << "\n";
		buf << "  - maxFramebufferLayers: " << deviceLimits.maxFramebufferLayers << "\n";
		buf << "  - framebufferColorSampleCounts: " << to_string(deviceLimits.framebufferColorSampleCounts) << "\n";
		buf << "  - framebufferDepthSampleCounts: " << to_string(deviceLimits.framebufferDepthSampleCounts) << "\n";
		buf << "  - framebufferStencilSampleCounts: " << to_string(deviceLimits.framebufferStencilSampleCounts) << "\n";
		buf << "  - framebufferNoAttachmentsSampleCounts: " << to_string(deviceLimits.framebufferNoAttachmentsSampleCounts) << "\n";
		buf << "  - maxColorAttachments: " << deviceLimits.maxColorAttachments << "\n";
		buf << "  - sampledImageColorSampleCounts: " << to_string(deviceLimits.sampledImageColorSampleCounts) << "\n";
		buf << "  - sampledImageIntegerSampleCounts: " << to_string(deviceLimits.sampledImageIntegerSampleCounts) << "\n";
		buf << "  - sampledImageDepthSampleCounts: " << to_string(deviceLimits.sampledImageDepthSampleCounts) << "\n";
		buf << "  - sampledImageStencilSampleCounts: " << to_string(deviceLimits.sampledImageStencilSampleCounts) << "\n";
		buf << "  - storageImageSampleCounts: " << to_string(deviceLimits.storageImageSampleCounts) << "\n";
		buf << "  - maxSampleMaskWords: " << deviceLimits.maxSampleMaskWords << "\n";
		buf << "  - timestampComputeAndGraphics: " << deviceLimits.timestampComputeAndGraphics << "\n";
		buf << "  - timestampComputeAndGraphics: " << deviceLimits.timestampPeriod << "\n";
		buf << "  - maxClipDistances: " << deviceLimits.maxClipDistances << "\n";
		buf << "  - maxCullDistances: " << deviceLimits.maxCullDistances << "\n";
		buf << "  - maxCombinedClipAndCullDistances: " << deviceLimits.maxCombinedClipAndCullDistances << "\n";
		buf << "  - discreteQueuePriorities: " << deviceLimits.discreteQueuePriorities << "\n";
		buf << "  - pointSizeRange[0]: " << deviceLimits.pointSizeRange[0] << "\n";
		buf << "  - pointSizeRange[1]: " << deviceLimits.pointSizeRange[1] << "\n";
		buf << "  - lineWidthRange[0]: " << deviceLimits.lineWidthRange[0] << "\n";
		buf << "  - lineWidthRange[1]: " << deviceLimits.lineWidthRange[1] << "\n";
		buf << "  - pointSizeGranularity: " << deviceLimits.pointSizeGranularity << "\n";
		buf << "  - lineWidthGranularity: " << deviceLimits.lineWidthGranularity << "\n";
		buf << "  - strictLines: " << deviceLimits.strictLines << "\n";
		buf << "  - standardSampleLocations: " << deviceLimits.standardSampleLocations << "\n";
		buf << "  - optimalBufferCopyOffsetAlignment: " << deviceLimits.optimalBufferCopyOffsetAlignment << "\n";
		buf << "  - optimalBufferCopyRowPitchAlignment: " << deviceLimits.optimalBufferCopyRowPitchAlignment << "\n";
		buf << "  - nonCoherentAtomSize: " << deviceLimits.nonCoherentAtomSize << "\n";

		buf << "\n";

		// Supported device extensions
		const auto deviceExtensionProperties = physicalDevice.enumerateDeviceExtensionProperties(nullptr, vkDynLoader);

		buf << "Device Extensions:\n";
		buf << "------------------\n";
		buf << "Count: " << deviceExtensionProperties.size() << "\n";
		for (auto & extensionProperties : deviceExtensionProperties)
		{
			buf << "- " << extensionProperties.extensionName << " (version: " << extensionProperties.specVersion << ")\n";
		}
		buf << "\n";

		// Device memory properties
		const auto memprops = physicalDevice.getMemoryProperties(vkDynLoader);

		buf << "VkPhysicalDeviceMemoryProperties:\n";
		buf << "---------------------------------\n";
		buf << "- memoryHeapCount = " << memprops.memoryHeapCount << "\n";
		for (uint32_t i = 0; i < memprops.memoryHeapCount; ++i)
		{
			buf << "- memoryHeaps[" << i << "]:\n";
			buf << "  - size = " << memprops.memoryHeaps[i].size << "\n";
			buf << "  - flags = " << to_string(memprops.memoryHeaps[i].flags) << "\n";
		}
		buf << "- memoryTypeCount = " << memprops.memoryTypeCount << "\n";
		for (uint32_t i = 0; i < memprops.memoryTypeCount; ++i)
		{
			buf << "- memoryTypes[" << i << "]:\n";
			buf << "  - heapIndex = " << memprops.memoryTypes[i].heapIndex << "\n";
			buf << "  - propertyFlags = " << to_string(memprops.memoryTypes[i].propertyFlags) << "\n";
		}
		buf << "\n";

		bool instance_Supports_physical_device_properties2_extension = supportsInstanceExtension(nullptr, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, vkDynLoader.vkGetInstanceProcAddr);
		bool physicalDevice_Supports_physical_device_properties2_extension =
		std::find_if(deviceExtensionProperties.begin(), deviceExtensionProperties.end(),
					 [](const vk::ExtensionProperties& props) {
						 return (strcmp(props.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0);
					 }) != deviceExtensionProperties.end();

		if (getProperties2 && instance_Supports_physical_device_properties2_extension && physicalDevice_Supports_physical_device_properties2_extension)
		{
			vk::PhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProps;
			vk::PhysicalDeviceMaintenance3Properties maintenance3Properties;
			if (deviceProperties.apiVersion >= VK_MAKE_VERSION(1, 1, 0))
			{
				auto properties2Chain = physicalDevice.getProperties2<
				vk::PhysicalDeviceProperties2,
				vk::PhysicalDevicePushDescriptorPropertiesKHR,
				vk::PhysicalDeviceMaintenance3Properties
				>(vkDynLoader);
				pushDescriptorProps = properties2Chain.template get<vk::PhysicalDevicePushDescriptorPropertiesKHR>();
				maintenance3Properties = properties2Chain.template get<vk::PhysicalDeviceMaintenance3Properties>();
			}
			else
			{
				auto properties2Chain = physicalDevice.getProperties2KHR<
				vk::PhysicalDeviceProperties2KHR,
				vk::PhysicalDevicePushDescriptorPropertiesKHR,
				vk::PhysicalDeviceMaintenance3Properties
				>(vkDynLoader);
				pushDescriptorProps = properties2Chain.template get<vk::PhysicalDevicePushDescriptorPropertiesKHR>();
				maintenance3Properties = properties2Chain.template get<vk::PhysicalDeviceMaintenance3Properties>();
			}

			// VkPhysicalDevicePushDescriptorProperties
			buf << "VkPhysicalDevicePushDescriptorProperties:\n";
			buf << "-----------------------------------------\n";
			buf << "- maxPushDescriptors = " << pushDescriptorProps.maxPushDescriptors << "\n";
			buf << "\n";

			// VkPhysicalDeviceMaintenance3Properties
			buf << "VkPhysicalDeviceMaintenance3Properties:\n";
			buf << "---------------------------------------\n";
			buf << "- maxPerSetDescriptors = " << maintenance3Properties.maxPerSetDescriptors << "\n";
			buf << "- maxMemoryAllocationSize = " << maintenance3Properties.maxMemoryAllocationSize << "\n";
			buf << "\n";
		}

		// VkPhysicalDeviceFeatures
		const auto deviceFeatures = physicalDevice.getFeatures(vkDynLoader);

		buf << "VkPhysicalDeviceFeatures:\n";
		buf << "-------------------------\n";
		buf << "- robustBufferAccess = " << deviceFeatures.robustBufferAccess << "\n";
		buf << "- fullDrawIndexUint32 = " << deviceFeatures.fullDrawIndexUint32 << "\n";
		buf << "- imageCubeArray = " << deviceFeatures.imageCubeArray << "\n";
		buf << "- independentBlend = " << deviceFeatures.independentBlend << "\n";
		buf << "- geometryShader = " << deviceFeatures.geometryShader << "\n";
		buf << "- tessellationShader = " << deviceFeatures.tessellationShader << "\n";
		buf << "- sampleRateShading = " << deviceFeatures.sampleRateShading << "\n";
		buf << "- dualSrcBlend = " << deviceFeatures.dualSrcBlend << "\n";
		buf << "- logicOp = " << deviceFeatures.logicOp << "\n";
		buf << "- multiDrawIndirect = " << deviceFeatures.multiDrawIndirect << "\n";
		buf << "- drawIndirectFirstInstance = " << deviceFeatures.drawIndirectFirstInstance << "\n";
		buf << "- depthClamp = " << deviceFeatures.depthClamp << "\n";
		buf << "- depthBiasClamp = " << deviceFeatures.depthBiasClamp << "\n";
		buf << "- fillModeNonSolid = " << deviceFeatures.fillModeNonSolid << "\n";
		buf << "- depthBounds = " << deviceFeatures.depthBounds << "\n";
		buf << "- wideLines = " << deviceFeatures.wideLines << "\n";
		buf << "- largePoints = " << deviceFeatures.largePoints << "\n";
		buf << "- alphaToOne = " << deviceFeatures.alphaToOne << "\n";
		buf << "- multiViewport = " << deviceFeatures.multiViewport << "\n";
		buf << "- samplerAnisotropy = " << deviceFeatures.samplerAnisotropy << "\n";
		buf << "- textureCompressionETC2 = " << deviceFeatures.textureCompressionETC2 << "\n";
		buf << "- textureCompressionASTC_LDR = " << deviceFeatures.textureCompressionASTC_LDR << "\n";
		buf << "- textureCompressionBC = " << deviceFeatures.textureCompressionBC << "\n";
		buf << "- occlusionQueryPrecise = " << deviceFeatures.occlusionQueryPrecise << "\n";
		buf << "- pipelineStatisticsQuery = " << deviceFeatures.pipelineStatisticsQuery << "\n";
		buf << "- vertexPipelineStoresAndAtomics = " << deviceFeatures.vertexPipelineStoresAndAtomics << "\n";
		buf << "- fragmentStoresAndAtomics = " << deviceFeatures.fragmentStoresAndAtomics << "\n";
		buf << "- shaderTessellationAndGeometryPointSize = " << deviceFeatures.shaderTessellationAndGeometryPointSize << "\n";
		buf << "- shaderImageGatherExtended = " << deviceFeatures.shaderImageGatherExtended << "\n";
		buf << "- shaderStorageImageExtendedFormats = " << deviceFeatures.shaderStorageImageExtendedFormats << "\n";
		buf << "- shaderStorageImageMultisample = " << deviceFeatures.shaderStorageImageMultisample << "\n";
		buf << "- shaderStorageImageReadWithoutFormat = " << deviceFeatures.shaderStorageImageReadWithoutFormat << "\n";
		buf << "- shaderStorageImageWriteWithoutFormat = " << deviceFeatures.shaderStorageImageWriteWithoutFormat << "\n";
		buf << "- shaderUniformBufferArrayDynamicIndexing = " << deviceFeatures.shaderUniformBufferArrayDynamicIndexing << "\n";
		buf << "- shaderSampledImageArrayDynamicIndexing = " << deviceFeatures.shaderSampledImageArrayDynamicIndexing << "\n";
		buf << "- shaderStorageBufferArrayDynamicIndexing = " << deviceFeatures.shaderStorageBufferArrayDynamicIndexing << "\n";
		buf << "- shaderStorageImageArrayDynamicIndexing = " << deviceFeatures.shaderStorageImageArrayDynamicIndexing << "\n";
		buf << "- shaderClipDistance = " << deviceFeatures.shaderClipDistance << "\n";
		buf << "- shaderCullDistance = " << deviceFeatures.shaderCullDistance << "\n";
		buf << "- shaderFloat64 = " << deviceFeatures.shaderFloat64 << "\n";
		buf << "- shaderInt64 = " << deviceFeatures.shaderInt64 << "\n";
		buf << "- shaderInt16 = " << deviceFeatures.shaderInt16 << "\n";
		buf << "- shaderResourceResidency = " << deviceFeatures.shaderResourceResidency << "\n";
		buf << "- shaderResourceMinLod = " << deviceFeatures.shaderResourceMinLod << "\n";
		buf << "- sparseBinding = " << deviceFeatures.sparseBinding << "\n";
		buf << "- sparseResidencyBuffer = " << deviceFeatures.sparseResidencyBuffer << "\n";
		buf << "- sparseResidencyImage2D = " << deviceFeatures.sparseResidencyImage2D << "\n";
		buf << "- sparseResidencyImage3D = " << deviceFeatures.sparseResidencyImage3D << "\n";
		buf << "- sparseResidency2Samples = " << deviceFeatures.sparseResidency2Samples << "\n";
		buf << "- sparseResidency4Samples = " << deviceFeatures.sparseResidency4Samples << "\n";
		buf << "- sparseResidency8Samples = " << deviceFeatures.sparseResidency8Samples << "\n";
		buf << "- sparseResidency16Samples = " << deviceFeatures.sparseResidency16Samples << "\n";
		buf << "- sparseResidencyAliased = " << deviceFeatures.sparseResidencyAliased << "\n";
		buf << "- variableMultisampleRate = " << deviceFeatures.variableMultisampleRate << "\n";
		buf << "- inheritedQueries = " << deviceFeatures.inheritedQueries << "\n";

		buf << "\n";

		// VkQueueFamilyProperties
		const auto queueFamilies = physicalDevice.getQueueFamilyProperties(vkDynLoader);
		buf << "VkQueueFamilyProperties:\n";
		buf << "------------------------\n";
		for (size_t idx = 0; idx < queueFamilies.size(); ++idx)
		{
			const auto & queueFamily = queueFamilies[idx];
			buf << "[Queue Family " << idx << "]\n";
			buf << "- queueFlags = " << to_string(queueFamily.queueFlags) << "\n";
			buf << "- queueCount = " << queueFamily.queueCount << "\n";
			buf << "- timestampValidBits = " << queueFamily.timestampValidBits << "\n";
			buf << "- minImageTransferGranularity = " << queueFamily.minImageTransferGranularity.width << " x " << queueFamily.minImageTransferGranularity.height << " x " << queueFamily.minImageTransferGranularity.depth << "\n";
		}

		buf << "\n";
	}

	if (outputHandler)
	{
		outputHandler(buf.str());
	}
}
