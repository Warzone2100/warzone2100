# FindVulkanHeaders.cmake - Find Vulkan headers
#
# This module defines the following variables:
#
# VulkanHeaders_FOUND          - "True" if Vulkan headers were found
# VulkanHeaders_INCLUDE_DIRS   - include directories for Vulkan headers
#
# The module will also define a cache variable:
#
# VulkanHeaders_INCLUDE_DIR    - the Vulkan headers include directory
#
#####################################################

set(VK_HEADER_VERSION_STRING "")

find_package(Vulkan)
if(Vulkan_FOUND)
	# Check to ensure that the headers are at least the minimum supported version
	if(Vulkan_INCLUDE_DIRS AND EXISTS "${Vulkan_INCLUDE_DIRS}/vulkan/vulkan_core.h")
		file(STRINGS "${Vulkan_INCLUDE_DIRS}/vulkan/vulkan_core.h" VK_HEADER_VERSION_STRING_LINE REGEX "^#define[ \t]+VK_HEADER_VERSION[ \t]+[\"]?[0-9]+[\"]?$")
		string(REGEX REPLACE "^#define[ \t]+VK_HEADER_VERSION[ \t]+[\"]?([0-9]+)[\"]?$" "\\1" VK_HEADER_VERSION_STRING "${VK_HEADER_VERSION_STRING_LINE}")
		unset(VK_HEADER_VERSION_STRING_LINE)
	else()
		if(Vulkan_INCLUDE_DIRS)
			message(WARNING "Can't find ${Vulkan_INCLUDE_DIRS}/vulkan/vulkan_core.h")
		endif()
		set(VK_HEADER_VERSION_STRING "")
	endif()

	set(VulkanHeaders_INCLUDE_DIR "${Vulkan_INCLUDE_DIRS}")
	message(STATUS "Detected Vulkan headers (VK_HEADER_VERSION: ${VK_HEADER_VERSION_STRING}): ${VulkanHeaders_INCLUDE_DIR}")
endif()

set(VulkanHeaders_INCLUDE_DIRS "${VulkanHeaders_INCLUDE_DIR}")
set(VulkanHeaders_VERSION_STRING "${VK_HEADER_VERSION_STRING}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VulkanHeaders REQUIRED_VARS VulkanHeaders_INCLUDE_DIR VERSION_VAR VulkanHeaders_VERSION_STRING)

mark_as_advanced(VulkanHeaders_INCLUDE_DIR)
