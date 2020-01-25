//
// VkhInfo
// Version: 1.3
//
// Copyright (c) 2019-2020 past-due
//
// https://github.com/past-due/vulkan-helpers
//
// Distributed under the MIT License.
// See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT
//

#pragma once

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4191 ) // warning C4191: '<function-style-cast>': unsafe conversion from 'PFN_vkVoidFunction' to 'PFN_vk<...>'
#endif
#define VULKAN_HPP_TYPESAFE_CONVERSION 1
#ifndef NOMINMAX
    #define NOMINMAX // For windows.h
#endif
#include <vulkan/vulkan.hpp>
#if defined( _MSC_VER )
#pragma warning( pop )
#endif

#include <string>
#include <functional>
#include <vector>

class VkhInfo
{
public:
	typedef std::function<void (const std::string& output)> outputHandlerFuncType;

	VkhInfo() {}
	VkhInfo(const outputHandlerFuncType& outputHandler);

	void setOutputHandler(const outputHandlerFuncType& outputHandler);

public:

	void Output_GlobalInstanceExtensions(PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr);
	void Output_InstanceLayerProperties(PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr);

	void Output_SurfaceInformation(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, const vk::DispatchLoaderDynamic& vkDynLoader);

	// If `getProperties2` is true, the instance `inst` *must* have been created with the "VK_KHR_get_physical_device_properties2" extension enabled
	void Output_PhysicalDevices(const vk::Instance& inst, const vk::ApplicationInfo& appInfo, std::vector<const char*> instanceExtensions, const vk::DispatchLoaderDynamic& vkDynLoader);

public:

	static inline std::string vulkan_apiversion_to_string(uint32_t apiVersion)
	{
		return std::to_string(VK_VERSION_MAJOR(apiVersion)) + "." + std::to_string(VK_VERSION_MINOR(apiVersion)) + "." + std::to_string(VK_VERSION_PATCH(apiVersion));
	}
	static bool getInstanceLayerProperties(std::vector<VkLayerProperties> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr);
	static bool getInstanceExtensions(const char * layerName, std::vector<VkExtensionProperties> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr);
	static bool supportsInstanceExtension(const char * layerName, const char * extensionName, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr);

private:
	outputHandlerFuncType outputHandler;
};
