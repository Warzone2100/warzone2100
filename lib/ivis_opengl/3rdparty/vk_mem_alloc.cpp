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

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE
#include "lib/framework/debug.h"

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunused-variable"
# pragma clang diagnostic ignored "-Wmissing-field-initializers"
# pragma clang diagnostic ignored "-Wunused-private-field"
# pragma clang diagnostic ignored "-Wcast-align"
# pragma clang diagnostic ignored "-Wunused-function"
# pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#  if defined(__APPLE__)
#    pragma clang diagnostic ignored "-Wnullability-completeness" // Warning triggered on newer Xcode
#  endif
#  if defined(__has_warning)
#    if __has_warning("-Wnullability-extension")
#      pragma clang diagnostic ignored "-Wnullability-extension"
#    endif
#    if __has_warning("-Wnullability-completeness")
#      pragma clang diagnostic ignored "-Wnullability-completeness"
#    endif
#  endif
#elif defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wtype-limits"
# pragma GCC diagnostic ignored "-Wunused-variable"
# pragma GCC diagnostic ignored "-Wmissing-field-initializers"
# pragma GCC diagnostic ignored "-Wmissing-noreturn"
# pragma GCC diagnostic ignored "-Wcast-align"
# pragma GCC diagnostic ignored "-Wunused-function"
# pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
# if 7 <= __GNUC__
#  pragma GCC diagnostic ignored "-Wnull-dereference"
# endif
#elif defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4189 ) // warning C4189: 'identifier' : local variable is initialized but not referenced
# pragma warning( disable : 4324 ) // warning C4324: 'struct_name' : structure was padded due to alignment specifier
# pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
# pragma warning( disable : 4505 ) // warning C4505: unreferenced function with internal linkage has been removed
#endif

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_ASSERT(expr) ASSERT(expr, "VMA_ASSERT failed")
#include <cstdio>

// The VMA_NULLABLE macro is defined to be _Nullable when compiling with Clang.
// see: https://clang.llvm.org/docs/AttributeReference.html#nullable
#ifndef VMA_NULLABLE
	#ifdef __clang__
		#define VMA_NULLABLE _Nullable
	#else
		#define VMA_NULLABLE
	#endif
#endif

// WZ Patch for C++14 compilation
#if (__cplusplus >= 201402L && __cplusplus < 201703L)
# if defined(__ANDROID_API__) && (__ANDROID_API__ < 16)
	// do not do anything - is handled by vk_mem_alloc
# elif defined(__APPLE__) || defined(__ANDROID__)
	// do not do anything - is handled by vk_mem_alloc
# elif defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__) || defined(__NetBSD__)
	// On C++14, implement for Linux + BSDs (until we can require C++17)
	// Reference: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/issues/332
	#include <cstdlib>
	static void* wz_vma_aligned_alloc(size_t alignment, size_t size)
	{
		// alignment must be >= sizeof(void*)
		if(alignment < sizeof(void*))
		{
			alignment = sizeof(void*);
		}

		void *pointer;
		if(posix_memalign(&pointer, alignment, size) == 0)
			return pointer;
		return nullptr;
	}
	static void wz_vma_aligned_free(void* VMA_NULLABLE ptr)
	{
		free(ptr);
	}
#  define VMA_SYSTEM_ALIGNED_MALLOC(size, alignment)	wz_vma_aligned_alloc((alignment), (size))
#  define VMA_SYSTEM_ALIGNED_FREE(ptr)					wz_vma_aligned_free(ptr)
# endif
#endif

#include "vk_mem_alloc.h"

#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__)
# pragma GCC diagnostic pop
#elif defined(_MSC_VER)
# pragma warning( pop )
#endif
