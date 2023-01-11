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
#elif defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4189 ) // warning C4189: 'identifier' : local variable is initialized but not referenced
# pragma warning( disable : 4324 ) // warning C4324: 'struct_name' : structure was padded due to alignment specifier
# pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
#endif

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_ASSERT(expr) ASSERT(expr, "VMA_ASSERT failed")
#include "vk_mem_alloc.h"

#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__)
# pragma GCC diagnostic pop
#elif defined(_MSC_VER)
# pragma warning( pop )
#endif
