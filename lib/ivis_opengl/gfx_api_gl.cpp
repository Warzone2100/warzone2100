/*
	This file is part of Warzone 2100.
	Copyright (C) 2017-2020  Warzone 2100 Project

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

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "screen.h"
#include "gfx_api_gl.h"
#include "lib/exceptionhandler/dumpinfo.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/wzpaths.h"
#include "lib/framework/stringsearch.h"
#include "piemode.h"

#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
// On Fedora 40, GCC 14 produces false-positive warnings for -Walloc-zero
// when compiling <regex> with optimizations. Silence these warnings.
#if !defined(__clang__) && !defined(__INTEL_COMPILER) && defined(__GNUC__) && __GNUC__ >= 14 && defined(__OPTIMIZE__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Walloc-zero"
#endif
# include <regex>
#if !defined(__clang__) && !defined(__INTEL_COMPILER) && defined(__GNUC__) && __GNUC__ >= 14 && defined(__OPTIMIZE__)
# pragma GCC diagnostic pop
#endif
#include <limits>
#include <typeindex>
#include <sstream>
#include <fmt/format.h>
#include <fmt/xchar.h>

#include <glm/gtc/type_ptr.hpp>

#if defined(WZ_CC_MSVC) && defined(DEBUG)
#include <debugapi.h>
#endif

#if GL_KHR_debug && !defined(__EMSCRIPTEN__)
# define WZ_GL_KHR_DEBUG_SUPPORTED
#endif
#if !defined(__EMSCRIPTEN__)
# define WZ_GL_TIMER_QUERY_SUPPORTED
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten/html5_webgl.h>
# if defined(WZ_STATIC_GL_BINDINGS)
#  include <GLES2/gl2ext.h>
# endif

// forward-declarations
static std::unordered_set<std::string> enabledWebGLExtensions;
static bool initWebGLExtensions();

static int GLAD_GL_ES_VERSION_3_0 = 0;
static int GLAD_GL_EXT_texture_filter_anisotropic = 0;

#ifndef GL_COMPRESSED_RGB8_ETC2
# define GL_COMPRESSED_RGB8_ETC2 0x9274
#endif
#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
# define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#endif
#ifndef GL_COMPRESSED_R11_EAC
# define GL_COMPRESSED_R11_EAC 0x9270
#endif
#ifndef GL_COMPRESSED_RG11_EAC
# define GL_COMPRESSED_RG11_EAC 0x9272
#endif

#ifndef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
# define GL_COMPRESSED_RGBA_ASTC_4x4_KHR 0x93B0
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
# define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA8
#endif

#endif

struct OPENGL_DATA
{
	char vendor[256] = {};
	char renderer[256] = {};
	char version[256] = {};
	char GLSLversion[256] = {};
};
OPENGL_DATA opengl;

#if defined(WZ_GL_TIMER_QUERY_SUPPORTED)
static GLuint perfpos[PERF_COUNT] = {};
static bool perfStarted = false;
#endif

#if defined(WZ_DEBUG_GFX_API_LEAKS)
static std::unordered_set<const gl_texture*> debugLiveTextures;
#endif

#if !defined(WZ_STATIC_GL_BINDINGS)
PFNGLDRAWARRAYSINSTANCEDPROC wz_dyn_glDrawArraysInstanced = nullptr;
PFNGLDRAWELEMENTSINSTANCEDPROC wz_dyn_glDrawElementsInstanced = nullptr;
PFNGLVERTEXATTRIBDIVISORPROC wz_dyn_glVertexAttribDivisor = nullptr;
#else
#define wz_dyn_glDrawArraysInstanced glDrawArraysInstanced
#define wz_dyn_glDrawElementsInstanced glDrawElementsInstanced
#define wz_dyn_glVertexAttribDivisor glVertexAttribDivisor
#endif

static const GLubyte* wzSafeGlGetString(GLenum name);

static GLenum to_gl_internalformat(const gfx_api::pixel_format& format, bool gles)
{
	switch (format)
	{
		// UNCOMPRESSED FORMATS
		case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
			return GL_RGBA8;
		case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
			return GL_RGBA8; // must store as RGBA8
		case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
			return GL_RGB8;
		case gfx_api::pixel_format::FORMAT_RG8_UNORM:
#if !defined(__EMSCRIPTEN__)
			if (gles && GLAD_GL_EXT_texture_rg)
			{
				// the internal format is GL_RG_EXT
				return GL_RG_EXT;
			}
			else
#endif
			{
				// for Desktop OpenGL (or WebGL 2.0), use GL_RG8 for the internal format
				return GL_RG8;
			}
		case gfx_api::pixel_format::FORMAT_R8_UNORM:
#if !defined(__EMSCRIPTEN__)
			if ((!gles && GLAD_GL_VERSION_3_0) || (gles && GLAD_GL_ES_VERSION_3_0))
			{
				// OpenGL 3.0+ or OpenGL ES 3.0+
				return GL_R8;
			}
			else
			{
				// older version fallback
				// use GL_LUMINANCE because:
				// (a) it's available and
				// (b) it ensures the single channel value ends up in "red" so the shaders don't have to care
				return GL_LUMINANCE;
			}
#else
			// WebGL 2.0
			return GL_R8;
#endif
		// COMPRESSED FORMAT
		case gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM:
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM:
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM:
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		case gfx_api::pixel_format::FORMAT_R_BC4_UNORM:
#if defined(__EMSCRIPTEN__) && defined(GL_EXT_texture_compression_rgtc)
			return GL_COMPRESSED_RED_RGTC1_EXT;
#else
			return GL_COMPRESSED_RED_RGTC1;
#endif
		case gfx_api::pixel_format::FORMAT_RG_BC5_UNORM:
#if defined(__EMSCRIPTEN__) && defined(GL_EXT_texture_compression_rgtc)
			return GL_COMPRESSED_RED_GREEN_RGTC2_EXT;
#else
			return GL_COMPRESSED_RG_RGTC2;
#endif
		case gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM:
			return GL_COMPRESSED_RGBA_BPTC_UNORM_EXT; // same value as GL_COMPRESSED_RGBA_BPTC_UNORM_ARB
		case gfx_api::pixel_format::FORMAT_RGB8_ETC1:
			return GL_ETC1_RGB8_OES;
		case gfx_api::pixel_format::FORMAT_RGB8_ETC2:
			return GL_COMPRESSED_RGB8_ETC2;
		case gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC:
			return GL_COMPRESSED_RGBA8_ETC2_EAC;
		case gfx_api::pixel_format::FORMAT_R11_EAC:
			return GL_COMPRESSED_R11_EAC;
		case gfx_api::pixel_format::FORMAT_RG11_EAC:
			return GL_COMPRESSED_RG11_EAC;
		case gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM:
			return GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
		default:
			debug(LOG_FATAL, "Unrecognised pixel format");
	}
	return GL_INVALID_ENUM;
}

static GLenum to_gl_format(const gfx_api::pixel_format& format, bool gles)
{
	switch (format)
	{
		// UNCOMPRESSED FORMATS
		case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
			return GL_RGBA;
		case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
#if defined(__EMSCRIPTEN__)
			return GL_INVALID_ENUM;
#else
			return GL_BGRA;
#endif
		case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
			return GL_RGB;
		case gfx_api::pixel_format::FORMAT_RG8_UNORM:
#if !defined(__EMSCRIPTEN__)
			if (gles && GLAD_GL_EXT_texture_rg)
			{
				// the internal format is GL_RG_EXT
				return GL_RG_EXT;
			}
			else
#endif
			{
				// for Desktop OpenGL or WebGL 2.0, use GL_RG for the format
				return GL_RG;
			}
		case gfx_api::pixel_format::FORMAT_R8_UNORM:
#if !defined(__EMSCRIPTEN__)
			if ((!gles && GLAD_GL_VERSION_3_0) || (gles && GLAD_GL_ES_VERSION_3_0))
			{
				// OpenGL 3.0+ or OpenGL ES 3.0+
				return GL_RED;
			}
			else
			{
				// older version fallback
				// use GL_LUMINANCE because:
				// (a) it's available and
				// (b) it ensures the single channel value ends up in "red" so the shaders don't have to care
				return GL_LUMINANCE;
			}
#else
			// WebGL 2.0
			return GL_RED;
#endif
		// COMPRESSED FORMAT
		default:
			return to_gl_internalformat(format, gles);
	}
	return GL_INVALID_ENUM;
}

static GLenum to_gl(const gfx_api::context::buffer_storage_hint& hint)
{
	switch (hint)
	{
		case gfx_api::context::buffer_storage_hint::static_draw:
			return GL_STATIC_DRAW;
		case gfx_api::context::buffer_storage_hint::dynamic_draw:
			return GL_DYNAMIC_DRAW;
		case gfx_api::context::buffer_storage_hint::stream_draw:
			return GL_STREAM_DRAW;
		default:
			debug(LOG_FATAL, "Unsupported buffer hint");
	}
	return GL_INVALID_ENUM;
}

static GLenum to_gl(const gfx_api::buffer::usage& usage)
{
	switch (usage)
	{
		case gfx_api::buffer::usage::index_buffer:
			return GL_ELEMENT_ARRAY_BUFFER;
		case gfx_api::buffer::usage::vertex_buffer:
			return GL_ARRAY_BUFFER;
		default:
			debug(LOG_FATAL, "Unrecognised buffer usage");
	}
	return GL_INVALID_ENUM;
}

static GLenum to_gl(const gfx_api::primitive_type& primitive)
{
	switch (primitive)
	{
		case gfx_api::primitive_type::lines:
			return GL_LINES;
		case gfx_api::primitive_type::line_strip:
			return GL_LINE_STRIP;
		case gfx_api::primitive_type::triangles:
			return GL_TRIANGLES;
		case gfx_api::primitive_type::triangle_strip:
			return GL_TRIANGLE_STRIP;
		default:
			debug(LOG_FATAL, "Unrecognised primitive type");
	}
	return GL_INVALID_ENUM;
}

static GLenum to_gl(const gfx_api::index_type& index)
{
	switch (index)
	{
		case gfx_api::index_type::u16:
			return GL_UNSIGNED_SHORT;
		case gfx_api::index_type::u32:
			return GL_UNSIGNED_INT;
		default:
			debug(LOG_FATAL, "Unrecognised index type");
	}
	return GL_INVALID_ENUM;
}

static GLenum to_gl(const gfx_api::context::context_value property)
{
	switch (property)
	{
		case gfx_api::context::context_value::MAX_ELEMENTS_VERTICES:
			return GL_MAX_ELEMENTS_VERTICES;
		case gfx_api::context::context_value::MAX_ELEMENTS_INDICES:
			return GL_MAX_ELEMENTS_INDICES;
		case gfx_api::context::context_value::MAX_TEXTURE_SIZE:
			return GL_MAX_TEXTURE_SIZE;
		case gfx_api::context::context_value::MAX_SAMPLES:
			return GL_MAX_SAMPLES;
		case gfx_api::context::context_value::MAX_ARRAY_TEXTURE_LAYERS:
			return GL_MAX_ARRAY_TEXTURE_LAYERS;
		case gfx_api::context::context_value::MAX_VERTEX_ATTRIBS:
			return GL_MAX_VERTEX_ATTRIBS;
		default:
			debug(LOG_FATAL, "Unrecognised property type");
	}
	return GL_INVALID_ENUM;
}

// MARK: GL error handling helpers

// NOTE: Not to be used in critical code paths, but more for initialization
static bool _wzGLCheckErrors(int line, const char *function)
{
#if !defined(WZ_STATIC_GL_BINDINGS)
	if (glGetError == nullptr)
	{
		// function not available? can't check...
		return true;
	}
#endif
	GLenum err = glGetError();
	if (err == GL_NO_ERROR)
	{
		return true;
	}

	// otherwise
	bool encounteredCriticalError = false;
	do
	{
		code_part part = LOG_ERROR;
		const char* errAsStr = "??";
		switch (err)
		{
			case GL_INVALID_ENUM:
				errAsStr = "GL_INVALID_ENUM";
				part = LOG_INFO;
				break;
			case GL_INVALID_VALUE:
				errAsStr = "GL_INVALID_VALUE";
				part = LOG_INFO;
				break;
			case GL_INVALID_OPERATION:
				errAsStr = "GL_INVALID_OPERATION";
				part = LOG_INFO;
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				errAsStr = "GL_INVALID_FRAMEBUFFER_OPERATION";
				break;
			case GL_OUT_OF_MEMORY:
				errAsStr = "GL_OUT_OF_MEMORY";
				part = LOG_FATAL;
				// Once GL_OUT_OF_MEMORY is set, the state of the OpenGL context is *undefined*
				encounteredCriticalError = true;
				break;
#ifdef GL_STACK_UNDERFLOW
			case GL_STACK_UNDERFLOW:
				errAsStr = "GL_STACK_UNDERFLOW";
				encounteredCriticalError = true;
				break;
#endif
#ifdef GL_STACK_OVERFLOW
			case GL_STACK_OVERFLOW:
				errAsStr = "GL_STACK_OVERFLOW";
				encounteredCriticalError = true;
				break;
#endif
		}

		if (enabled_debug[part])
		{
			_debug(line, part, function, "Encountered OpenGL Error: %s", errAsStr);
		}
	} while ((err = glGetError()) != GL_NO_ERROR);

	return !encounteredCriticalError;
}

static void _wzGLClearErrors()
{
	// clear OpenGL error queue
#if !defined(WZ_STATIC_GL_BINDINGS)
	if (glGetError != nullptr)
#endif
	{
		while(glGetError() != GL_NO_ERROR) { } // clear the OpenGL error queue
	}
}

// NOTE: Not to be used in critical code paths, but more for initialization
#define wzGLCheckErrors() _wzGLCheckErrors(__LINE__, __FUNCTION__);
#define wzGLClearErrors() _wzGLClearErrors()

// NOTE: Not to be used in critical code paths, but more for initialization
#define ASSERT_GL_NOERRORS_OR_RETURN(retval) \
	do { bool _wzeval = wzGLCheckErrors(); if (!_wzeval) { return retval; } } while (0)

// MARK: gl_gpurendered_texture

gl_gpurendered_texture::gl_gpurendered_texture()
{
	glGenTextures(1, &_id);
}

gl_gpurendered_texture::~gl_gpurendered_texture()
{
	glDeleteTextures(1, &_id);
}

void gl_gpurendered_texture::bind()
{
	glBindTexture((_isArray) ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D, _id);
}

GLenum gl_gpurendered_texture::target() const
{
	return (_isArray) ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
}

unsigned gl_gpurendered_texture::id() const
{
	return _id;
}

size_t gl_gpurendered_texture::backend_internal_value() const
{
	// not currently used in GL backend
	return 0;
}

void gl_gpurendered_texture::unbind()
{
	glBindTexture((_isArray) ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D, 0);
}

// MARK: gl_texture

gl_texture::gl_texture()
{
	glGenTextures(1, &_id);
#if defined(WZ_DEBUG_GFX_API_LEAKS)
	debugLiveTextures.insert(this);
#endif
}

gl_texture::~gl_texture()
{
	glDeleteTextures(1, &_id);
#if defined(WZ_DEBUG_GFX_API_LEAKS)
	debugLiveTextures.erase(this);
#endif
}

void gl_texture::bind()
{
	glBindTexture(GL_TEXTURE_2D, _id);
}

size_t gl_texture::backend_internal_value() const
{
	// not currently used in GL backend
	return 0;
}

void gl_texture::unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

bool gl_texture::upload_internal(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const iV_BaseImage& image)
{
	ASSERT_OR_RETURN(false, image.data() != nullptr, "Attempt to upload image without data");
	ASSERT_OR_RETURN(false, image.pixel_format() == internal_format, "Uploading image to texture with different format");
	size_t width = image.width();
	size_t height = image.height();
	ASSERT(width > 0 && height > 0, "Attempt to upload texture with width or height of 0 (width: %zu, height: %zu)", width, height);

	ASSERT(mip_level <= static_cast<size_t>(std::numeric_limits<GLint>::max()), "mip_level (%zu) exceeds GLint max", mip_level);
	ASSERT(offset_x <= static_cast<size_t>(std::numeric_limits<GLint>::max()), "offset_x (%zu) exceeds GLint max", offset_x);
	ASSERT(offset_y <= static_cast<size_t>(std::numeric_limits<GLint>::max()), "offset_y (%zu) exceeds GLint max", offset_y);
	ASSERT(width <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "width (%zu) exceeds GLsizei max", width);
	ASSERT(height <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "height (%zu) exceeds GLsizei max", height);
	ASSERT(image.data_size() <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "data_size (%zu) exceeds GLsizei max", image.data_size());
	bind();
	ASSERT(gfx_api::format_memory_size(image.pixel_format(), width, height) == image.data_size(), "data_size (%zu) does not match expected format_memory_size(%s, %zu, %zu)=%zu", image.data_size(), gfx_api::format_to_str(image.pixel_format()), width, height, gfx_api::format_memory_size(image.pixel_format(), width, height));
	if (is_uncompressed_format(image.pixel_format()))
	{
		glTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(mip_level), static_cast<GLint>(offset_x), static_cast<GLint>(offset_y), static_cast<GLsizei>(width), static_cast<GLsizei>(height), to_gl_format(image.pixel_format(), gles), GL_UNSIGNED_BYTE, image.data());
	}
	else
	{
		ASSERT_OR_RETURN(false, offset_x == 0 && offset_y == 0, "Trying to upload compressed sub texture");
		GLenum glFormat = to_gl_internalformat(image.pixel_format(), gles);
		glCompressedTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(mip_level), glFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, static_cast<GLsizei>(image.data_size()), image.data());
	}
	unbind();
	return true;
}

bool gl_texture::upload(const size_t& mip_level, const iV_BaseImage& image)
{
	return upload_internal(mip_level, 0, 0, image);
}

bool gl_texture::upload_sub(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const iV_Image& image)
{
	return upload_internal(mip_level, offset_x, offset_y, image);
}

unsigned gl_texture::id()
{
	return _id;
}

gfx_api::texture2dDimensions gl_texture::get_dimensions() const
{
	return {tex_width, tex_height};
}

// MARK: texture_array_mip_level_buffer

struct texture_array_mip_level_buffer
{
public:
	struct MipLevel
	{
		std::vector<uint8_t> buffer;
		size_t width = 0;
		size_t height = 0;
		size_t memorySizePerLayer = 0;
	};
private:
	size_t mip_levels;
	size_t layer_count;
	size_t width;
	size_t height;
	gfx_api::pixel_format internal_format;
	std::unordered_map<size_t, MipLevel> mipLevelDataBuffer;

private:
	MipLevel& getMipLevel(size_t mip_level)
	{
		auto it = mipLevelDataBuffer.find(mip_level);
		if (it != mipLevelDataBuffer.end())
		{
			return it->second;
		}
		MipLevel& level = mipLevelDataBuffer[mip_level];
		level.width = std::max<size_t>(1, width >> mip_level);
		level.height = std::max<size_t>(1, height >> mip_level);
		level.memorySizePerLayer = gfx_api::format_memory_size(internal_format, level.width, level.height);
		size_t mipLevelBufferSize = mipLevelDataBuffer[mip_level].memorySizePerLayer * layer_count;
		level.buffer.resize(mipLevelBufferSize);
		return level;
	}

public:
	texture_array_mip_level_buffer(const size_t& _mipmap_count, const size_t& _layer_count, const size_t& _width, const size_t& _height, const gfx_api::pixel_format& _internal_format)
	: mip_levels(_mipmap_count)
	, layer_count(_layer_count)
	, width(_width)
	, height(_height)
	, internal_format(_internal_format)
	{ }
public:

	bool copy_data_to_buffer(size_t mip_level, size_t layer, const void* data, size_t data_size)
	{
		ASSERT_OR_RETURN(false, mip_level < mip_levels, "Invalid mip_level (%zu)", mip_level);
		ASSERT_OR_RETURN(false, layer < layer_count, "Invalid layer (%zu)", layer);
		MipLevel& level = getMipLevel(mip_level);
		ASSERT_OR_RETURN(false, data_size == level.memorySizePerLayer, "Invalid data_size (%zu, expecting: %zu)", data_size, level.memorySizePerLayer);
		size_t startingBufferOffset = level.memorySizePerLayer * layer;
		memcpy(&(level.buffer[startingBufferOffset]), data, data_size);
		return true;
	}

	const MipLevel* get_mip_level(size_t mip_level) const
	{
		auto it = mipLevelDataBuffer.find(mip_level);
		if (it == mipLevelDataBuffer.end())
		{
			return nullptr;
		}
		return &(it->second);
	}

	const uint8_t* get_read_pointer(size_t mip_level) const
	{
		auto it = mipLevelDataBuffer.find(mip_level);
		if (it == mipLevelDataBuffer.end())
		{
			return nullptr;
		}
		return it->second.buffer.data();
	}

	size_t get_buffer_size(size_t mip_level, size_t layer) const
	{
		auto it = mipLevelDataBuffer.find(mip_level);
		if (it == mipLevelDataBuffer.end())
		{
			return 0;
		}
		return it->second.memorySizePerLayer;
	}

	void clear()
	{
		mipLevelDataBuffer.clear();
	}
};

// MARK: gl_texture_array

gl_texture_array::gl_texture_array()
{
	glGenTextures(1, &_id);
}

gl_texture_array::~gl_texture_array()
{
	glDeleteTextures(1, &_id);
	delete pInternalBuffer;
}

void gl_texture_array::bind()
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, _id);
}

void gl_texture_array::unbind()
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

size_t gl_texture_array::backend_internal_value() const
{
	// not currently used in GL backend
	return 0;
}

bool gl_texture_array::upload_internal(const size_t& layer, const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const iV_BaseImage& image)
{
	ASSERT_OR_RETURN(false, image.data() != nullptr, "Attempt to upload image without data");
	ASSERT_OR_RETURN(false, image.pixel_format() == internal_format, "Uploading image to texture with different format");
	ASSERT_OR_RETURN(false, mip_level < mip_count, "mip_level (%zu) >= mip_count (%zu)", mip_level, mip_count);
	size_t width = image.width();
	size_t height = image.height();
	ASSERT(width > 0 && height > 0, "Attempt to upload texture with width or height of 0 (width: %zu, height: %zu)", width, height);

	ASSERT(layer <= static_cast<size_t>(std::numeric_limits<GLint>::max()), "layer (%zu) exceeds GLint max", layer);
	ASSERT(mip_level <= static_cast<size_t>(std::numeric_limits<GLint>::max()), "mip_level (%zu) exceeds GLint max", mip_level);
	ASSERT(offset_x <= static_cast<size_t>(std::numeric_limits<GLint>::max()), "offset_x (%zu) exceeds GLint max", offset_x);
	ASSERT(offset_y <= static_cast<size_t>(std::numeric_limits<GLint>::max()), "offset_y (%zu) exceeds GLint max", offset_y);
	ASSERT(width <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "width (%zu) exceeds GLsizei max", width);
	ASSERT(height <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "height (%zu) exceeds GLsizei max", height);
	ASSERT(image.data_size() <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "data_size (%zu) exceeds GLsizei max", image.data_size());
	bind();
	ASSERT(gfx_api::format_memory_size(image.pixel_format(), width, height) == image.data_size(), "data_size (%zu) does not match expected format_memory_size(%s, %zu, %zu)=%zu", image.data_size(), gfx_api::format_to_str(image.pixel_format()), width, height, gfx_api::format_memory_size(image.pixel_format(), width, height));

	// Copy to an internal buffer for upload on flush
	ASSERT_OR_RETURN(false, offset_x == 0 && offset_y == 0, "Trying to upload compressed sub texture");
	pInternalBuffer->copy_data_to_buffer(mip_level, layer, image.data(), image.data_size());

	unbind();
	return true;
}

bool gl_texture_array::upload_layer(const size_t& layer, const size_t& mip_level, const iV_BaseImage& image)
{
	return upload_internal(layer, mip_level, 0, 0, image);
}

unsigned gl_texture_array::id()
{
	return _id;
}

void gl_texture_array::flush()
{
	if (pInternalBuffer)
	{
		// If compressed texture, upload each mip level with glCompressedTexImage3D from the client-side buffer
		bind();
		for (size_t i = 0; i < mip_count; ++i)
		{
			const texture_array_mip_level_buffer::MipLevel* pLevel = pInternalBuffer->get_mip_level(i);
			if (!pLevel)
			{
				continue;
			}

			if (is_uncompressed_format(internal_format))
			{
				glTexImage3D(GL_TEXTURE_2D_ARRAY, static_cast<GLint>(i), to_gl_internalformat(internal_format, gles), static_cast<GLsizei>(pLevel->width), static_cast<GLsizei>(pLevel->height), static_cast<GLsizei>(layer_count), 0, to_gl_format(internal_format, gles), GL_UNSIGNED_BYTE,  pLevel->buffer.data());
			}
			else
			{
				glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, static_cast<GLint>(i), to_gl_internalformat(internal_format, gles), static_cast<GLsizei>(pLevel->width), static_cast<GLsizei>(pLevel->height), static_cast<GLsizei>(layer_count), 0, static_cast<GLsizei>(pLevel->buffer.size()), pLevel->buffer.data());
			}
		}
		unbind();
		pInternalBuffer->clear();
	}
}

// MARK: gl_buffer

gl_buffer::gl_buffer(const gfx_api::buffer::usage& usage, const gfx_api::context::buffer_storage_hint& hint)
: usage(usage)
, hint(hint)
{
	glGenBuffers(1, &buffer);
}

gl_buffer::~gl_buffer()
{
	glDeleteBuffers(1, &buffer);
}

void gl_buffer::bind()
{
	glBindBuffer(to_gl(usage), buffer);
}

void gl_buffer::unbind()
{
	glBindBuffer(to_gl(usage), 0);
}

void gl_buffer::upload(const size_t & size, const void * data)
{
	size_t current_FrameNum = gfx_api::context::get().current_FrameNum();
#if defined(DEBUG)
	ASSERT(lastUploaded_FrameNum != current_FrameNum, "Attempt to upload to buffer more than once per frame");
#endif
	lastUploaded_FrameNum = current_FrameNum;

	ASSERT(size > 0, "Attempt to upload buffer of size 0");
	glBindBuffer(to_gl(usage), buffer);
	glBufferData(to_gl(usage), size, data, to_gl(hint));
	buffer_size = size;
	glBindBuffer(to_gl(usage), 0);
}

void gl_buffer::update(const size_t & start, const size_t & size, const void * data, const update_flag flag)
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
	glBindBuffer(to_gl(usage), buffer);
	glBufferSubData(to_gl(usage), start, size, data);
	glBindBuffer(to_gl(usage), 0);
}

size_t gl_buffer::current_buffer_size()
{
	return buffer_size;
}

// MARK: gl_pipeline_state_object

struct program_data
{
	std::string friendly_name;
	std::string vertex_file;
	std::string fragment_file;
	std::vector<std::string> uniform_names;
	std::vector<std::tuple<std::string, GLint>> additional_samplers = {};
};

static const std::map<SHADER_MODE, program_data> shader_to_file_table =
{
	std::make_pair(SHADER_COMPONENT, program_data{ "Component program", "shaders/tcmask.vert", "shaders/tcmask.frag",
		{
			// per-frame global uniforms
			"ProjectionMatrix", "ViewMatrix", "ShadowMapMVPMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular", "fogColor", "fogEnd", "fogStart", "graphicsCycle", "fogEnabled",
			// per-mesh uniforms
			"tcmask", "normalmap", "specularmap", "hasTangents",
			// per-instance uniforms
			"ModelViewMatrix", "NormalMatrix", "colour", "teamcolour", "stretch", "animFrameNumber", "ecmEffect", "alphaTest"
		} }),
	std::make_pair(SHADER_COMPONENT_INSTANCED, program_data{ "Component program", "shaders/tcmask_instanced.vert", "shaders/tcmask_instanced.frag",
		{
			// per-frame global uniforms
			"ProjectionMatrix", "ViewMatrix", "ModelUVLightmapMatrix", "ShadowMapMVPMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular", "fogColor", "ShadowMapCascadeSplits", "ShadowMapSize", "fogEnd", "fogStart", "graphicsCycle", "fogEnabled", "PointLightsPosition", "PointLightsColorAndEnergy", "bucketOffsetAndSize", "PointLightsIndex", "bucketDimensionUsed", "viewportWidth", "viewportHeight",
			// per-mesh uniforms
			"tcmask", "normalmap", "specularmap", "hasTangents", "shieldEffect",
		},
		{
			{"shadowMap", 4},
			{"lightmap_tex", 5}
		} }),
	std::make_pair(SHADER_COMPONENT_DEPTH_INSTANCED, program_data{ "Component program", "shaders/tcmask_depth_instanced.vert", "shaders/tcmask_depth_instanced.frag",
		{
			// per-frame global uniforms
			"ProjectionMatrix", "ViewMatrix"
		} }),
	std::make_pair(SHADER_NOLIGHT, program_data{ "Plain program", "shaders/nolight.vert", "shaders/nolight.frag",
		{
			// per-frame global uniforms
			"ProjectionMatrix", "ViewMatrix", "ShadowMapMVPMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular", "fogColor", "fogEnd", "fogStart", "graphicsCycle", "fogEnabled",
			// per-mesh uniforms
			"tcmask", "normalmap", "specularmap", "hasTangents",
			// per-instance uniforms
			"ModelViewMatrix", "NormalMatrix", "colour", "teamcolour", "stretch", "animFrameNumber", "ecmEffect", "alphaTest"
		} }),
	std::make_pair(SHADER_NOLIGHT_INSTANCED, program_data{ "Plain program", "shaders/nolight_instanced.vert", "shaders/nolight_instanced.frag",
		{
			// per-frame global uniforms
			"ProjectionMatrix", "ViewMatrix", "ModelUVLightmapMatrix", "ShadowMapMVPMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular", "fogColor", "ShadowMapCascadeSplits", "ShadowMapSize", "fogEnd", "fogStart", "graphicsCycle", "fogEnabled", "PointLightsPosition", "PointLightsColorAndEnergy", "bucketOffsetAndSize", "PointLightsIndex", "bucketDimensionUsed", "viewportWidth", "viewportHeight",
			// per-mesh uniforms
			"tcmask", "normalmap", "specularmap", "hasTangents", "shieldEffect",
		},
		{
			{"shadowMap", 4}
		} }),
	std::make_pair(SHADER_TERRAIN, program_data{ "terrain program", "shaders/terrain.vert", "shaders/terrain.frag",
		{ "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "tex", "lightmap_tex", "textureMatrix1", "textureMatrix2",
			"fogColor", "fogEnabled", "fogEnd", "fogStart" } }),
	std::make_pair(SHADER_TERRAIN_DEPTH, program_data{ "terrain_depth program", "shaders/terrain_depth.vert", "shaders/terraindepth.frag",
		{ "ModelViewProjectionMatrix", "paramx2", "paramy2", "lightmap_tex", "paramx2", "paramy2", "fogEnabled", "fogEnd", "fogStart" } }),
	std::make_pair(SHADER_TERRAIN_DEPTHMAP, program_data{ "terrain_depthmap program", "shaders/terrain_depth_only.vert", "shaders/terrain_depth_only.frag",
		{ "ModelViewProjectionMatrix", "fogEnabled", "fogEnd", "fogStart" } }),
	std::make_pair(SHADER_DECALS, program_data{ "decals program", "shaders/decals.vert", "shaders/decals.frag",
		{ "ModelViewProjectionMatrix", "lightTextureMatrix", "paramxlight", "paramylight",
			"fogColor", "fogEnabled", "fogEnd", "fogStart", "tex", "lightmap_tex" } }),
	std::make_pair(SHADER_TERRAIN_COMBINED_CLASSIC, program_data{ "terrain decals program", "shaders/terrain_combined.vert", "shaders/terrain_combined_classic.frag",
			{ "ModelViewProjectionMatrix", "ViewMatrix", "ModelUVLightmapMatrix", "ShadowMapMVPMatrix", "groundScale",
				"cameraPos", "sunPos", "emissiveLight", "ambientLight", "diffuseLight", "specularLight",
				"fogColor", "ShadowMapCascadeSplits", "ShadowMapSize", "fogEnabled", "fogEnd", "fogStart", "quality", "PointLightsPosition", "PointLightsColorAndEnergy", "bucketOffsetAndSize", "PointLightsIndex", "bucketDimensionUsed", "viewportWidth", "viewportHeight",
				"lightmap_tex",
				"groundTex", "groundNormal", "groundSpecular", "groundHeight",
				"decalTex",  "decalNormal",  "decalSpecular",  "decalHeight", "shadowMap" } }),
	std::make_pair(SHADER_TERRAIN_COMBINED_MEDIUM, program_data{ "terrain decals program", "shaders/terrain_combined.vert", "shaders/terrain_combined_medium.frag",
			{ "ModelViewProjectionMatrix", "ViewMatrix", "ModelUVLightmapMatrix", "ShadowMapMVPMatrix", "groundScale",
				"cameraPos", "sunPos", "emissiveLight", "ambientLight", "diffuseLight", "specularLight",
				"fogColor", "ShadowMapCascadeSplits", "ShadowMapSize", "fogEnabled", "fogEnd", "fogStart", "quality", "PointLightsPosition", "PointLightsColorAndEnergy", "bucketOffsetAndSize", "PointLightsIndex", "bucketDimensionUsed", "viewportWidth", "viewportHeight",
				"lightmap_tex",
				"groundTex", "groundNormal", "groundSpecular", "groundHeight",
				"decalTex",  "decalNormal",  "decalSpecular",  "decalHeight", "shadowMap" } }),
	std::make_pair(SHADER_TERRAIN_COMBINED_HIGH, program_data{ "terrain decals program", "shaders/terrain_combined.vert", "shaders/terrain_combined_high.frag",
			{ "ModelViewProjectionMatrix", "ViewMatrix", "ModelUVLightmapMatrix", "ShadowMapMVPMatrix", "groundScale",
				"cameraPos", "sunPos", "emissiveLight", "ambientLight", "diffuseLight", "specularLight",
				"fogColor", "ShadowMapCascadeSplits", "ShadowMapSize", "fogEnabled", "fogEnd", "fogStart", "quality", "PointLightsPosition", "PointLightsColorAndEnergy", "bucketOffsetAndSize", "PointLightsIndex", "bucketDimensionUsed", "viewportWidth", "viewportHeight",
				"lightmap_tex",
				"groundTex", "groundNormal", "groundSpecular", "groundHeight",
				"decalTex",  "decalNormal",  "decalSpecular",  "decalHeight", "shadowMap" } }),
	std::make_pair(SHADER_WATER, program_data{ "water program", "shaders/terrain_water.vert", "shaders/water.frag",
		{ "ModelViewProjectionMatrix", "ModelUVLightmapMatrix", "ModelUV1Matrix", "ModelUV2Matrix",
			"cameraPos", "sunPos",
			"emissiveLight", "ambientLight", "diffuseLight", "specularLight",
			"fogColor", "fogEnabled", "fogEnd", "fogStart", "timeSec",
			"tex1", "tex2", "lightmap_tex" } }),
	std::make_pair(SHADER_WATER_HIGH, program_data{ "high water program", "shaders/terrain_water_high.vert", "shaders/terrain_water_high.frag",
		{ "ModelViewProjectionMatrix", "ViewMatrix", "ModelUVLightmapMatrix", "ShadowMapMVPMatrix",
			"cameraPos", "sunPos",
			"emissiveLight", "ambientLight", "diffuseLight", "specularLight",
			"fogColor", "ShadowMapCascadeSplits", "ShadowMapSize", "fogEnabled", "fogEnd", "fogStart", "timeSec",
			"tex", "tex_nm", "tex_sm", "lightmap_tex"
		},
		{
			{"shadowMap", 4}
		} }),
	std::make_pair(SHADER_WATER_CLASSIC, program_data{ "classic water program", "shaders/terrain_water_classic.vert", "shaders/terrain_water_classic.frag",
		{ "ModelViewProjectionMatrix", "ModelUVLightmapMatrix", "ShadowMapMVPMatrix", "ModelUV1Matrix", "ModelUV2Matrix",
			"cameraPos", "sunPos",
			"fogColor", "fogEnabled", "fogEnd", "fogStart", "timeSec",
			"lightmap_tex", "tex2"} }),
	std::make_pair(SHADER_RECT, program_data{ "Rect program", "shaders/rect.vert", "shaders/rect.frag",
		{ "transformationMatrix", "color" } }),
	std::make_pair(SHADER_RECT_INSTANCED, program_data{ "Rect program", "shaders/rect_instanced.vert", "shaders/rect_instanced.frag",
		{ "ProjectionMatrix" } }),
	std::make_pair(SHADER_TEXRECT, program_data{ "Textured rect program", "shaders/rect.vert", "shaders/texturedrect.frag",
		{ "transformationMatrix", "tuv_offset", "tuv_scale", "color" } }),
	std::make_pair(SHADER_GFX_COLOUR, program_data{ "gfx_color program", "shaders/gfx_color.vert", "shaders/gfx.frag",
		{ "posMatrix" } }),
	std::make_pair(SHADER_GFX_TEXT, program_data{ "gfx_text program", "shaders/gfx_text.vert", "shaders/texturedrect.frag",
		{ "posMatrix", "color" } }),
	std::make_pair(SHADER_SKYBOX, program_data{ "skybox program", "shaders/skybox.vert", "shaders/skybox.frag",
		{ "posMatrix", "color", "fog_color", "fog_enabled" } }),
	std::make_pair(SHADER_GENERIC_COLOR, program_data{ "generic color program", "shaders/generic.vert", "shaders/rect.frag",{ "ModelViewProjectionMatrix", "color" } }),
	std::make_pair(SHADER_LINE, program_data{ "line program", "shaders/line.vert", "shaders/rect.frag",{ "from", "to", "color", "ModelViewProjectionMatrix" } }),
	std::make_pair(SHADER_TEXT, program_data{ "Text program", "shaders/rect.vert", "shaders/text.frag",
		{ "transformationMatrix", "tuv_offset", "tuv_scale", "color" } }),
	std::make_pair(SHADER_DEBUG_TEXTURE2D_QUAD, program_data{ "Debug texture quad program", "shaders/quad_texture2d.vert", "shaders/quad_texture2d.frag",
		{ "transformationMatrix", "uvTransformMatrix", "swizzle", "color", "texture" } }),
	std::make_pair(SHADER_DEBUG_TEXTURE2DARRAY_QUAD, program_data{ "Debug texture array quad program", "shaders/quad_texture2darray.vert", "shaders/quad_texture2darray.frag",
		{ "transformationMatrix", "uvTransformMatrix", "swizzle", "color", "layer", "texture" } }),
	std::make_pair(SHADER_WORLD_TO_SCREEN, program_data{ "World to screen quad program", "shaders/world_to_screen.vert", "shaders/world_to_screen.frag",
		{ "gamma" } })
};

enum SHADER_VERSION
{
	VERSION_120,
	VERSION_130,
	VERSION_140,
	VERSION_150_CORE,
	VERSION_330_CORE,
	VERSION_400_CORE,
	VERSION_410_CORE,
	VERSION_FIXED_IN_FILE,
	VERSION_AUTODETECT_FROM_LEVEL_LOAD
};

enum SHADER_VERSION_ES
{
	VERSION_ES_100,
	VERSION_ES_300
};

const char * shaderVersionString(SHADER_VERSION version)
{
	switch(version)
	{
		case VERSION_120:
			return "#version 120\n";
		case VERSION_130:
			return "#version 130\n";
		case VERSION_140:
			return "#version 140\n";
		case VERSION_150_CORE:
			return "#version 150 core\n";
		case VERSION_330_CORE:
			return "#version 330 core\n";
		case VERSION_400_CORE:
			return "#version 400 core\n";
		case VERSION_410_CORE:
			return "#version 410 core\n";
		case VERSION_AUTODETECT_FROM_LEVEL_LOAD:
			return "";
		case VERSION_FIXED_IN_FILE:
			return "";
			// Deliberately omit "default:" case to trigger a compiler warning if the SHADER_VERSION enum is expanded but the new cases aren't handled here
	}
	return ""; // Should not not reach here - silence a GCC warning
}

const char * shaderVersionString(SHADER_VERSION_ES version)
{
	switch(version)
	{
		case VERSION_ES_100:
			return "#version 100\n";
		case VERSION_ES_300:
			return "#version 300 es\n";
			// Deliberately omit "default:" case to trigger a compiler warning if the SHADER_VERSION_ES enum is expanded but the new cases aren't handled here
	}
	return ""; // Should not reach here - silence a GCC warning
}

GLint wz_GetGLIntegerv(GLenum pname, GLint defaultValue = 0)
{
	GLint retVal = defaultValue;
#if !defined(WZ_STATIC_GL_BINDINGS)
	ASSERT_OR_RETURN(retVal, glGetIntegerv != nullptr, "glGetIntegerv is null");
	if (glGetError != nullptr)
#endif
	{
		while(glGetError() != GL_NO_ERROR) { } // clear the OpenGL error queue
	}
	glGetIntegerv(pname, &retVal);
	GLenum err = GL_NO_ERROR;
#if !defined(WZ_STATIC_GL_BINDINGS)
	if (glGetError != nullptr)
#endif
	{
		err = glGetError();
	}
	if (err != GL_NO_ERROR)
	{
		retVal = defaultValue;
	}
	return retVal;
}

SHADER_VERSION getMinimumShaderVersionForCurrentGLContext()
{
	// Determine the shader version directive we should use by examining the current OpenGL context
	GLint gl_majorversion = wz_GetGLIntegerv(GL_MAJOR_VERSION, 0);
	GLint gl_minorversion = wz_GetGLIntegerv(GL_MINOR_VERSION, 0);

	SHADER_VERSION version = VERSION_120; // for OpenGL < 3.2, we default to VERSION_120 shaders
	if ((gl_majorversion > 3) || ((gl_majorversion == 3) && (gl_minorversion >= 2)))
	{
		// OpenGL 3.2+
		// Since WZ only supports Core contexts with OpenGL 3.2+, we cannot use the version_120 directive
		// (which only works in compatibility contexts). Instead, use GLSL version "150 core".
		version = VERSION_150_CORE;
	}
	return version;
}

SHADER_VERSION getMaximumShaderVersionForCurrentGLContext(SHADER_VERSION minSupportedShaderVersion = VERSION_120, SHADER_VERSION maxSupportedShaderVersion = VERSION_410_CORE)
{
	ASSERT(minSupportedShaderVersion <= maxSupportedShaderVersion, "minSupportedShaderVersion > maxSupportedShaderVersion");

	// Instead of querying the GL_SHADING_LANGUAGE_VERSION string and trying to parse it,
	// which is rife with difficulties because drivers can report very different strings (and formats),
	// use the known (and explicit) mapping table between OpenGL version and supported GLSL version.

	GLint gl_majorversion = wz_GetGLIntegerv(GL_MAJOR_VERSION, 0);
	GLint gl_minorversion = wz_GetGLIntegerv(GL_MINOR_VERSION, 0);

	// For OpenGL < 3.2, default to VERSION_120 shaders
	SHADER_VERSION version = VERSION_120;
	if(gl_majorversion == 3)
	{
		switch(gl_minorversion)
		{
			case 0: // 3.0 => 1.30
				version = VERSION_130;
				break;
			case 1: // 3.1 => 1.40
				version = VERSION_140;
				break;
			case 2: // 3.2 => 1.50
				version = VERSION_150_CORE;
				break;
			case 3: // 3.3 => 3.30
				version = VERSION_330_CORE;
				break;
			default:
				// Return the 3.3 value
				version = VERSION_330_CORE;
				break;
		}
	}
	else if (gl_majorversion == 4)
	{
		switch(gl_minorversion)
		{
			case 0: // 4.0 => 4.00
				version = VERSION_400_CORE;
				break;
			case 1: // 4.1 => 4.10
				version = VERSION_410_CORE;
				break;
			default:
				// Return the 4.1 value
				// NOTE: Nothing above OpenGL 4.1 is supported on macOS
				version = VERSION_410_CORE;
				break;
		}
	}
	else if (gl_majorversion > 4)
	{
		// Return the OpenGL 4.1 value (for now)
		version = VERSION_410_CORE;
	}

	version = std::max(version, minSupportedShaderVersion);
	version = std::min(version, maxSupportedShaderVersion);

	return version;
}

SHADER_VERSION_ES getMaximumShaderVersionForCurrentGLESContext(SHADER_VERSION_ES minSupportedShaderVersion = VERSION_ES_100, SHADER_VERSION_ES maxSupportedShaderVersion = VERSION_ES_300)
{
	ASSERT(minSupportedShaderVersion <= maxSupportedShaderVersion, "minSupportedShaderVersion > maxSupportedShaderVersion");

	// Instead of querying the GL_SHADING_LANGUAGE_VERSION string and trying to parse it,
	// which is rife with difficulties because drivers can report very different strings (and formats),
	// use the known (and explicit) mapping table between OpenGL version and supported GLSL version.

	GLint gl_majorversion = wz_GetGLIntegerv(GL_MAJOR_VERSION, 0);
	//GLint gl_minorversion = wz_GetGLIntegerv(GL_MINOR_VERSION, 0);

	// For OpenGL ES < 3.0, default to VERSION_ES_100 shaders
	SHADER_VERSION_ES version = VERSION_ES_100;
	if(gl_majorversion == 3)
	{
		return VERSION_ES_300;
	}
	else if (gl_majorversion > 3)
	{
		// Return the OpenGL ES 3.0 value (for now)
		version = VERSION_ES_300;
	}

	version = std::max(version, minSupportedShaderVersion);
	version = std::min(version, maxSupportedShaderVersion);

	return version;
}

template<SHADER_MODE shader>
typename std::pair<std::type_index, std::function<void(const void*, size_t)>> gl_pipeline_state_object::uniform_binding_entry()
{
	return std::make_pair(std::type_index(typeid(gfx_api::constant_buffer_type<shader>)), [this](const void* buffer, size_t buflen) {
		ASSERT_OR_RETURN(, buflen == sizeof(const gfx_api::constant_buffer_type<shader>), "Unexpected buffer size; received %zu, expecting %zu", buflen, sizeof(const gfx_api::constant_buffer_type<shader>));
		this->set_constants(*reinterpret_cast<const gfx_api::constant_buffer_type<shader>*>(buffer));
	});
}

template<typename T>
typename std::pair<std::type_index, std::function<void(const void*, size_t)>>gl_pipeline_state_object::uniform_setting_func()
{
	return std::make_pair(std::type_index(typeid(T)), [this](const void* buffer, size_t buflen) {
		ASSERT_OR_RETURN(, buflen == sizeof(const T), "Unexpected buffer size; received %zu, expecting %zu", buflen, sizeof(const T));
		this->set_constants(*reinterpret_cast<const T*>(buffer));
	});
}

gl_pipeline_state_object::gl_pipeline_state_object(gl_context& ctx, bool fragmentHighpFloatAvailable, bool fragmentHighpIntAvailable, bool patchFragmentShaderMipLodBias, const gfx_api::pipeline_create_info& createInfo, optional<float> mipLodBias, const gfx_api::lighting_constants& shadowConstants) :
desc(createInfo.state_desc), vertex_buffer_desc(createInfo.attribute_descriptions)
{
	std::string vertexShaderHeader;
	std::string fragmentShaderHeader;

	if (!ctx.gles)
	{
		// Determine the shader version directive we should use by examining the current OpenGL context
		// (The built-in shaders support (and have been tested with) VERSION_120, VERSION_150_CORE, VERSION_330_CORE)
		const char *shaderVersionStr = shaderVersionString(getMaximumShaderVersionForCurrentGLContext(VERSION_120, VERSION_330_CORE));

		vertexShaderHeader = shaderVersionStr;
		fragmentShaderHeader = shaderVersionStr;
	}
	else
	{
		// Determine the shader version directive we should use by examining the current OpenGL ES context
		// (The built-in shaders support (and have been tested with) VERSION_ES_100 and VERSION_ES_300)
		const char *shaderVersionStr = shaderVersionString(getMaximumShaderVersionForCurrentGLESContext(VERSION_ES_100, VERSION_ES_300));

		vertexShaderHeader = shaderVersionStr;
		fragmentShaderHeader = shaderVersionStr;
		// OpenGL ES Shading Language - 4. Variables and Types - pp. 35-36
		// https://www.khronos.org/registry/gles/specs/2.0/GLSL_ES_Specification_1.0.17.pdf?#page=41
		//
		// > The fragment language has no default precision qualifier for floating point types.
		// > Hence for float, floating point vector and matrix variable declarations, either the
		// > declaration must include a precision qualifier or the default float precision must
		// > have been previously declared.
#if defined(__EMSCRIPTEN__)
		vertexShaderHeader += "precision highp float;\n";
		fragmentShaderHeader += "precision highp float;precision highp int;\n";
#else
		fragmentShaderHeader += "#if GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\nprecision highp int;\n#else\nprecision mediump float;\n#endif\n";
#endif
		fragmentShaderHeader += "#if __VERSION__ >= 300 || defined(GL_EXT_texture_array)\nprecision lowp sampler2DArray;\n#endif\n";
		fragmentShaderHeader += "#if __VERSION__ >= 300\nprecision lowp sampler2DShadow;\nprecision lowp sampler2DArrayShadow;\n#endif\n";
	}

	build_program(ctx,
				  fragmentHighpFloatAvailable, fragmentHighpIntAvailable, patchFragmentShaderMipLodBias,
				  shader_to_file_table.at(createInfo.shader_mode).friendly_name,
				  vertexShaderHeader.c_str(),
				  shader_to_file_table.at(createInfo.shader_mode).vertex_file,
				  fragmentShaderHeader.c_str(),
				  shader_to_file_table.at(createInfo.shader_mode).fragment_file,
				  shader_to_file_table.at(createInfo.shader_mode).uniform_names,
				  shader_to_file_table.at(createInfo.shader_mode).additional_samplers,
				  mipLodBias, shadowConstants);

	const std::unordered_map < std::type_index, std::function<void(const void*, size_t)>> uniforms_bind_table =
	{
		uniform_setting_func<gfx_api::Draw3DShapeGlobalUniforms>(),
		uniform_setting_func<gfx_api::Draw3DShapePerMeshUniforms>(),
		uniform_setting_func<gfx_api::Draw3DShapePerInstanceUniforms>(),
		uniform_setting_func<gfx_api::Draw3DShapeInstancedGlobalUniforms>(),
		uniform_setting_func<gfx_api::Draw3DShapeInstancedPerMeshUniforms>(),
		uniform_setting_func<gfx_api::Draw3DShapeInstancedDepthOnlyGlobalUniforms>(),
		uniform_binding_entry<SHADER_TERRAIN>(),
		uniform_binding_entry<SHADER_TERRAIN_DEPTH>(),
		uniform_binding_entry<SHADER_TERRAIN_DEPTHMAP>(),
		uniform_binding_entry<SHADER_DECALS>(),
		uniform_setting_func<gfx_api::TerrainCombinedUniforms>(),
		uniform_binding_entry<SHADER_WATER>(),
		uniform_binding_entry<SHADER_WATER_HIGH>(),
		uniform_binding_entry<SHADER_WATER_CLASSIC>(),
		uniform_binding_entry<SHADER_RECT>(),
		uniform_binding_entry<SHADER_TEXRECT>(),
		uniform_binding_entry<SHADER_GFX_COLOUR>(),
		uniform_binding_entry<SHADER_GFX_TEXT>(),
		uniform_binding_entry<SHADER_SKYBOX>(),
		uniform_binding_entry<SHADER_GENERIC_COLOR>(),
		uniform_binding_entry<SHADER_RECT_INSTANCED>(),
		uniform_binding_entry<SHADER_LINE>(),
		uniform_binding_entry<SHADER_TEXT>(),
		uniform_binding_entry<SHADER_DEBUG_TEXTURE2D_QUAD>(),
		uniform_binding_entry<SHADER_DEBUG_TEXTURE2DARRAY_QUAD>(),
		uniform_binding_entry<SHADER_WORLD_TO_SCREEN>()
	};

	for (auto& uniform_block : createInfo.uniform_blocks)
	{
		auto it = uniforms_bind_table.find(uniform_block);
		if (it == uniforms_bind_table.end())
		{
			ASSERT(false, "Missing mapping for uniform block type: %s", uniform_block.name());
			uniform_bind_functions.push_back(nullptr);
			continue;
		}
		uniform_bind_functions.push_back(it->second);
	}
}

void gl_pipeline_state_object::set_constants(const void* buffer, const size_t& size)
{
	uniform_bind_functions[0](buffer, size);
}

void gl_pipeline_state_object::set_uniforms(const size_t& first, const std::vector<std::tuple<const void*, size_t>>& uniform_blocks)
{
	for (size_t i = 0, e = uniform_blocks.size(); i < e && (first + i) < uniform_bind_functions.size(); ++i)
	{
		const auto& uniform_bind_function = uniform_bind_functions[first + i];
		auto* buffer = std::get<0>(uniform_blocks[i]);
		if (buffer == nullptr)
		{
			continue;
		}
		uniform_bind_function(buffer, std::get<1>(uniform_blocks[i]));
	}
}


void gl_pipeline_state_object::bind()
{
	glUseProgram(program);
	switch (desc.blend_state)
	{
		case REND_OPAQUE:
			glDisable(GL_BLEND);
			break;

		case REND_ALPHA:
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case REND_ADDITIVE:
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;

		case REND_MULTIPLICATIVE:
			glEnable(GL_BLEND);
			glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case REND_PREMULTIPLIED:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case REND_TEXT:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA /* Should be GL_ONE_MINUS_SRC1_COLOR, if supported. Also, gl_FragData[1] then needs to be set in text.frag. */);
			break;
	}

	switch (desc.depth_mode)
	{
		case DEPTH_CMP_LEQ_WRT_OFF:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDepthMask(GL_FALSE);
			break;
		case DEPTH_CMP_LEQ_WRT_ON:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDepthMask(GL_TRUE);
			break;
		case DEPTH_CMP_ALWAYS_WRT_ON:
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			break;

		case DEPTH_CMP_ALWAYS_WRT_OFF:
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			break;
	}

	if (desc.output_mask == 0)
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	else
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	if (desc.offset)
		glEnable(GL_POLYGON_OFFSET_FILL);
	else
		glDisable(GL_POLYGON_OFFSET_FILL);

	switch (desc.stencil)
	{
		case gfx_api::stencil_mode::stencil_shadow_quad:
			glEnable(GL_STENCIL_TEST);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glStencilMask(~0);
			glStencilFunc(GL_LESS, 0, ~0);
			break;
		case gfx_api::stencil_mode::stencil_shadow_silhouette:
			glEnable(GL_STENCIL_TEST);
			glStencilMask(~0);
			glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
			glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
			glStencilFunc(GL_ALWAYS, 0, ~0);

			break;
		case gfx_api::stencil_mode::stencil_disabled:
			glDisable(GL_STENCIL_TEST);
			//glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
			break;
	}

	switch (desc.cull)
	{
		case gfx_api::cull_mode::back:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;
		case gfx_api::cull_mode::shadow_mapping:
		case gfx_api::cull_mode::front:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			break;
		case gfx_api::cull_mode::none:
			glDisable(GL_CULL_FACE);
			break;
	}
}

// Read shader into text buffer
std::string gl_pipeline_state_object::readShaderBuf(const std::string& name, std::vector<std::string> ancestorIncludePaths)
{
	PHYSFS_file	*fp;
	int	filesize;
	char *buffer;

	fp = PHYSFS_openRead(name.c_str());
	if (fp == nullptr)
	{
		debug(LOG_FATAL, "Failed to read required shader file: %s\nPlease remove any mods and/or reinstall Warzone 2100.", name.c_str());
		return "";
	}
	debug(LOG_3D, "Reading...[directory: %s] %s", PHYSFS_getRealDir(name.c_str()), name.c_str());
	filesize = PHYSFS_fileLength(fp);
	buffer = (char *)malloc(filesize + 1);
	if (buffer)
	{
		WZ_PHYSFS_readBytes(fp, buffer, filesize);
		buffer[filesize] = '\0';
	}
	PHYSFS_close(fp);

	std::string strResult;
	if (buffer)
	{
		strResult = buffer;
		free(buffer);
	}

	std::vector<std::string> ancestorIncludesPlusMe = ancestorIncludePaths;
	ancestorIncludesPlusMe.push_back(name);
	patchShaderHandleIncludes(strResult, ancestorIncludesPlusMe);

	return strResult;
}

static bool regex_replace_func(std::string& input, const std::regex& re, const std::function<std::string (const std::smatch& match)>& replaceFunc, std::regex_constants::match_flag_type flags = std::regex_constants::match_default)
{
	std::string result;
	auto m = std::sregex_iterator(input.begin(), input.end(), re, flags);
	auto end = std::sregex_iterator();
	auto last_m = m;
	size_t num_replacements = 0;

	auto out = std::back_inserter(result);

	for (; m != end; ++m)
	{
		out = std::copy(m->prefix().first, m->prefix().second, out);
		out = m->format(out, replaceFunc(*m), flags);
		last_m = m;
		++num_replacements;
	}

	if (num_replacements == 0)
	{
		return false;
	}

	out = std::copy(last_m->suffix().first, last_m->suffix().second, out);

	input = std::move(result);

	return true;
}

void gl_pipeline_state_object::patchShaderHandleIncludes(std::string& shaderStr, std::vector<std::string> ancestorIncludePaths)
{
	ASSERT_OR_RETURN(, !ancestorIncludePaths.empty(), "No ancestors?");

	const auto re = std::regex("\\s*#include\\s+(\\\".*\\\")\\s*", std::regex_constants::ECMAScript);

	// compute parent path basedir
	auto parentPathInfo = WzPathInfo::fromPlatformIndependentPath(ancestorIncludePaths.back());

	regex_replace_func(shaderStr, re, [&parentPathInfo, &ancestorIncludePaths](const std::smatch& match) -> std::string {
		std::string relativeIncludePath = match.str(1);

		// remove any surrounding " "
		relativeIncludePath.erase(relativeIncludePath.begin(), std::find_if(relativeIncludePath.begin(), relativeIncludePath.end(), [](char ch) {
			return ch != '"';
		}));
		relativeIncludePath.erase(std::find_if(relativeIncludePath.rbegin(), relativeIncludePath.rend(), [](char ch) {
			return ch != '"';
		}).base(), relativeIncludePath.end());

		std::string fullIncludePath = parentPathInfo.path() + "/" + relativeIncludePath;

		if (ancestorIncludePaths.size() > 5)
		{
			debug(LOG_ERROR, "Nested includes depth > 5 - not loading: %s", fullIncludePath.c_str());
			return "\n";
		}

		return "\n" + readShaderBuf(fullIncludePath, ancestorIncludePaths) + "\n";
	});
}

// Retrieve shader compilation errors
void gl_pipeline_state_object::printShaderInfoLog(code_part part, GLuint shader)
{
	GLint infologLen = 0;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0)
	{
		GLint charsWritten = 0;
		GLchar *infoLog = (GLchar *)malloc(infologLen);

		glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);

		// Display log in VS output log
#if defined(WZ_CC_MSVC) && defined(DEBUG)
		OutputDebugStringA(infoLog);
#endif

		debug(part, "Shader info log: %s", infoLog);
		free(infoLog);
	}
}

// Retrieve shader linkage errors
void gl_pipeline_state_object::printProgramInfoLog(code_part part, GLuint program)
{
	GLint infologLen = 0;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0)
	{
		GLint charsWritten = 0;
		GLchar *infoLog = (GLchar *)malloc(infologLen);

		glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog);
		debug(part, "Program info log: %s", infoLog);
		free(infoLog);
	}
}

void gl_pipeline_state_object::getLocs(const std::vector<std::tuple<std::string, GLint>> &samplersToBind)
{
	glUseProgram(program);

	// Uniforms, these never change.
	GLint locTex[4] = {-1};
	locTex[0] = glGetUniformLocation(program, "Texture");
	locTex[1] = glGetUniformLocation(program, "TextureTcmask");
	locTex[2] = glGetUniformLocation(program, "TextureNormal");
	locTex[3] = glGetUniformLocation(program, "TextureSpecular");

	for (GLint i = 0; i < 4; ++i)
	{
		if (locTex[i] != -1)
		{
			glUniform1i(locTex[i], i);
		}
	}

	// additional sampler uniforms
	for (const auto& uniformSampler : samplersToBind)
	{
		GLint loc = glGetUniformLocation(program, std::get<0>(uniformSampler).c_str());
		if (loc != -1)
		{
			glUniform1i(loc, std::get<1>(uniformSampler));
		}
		else
		{
			debug(LOG_3D, "Missing expected sampler uniform: %s", std::get<0>(uniformSampler).c_str());
		}
	}

}

static std::unordered_set<std::string> getUniformNamesFromSource(const char* shaderContents)
{
	if (shaderContents == nullptr)
	{
		debug(LOG_INFO, "shaderContents is null");
		return {};
	}

	std::unordered_set<std::string> uniformNames;

	// White space: the space character, horizontal tab, vertical tab, form feed, carriage-return, and line-feed.
	const char glsl_whitespace_chars[] = " \t\v\f\r\n";

	const auto re = std::regex("uniform.*(?![^\\{]*\\})(?=[=\\[;])", std::regex_constants::ECMAScript);

	std::cregex_iterator next(shaderContents, shaderContents + strlen(shaderContents), re);
	std::cregex_iterator end;
	while (next != end)
	{
		std::cmatch uniformLineMatch = *next;
		std::string uniformLineStr = uniformLineMatch.str();

		// trim glsl whitespace chars from beginning / end
		uniformLineStr = uniformLineStr.substr(uniformLineStr.find_first_not_of(glsl_whitespace_chars));
		uniformLineStr.erase(uniformLineStr.find_last_not_of(glsl_whitespace_chars)+1);

		size_t lastWhitespaceIdx = uniformLineStr.find_last_of(glsl_whitespace_chars);
		if (lastWhitespaceIdx != std::string::npos)
		{
			std::string uniformName = uniformLineMatch.str().substr(lastWhitespaceIdx + 1);
			if (!uniformName.empty())
			{
				uniformNames.insert(uniformName);
			}
		}
		next++;
	}

	return uniformNames;
}

static std::tuple<std::string, std::unordered_map<std::string, std::string>> renameDuplicateFragmentShaderUniforms(const std::string& vertexShaderContents, const std::string& fragmentShaderContents)
{
	std::unordered_map<std::string, std::string> duplicateFragmentUniformNameMap;

	const auto vertexUniformNames = getUniformNamesFromSource(vertexShaderContents.c_str());
	const auto fragmentUniformNames = getUniformNamesFromSource(fragmentShaderContents.c_str());

	std::string modifiedFragmentShaderSource = fragmentShaderContents;
	for (const auto& fragmentUniform : fragmentUniformNames)
	{
		if (vertexUniformNames.count(fragmentUniform) > 0)
		{
			// duplicate uniform name found - rename it!
			const std::string replacementUniformName = std::string("wzfix_frag_") + fragmentUniform;
			duplicateFragmentUniformNameMap[fragmentUniform] = replacementUniformName;

			// and replace it in the fragment shader source
			modifiedFragmentShaderSource = std::regex_replace(
				modifiedFragmentShaderSource,
				std::regex(std::string("\\b") + fragmentUniform + "\\b", std::regex_constants::ECMAScript),
				replacementUniformName
			);
		}
	}

	return std::tuple<std::string, std::unordered_map<std::string, std::string>>(modifiedFragmentShaderSource, duplicateFragmentUniformNameMap);
}

static bool regex_replace_wrapper(std::string& input, const std::regex& re, const std::string& replace, std::regex_constants::match_flag_type flags = std::regex_constants::match_default)
{
	std::string result;
	auto m = std::sregex_iterator(input.begin(), input.end(), re, flags);
	auto end = std::sregex_iterator();
	auto last_m = m;
	size_t num_replacements = 0;

	auto out = std::back_inserter(result);

	for (; m != end; ++m)
	{
		out = std::copy(m->prefix().first, m->prefix().second, out);
		out = m->format(out, replace, flags);
		last_m = m;
		++num_replacements;
	}

	if (num_replacements == 0)
	{
		return false;
	}

	out = std::copy(last_m->suffix().first, last_m->suffix().second, out);

	input = std::move(result);

	return true;
}

static void patchFragmentShaderTextureLodBias(std::string& fragmentShaderStr, float mipLodBias)
{
	// Look for:
	// #define WZ_MIP_LOAD_BIAS 0.f
	const auto re = std::regex("#define WZ_MIP_LOAD_BIAS .*", std::regex_constants::ECMAScript);

	std::string floatAsString = astringf("%f", mipLodBias);
	size_t lastNon0Pos = floatAsString.find_last_not_of('0');
	if (lastNon0Pos != std::string::npos)
	{
		floatAsString = floatAsString.substr(0, lastNon0Pos + 1);
	}
	else
	{
		// only 0?
		return;
	}
	ASSERT(floatAsString.find_last_not_of("0123456789.-") == std::string::npos, "Found unexpected / invalid character in: %s", floatAsString.c_str());

	fragmentShaderStr = std::regex_replace(fragmentShaderStr, re, astringf("#define WZ_MIP_LOAD_BIAS %s", floatAsString.c_str()));
}

static bool patchFragmentShaderPointLightsDefines(std::string& fragmentShaderStr, const gfx_api::lighting_constants& lightingConstants)
{
	const auto defines = {
		std::make_pair("WZ_MAX_POINT_LIGHTS", gfx_api::max_lights),
		std::make_pair("WZ_MAX_INDEXED_POINT_LIGHTS", gfx_api::max_indexed_lights),
		std::make_pair("WZ_BUCKET_DIMENSION", gfx_api::bucket_dimension),
		std::make_pair("WZ_POINT_LIGHT_ENABLED", static_cast<size_t>(lightingConstants.isPointLightPerPixelEnabled)),
	};

	const auto& replacer = [&fragmentShaderStr](const std::string& define, const auto& value) -> bool {
		const auto re_1 = std::regex(fmt::format("#define {} .*", define), std::regex_constants::ECMAScript);
		return regex_replace_wrapper(fragmentShaderStr, re_1, fmt::format("#define {} {}", define, value));
	};
	bool foundAndReplaced_PointLightsDefine = false;
	for (const auto& p : defines)
	{
		foundAndReplaced_PointLightsDefine = replacer(p.first, p.second) || foundAndReplaced_PointLightsDefine;
	}
	return foundAndReplaced_PointLightsDefine;
}

static bool patchFragmentShaderShadowConstants(std::string& fragmentShaderStr, const gfx_api::lighting_constants& shadowConstants)
{
	// #define WZ_SHADOW_MODE <number>
	const auto re_1 = std::regex("#define WZ_SHADOW_MODE .*", std::regex_constants::ECMAScript);
	bool foundAndReplaced_shadowMode = regex_replace_wrapper(fragmentShaderStr, re_1, astringf("#define WZ_SHADOW_MODE %u", shadowConstants.shadowMode));

	// #define WZ_SHADOW_FILTER_SIZE <number>
	const auto re_2 = std::regex("#define WZ_SHADOW_FILTER_SIZE .*", std::regex_constants::ECMAScript);
	bool foundAndReplaced_shadowFilterSize = regex_replace_wrapper(fragmentShaderStr, re_2, astringf("#define WZ_SHADOW_FILTER_SIZE %u", shadowConstants.shadowFilterSize));

	// #define WZ_SHADOW_CASCADES_COUNT <number>
	const auto re_3 = std::regex("#define WZ_SHADOW_CASCADES_COUNT .*", std::regex_constants::ECMAScript);
	bool foundAndReplaced_shadowCascadesCount = regex_replace_wrapper(fragmentShaderStr, re_3, astringf("#define WZ_SHADOW_CASCADES_COUNT %u", shadowConstants.shadowCascadesCount));

	return foundAndReplaced_shadowMode || foundAndReplaced_shadowFilterSize || foundAndReplaced_shadowCascadesCount;
}

void gl_pipeline_state_object::build_program(gl_context& ctx, bool fragmentHighpFloatAvailable, bool fragmentHighpIntAvailable,
											 bool patchFragmentShaderMipLodBias,
											 const std::string& programName,
											 const char * vertex_header, const std::string& vertexPath,
											 const char * fragment_header, const std::string& fragmentPath,
											 const std::vector<std::string> &uniformNames,
											 const std::vector<std::tuple<std::string, GLint>> &samplersToBind,
											 optional<float> mipLodBias, const gfx_api::lighting_constants& lightingConstants)
{
	GLint status;
	bool success = true; // Assume overall success

	std::unordered_set<std::size_t> expectedVertexAttribLoc;
	for (const auto& buffDesc : vertex_buffer_desc)
	{
		for (const auto& attrib : buffDesc.attributes)
		{
			expectedVertexAttribLoc.insert(attrib.id);
		}
	}

	auto bindVertexAttribLocationIfUsed = [&expectedVertexAttribLoc](GLuint prg, GLuint index, const GLchar *name) {
		if (expectedVertexAttribLoc.count(index) > 0)
		{
			glBindAttribLocation(prg, index, name);
		}
	};

	program = glCreateProgram();
	bindVertexAttribLocationIfUsed(program, 0, "vertex");
	bindVertexAttribLocationIfUsed(program, 1, "vertexTexCoord");
	bindVertexAttribLocationIfUsed(program, 2, "vertexColor");
	bindVertexAttribLocationIfUsed(program, 3, "vertexNormal");
	bindVertexAttribLocationIfUsed(program, 4, "vertexTangent");
	// only needed for instanced mesh rendering
	bindVertexAttribLocationIfUsed(program, gfx_api::instance_modelMatrix, "instanceModelMatrix"); // uses 4 slots
	static_assert(gfx_api::instance_modelMatrix + 4 == gfx_api::instance_packedValues, "");
	bindVertexAttribLocationIfUsed(program, gfx_api::instance_packedValues, "instancePackedValues");
	bindVertexAttribLocationIfUsed(program, gfx_api::instance_Colour, "instanceColour");
	bindVertexAttribLocationIfUsed(program, gfx_api::instance_TeamColour, "instanceTeamColour");
	ASSERT_OR_RETURN(, program, "Could not create shader program!");
	// only needed for new terrain renderer
	bindVertexAttribLocationIfUsed(program, gfx_api::terrain_tileNo, "tileNo");
	bindVertexAttribLocationIfUsed(program, gfx_api::terrain_grounds, "grounds");
	bindVertexAttribLocationIfUsed(program, gfx_api::terrain_groundWeights, "groundWeights");

	std::string vertexShaderContents;

	if (!vertexPath.empty())
	{
		success = false; // Assume failure before reading shader file

		vertexShaderContents = readShaderBuf(vertexPath);
		if (!vertexShaderContents.empty())
		{
			GLuint shader = glCreateShader(GL_VERTEX_SHADER);
			vertexShader = shader;

			const char* ShaderStrings[2] = { vertex_header, vertexShaderContents.c_str() };

			glShaderSource(shader, 2, ShaderStrings, nullptr);
			glCompileShader(shader);

			// Check for compilation errors
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status)
			{
				debug(LOG_ERROR, "Vertex shader compilation has failed [%s]", vertexPath.c_str());
				printShaderInfoLog(LOG_ERROR, shader);
			}
			else
			{
				printShaderInfoLog(LOG_3D, shader);
				glAttachShader(program, shader);
				success = true;
			}
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
			ctx.wzGLObjectLabel(GL_SHADER, shader, -1, vertexPath.c_str());
#endif
		}
	}

	std::vector<std::string> duplicateFragmentUniformNames;

	if (success && !fragmentPath.empty())
	{
		success = false; // Assume failure before reading shader file

		std::string fragmentShaderStr = readShaderBuf(fragmentPath);
		if (!fragmentShaderStr.empty())
		{
			GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
			fragmentShader = shader;

			if (!fragmentHighpFloatAvailable || !fragmentHighpIntAvailable)
			{
				// rename duplicate uniforms
				// to avoid conflicting uniform precision issues
				std::unordered_map<std::string, std::string> fragmentUniformNameChanges;
				std::tie(fragmentShaderStr, fragmentUniformNameChanges) = renameDuplicateFragmentShaderUniforms(vertexShaderContents, fragmentShaderStr);

				duplicateFragmentUniformNames.resize(uniformNames.size());
				for (const auto& it : fragmentUniformNameChanges)
				{
					const auto& originalUniformName = it.first;
					const auto& replacementUniformName = it.second;

					debug(LOG_3D, " - Found duplicate uniform name in fragment shader: %s", originalUniformName.c_str());

					// find uniform index in global uniforms array
					const auto itr = std::find(uniformNames.begin(), uniformNames.end(), originalUniformName);
					if (itr != uniformNames.end())
					{
						size_t uniformIdx = std::distance(uniformNames.begin(), itr);
						duplicateFragmentUniformNames[uniformIdx] = replacementUniformName;

						debug(LOG_3D, "  - Renaming \"%s\" -> \"%s\" in fragment shader", originalUniformName.c_str(), replacementUniformName.c_str());
					}
				}
			}

			if (patchFragmentShaderMipLodBias && mipLodBias.has_value())
			{
				patchFragmentShaderTextureLodBias(fragmentShaderStr, mipLodBias.value());
			}
			hasSpecializationConstant_ShadowConstants = patchFragmentShaderShadowConstants(fragmentShaderStr, lightingConstants);
			hasSpecializationConstants_PointLights = patchFragmentShaderPointLightsDefines(fragmentShaderStr, lightingConstants);

			const char* ShaderStrings[2] = { fragment_header, fragmentShaderStr.c_str() };

			glShaderSource(shader, 2, ShaderStrings, nullptr);
			glCompileShader(shader);

			// Check for compilation errors
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status)
			{
				debug(LOG_ERROR, "Fragment shader compilation has failed [%s]", fragmentPath.c_str());
				printShaderInfoLog(LOG_ERROR, shader);
			}
			else
			{
				printShaderInfoLog(LOG_3D, shader);
				glAttachShader(program, shader);
				success = true;
			}
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
			ctx.wzGLObjectLabel(GL_SHADER, shader, -1, fragmentPath.c_str());
#endif
		}
	}

	if (success)
	{
		glLinkProgram(program);

		// Check for linkage errors
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (!status)
		{
			debug(LOG_ERROR, "Shader program linkage has failed [%s, %s]", vertexPath.c_str(), fragmentPath.c_str());
			printProgramInfoLog(LOG_ERROR, program);
			success = false;
		}
		else
		{
			printProgramInfoLog(LOG_3D, program);
		}
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
		ctx.wzGLObjectLabel(GL_PROGRAM, program, -1, programName.c_str());
#endif
	}
	fetch_uniforms(uniformNames, duplicateFragmentUniformNames, programName);
	getLocs(samplersToBind);
	broken |= !success;
}

void gl_pipeline_state_object::fetch_uniforms(const std::vector<std::string>& uniformNames, const std::vector<std::string>& duplicateFragmentUniformNames, const std::string& programName)
{
	std::transform(uniformNames.begin(), uniformNames.end(),
				   std::back_inserter(locations),
				   [&](const std::string& name)
	{
		GLint result = glGetUniformLocation(program, name.data());
		if (result == -1)
		{
			debug(LOG_3D, "[%s]: Did not find uniform: %s", programName.c_str(), name.c_str());
		}
		return result;
	});
	if (!duplicateFragmentUniformNames.empty())
	{
		std::transform(duplicateFragmentUniformNames.begin(), duplicateFragmentUniformNames.end(),
					   std::back_inserter(duplicateFragmentUniformLocations),
					   [&](const std::string& name) {
			if (name.empty())
			{
				return -1;
			}
			return glGetUniformLocation(program, name.data());
		});
	}
	else
	{
		duplicateFragmentUniformLocations.resize(uniformNames.size(), -1);
	}
}

void gl_pipeline_state_object::setUniforms(size_t uniformIdx, const ::glm::vec4 &v)
{
	glUniform4f(locations[uniformIdx], v.x, v.y, v.z, v.w);
	if (duplicateFragmentUniformLocations[uniformIdx] != -1)
	{
		glUniform4f(duplicateFragmentUniformLocations[uniformIdx], v.x, v.y, v.z, v.w);
	}
}

void gl_pipeline_state_object::setUniforms(size_t uniformIdx, const ::glm::mat4 &m)
{
	glUniformMatrix4fv(locations[uniformIdx], 1, GL_FALSE, glm::value_ptr(m));
	if (duplicateFragmentUniformLocations[uniformIdx] != -1)
	{
		glUniformMatrix4fv(duplicateFragmentUniformLocations[uniformIdx], 1, GL_FALSE, glm::value_ptr(m));
	}
}

void gl_pipeline_state_object::setUniforms(size_t uniformIdx, const ::glm::mat4 *m, size_t count)
{
	glUniformMatrix4fv(locations[uniformIdx], static_cast<GLsizei>(count), GL_FALSE, glm::value_ptr(*m));
	if (duplicateFragmentUniformLocations[uniformIdx] != -1)
	{
		glUniformMatrix4fv(duplicateFragmentUniformLocations[uniformIdx], static_cast<GLsizei>(count), GL_FALSE, glm::value_ptr(*m));
	}
}

void gl_pipeline_state_object::setUniforms(size_t uniformIdx, const ::glm::vec4 *m, size_t count)
{
	glUniform4fv(locations[uniformIdx], static_cast<GLsizei>(count), glm::value_ptr(*m));
	if (duplicateFragmentUniformLocations[uniformIdx] != -1)
	{
		glUniform4fv(duplicateFragmentUniformLocations[uniformIdx], static_cast<GLsizei>(count), glm::value_ptr(*m));
	}
}

void gl_pipeline_state_object::setUniforms(size_t uniformIdx, const ::glm::ivec4 *m, size_t count)
{
	glUniform4iv(locations[uniformIdx], static_cast<GLsizei>(count), glm::value_ptr(*m));
	if (duplicateFragmentUniformLocations[uniformIdx] != -1)
	{
		glUniform4iv(duplicateFragmentUniformLocations[uniformIdx], static_cast<GLsizei>(count), glm::value_ptr(*m));
	}
}

void gl_pipeline_state_object::setUniforms(size_t uniformIdx, const float *v, size_t count)
{
	glUniform1fv(locations[uniformIdx], static_cast<GLsizei>(count), v);
	if (duplicateFragmentUniformLocations[uniformIdx] != -1)
	{
		glUniform1fv(duplicateFragmentUniformLocations[uniformIdx], static_cast<GLsizei>(count), v);
	}
}

void gl_pipeline_state_object::setUniforms(size_t uniformIdx, const ::glm::ivec4 &v)
{
	glUniform4i(locations[uniformIdx], v.x, v.y, v.z, v.w);
	if (duplicateFragmentUniformLocations[uniformIdx] != -1)
	{
		glUniform4i(duplicateFragmentUniformLocations[uniformIdx], v.x, v.y, v.z, v.w);
	}
}

void gl_pipeline_state_object::setUniforms(size_t uniformIdx, const ::glm::ivec2 &v)
{
	glUniform2i(locations[uniformIdx], v.x, v.y);
	if (duplicateFragmentUniformLocations[uniformIdx] != -1)
	{
		glUniform2i(duplicateFragmentUniformLocations[uniformIdx], v.x, v.y);
	}
}

void gl_pipeline_state_object::setUniforms(size_t uniformIdx, const ::glm::vec2 &v)
{
	glUniform2f(locations[uniformIdx], v.x, v.y);
	if (duplicateFragmentUniformLocations[uniformIdx] != -1)
	{
		glUniform2f(duplicateFragmentUniformLocations[uniformIdx], v.x, v.y);
	}
}

void gl_pipeline_state_object::setUniforms(size_t uniformIdx, const int32_t &v)
{
	glUniform1i(locations[uniformIdx], v);
	if (duplicateFragmentUniformLocations[uniformIdx] != -1)
	{
		glUniform1i(duplicateFragmentUniformLocations[uniformIdx], v);
	}
}

void gl_pipeline_state_object::setUniforms(size_t uniformIdx, const float &v)
{
	glUniform1f(locations[uniformIdx], v);
	if (duplicateFragmentUniformLocations[uniformIdx] != -1)
	{
		glUniform1f(duplicateFragmentUniformLocations[uniformIdx], v);
	}
}

// MARK: -

//template<typename T>
//void set_constants_for_component(gl_pipeline_state_object *obj, const T& cbuf)
//{
//	setUniforms(obj->locations[0], cbuf.colour);
//	setUniforms(obj->locations[1], cbuf.teamcolour);
//	setUniforms(obj->locations[2], cbuf.shaderStretch);
//	setUniforms(obj->locations[3], cbuf.tcmask);
//	setUniforms(obj->locations[4], cbuf.fogEnabled);
//	setUniforms(obj->locations[5], cbuf.normalMap);
//	setUniforms(obj->locations[6], cbuf.specularMap);
//	setUniforms(obj->locations[7], cbuf.ecmState);
//	setUniforms(obj->locations[8], cbuf.alphaTest);
//	setUniforms(obj->locations[9], cbuf.timeState);
//	setUniforms(obj->locations[10], cbuf.ModelViewMatrix);
//	setUniforms(obj->locations[11], cbuf.ModelViewProjectionMatrix);
//	setUniforms(obj->locations[12], cbuf.NormalMatrix);
//	setUniforms(obj->locations[13], cbuf.sunPos);
//	setUniforms(obj->locations[14], cbuf.sceneColor);
//	setUniforms(obj->locations[15], cbuf.ambient);
//	setUniforms(obj->locations[16], cbuf.diffuse);
//	setUniforms(obj->locations[17], cbuf.specular);
//	setUniforms(obj->locations[18], cbuf.fogEnd);
//	setUniforms(obj->locations[19], cbuf.fogBegin);
//	setUniforms(obj->locations[20], cbuf.fogColour);
//}

gl_pipeline_state_object::~gl_pipeline_state_object()
{
	if (this->vertexShader) glDeleteShader(this->vertexShader);
	if (this->fragmentShader) glDeleteShader(this->fragmentShader);
	glDeleteProgram(this->program);
}

void gl_pipeline_state_object::set_constants(const gfx_api::Draw3DShapeGlobalUniforms& cbuf)
{
	setUniforms(0, cbuf.ProjectionMatrix);
	setUniforms(1, cbuf.ViewMatrix);
	setUniforms(2, cbuf.ShadowMapMVPMatrix);
	setUniforms(3, cbuf.sunPos);
	setUniforms(4, cbuf.sceneColor);
	setUniforms(5, cbuf.ambient);
	setUniforms(6, cbuf.diffuse);
	setUniforms(7, cbuf.specular);
	setUniforms(8, cbuf.fogColour);
	setUniforms(9, cbuf.fogEnd);
	setUniforms(10, cbuf.fogBegin);
	setUniforms(11, cbuf.timeState);
	setUniforms(12, cbuf.fogEnabled);
}

void gl_pipeline_state_object::set_constants(const gfx_api::Draw3DShapePerMeshUniforms& cbuf)
{
	setUniforms(13, cbuf.tcmask);
	setUniforms(14, cbuf.normalMap);
	setUniforms(15, cbuf.specularMap);
	setUniforms(16, cbuf.hasTangents);
}

void gl_pipeline_state_object::set_constants(const gfx_api::Draw3DShapePerInstanceUniforms& cbuf)
{
	setUniforms(17, cbuf.ModelViewMatrix);
	setUniforms(18, cbuf.NormalMatrix);
	setUniforms(19, cbuf.colour);
	setUniforms(20, cbuf.teamcolour);
	setUniforms(21, cbuf.shaderStretch);
	setUniforms(22, cbuf.animFrameNumber);
	setUniforms(23, cbuf.ecmState);
	setUniforms(24, cbuf.alphaTest);
}

void gl_pipeline_state_object::set_constants(const gfx_api::Draw3DShapeInstancedGlobalUniforms& cbuf)
{
	setUniforms(0, cbuf.ProjectionMatrix);
	setUniforms(1, cbuf.ViewMatrix);
	setUniforms(2, cbuf.ModelUVLightmapMatrix);
	setUniforms(3, cbuf.ShadowMapMVPMatrix, WZ_MAX_SHADOW_CASCADES);
	setUniforms(4, cbuf.sunPos);
	setUniforms(5, cbuf.sceneColor);
	setUniforms(6, cbuf.ambient);
	setUniforms(7, cbuf.diffuse);
	setUniforms(8, cbuf.specular);
	setUniforms(9, cbuf.fogColour);
	setUniforms(10, cbuf.ShadowMapCascadeSplits);
	setUniforms(11, cbuf.ShadowMapSize);
	setUniforms(12, cbuf.fogEnd);
	setUniforms(13, cbuf.fogBegin);
	setUniforms(14, cbuf.timeState);
	setUniforms(15, cbuf.fogEnabled);
	setUniforms(16, cbuf.PointLightsPosition);
	setUniforms(17, cbuf.PointLightsColorAndEnergy);
	setUniforms(18, cbuf.bucketOffsetAndSize);
	setUniforms(19, cbuf.indexed_lights);
	setUniforms(20, cbuf.bucketDimensionUsed);
	setUniforms(21, cbuf.viewportWidth);
	setUniforms(22, cbuf.viewportheight);
}

void gl_pipeline_state_object::set_constants(const gfx_api::Draw3DShapeInstancedPerMeshUniforms& cbuf)
{
	setUniforms(23, cbuf.tcmask);
	setUniforms(24, cbuf.normalMap);
	setUniforms(25, cbuf.specularMap);
	setUniforms(26, cbuf.hasTangents);
	setUniforms(27, cbuf.shieldEffect);
}

void gl_pipeline_state_object::set_constants(const gfx_api::Draw3DShapeInstancedDepthOnlyGlobalUniforms& cbuf)
{
	setUniforms(0, cbuf.ProjectionMatrix);
	setUniforms(1, cbuf.ViewMatrix);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_TERRAIN>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
	setUniforms(1, cbuf.paramX);
	setUniforms(2, cbuf.paramY);
	setUniforms(3, cbuf.paramXLight);
	setUniforms(4, cbuf.paramYLight);
	setUniforms(5, cbuf.texture0);
	setUniforms(6, cbuf.texture1);
	setUniforms(7, cbuf.unused);
	setUniforms(8, cbuf.texture_matrix);
	setUniforms(9, cbuf.fog_colour);
	setUniforms(10, cbuf.fog_enabled);
	setUniforms(11, cbuf.fog_begin);
	setUniforms(12, cbuf.fog_end);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_TERRAIN_DEPTH>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
	setUniforms(1, cbuf.paramX);
	setUniforms(2, cbuf.paramY);
	setUniforms(3, cbuf.texture0);
	setUniforms(4, cbuf.paramXLight);
	setUniforms(5, cbuf.paramYLight);
	setUniforms(6, cbuf.fog_enabled);
	setUniforms(7, cbuf.fog_begin);
	setUniforms(8, cbuf.fog_end);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_TERRAIN_DEPTHMAP>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
//	setUniforms(1, cbuf.paramX);
//	setUniforms(2, cbuf.paramY);
//	setUniforms(3, cbuf.texture0);
//	setUniforms(4, cbuf.paramXLight);
//	setUniforms(5, cbuf.paramYLight);
	setUniforms(1, cbuf.fog_enabled);
	setUniforms(2, cbuf.fog_begin);
	setUniforms(3, cbuf.fog_end);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_DECALS>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
	setUniforms(1, cbuf.texture_matrix);
	setUniforms(2, cbuf.param1);
	setUniforms(3, cbuf.param2);
	setUniforms(4, cbuf.fog_colour);
	setUniforms(5, cbuf.fog_enabled);
	setUniforms(6, cbuf.fog_begin);
	setUniforms(7, cbuf.fog_end);
	setUniforms(8, cbuf.texture0);
	setUniforms(9, cbuf.texture1);
}

void gl_pipeline_state_object::set_constants(const gfx_api::TerrainCombinedUniforms& cbuf)
{
	int i = 0;
	setUniforms(i++, cbuf.ModelViewProjectionMatrix);
	setUniforms(i++, cbuf.ViewMatrix);
	setUniforms(i++, cbuf.ModelUVLightmapMatrix);
	setUniforms(i++, cbuf.ShadowMapMVPMatrix, WZ_MAX_SHADOW_CASCADES);
	setUniforms(i++, cbuf.groundScale);
	setUniforms(i++, cbuf.cameraPos);
	setUniforms(i++, cbuf.sunPos);
	setUniforms(i++, cbuf.emissiveLight);
	setUniforms(i++, cbuf.ambientLight);
	setUniforms(i++, cbuf.diffuseLight);
	setUniforms(i++, cbuf.specularLight);
	setUniforms(i++, cbuf.fog_colour);
	setUniforms(i++, cbuf.ShadowMapCascadeSplits);
	setUniforms(i++, cbuf.ShadowMapSize);
	setUniforms(i++, cbuf.fog_enabled);
	setUniforms(i++, cbuf.fog_begin);
	setUniforms(i++, cbuf.fog_end);
	setUniforms(i++, cbuf.quality);
	setUniforms(i++, cbuf.PointLightsPosition);
	setUniforms(i++, cbuf.PointLightsColorAndEnergy);
	setUniforms(i++, cbuf.bucketOffsetAndSize);
	setUniforms(i++, cbuf.indexed_lights);
	setUniforms(i++, cbuf.bucketDimensionUsed);
	setUniforms(i++, cbuf.viewportWidth);
	setUniforms(i++, cbuf.viewportheight);
	setUniforms(i++, 0); // lightmap_tex
	setUniforms(i++, 1); // ground
	setUniforms(i++, 2); // groundNormal
	setUniforms(i++, 3); // groundSpecular
	setUniforms(i++, 4); // groundHeight
	setUniforms(i++, 5); // decal
	setUniforms(i++, 6); // decalNormal
	setUniforms(i++, 7); // decalSpecular
	setUniforms(i++, 8); // decalHeight
	setUniforms(i++, 9); // shadowMap
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_WATER>& cbuf)
{
	int i = 0;
	setUniforms(i++, cbuf.ModelViewProjectionMatrix);
	setUniforms(i++, cbuf.ModelUVLightmapMatrix);
	setUniforms(i++, cbuf.ModelUV1Matrix);
	setUniforms(i++, cbuf.ModelUV2Matrix);
	setUniforms(i++, cbuf.cameraPos);
	setUniforms(i++, cbuf.sunPos);
	setUniforms(i++, cbuf.emissiveLight);
	setUniforms(i++, cbuf.ambientLight);
	setUniforms(i++, cbuf.diffuseLight);
	setUniforms(i++, cbuf.specularLight);
	setUniforms(i++, cbuf.fog_colour);
	setUniforms(i++, cbuf.fog_enabled);
	setUniforms(i++, cbuf.fog_begin);
	setUniforms(i++, cbuf.fog_end);
	setUniforms(i++, cbuf.timeSec);
	 // textures:
	setUniforms(i++, 0);
	setUniforms(i++, 1);
	setUniforms(i++, 2); // lightmap_tex
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_WATER_HIGH>& cbuf)
{
	int i = 0;
	setUniforms(i++, cbuf.ModelViewProjectionMatrix);
	setUniforms(i++, cbuf.ViewMatrix);
	setUniforms(i++, cbuf.ModelUVLightmapMatrix);
	setUniforms(i++, cbuf.ShadowMapMVPMatrix, WZ_MAX_SHADOW_CASCADES);
	setUniforms(i++, cbuf.cameraPos);
	setUniforms(i++, cbuf.sunPos);
	setUniforms(i++, cbuf.emissiveLight);
	setUniforms(i++, cbuf.ambientLight);
	setUniforms(i++, cbuf.diffuseLight);
	setUniforms(i++, cbuf.specularLight);
	setUniforms(i++, cbuf.fog_colour);
	setUniforms(i++, cbuf.ShadowMapCascadeSplits);
	setUniforms(i++, cbuf.ShadowMapSize);
	setUniforms(i++, cbuf.fog_enabled);
	setUniforms(i++, cbuf.fog_begin);
	setUniforms(i++, cbuf.fog_end);
	setUniforms(i++, cbuf.timeSec);
	 // textures:
	setUniforms(i++, 0);
	setUniforms(i++, 1);
	setUniforms(i++, 2);
	setUniforms(i++, 3); // lightmap_tex
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_WATER_CLASSIC>& cbuf)
{
	int i = 0;
	setUniforms(i++, cbuf.ModelViewProjectionMatrix);
	setUniforms(i++, cbuf.ModelUVLightmapMatrix);
	setUniforms(i++, cbuf.ShadowMapMVPMatrix);
	setUniforms(i++, cbuf.ModelUV1Matrix);
	setUniforms(i++, cbuf.ModelUV2Matrix);
	setUniforms(i++, cbuf.cameraPos);
	setUniforms(i++, cbuf.sunPos);
	setUniforms(i++, cbuf.fog_colour);
	setUniforms(i++, cbuf.fog_enabled);
	setUniforms(i++, cbuf.fog_begin);
	setUniforms(i++, cbuf.fog_end);
	setUniforms(i++, cbuf.timeSec);
	 // textures:
	setUniforms(i++, 0);
	setUniforms(i++, 1);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_RECT>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
	setUniforms(1, cbuf.colour);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_TEXRECT>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
	setUniforms(1, cbuf.offset);
	setUniforms(2, cbuf.size);
	setUniforms(3, cbuf.color);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_GFX_COLOUR>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_GFX_TEXT>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
	setUniforms(1, cbuf.color);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_SKYBOX>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
	setUniforms(1, cbuf.color);
	setUniforms(2, cbuf.fog_color);
	setUniforms(3, cbuf.fog_enabled);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_GENERIC_COLOR>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
	setUniforms(1, cbuf.colour);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_RECT_INSTANCED>& cbuf)
{
	setUniforms(0, cbuf.ProjectionMatrix);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_LINE>& cbuf)
{
	setUniforms(0, cbuf.p0);
	setUniforms(1, cbuf.p1);
	setUniforms(2, cbuf.colour);
	setUniforms(3, cbuf.mat);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_TEXT>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
	setUniforms(1, cbuf.offset);
	setUniforms(2, cbuf.size);
	setUniforms(3, cbuf.color);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_DEBUG_TEXTURE2D_QUAD>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
	setUniforms(1, cbuf.uv_transform_matrix);
	setUniforms(2, cbuf.swizzle);
	setUniforms(3, cbuf.color);
	setUniforms(4, cbuf.texture);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_DEBUG_TEXTURE2DARRAY_QUAD>& cbuf)
{
	setUniforms(0, cbuf.transform_matrix);
	setUniforms(1, cbuf.uv_transform_matrix);
	setUniforms(2, cbuf.swizzle);
	setUniforms(3, cbuf.color);
	setUniforms(4, cbuf.layer);
	setUniforms(5, cbuf.texture);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_WORLD_TO_SCREEN>& cbuf)
{
	setUniforms(0, cbuf.gamma);
}

GLint get_size(const gfx_api::vertex_attribute_type& type)
{
	switch (type)
	{
		case gfx_api::vertex_attribute_type::int1:
			return 1;
		case gfx_api::vertex_attribute_type::float2:
			return 2;
		case gfx_api::vertex_attribute_type::float3:
			return 3;
		case gfx_api::vertex_attribute_type::float4:
		case gfx_api::vertex_attribute_type::u8x4_uint:
		case gfx_api::vertex_attribute_type::u8x4_norm:
			return 4;
	}
	debug(LOG_FATAL, "get_size(%d) failed", (int)type);
	return 0; // silence warning
}

GLenum get_type(const gfx_api::vertex_attribute_type& type)
{
	switch (type)
	{
		case gfx_api::vertex_attribute_type::float2:
		case gfx_api::vertex_attribute_type::float3:
		case gfx_api::vertex_attribute_type::float4:
			return GL_FLOAT;
		case gfx_api::vertex_attribute_type::u8x4_uint:
		case gfx_api::vertex_attribute_type::u8x4_norm:
			return GL_UNSIGNED_BYTE;
		case gfx_api::vertex_attribute_type::int1:
			return GL_INT;
	}
	debug(LOG_FATAL, "get_type(%d) failed", (int)type);
	return GL_INVALID_ENUM; // silence warning
}

GLboolean get_normalisation(const gfx_api::vertex_attribute_type& type)
{
	switch (type)
	{
		case gfx_api::vertex_attribute_type::float2:
		case gfx_api::vertex_attribute_type::float3:
		case gfx_api::vertex_attribute_type::float4:
		case gfx_api::vertex_attribute_type::int1:
		case gfx_api::vertex_attribute_type::u8x4_uint:
			return GL_FALSE;
		case gfx_api::vertex_attribute_type::u8x4_norm:
			return true;
	}
	debug(LOG_FATAL, "get_normalisation(%d) failed", (int)type);
	return GL_FALSE; // silence warning
}

// MARK: gl_context

gl_context::~gl_context()
{
	// nothing - all cleanup should occur in gl_context::shutdown()
}

gfx_api::texture* gl_context::create_texture(const size_t& mipmap_count, const size_t & width, const size_t & height, const gfx_api::pixel_format & internal_format, const std::string& filename)
{
	ASSERT(mipmap_count > 0, "mipmap_count must be > 0");
	ASSERT(mipmap_count <= static_cast<size_t>(std::numeric_limits<GLint>::max()), "mipmap_count (%zu) exceeds GLint max", mipmap_count);
	ASSERT(width <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "width (%zu) exceeds GLsizei max", width);
	ASSERT(height <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "height (%zu) exceeds GLsizei max", height);
	auto* new_texture = new gl_texture();
	new_texture->gles = gles;
	new_texture->mip_count = mipmap_count;
	new_texture->internal_format = internal_format;
	new_texture->tex_width = width;
	new_texture->tex_height = height;
#if defined(WZ_DEBUG_GFX_API_LEAKS)
	new_texture->debugName = filename;
#endif
	new_texture->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mipmap_count - 1));
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	if (!filename.empty())
	{
		wzGLObjectLabel(GL_TEXTURE, new_texture->id(), -1, filename.c_str());
	}
#endif
	for (GLint i = 0, e = static_cast<GLint>(mipmap_count); i < e; ++i)
	{
		if (is_uncompressed_format(internal_format))
		{
			// Allocate an empty buffer of the full size
			// (As uncompressed textures support the upload_sub() function that permits sub texture uploads)
			glTexImage2D(GL_TEXTURE_2D, i, to_gl_internalformat(internal_format, gles), static_cast<GLsizei>(width >> i), static_cast<GLsizei>(height >> i), 0, to_gl_format(internal_format, gles), GL_UNSIGNED_BYTE, nullptr);
		}
		else
		{
			// Can't use glCompressedTexImage2D with a null buffer (it's not permitted by the standard, and will crash depending on the driver)
			// For now, we don't allocate a buffer and instead: only the upload() function that uploads full images is supported
		}
	}
	return new_texture;
}

gfx_api::texture_array* gl_context::create_texture_array(const size_t& mipmap_count, const size_t& layer_count, const size_t& width, const size_t& height, const gfx_api::pixel_format& internal_format, const std::string& filename)
{
	ASSERT(mipmap_count > 0, "mipmap_count must be > 0");
	ASSERT(mipmap_count <= static_cast<size_t>(std::numeric_limits<GLint>::max()), "mipmap_count (%zu) exceeds GLint max", mipmap_count);
	ASSERT(width <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "width (%zu) exceeds GLsizei max", width);
	ASSERT(height <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "height (%zu) exceeds GLsizei max", height);
	auto* new_texture = new gl_texture_array();
	new_texture->gles = gles;
	new_texture->mip_count = mipmap_count;
	new_texture->layer_count = layer_count;
	new_texture->internal_format = internal_format;
#if defined(WZ_DEBUG_GFX_API_LEAKS)
	new_texture->debugName = filename;
#endif
	new_texture->bind();
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mipmap_count - 1));
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	if (!filename.empty())
	{
		wzGLObjectLabel(GL_TEXTURE, new_texture->id(), -1, filename.c_str());
	}
#endif
	// Allocate a client-side buffer
	new_texture->pInternalBuffer = new texture_array_mip_level_buffer(mipmap_count, layer_count, width, height, internal_format);
	new_texture->unbind();
	return new_texture;
}

gl_gpurendered_texture* gl_context::create_depthmap_texture(const size_t& layer_count, const size_t& width, const size_t& height, const std::string& filename)
{
	GLenum depthInternalFormat = GL_DEPTH_COMPONENT32F;
	return create_gpurendered_texture_array(depthInternalFormat, GL_DEPTH_COMPONENT, GL_FLOAT, width, height, layer_count, filename);
}

gl_gpurendered_texture* gl_context::create_gpurendered_texture(GLenum internalFormat, GLenum format, GLenum type, const size_t& width, const size_t& height, const std::string& filename)
{
	ASSERT(width <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "width (%zu) exceeds GLsizei max", width);
	ASSERT(height <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "height (%zu) exceeds GLsizei max", height);
	auto* new_texture = new gl_gpurendered_texture();
	new_texture->gles = gles;
	new_texture->_isArray = false;
#if defined(WZ_DEBUG_GFX_API_LEAKS)
	new_texture->debugName = filename;
#endif
	new_texture->bind();
	glTexParameteri(new_texture->target(), GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(new_texture->target(), GL_TEXTURE_MAX_LEVEL, 0);
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	if (!filename.empty())
	{
		wzGLObjectLabel(GL_TEXTURE, new_texture->id(), -1, filename.c_str());
	}
#endif

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, type, nullptr);

	new_texture->unbind();
	return new_texture;
}

gl_gpurendered_texture* gl_context::create_gpurendered_texture_array(GLenum internalFormat, GLenum format, GLenum type, const size_t& width, const size_t& height, const size_t& layer_count, const std::string& filename)
{
	ASSERT(width <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "width (%zu) exceeds GLsizei max", width);
	ASSERT(height <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "height (%zu) exceeds GLsizei max", height);
	auto* new_texture = new gl_gpurendered_texture();
	new_texture->gles = gles;
	new_texture->_isArray = true;
#if defined(WZ_DEBUG_GFX_API_LEAKS)
	new_texture->debugName = filename;
#endif
	new_texture->bind();
	glTexParameteri(new_texture->target(), GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(new_texture->target(), GL_TEXTURE_MAX_LEVEL, 0);
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	if (!filename.empty())
	{
		wzGLObjectLabel(GL_TEXTURE, new_texture->id(), -1, filename.c_str());
	}
#endif

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(layer_count), 0, format, type, nullptr);

	new_texture->unbind();
	return new_texture;
}

gl_gpurendered_texture* gl_context::create_framebuffer_color_texture(GLenum internalFormat, GLenum format, GLenum type, const size_t& width, const size_t& height, const std::string& filename)
{
	return create_gpurendered_texture(internalFormat, format, type, width, height, filename);
}

gfx_api::buffer * gl_context::create_buffer_object(const gfx_api::buffer::usage &usage, const buffer_storage_hint& hint /*= buffer_storage_hint::static_draw*/, const std::string& debugName /*= ""*/)
{
	return new gl_buffer(usage, hint);
}

gfx_api::pipeline_state_object* gl_context::build_pipeline(gfx_api::pipeline_state_object *existing_pso, const gfx_api::pipeline_create_info& createInfo)
{
	optional<size_t> psoID;
	if (existing_pso)
	{
		gl_pipeline_id* existingPSOId = static_cast<gl_pipeline_id*>(existing_pso);
		psoID = existingPSOId->psoID;
	}

	bool patchFragmentShaderMipLodBias = true; // provide the constant to the shader directly
	auto pipeline = new gl_pipeline_state_object(*this, fragmentHighpFloatAvailable, fragmentHighpIntAvailable, patchFragmentShaderMipLodBias, createInfo, mipLodBias, shadowConstants);
	if (!psoID.has_value())
	{
		createdPipelines.emplace_back(createInfo);
		psoID = createdPipelines.size() - 1;
		createdPipelines[psoID.value()].pso = pipeline;
	}
	else
	{
		auto& builtPipelineRegistry = createdPipelines[psoID.value()];
		if (builtPipelineRegistry.pso != nullptr)
		{
			delete builtPipelineRegistry.pso;
		}
		builtPipelineRegistry.pso = pipeline;
	}

	return new gl_pipeline_id(psoID.value(), pipeline->broken); // always return a new indirect reference
}

void gl_context::bind_pipeline(gfx_api::pipeline_state_object* pso, bool notextures)
{
	gl_pipeline_id* newPSOId = static_cast<gl_pipeline_id*>(pso);
	// lookup pipeline
	auto& pipelineInfo = createdPipelines[newPSOId->psoID];
	gl_pipeline_state_object* new_program = pipelineInfo.pso;
	if (current_program != new_program)
	{
		current_program = new_program;
		current_program->bind();
		if (notextures)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		}
	}
}

inline void gl_context::enableVertexAttribArray(GLuint index)
{
	glEnableVertexAttribArray(index);
	ASSERT(enabledVertexAttribIndexes.capacity() >= static_cast<size_t>(index), "Insufficient room in enabledVertexAttribIndexes for: %u", (unsigned int) index);
	enabledVertexAttribIndexes[static_cast<size_t>(index)] = true;
}

inline void gl_context::disableVertexAttribArray(GLuint index)
{
	glDisableVertexAttribArray(index);
	ASSERT(enabledVertexAttribIndexes.size() >= static_cast<size_t>(index), "Insufficient room in enabledVertexAttribIndexes for: %u", (unsigned int) index);
	enabledVertexAttribIndexes[static_cast<size_t>(index)] = false;
}

void gl_context::bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	for (size_t i = 0, e = vertex_buffers_offset.size(); i < e && (first + i) < current_program->vertex_buffer_desc.size(); ++i)
	{
		const auto& buffer_desc = current_program->vertex_buffer_desc[first + i];
		auto* buffer = static_cast<gl_buffer*>(std::get<0>(vertex_buffers_offset[i]));
		if (buffer == nullptr)
		{
			continue;
		}
		ASSERT(buffer->usage == gfx_api::buffer::usage::vertex_buffer, "bind_vertex_buffers called with non-vertex-buffer");
		buffer->bind();
		for (const auto& attribute : buffer_desc.attributes)
		{
			enableVertexAttribArray(static_cast<GLuint>(attribute.id));

			if (get_type(attribute.type) == GL_INT || attribute.type == gfx_api::vertex_attribute_type::u8x4_uint)
			{
				#if !defined(WZ_STATIC_GL_BINDINGS)
					// glVertexAttribIPointer only supported in: OpenGL 3.0+, OpenGL ES 3.0+
					ASSERT(glVertexAttribIPointer != nullptr, "Missing glVertexAttribIPointer?");
				#endif
				glVertexAttribIPointer(static_cast<GLuint>(attribute.id), get_size(attribute.type), get_type(attribute.type), static_cast<GLsizei>(buffer_desc.stride), reinterpret_cast<void*>(attribute.offset + std::get<1>(vertex_buffers_offset[i])));
			}
			else
			{
				glVertexAttribPointer(static_cast<GLuint>(attribute.id), get_size(attribute.type), get_type(attribute.type), get_normalisation(attribute.type), static_cast<GLsizei>(buffer_desc.stride), reinterpret_cast<void*>(attribute.offset + std::get<1>(vertex_buffers_offset[i])));
			}

			if (buffer_desc.rate == gfx_api::vertex_attribute_input_rate::instance)
			{
				if (hasInstancedRenderingSupport)
				{
					wz_dyn_glVertexAttribDivisor(static_cast<GLuint>(attribute.id), 1);
				}
				else
				{
					debug(LOG_ERROR, "Instanced rendering is unsupported, but instanced rendering attribute used");
				}
			}
		}
	}
}

void gl_context::unbind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	for (size_t i = 0, e = vertex_buffers_offset.size(); i < e && (first + i) < current_program->vertex_buffer_desc.size(); ++i)
	{
		const auto& buffer_desc = current_program->vertex_buffer_desc[first + i];
		for (const auto& attribute : buffer_desc.attributes)
		{
			if (buffer_desc.rate == gfx_api::vertex_attribute_input_rate::instance)
			{
				if (hasInstancedRenderingSupport)
				{
					wz_dyn_glVertexAttribDivisor(static_cast<GLuint>(attribute.id), 0); // reset
				}
			}
			disableVertexAttribArray(static_cast<GLuint>(attribute.id));
		}
	}
	glBindBuffer(to_gl(gfx_api::buffer::usage::vertex_buffer), 0);
}

void gl_context::disable_all_vertex_buffers()
{
	for (GLuint index = 0; index < static_cast<GLuint>(enabledVertexAttribIndexes.size()); ++index)
	{
		if(enabledVertexAttribIndexes[index])
		{
			if (hasInstancedRenderingSupport)
			{
				wz_dyn_glVertexAttribDivisor(index, 0); // reset
			}
			disableVertexAttribArray(index);
		}
	}
	glBindBuffer(to_gl(gfx_api::buffer::usage::vertex_buffer), 0);
}

void gl_context::bind_streamed_vertex_buffers(const void* data, const std::size_t size)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	ASSERT(size > 0, "bind_streamed_vertex_buffers called with size 0");
	glBindBuffer(GL_ARRAY_BUFFER, scratchbuffer);
	if (scratchbuffer_size > 0)
	{
		glBufferData(GL_ARRAY_BUFFER, scratchbuffer_size, nullptr, GL_STREAM_DRAW); // orphan previous buffer
	}
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STREAM_DRAW);
	scratchbuffer_size = size;
	const auto& buffer_desc = current_program->vertex_buffer_desc[0];
	for (const auto& attribute : buffer_desc.attributes)
	{
		enableVertexAttribArray(static_cast<GLuint>(attribute.id));
		glVertexAttribPointer(static_cast<GLuint>(attribute.id), get_size(attribute.type), get_type(attribute.type), get_normalisation(attribute.type), static_cast<GLsizei>(buffer_desc.stride), nullptr);
	}
}

void gl_context::bind_index_buffer(gfx_api::buffer& _buffer, const gfx_api::index_type&)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	auto& buffer = static_cast<gl_buffer&>(_buffer);
	ASSERT(buffer.usage == gfx_api::buffer::usage::index_buffer, "Passed gfx_api::buffer is not an index buffer");
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.buffer);
}

void gl_context::unbind_index_buffer(gfx_api::buffer&)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static float transparentBlackFloats[] = {0.0, 0.0, 0.0, 0.0};
static float opaqueBlackFloats[] = {0.0, 0.0, 0.0, 1.0};
static float opaqueWhiteFloats[] = {1.0, 1.0, 1.0, 1.0};
static float* to_gl(gfx_api::border_color border)
{
	switch (border)
	{
		case gfx_api::border_color::none:
		case gfx_api::border_color::transparent_black:
			return transparentBlackFloats;
		case gfx_api::border_color::opaque_black:
			return opaqueBlackFloats;
		case gfx_api::border_color::opaque_white:
			return opaqueWhiteFloats;
	}
	debug(LOG_FATAL, "Unsupported border_color");
	return transparentBlackFloats;
}

void gl_context::bind_textures(const std::vector<gfx_api::texture_input>& texture_descriptions, const std::vector<gfx_api::abstract_texture*>& textures)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	ASSERT(textures.size() <= texture_descriptions.size(), "Received more textures than expected");
	for (size_t i = 0; i < texture_descriptions.size() && i < textures.size(); ++i)
	{
		const auto& desc = texture_descriptions[i];
		gfx_api::abstract_texture *pTextureToBind = textures[i];
		glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + desc.id));
		if (pTextureToBind == nullptr)
		{
			switch (desc.target)
			{
				case gfx_api::pixel_format_target::texture_2d:
					pTextureToBind = pDefaultTexture;
					break;
				case gfx_api::pixel_format_target::texture_2d_array:
					pTextureToBind = pDefaultArrayTexture;
					break;
				case gfx_api::pixel_format_target::depth_map:
					pTextureToBind = pDefaultDepthTexture;
					break;
			}
		}
		ASSERT(pTextureToBind && (pTextureToBind->isArray() == (desc.target == gfx_api::pixel_format_target::texture_2d_array || desc.target == gfx_api::pixel_format_target::depth_map)), "Found a mismatch!");
		const auto type = pTextureToBind->isArray() ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
		const auto unusedtype = pTextureToBind->isArray() ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
		glBindTexture(unusedtype, 0);
		pTextureToBind->bind();
		if (textures[i] == nullptr)
		{
			// we're using a default texture, so skip changing filters
			continue;
		}
		switch (desc.sampler)
		{
			case gfx_api::sampler_type::nearest_clamped:
				glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			case gfx_api::sampler_type::nearest_border:
				glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#if !defined(__EMSCRIPTEN__)
				if (hasBorderClampSupport)
				{
					glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
					glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
					glTexParameterfv(type, GL_TEXTURE_BORDER_COLOR, to_gl(desc.border));
					break;
				}
#else
				// POSSIBLE FIXME: Emulate GL_CLAMP_TO_BORDER for WebGL?
#endif
				break;
			case gfx_api::sampler_type::bilinear:
				glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			case gfx_api::sampler_type::bilinear_repeat:
				glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_REPEAT);
				break;
			case gfx_api::sampler_type::bilinear_border:
				glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#if !defined(__EMSCRIPTEN__)
				if (hasBorderClampSupport)
				{
					glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
					glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
					glTexParameterfv(type, GL_TEXTURE_BORDER_COLOR, to_gl(desc.border));
					break;
				}
#else
				// POSSIBLE FIXME: Emulate GL_CLAMP_TO_BORDER for WebGL?
#endif
				break;
			case gfx_api::sampler_type::anisotropic_repeat:
				glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_REPEAT);
				if (GLAD_GL_EXT_texture_filter_anisotropic)
				{
					glTexParameterf(type, GL_TEXTURE_MAX_ANISOTROPY_EXT, MIN(4.0f, maxTextureAnisotropy));
				}
				break;
			case gfx_api::sampler_type::anisotropic:
				glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				if (GLAD_GL_EXT_texture_filter_anisotropic)
				{
					glTexParameterf(type, GL_TEXTURE_MAX_ANISOTROPY_EXT, MIN(4.0f, maxTextureAnisotropy));
				}
				break;
		}
	}
	glActiveTexture(GL_TEXTURE0);
}

void gl_context::set_constants(const void* buffer, const size_t& size)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	current_program->set_constants(buffer, size);
}

void gl_context::set_uniforms(const size_t& first, const std::vector<std::tuple<const void*, size_t>>& uniform_blocks)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	current_program->set_uniforms(first, uniform_blocks);
}

void gl_context::draw(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive)
{
	ASSERT(offset <= static_cast<size_t>(std::numeric_limits<GLint>::max()), "offset (%zu) exceeds GLint max", offset);
	ASSERT(count <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "count (%zu) exceeds GLsizei max", count);
	glDrawArrays(to_gl(primitive), static_cast<GLint>(offset), static_cast<GLsizei>(count));
}

void gl_context::draw_instanced(const std::size_t& offset, const std::size_t &count, const gfx_api::primitive_type &primitive, std::size_t instance_count)
{
	ASSERT_OR_RETURN(, hasInstancedRenderingSupport, "Instanced rendering is unavailable");
	ASSERT(offset <= static_cast<size_t>(std::numeric_limits<GLint>::max()), "offset (%zu) exceeds GLint max", offset);
	ASSERT(count <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "count (%zu) exceeds GLsizei max", count);
	ASSERT(instance_count <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "instance_count (%zu) exceeds GLsizei max", count);
	wz_dyn_glDrawArraysInstanced(to_gl(primitive), static_cast<GLint>(offset), static_cast<GLsizei>(count), static_cast<GLsizei>(instance_count));
}

void gl_context::draw_elements(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive, const gfx_api::index_type& index)
{
	ASSERT(count <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "count (%zu) exceeds GLsizei max", count);
	glDrawElements(to_gl(primitive), static_cast<GLsizei>(count), to_gl(index), reinterpret_cast<void*>(offset));
}

void gl_context::draw_elements_instanced(const std::size_t& offset, const std::size_t &count, const gfx_api::primitive_type &primitive, const gfx_api::index_type& index, std::size_t instance_count)
{
	ASSERT_OR_RETURN(, hasInstancedRenderingSupport, "Instanced rendering is unavailable");
	ASSERT(count <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "count (%zu) exceeds GLsizei max", count);
	ASSERT(instance_count <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()), "instance_count (%zu) exceeds GLsizei max", count);
	wz_dyn_glDrawElementsInstanced(to_gl(primitive), static_cast<GLsizei>(count), to_gl(index), reinterpret_cast<void*>(offset),  static_cast<GLsizei>(instance_count));
}

void gl_context::set_polygon_offset(const float& offset, const float& slope)
{
	glPolygonOffset(offset, slope);
}

void gl_context::set_depth_range(const float& min, const float& max)
{
#if !defined(__EMSCRIPTEN__)
	if (!gles)
	{
		glDepthRange(min, max);
	}
	else
#endif
	{
		glDepthRangef(min, max);
	}
}

int32_t gl_context::get_context_value(const context_value property)
{
	GLint value = 0;
	switch (property)
	{
		case gfx_api::context::context_value::MAX_SAMPLES:
			return maxMultiSampleBufferFormatSamples;
		case gfx_api::context::context_value::MAX_VERTEX_OUTPUT_COMPONENTS:
			// special-handling for MAX_VERTEX_OUTPUT_COMPONENTS
#if !defined(__EMSCRIPTEN__)
			if (!gles)
			{
				if (GLAD_GL_VERSION_3_2)
				{
					glGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &value);
				}
				else
				{
					// for earlier OpenGL, just default to MAX_VARYING_FLOATS
					glGetIntegerv(GL_MAX_VARYING_FLOATS, &value);
				}
			}
			else
			{
				if (GLAD_GL_ES_VERSION_3_0)
				{
					glGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &value);
				}
				else
				{
					// for OpenGL ES 2.0, use GL_MAX_VARYING_VECTORS * 4 ...
					glGetIntegerv(GL_MAX_VARYING_VECTORS, &value);
					value *= 4;
				}
			}
#else
			// For WebGL 2
			glGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &value);
#endif
			break;
		default:
			glGetIntegerv(to_gl(property), &value);
			break;
	}

	return value;
}

#if !defined(__EMSCRIPTEN__)

uint64_t gl_context::get_estimated_vram_mb(bool dedicatedOnly)
{
	if (GLAD_GL_NVX_gpu_memory_info)
	{
		// If GL_NVX_gpu_memory_info is available, get the total dedicated graphics memory
		GLint total_graphics_mem_kb = 0;
		glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &total_graphics_mem_kb);
		debug(LOG_3D, "GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX: %d", total_graphics_mem_kb);

		if (total_graphics_mem_kb > 0)
		{
			return static_cast<uint64_t>(total_graphics_mem_kb / 1024);
		}
	}
	else if (GLAD_GL_ATI_meminfo && !dedicatedOnly)
	{
		// For GL_ATI_meminfo, get the current free texture memory (stats_kb[0])
		GLint stats_kb[4] = {0, 0, 0, 0};
		glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, stats_kb);
		if (stats_kb[0] > 0)
		{
			debug(LOG_3D, "GL_TEXTURE_FREE_MEMORY_ATI [0: total pool avail]: %d", stats_kb[0]);
			debug(LOG_3D, "GL_TEXTURE_FREE_MEMORY_ATI [1: largest pool avail]: %d", stats_kb[1]);

			uint64_t currentFreeTextureMemory_mb = static_cast<uint64_t>(stats_kb[0] / 1024);
			return currentFreeTextureMemory_mb;
		}
	}

#if defined (__APPLE__)
	WzString openGL_vendor = (const char*)wzSafeGlGetString(GL_VENDOR);
	WzString openGL_renderer = (const char*)wzSafeGlGetString(GL_RENDERER);
	if (openGL_vendor == "Apple" && openGL_renderer.startsWith("Apple"))
	{
		// For Apple GPUs, use system ("unified") RAM value
		auto systemRAMinMiB = wzGetCurrentSystemRAM();
		return systemRAMinMiB;
	}
#endif

	return 0;
}

#else

uint64_t gl_context::get_estimated_vram_mb(bool dedicatedOnly)
{
	return 0;
}

#endif

// MARK: gl_context - debug

void gl_context::debugStringMarker(const char *str)
{
#if defined(GL_GREMEDY_string_marker)
	if (GLAD_GL_GREMEDY_string_marker)
	{
		glStringMarkerGREMEDY(0, str);
	}
#else
	// not supported
#endif
}

void gl_context::debugSceneBegin(const char *descr)
{
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	if (khr_debug)
	{
		wzGLPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, descr);
	}
#else
	// not supported
#endif
}

void gl_context::debugSceneEnd(const char *descr)
{
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	if (khr_debug)
	{
		wzGLPopDebugGroup();
	}
#else
	// not supported
#endif
}

bool gl_context::debugPerfAvailable()
{
#if defined(WZ_GL_TIMER_QUERY_SUPPORTED)
	return GLAD_GL_ARB_timer_query;
#else
	return false;
#endif
}

bool gl_context::debugPerfStart(size_t sample)
{
#if defined(WZ_GL_TIMER_QUERY_SUPPORTED)
	if (GLAD_GL_ARB_timer_query)
	{
		char text[80];
		ssprintf(text, "Starting performance sample %02d", sample);
		debugStringMarker(text);
		perfStarted = true;
		return true;
	}
	return false;
#else
	return false;
#endif
}

void gl_context::debugPerfStop()
{
#if defined(WZ_GL_TIMER_QUERY_SUPPORTED)
	perfStarted = false;
#endif
}

void gl_context::debugPerfBegin(PERF_POINT pp, const char *descr)
{
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	if (khr_debug)
	{
		wzGLPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, pp, -1, descr);
	}
#endif
#if defined(WZ_GL_TIMER_QUERY_SUPPORTED)
	if (!perfStarted)
	{
		return;
	}
	glBeginQuery(GL_TIME_ELAPSED, perfpos[pp]);
#endif
}

void gl_context::debugPerfEnd(PERF_POINT pp)
{
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	if (khr_debug)
	{
		wzGLPopDebugGroup();
	}
#endif
#if defined(WZ_GL_TIMER_QUERY_SUPPORTED)
	if (!perfStarted)
	{
		return;
	}
	glEndQuery(GL_TIME_ELAPSED);
#endif
}

uint64_t gl_context::debugGetPerfValue(PERF_POINT pp)
{
#if defined(WZ_GL_TIMER_QUERY_SUPPORTED)
	GLuint64 count;
	glGetQueryObjectui64v(perfpos[pp], GL_QUERY_RESULT, &count);
	return count;
#else
	return 0;
#endif
}

static std::vector<std::string> splitStringIntoVector(const char* pStr, char delimeter)
{
	const char *pStrBegin = pStr;
	const char *pStrEnd = pStrBegin + strlen(pStr);
	std::vector<std::string> result;
	for (const char *i = pStrBegin; i < pStrEnd;)
	{
		const char *j = std::find(i, pStrEnd, delimeter);
		result.push_back(std::string(i, j));
		i = j + 1;
	}
	return result;
}

static std::vector<std::string> wzGLGetStringi_GL_EXTENSIONS_Impl()
{
	std::vector<std::string> extensions;

#if !defined(WZ_STATIC_GL_BINDINGS)
	if (!glGetIntegerv || !glGetStringi)
	{
		return extensions;
	}
#endif

	GLint ext_count = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count);
	if (ext_count < 0)
	{
		ext_count = 0;
	}
	for (GLint i = 0; i < ext_count; i++)
	{
		const char *pGLStr = (const char*) glGetStringi(GL_EXTENSIONS, i);
		if (pGLStr != nullptr)
		{
			extensions.push_back(std::string(pGLStr));
		}
	}

	return extensions;
}

#if !defined(__EMSCRIPTEN__)

// Returns a space-separated list of OpenGL extensions
static std::vector<std::string> getGLExtensions()
{
	std::vector<std::string> extensions;
	if (GLAD_GL_VERSION_3_0)
	{
		// OpenGL 3.0+
		return wzGLGetStringi_GL_EXTENSIONS_Impl();
	}
	else
	{
		// OpenGL < 3.0
		ASSERT_OR_RETURN(extensions, glGetString != nullptr, "glGetString is null");
		const char *pExtensionsStr = (const char *) glGetString(GL_EXTENSIONS);
		if (pExtensionsStr != nullptr)
		{
			extensions = splitStringIntoVector(pExtensionsStr, ' ');
		}
	}
	return extensions;
}

#else

// Return a list of *enabled* WebGL extensions
static std::vector<std::string> getGLExtensions()
{
	// Note: Only works after initWebGLExtensions() has been called
	return std::vector<std::string>(enabledWebGLExtensions.begin(), enabledWebGLExtensions.end());
}

#endif // !defined(__EMSCRIPTEN__)

std::map<std::string, std::string> gl_context::getBackendGameInfo()
{
	std::map<std::string, std::string> backendGameInfo;
	backendGameInfo["openGL_vendor"] = opengl.vendor;
	backendGameInfo["openGL_renderer"] = opengl.renderer;
	backendGameInfo["openGL_version"] = opengl.version;
	backendGameInfo["openGL_GLSL_version"] = opengl.GLSLversion;
#if !defined(__EMSCRIPTEN__)
	backendGameInfo["GL_EXTENSIONS"] = fmt::format("{}",fmt::join(getGLExtensions()," "));
#else
	backendGameInfo["GL_EXTENSIONS"] = fmt::format("{}",fmt::join(enabledWebGLExtensions," "));
#endif
	return backendGameInfo;
}

const std::string& gl_context::getFormattedRendererInfoString() const
{
	return formattedRendererInfoString;
}

std::string gl_context::calculateFormattedRendererInfoString() const
{
	std::string openGLVersionStr;
	GLint gl_majorversion = wz_GetGLIntegerv(GL_MAJOR_VERSION, 0);
	GLint gl_minorversion = wz_GetGLIntegerv(GL_MINOR_VERSION, 0);
	if (gl_majorversion > 0)
	{
		openGLVersionStr = std::to_string(gl_majorversion) + "." + std::to_string(gl_minorversion);
	}
	else
	{
		// Failed to retrieve OpenGL version from GL_MAJOR_VERSION / GL_MINOR_VERSION
		// Likely < OpenGL 3.0
		// Extract version from GL_VERSION

		int major = 0, minor = 0;

		const char* version = nullptr;
		const char* prefixes[] = {
			"OpenGL ES-CM ",
			"OpenGL ES-CL ",
			"OpenGL ES ",
			NULL
		};

		version = (const char*) glGetString(GL_VERSION);
		if (version)
		{
			for (size_t i = 0;  prefixes[i];  i++) {
				const size_t length = strlen(prefixes[i]);
				if (strncmp(version, prefixes[i], length) == 0) {
					version += length;
					break;
				}
			}

			#ifdef _MSC_VER
				sscanf_s(version, "%d.%d", &major, &minor);
			#else
				sscanf(version, "%d.%d", &major, &minor);
			#endif

			openGLVersionStr = std::to_string(major) + "." + std::to_string(minor);
		}
		else
		{
			openGLVersionStr = "<unknown version>";
		}
	}
	std::string formattedStr = std::string("OpenGL ");
	if (gles)
	{
		formattedStr += "ES ";
	}
	formattedStr += openGLVersionStr;
	const char * pRendererStr = (const char*) glGetString(GL_RENDERER);
	if (pRendererStr)
	{
		formattedStr += std::string(" (") + pRendererStr + ")";
	}

	return formattedStr;
}

static const unsigned int channelsPerPixel = 3;

bool gl_context::getScreenshot(std::function<void (std::unique_ptr<iV_Image>)> callback)
{
	ASSERT_OR_RETURN(false, callback.operator bool(), "Must provide a valid callback");

	// IMPORTANT: Must get the size of the viewport directly from the viewport, to account for
	//            high-DPI / display scaling factors (or only a sub-rect of the full viewport
	//            will be captured, as the logical screenWidth/Height may differ from the
	//            underlying viewport pixel dimensions).
	GLint m_viewport[4] = {0,0,0,0};
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	if (m_viewport[2] == 0 || m_viewport[3] == 0)
	{
		// Failed to get useful / non-0 viewport size
		debug(LOG_3D, "GL_VIEWPORT either failed or returned an invalid size");
		return false;
	}

	auto image = std::make_unique<iV_Image>();
	auto width = m_viewport[2];
	auto height = m_viewport[3];
	bool allocateResult = image->allocate(width, height, channelsPerPixel); // RGB
	ASSERT_OR_RETURN(false, allocateResult, "Failed to allocate buffer");

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, image->width(), image->height(), GL_RGB, GL_UNSIGNED_BYTE, image->bmp_w());

	callback(std::move(image));

	return true;
}

// MARK: khr_callback

#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)

static const PreprocessedASCIISearchString OOM_GL_ERROR_SEARCH_STRING = PreprocessedASCIISearchString("GL_OUT_OF_MEMORY");
static std::atomic<bool> khrCallbackOomDetected(false);

static const char *cbsource(GLenum source)
{
	switch (source)
	{
		case GL_DEBUG_SOURCE_API: return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WM";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SC";
		case GL_DEBUG_SOURCE_THIRD_PARTY: return "3P";
		case GL_DEBUG_SOURCE_APPLICATION: return "WZ";
		default: case GL_DEBUG_SOURCE_OTHER: return "OTHER";
	}
}

static const char *cbtype(GLenum type)
{
	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR: return "Error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "Undefined";
		case GL_DEBUG_TYPE_PORTABILITY: return "Portability";
		case GL_DEBUG_TYPE_PERFORMANCE: return "Performance";
		case GL_DEBUG_TYPE_MARKER: return "Marker";
		default: case GL_DEBUG_TYPE_OTHER: return "Other";
	}
}

static const char *cbseverity(GLenum severity)
{
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH: return "High";
		case GL_DEBUG_SEVERITY_MEDIUM: return "Medium";
		case GL_DEBUG_SEVERITY_LOW: return "Low";
		case GL_DEBUG_SEVERITY_NOTIFICATION: return "Notification";
		default: return "Other";
	}
}

static void GLAPIENTRY khr_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	(void)userParam; // we pass in NULL here
	(void)length; // length of message, buggy on some drivers, don't use
	(void)id; // message id
	code_part log_level = LOG_INFO;
	if (type == GL_DEBUG_TYPE_ERROR)
	{
		if (severity == GL_DEBUG_SEVERITY_HIGH)
		{
			log_level = LOG_ERROR;

			// detect OUT_OF_MEMORY errors, set flag
			if (message != nullptr && containsSubstringPreprocessed(message, std::min<size_t>(length, strnlen(message, length)), OOM_GL_ERROR_SEARCH_STRING))
			{
				khrCallbackOomDetected.store(true);
			}
		}
	}
	debugLogFromGfxCallback(log_level, "GL::%s(%s:%s) : %s", cbsource(source), cbtype(type), cbseverity(severity), (message != nullptr) ? message : "");
}

#endif // defined(WZ_GL_KHR_DEBUG_SUPPORTED)

uint32_t gl_context::getSuggestedDefaultDepthBufferResolution() const
{
#if defined(GL_NVX_gpu_memory_info)
	// Use a (very simple) heuristic, that may or may not be useful - but basically try to find graphics cards that have lots of memory...
	if (GLAD_GL_NVX_gpu_memory_info)
	{
		// If GL_NVX_gpu_memory_info is available, get the total dedicated graphics memory
		GLint total_graphics_mem_kb = 0;
		glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &total_graphics_mem_kb);
		debug(LOG_3D, "GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX: %d", total_graphics_mem_kb);

		if ((total_graphics_mem_kb / 1024) >= 8192) // If >= 8 GiB graphics memory
		{
			return 4096;
		}
		else
		{
			return 2048;
		}
	}
#endif
//	else if (GLAD_GL_ATI_meminfo)
//	{
//		// For GL_ATI_meminfo, we could get the current free texture memory (w/ GL_TEXTURE_FREE_MEMORY_ATI, checking stats_kb[0])
//		// However we don't really have any way of differentiating between dedicated VRAM and shared system memory (ex. with integrated graphics)
//		// So instead, just ignore this
//	}

	// don't currently have a good way of checking video memory on this system
	// check some specific GL_RENDERER values...
#if defined(WZ_OS_WIN)
	WzString openGL_renderer = (const char*)wzSafeGlGetString(GL_RENDERER);
	if (openGL_renderer.startsWith("Intel(R) HD Graphics"))
	{
		// always default to 2048 on Intel HD Graphics...
		return 2048;
	}
#elif defined (__APPLE__)
	WzString openGL_vendor = (const char*)wzSafeGlGetString(GL_VENDOR);
	WzString openGL_renderer = (const char*)wzSafeGlGetString(GL_RENDERER);
	if (openGL_vendor == "Apple" && openGL_renderer.startsWith("Apple"))
	{
		// For Apple GPUs, check system RAM
		auto systemRAMinMiB = wzGetCurrentSystemRAM();
		if (systemRAMinMiB >= 16384) // If >= 16 GB of system (unified) RAM
		{
			return 4096;
		}
		else
		{
			return 2048;
		}
	}
#endif

	// In all other cases, default to 2048 for better performance by default
	return 2048;
}

bool gl_context::_initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode mode, optional<float> _mipLodBias, uint32_t _depthMapResolution)
{
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	khrCallbackOomDetected.store(false);
#endif

	// obtain backend_OpenGL_Impl from impl
	backend_impl = impl.createOpenGLBackendImpl();
	if (!backend_impl)
	{
		debug(LOG_ERROR, "Failed to get OpenGL backend implementation");
		return false;
	}

	if (!backend_impl->createGLContext())
	{
		debug(LOG_ERROR, "Failed to create OpenGL context");
		return false;
	}

	if (!initGLContext())
	{
		backend_impl->destroyGLContext();
		return false;
	}

	multisamples = (antialiasing > 0) ? static_cast<uint32_t>(antialiasing) : 0;

	initPixelFormatsSupport();
	hasInstancedRenderingSupport = initInstancedFunctions();
	debug(LOG_INFO, "  * Instanced rendering support %s detected", hasInstancedRenderingSupport ? "was" : "was NOT");
	hasBorderClampSupport = initCheckBorderClampSupport();

	int width, height = 0;
	backend_impl->getDrawableSize(&width, &height);
	debug(LOG_WZ, "Drawable Size: %d x %d", width, height);
	width = std::max<int>(width, 0);
	height = std::max<int>(height, 0);

	wzGLClearErrors();

	glViewport(0, 0, width, height);
	wzGLCheckErrors();
	viewportWidth = static_cast<uint32_t>(width);
	viewportHeight = static_cast<uint32_t>(height);
	glCullFace(GL_FRONT);
	//	glEnable(GL_CULL_FACE);
	wzGLCheckErrors();

	sceneFramebufferWidth = std::max<uint32_t>(viewportWidth, 2);
	sceneFramebufferHeight = std::max<uint32_t>(viewportHeight, 2);

	// initialize default (0) textures
	if (!createDefaultTextures())
	{
		// Failed initializing one or more default textures
		shutdown();
		wzResetGfxSettingsOnFailure(); // reset certain settings (like MSAA) that could be contributing to OUT_OF_MEMORY (or other) errors
		return false;
	}

	if (!setSwapIntervalInternal(mode))
	{
		// default to vsync on
		debug(LOG_3D, "Failed to set swap interval: %d; defaulting to vsync on", to_int(mode));
		setSwapIntervalInternal(gfx_api::context::swap_interval_mode::vsync);
	}

	mipLodBias = _mipLodBias;
	depthBufferResolution = _depthMapResolution;
	if (depthBufferResolution == 0)
	{
		depthBufferResolution = getSuggestedDefaultDepthBufferResolution();
	}

	if (!createSceneRenderpass())
	{
		// Treat failure to create the scene render pass as a fatal error
		shutdown();
		wzResetGfxSettingsOnFailure(); // reset certain settings (like MSAA) that could be contributing to OUT_OF_MEMORY (or other) errors
		return false;
	}
	initDepthPasses(depthBufferResolution);

#if !defined(__EMSCRIPTEN__)
	_beginRenderPassImpl();
#endif

	return true;
}

bool gl_context::createDefaultTextures()
{
	wzGLClearErrors(); // clear OpenGL error states

	// initialize default (0) textures
	const size_t defaultTexture_width = 2;
	const size_t defaultTexture_height = 2;
	pDefaultTexture = dynamic_cast<gl_texture*>(create_texture(1, defaultTexture_width, defaultTexture_height, gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8, "<default_texture>"));
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	pDefaultArrayTexture = dynamic_cast<gl_texture_array*>(create_texture_array(1, 1, defaultTexture_width, defaultTexture_height, gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8, "<default_array_texture>"));
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	pDefaultDepthTexture = create_depthmap_texture(WZ_MAX_SHADOW_CASCADES, 2, 2, "<default_depth_map>");
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	if (!pDefaultDepthTexture)
	{
		debug(LOG_INFO, "Failed to create default depth texture??");
	}

	iV_Image defaultTexture;
	defaultTexture.allocate(defaultTexture_width, defaultTexture_height, 4, true);
	if (!pDefaultTexture->upload(0, defaultTexture))
	{
		debug(LOG_ERROR, "Failed to upload default texture??");
	}
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	if (!pDefaultArrayTexture->upload_layer(0, 0, defaultTexture))
	{
		debug(LOG_ERROR, "Failed to upload default array texture??");
	}
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	pDefaultArrayTexture->flush();
	ASSERT_GL_NOERRORS_OR_RETURN(false);

	return true;
}

static const GLubyte* wzSafeGlGetString(GLenum name)
{
	static const GLubyte emptyString[1] = {0};
#if !defined(WZ_STATIC_GL_BINDINGS)
	ASSERT_OR_RETURN(emptyString, glGetString != nullptr, "glGetString is null");
#endif
	auto result = glGetString(name);
	if (result == nullptr)
	{
		return emptyString;
	}
	return result;
}

bool gl_context::isBlocklistedGraphicsDriver() const
{
	WzString openGL_renderer = (const char*)wzSafeGlGetString(GL_RENDERER);
	WzString openGL_version = (const char*)wzSafeGlGetString(GL_VERSION);

	debug(LOG_3D, "Checking: Renderer: \"%s\", Version: \"%s\"", openGL_renderer.toUtf8().c_str(), openGL_version.toUtf8().c_str());

#if defined(WZ_OS_WIN)
	// Renderer: Intel(R) HD Graphics 4000
	if (openGL_renderer == "Intel(R) HD Graphics 4000")
	{
		// Version: 3.1.0 - Build 10.18.10.3304
		// Version: <opengl version> - Build 10.18.10.3304
		// This is a problematic old driver on Windows that just crashes after init.
		if (openGL_version.endsWith("Build 10.18.10.3304"))
		{
			return true;
		}

		// Version: 3.1.0 - Build 9.17.10.2843
		// Version: <opengl version> - Build 9.17.10.2843
		// This is a problematic old driver (seen on Windows 8) that likes to crash during gameplay or sometimes at init.
		if (openGL_version.endsWith("Build 9.17.10.2843"))
		{
			return true;
		}
	}

	// Renderer: Intel(R) HD Graphics
	if (openGL_renderer == "Intel(R) HD Graphics")
	{
		// Version: 3.1.0 - Build 10.18.10.3277
		// Version: <opengl version> - Build 10.18.10.3277
		// This is a problematic old driver on Windows that likes to crash during gameplay (and throw various errors about the shaders).
		if (openGL_version.endsWith("Build 10.18.10.3277"))
		{
			return true;
		}
	}

	// Renderer: Intel(R) Graphics Media Accelerator 3600 Series
	if (openGL_renderer == "Intel(R) Graphics Media Accelerator 3600 Series")
	{
		// Does not work with WZ. (No indications that there is a driver version that does not crash.)
		return true;
	}

	// Renderer: softpipe
	// (From the OpenGLOn12 / "OpenGL Compatibility Pack")
	if (openGL_renderer == "softpipe")
	{
		WzString openGL_vendor = (const char*)wzSafeGlGetString(GL_VENDOR);
		if (openGL_vendor == "Mesa")
		{
			// Does not work performantly, can cause crashes (as of "Mesa 24.2.0-devel (git-57f4f8520a)")
			// Since libANGLE is very likely to work better, reject this (for now)
			return true;
		}
	}
#endif

	return false;
}

#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)

bool gl_context::enableDebugMessageCallbacks()
{
	if (!useKHRSuffixedDebugFuncs())
	{
		if (glDebugMessageCallback && glDebugMessageControl)
		{
			glDebugMessageCallback(khr_callback, nullptr);
			wzGLCheckErrors();
			glEnable(GL_DEBUG_OUTPUT);
			wzGLCheckErrors();
			// Do not want to output notifications. Some drivers spam them too much.
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
			wzGLCheckErrors();
			debug(LOG_3D, "Enabling KHR_debug message callback");
			return true;
		}
		else
		{
			debug(LOG_3D, "Failed to enable KHR_debug message callback");
		}
	}
	else
	{
		if (glDebugMessageCallbackKHR && glDebugMessageControlKHR)
		{
			glDebugMessageCallbackKHR(khr_callback, nullptr);
			wzGLCheckErrors();
			glEnable(GL_DEBUG_OUTPUT_KHR);
			wzGLCheckErrors();
			// Do not want to output notifications. Some drivers spam them too much.
			glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION_KHR, 0, nullptr, GL_FALSE);
			wzGLCheckErrors();
			debug(LOG_3D, "Enabling KHR_debug message callback");
			return true;
		}
		else
		{
			debug(LOG_3D, "Failed to enable KHR_debug message callback");
		}
	}

	return false;
}

void gl_context::wzGLObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar *label)
{
	if (!(/*GLAD_GL_VERSION_4_3 ||*/ GLAD_GL_ES_VERSION_3_2 || GLAD_GL_KHR_debug))
	{
		return;
	}

	if (!useKHRSuffixedDebugFuncs())
	{
		if (glObjectLabel)
		{
			glObjectLabel(identifier, name, length, label);
		}
	}
	else
	{
		if (glObjectLabelKHR)
		{
			glObjectLabelKHR(identifier, name, length, label);
		}
	}
}

void gl_context::wzGLPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar *message)
{
	if (!(/*GLAD_GL_VERSION_4_3 ||*/ GLAD_GL_ES_VERSION_3_2 || GLAD_GL_KHR_debug))
	{
		return;
	}

	if (!useKHRSuffixedDebugFuncs())
	{
		if (glPushDebugGroup)
		{
			glPushDebugGroup(source, id, length, message);
		}
	}
	else
	{
		if (glPushDebugGroupKHR)
		{
			glPushDebugGroupKHR(source, id, length, message);
		}
	}
}

void gl_context::wzGLPopDebugGroup()
{
	if (!(/*GLAD_GL_VERSION_4_3 ||*/ GLAD_GL_ES_VERSION_3_2 || GLAD_GL_KHR_debug))
	{
		return;
	}

	if (!useKHRSuffixedDebugFuncs())
	{
		if (glPopDebugGroup)
		{
			glPopDebugGroup();
		}
	}
	else
	{
		if (glPopDebugGroupKHR)
		{
			glPopDebugGroupKHR();
		}
	}
}

bool gl_context::useKHRSuffixedDebugFuncs()
{
	if (!gles)
	{
		// Desktop OpenGL does *not* use KHR-suffixed entrypoints
		return false;
	}

	// OpenGL ES:
	//
	// OpenGL ES 3.2 makes this "core" (without the suffix)
	if (GLAD_GL_ES_VERSION_3_2)
	{
		return false;
	}
	// When KHR_debug is implemented in an OpenGL ES context, all entry points defined must have a "KHR" suffix.
	// Reference: https://registry.khronos.org/OpenGL/extensions/KHR/KHR_debug.txt
	if (GLAD_GL_KHR_debug)
	{
		return true;
	}
	return false;
}

#else

bool gl_context::enableDebugMessageCallbacks()
{
	return false;
}

void gl_context::wzGLObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar *label)
{
	// no-op
}

void gl_context::wzGLPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar *message)
{
	// no-op
}

void gl_context::wzGLPopDebugGroup()
{
	// no-op
}

bool gl_context::useKHRSuffixedDebugFuncs()
{
	return false;
}

#endif



bool gl_context::initGLContext()
{
	frameNum = 1;
	gles = backend_impl->isOpenGLES();

#if !defined(WZ_STATIC_GL_BINDINGS)
	GLADloadproc func_GLGetProcAddress = backend_impl->getGLGetProcAddress();
	if (!func_GLGetProcAddress)
	{
		debug(LOG_FATAL, "backend_impl->getGLGetProcAddress() returned NULL");
		return false;
	}
	if (!gles)
	{
		if (!gladLoadGLLoader(func_GLGetProcAddress))
		{
			debug(LOG_FATAL, "gladLoadGLLoader failed");
			return false;
		}
	}
	else
	{
		if (!gladLoadGLES2Loader(func_GLGetProcAddress))
		{
			debug(LOG_FATAL, "gladLoadGLLoader failed");
			return false;
		}
	}
#endif

	/* Dump general information about OpenGL implementation to the console and the dump file */
	ssprintf(opengl.vendor, "OpenGL Vendor: %s", wzSafeGlGetString(GL_VENDOR));
	addDumpInfo(opengl.vendor);
	debug(LOG_INFO, "%s", opengl.vendor);
	ssprintf(opengl.renderer, "OpenGL Renderer: %s", wzSafeGlGetString(GL_RENDERER));
	addDumpInfo(opengl.renderer);
	debug(LOG_INFO, "%s", opengl.renderer);
	ssprintf(opengl.version, "OpenGL Version: %s", wzSafeGlGetString(GL_VERSION));
	addDumpInfo(opengl.version);
	debug(LOG_INFO, "%s", opengl.version);

	formattedRendererInfoString = calculateFormattedRendererInfoString();

	if (isBlocklistedGraphicsDriver())
	{
		debug(LOG_INFO, "Warzone's OpenGL backend is not supported on this system. (%s, %s)", opengl.renderer, opengl.version);
		debug(LOG_INFO, "Please update your graphics drivers or try a different backend.");
		return false;
	}

#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	khr_debug = (/*GLAD_GL_VERSION_4_3 ||*/ GLAD_GL_ES_VERSION_3_2 || GLAD_GL_KHR_debug);
#else
	khr_debug = false;
#endif

#if !defined(__EMSCRIPTEN__)

	/* Dump extended information about OpenGL implementation to the console */

	std::vector<std::string> glExtensions = getGLExtensions();
	std::string line;
	for (unsigned n = 0; n < glExtensions.size(); ++n)
	{
		std::string word = " ";
		word += glExtensions[n];
		if (n + 1 != glExtensions.size())
		{
			word += ',';
		}
		if (line.size() + word.size() > 160)
		{
			debug(LOG_3D, "OpenGL Extensions:%s", line.c_str());
			line.clear();
		}
		line += word;
	}
	debug(LOG_3D, "OpenGL Extensions:%s", line.c_str());
	if (!gles)
	{
		debug(LOG_3D, "Notable OpenGL features:");
		debug(LOG_3D, "  * OpenGL 2.0 %s supported!", GLAD_GL_VERSION_2_0 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL 2.1 %s supported!", GLAD_GL_VERSION_2_1 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL 3.0 %s supported!", GLAD_GL_VERSION_3_0 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL 3.1 %s supported!", GLAD_GL_VERSION_3_1 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL 3.2 %s supported!", GLAD_GL_VERSION_3_2 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL 3.3 %s supported!", GLAD_GL_VERSION_3_3 ? "is" : "is NOT");
	#ifdef GLAD_GL_VERSION_4_0
		debug(LOG_3D, "  * OpenGL 4.0 %s supported!", GLAD_GL_VERSION_4_0 ? "is" : "is NOT");
	#endif
	#ifdef GLAD_GL_VERSION_4_1
		debug(LOG_3D, "  * OpenGL 4.1 %s supported!", GLAD_GL_VERSION_4_1 ? "is" : "is NOT");
	#endif

		debug(LOG_3D, "  * Stencil wrap %s supported.", GLAD_GL_EXT_stencil_wrap ? "is" : "is NOT");
		debug(LOG_3D, "  * Rectangular texture %s supported.", GLAD_GL_ARB_texture_rectangle ? "is" : "is NOT");
		debug(LOG_3D, "  * FrameBuffer Object (FBO) %s supported.", GLAD_GL_EXT_framebuffer_object ? "is" : "is NOT");
		debug(LOG_3D, "  * ARB Vertex Buffer Object (VBO) %s supported.", GLAD_GL_ARB_vertex_buffer_object ? "is" : "is NOT");
		debug(LOG_3D, "  * NPOT %s supported.", GLAD_GL_ARB_texture_non_power_of_two ? "is" : "is NOT");
		debug(LOG_3D, "  * texture cube_map %s supported.", GLAD_GL_ARB_texture_cube_map ? "is" : "is NOT");
	#if defined(WZ_GL_TIMER_QUERY_SUPPORTED)
		debug(LOG_3D, "  * GL_ARB_timer_query %s supported!", GLAD_GL_ARB_timer_query ? "is" : "is NOT");
	#endif
	}
	else
	{
		debug(LOG_3D, "  * OpenGL ES 2.0 %s supported!", GLAD_GL_ES_VERSION_2_0 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL ES 3.0 %s supported!", GLAD_GL_ES_VERSION_3_0 ? "is" : "is NOT");
	}
	debug(LOG_3D, "  * Anisotropic filtering %s supported.", GLAD_GL_EXT_texture_filter_anisotropic ? "is" : "is NOT");
#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	debug(LOG_3D, "  * KHR_DEBUG support %s detected", khr_debug ? "was" : "was NOT");
#endif
	debug(LOG_3D, "  * glGenerateMipmap support %s detected", glGenerateMipmap ? "was" : "was NOT");

	if (!GLAD_GL_VERSION_3_0 && !GLAD_GL_ES_VERSION_3_0)
	{
		debug(LOG_POPUP, "OpenGL 3.0+ / OpenGL ES 3.0+ not supported! Please upgrade your drivers.");
		return false;
	}

	if (!gles)
	{
		if (!GLAD_GL_VERSION_3_1)
		{
			// non-fatal pop-up, to warn about OpenGL < 3.1 (we may require 3.1+ in the future)
			debug(LOG_POPUP, "OpenGL 3.1+ is not supported - Please upgrade your drivers or try a different graphics backend.");
		}
	}

#else

	// Emscripten-specific

	const char* version = (const char*)wzSafeGlGetString(GL_VERSION);
	bool WZ_WEB_GL_VERSION_2_0 = false;
	if (strncmp(version, "OpenGL ES 2.0", 13) == 0)
	{
		// WebGL 1 - not supported
		debug(LOG_POPUP, "WebGL 2.0 not supported. Please upgrade your browser / drivers.");
		return false;
	}
	else if (strncmp(version, "OpenGL ES 3.0", 13) == 0)
	{
		// WebGL 2
		WZ_WEB_GL_VERSION_2_0 = true;
		GLAD_GL_ES_VERSION_3_0 = 1;
	}
	else
	{
		debug(LOG_POPUP, "Unsupported WebGL version string: %s", version);
		return false;
	}

	debug(LOG_3D, "  * WebGL 2.0 %s supported!", WZ_WEB_GL_VERSION_2_0 ? "is" : "is NOT");

	if (!initWebGLExtensions())
	{
		debug(LOG_ERROR, "Failed to get WebGL extensions");
	}
	GLAD_GL_EXT_texture_filter_anisotropic = enabledWebGLExtensions.count("EXT_texture_filter_anisotropic") > 0;

	debug(LOG_3D, "  * Anisotropic filtering %s supported.", GLAD_GL_EXT_texture_filter_anisotropic ? "is" : "is NOT");
	// FUTURE TODO: Check and output other extensions

#endif

	fragmentHighpFloatAvailable = true;
	fragmentHighpIntAvailable = true;
	if (gles)
	{
		GLboolean bShaderCompilerSupported = GL_FALSE;
		glGetBooleanv(GL_SHADER_COMPILER, &bShaderCompilerSupported);
		if (bShaderCompilerSupported != GL_TRUE)
		{
			debug(LOG_FATAL, "Shader compiler is not supported! (GL_SHADER_COMPILER == GL_FALSE)");
			return false;
		}

		int highpFloatRange[2] = {0}, highpFloatPrecision = 0;
		glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, highpFloatRange, &highpFloatPrecision);
		fragmentHighpFloatAvailable = highpFloatPrecision != 0;

		int highpIntRange[2] = {0}, highpIntPrecision = 0;
		glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_INT, highpIntRange, &highpIntPrecision);
		fragmentHighpIntAvailable = highpFloatRange[1] != 0;

		if (!fragmentHighpFloatAvailable || !fragmentHighpIntAvailable)
		{
			// This can lead to a uniform precision mismatch between the vertex and fragment shaders
			// (if there are duplicate uniform names).
			//
			// This is now handled with a workaround when processing shaders, so just log it.
			debug(LOG_FATAL, "OpenGL ES: Fragment shaders do not support high precision: (highpFloat: %d; highpInt: %d)", (int)fragmentHighpFloatAvailable, (int)fragmentHighpIntAvailable);
			// FUTURE TODO: return false;
		}
	}

	std::pair<int, int> glslVersion(0, 0);
	sscanf((char const *)wzSafeGlGetString(GL_SHADING_LANGUAGE_VERSION), "%d.%d", &glslVersion.first, &glslVersion.second);

	/* Dump information about OpenGL 2.0+ implementation to the console and the dump file */
	GLint glMaxTIUs = 0, glMaxTIUAs = 0, glmaxSamples = 0, glmaxSamplesbuf = 0, glmaxVertexAttribs = 0, glMaxArrayTextureLayers = 0;

	debug(LOG_3D, "  * OpenGL GLSL Version : %s", wzSafeGlGetString(GL_SHADING_LANGUAGE_VERSION));
	ssprintf(opengl.GLSLversion, "OpenGL GLSL Version : %s", wzSafeGlGetString(GL_SHADING_LANGUAGE_VERSION));
	addDumpInfo(opengl.GLSLversion);

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &glMaxTIUs);
	debug(LOG_3D, "  * Total number of Texture Image Units (TIUs) supported is %d.", (int) glMaxTIUs);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &glMaxTIUAs);
	debug(LOG_3D, "  * Total number of Texture Image Units ARB(TIUAs) supported is %d.", (int) glMaxTIUAs);
	glGetIntegerv(GL_SAMPLE_BUFFERS, &glmaxSamplesbuf);
	debug(LOG_3D, "  * (current) Max Sample buffer is %d.", (int) glmaxSamplesbuf);
	glGetIntegerv(GL_SAMPLES, &glmaxSamples);
	debug(LOG_3D, "  * (current) Max Sample level is %d.", (int) glmaxSamples);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &glmaxVertexAttribs);
	debug(LOG_3D, "  * (current) Max vertex attribute locations is %d.", (int) glmaxVertexAttribs);
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &glMaxArrayTextureLayers);
	debug(LOG_3D, "  * (current) Max array texture layers is %d.", (int) glMaxArrayTextureLayers);
	maxArrayTextureLayers = glMaxArrayTextureLayers;

	uint32_t estimatedVRAMinMiB = get_estimated_vram_mb(false);
	if (estimatedVRAMinMiB > 0)
	{
		debug(LOG_3D, "  * Estimated VRAM is %" PRIu32 " MiB", estimatedVRAMinMiB);
	}

	// IMPORTANT: Reserve enough slots in enabledVertexAttribIndexes based on glmaxVertexAttribs
	if (glmaxVertexAttribs == 0)
	{
		debug(LOG_3D, "GL_MAX_VERTEX_ATTRIBS did not return a value - defaulting to 8");
		glmaxVertexAttribs = 8;
	}
	enabledVertexAttribIndexes.resize(static_cast<size_t>(glmaxVertexAttribs), false);

	wzGLClearErrors();

#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	if (khr_debug)
	{
		enableDebugMessageCallbacks();
	}
#endif

#if !defined(__EMSCRIPTEN__)
	if (GLAD_GL_VERSION_3_0) // if context is OpenGL 3.0+
	{
		// Very simple VAO code - just bind a single global VAO (this gets things working, but is not optimal)
		vaoId = 0;
		if (glGenVertexArrays == nullptr)
		{
			debug(LOG_FATAL, "glGenVertexArrays is not available, but context is OpenGL 3.0+");
			return false;
		}
		glGenVertexArrays(1, &vaoId);
		wzGLCheckErrors();
		glBindVertexArray(vaoId);
		wzGLCheckErrors();
	}
#endif

#if defined(WZ_GL_TIMER_QUERY_SUPPORTED)
	if (GLAD_GL_ARB_timer_query)
	{
		glGenQueries(PERF_COUNT, perfpos);
		wzGLCheckErrors();
	}
#endif

	if (GLAD_GL_EXT_texture_filter_anisotropic)
	{
		maxTextureAnisotropy = 0.f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxTextureAnisotropy);
		debug(LOG_3D, "  * (current) maxTextureAnisotropy: %f", maxTextureAnisotropy);
		wzGLCheckErrors();
	}

	glGenBuffers(1, &scratchbuffer);
	wzGLCheckErrors();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	wzGLCheckErrors();

	debug(LOG_INFO, "Success");

	return true;
}

size_t gl_context::numDepthPasses()
{
	return depthFBO.size();
}

bool gl_context::setDepthPassProperties(size_t _numDepthPasses, size_t _depthBufferResolution)
{
	if (depthPassCount == _numDepthPasses
		&& depthBufferResolution == _depthBufferResolution)
	{
		// nothing to do
		return true;
	}

	depthPassCount = _numDepthPasses;
	depthBufferResolution = _depthBufferResolution;

	// reinitialize depth passes
	initDepthPasses(depthBufferResolution);

	return true;
}

void gl_context::beginDepthPass(size_t idx)
{
	ASSERT_OR_RETURN(, idx < depthFBO.size(), "Invalid depth pass #: %zu", idx);
	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO[idx]);
	glViewport(0, 0, static_cast<GLsizei>(depthBufferResolution), static_cast<GLsizei>(depthBufferResolution));
	glClear(GL_DEPTH_BUFFER_BIT);
}

size_t gl_context::getDepthPassDimensions(size_t idx)
{
	return depthBufferResolution;
}

void gl_context::endCurrentDepthPass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	glViewport(0, 0, viewportWidth, viewportHeight);
//	glDepthMask(GL_TRUE);
}

gfx_api::abstract_texture* gl_context::getDepthTexture()
{
	return depthTexture;
}

void gl_context::_beginRenderPassImpl()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, viewportWidth, viewportHeight);
	GLbitfield clearFlags = 0;
	glDepthMask(GL_TRUE);
	clearFlags = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	glClear(clearFlags);
}

void gl_context::beginRenderPass()
{
#if defined(__EMSCRIPTEN__)
	_beginRenderPassImpl();
#else
	// no-op everywhere else
#endif
}

[[noreturn]] static void glContextHandleOOMError()
{
	wzDisplayFatalGfxBackendFailure("GL_OUT_OF_MEMORY");
	wzResetGfxSettingsOnFailure();
	abort();
}

void gl_context::endRenderPass()
{
	frameNum = std::max<size_t>(frameNum + 1, 1);
	backend_impl->swapWindow();
	glUseProgram(0);
	current_program = nullptr;
#if !defined(__EMSCRIPTEN__)
	_beginRenderPassImpl();
#endif

#if defined(WZ_GL_KHR_DEBUG_SUPPORTED)
	if (khrCallbackOomDetected.load())
	{
		glContextHandleOOMError();
	}
#endif
}

bool gl_context::supportsInstancedRendering()
{
	return hasInstancedRenderingSupport;
}

bool gl_context::textureFormatIsSupported(gfx_api::pixel_format_target target, gfx_api::pixel_format format, gfx_api::pixel_format_usage::flags usage)
{
	size_t formatIdx = static_cast<size_t>(format);
	ASSERT_OR_RETURN(false, formatIdx < textureFormatsSupport[static_cast<size_t>(target)].size(), "Invalid format index: %zu", formatIdx);
	return (textureFormatsSupport[static_cast<size_t>(target)][formatIdx] & usage) == usage;
}

#if defined(__EMSCRIPTEN__)

static gfx_api::pixel_format_usage::flags getPixelFormatUsageSupport_gl(GLenum target, gfx_api::pixel_format format, bool gles)
{
	gfx_api::pixel_format_usage::flags retVal = gfx_api::pixel_format_usage::none;

	switch (format)
	{
		// UNCOMPRESSED FORMATS
		case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
			retVal |= gfx_api::pixel_format_usage::sampled_image;
			break;
		case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
			retVal |= gfx_api::pixel_format_usage::sampled_image;
			break;
		case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
			retVal |= gfx_api::pixel_format_usage::sampled_image;
			break;
		case gfx_api::pixel_format::FORMAT_RG8_UNORM:
			// supported in WebGL2
			retVal |= gfx_api::pixel_format_usage::sampled_image;
			break;
		case gfx_api::pixel_format::FORMAT_R8_UNORM:
			retVal |= gfx_api::pixel_format_usage::sampled_image;
			break;
		// COMPRESSED FORMAT
		case gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM:
		case gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM:
		case gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM:
			if (enabledWebGLExtensions.count("WEBGL_compressed_texture_s3tc") > 0)
			{
				retVal |= gfx_api::pixel_format_usage::sampled_image;
			}
			break;
		case gfx_api::pixel_format::FORMAT_R_BC4_UNORM:
		case gfx_api::pixel_format::FORMAT_RG_BC5_UNORM:
			if (enabledWebGLExtensions.count("EXT_texture_compression_rgtc") > 0)
			{
				retVal |= gfx_api::pixel_format_usage::sampled_image;
			}
			break;
		case gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM:
			if (enabledWebGLExtensions.count("EXT_texture_compression_bptc") > 0)
			{
				retVal |= gfx_api::pixel_format_usage::sampled_image;
			}
			break;
		case gfx_api::pixel_format::FORMAT_RGB8_ETC1:
			// not supported
			break;
		case gfx_api::pixel_format::FORMAT_RGB8_ETC2:
		case gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC:
		case gfx_api::pixel_format::FORMAT_R11_EAC:
		case gfx_api::pixel_format::FORMAT_RG11_EAC:
			if (enabledWebGLExtensions.count("WEBGL_compressed_texture_etc") > 0)
			{
				retVal |= gfx_api::pixel_format_usage::sampled_image;
			}
			break;
		case gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM:
			if (enabledWebGLExtensions.count("WEBGL_compressed_texture_astc") > 0)
			{
				retVal |= gfx_api::pixel_format_usage::sampled_image;
			}
			break;
		default:
			debug(LOG_INFO, "Unrecognised pixel format");
	}

	return retVal;
}

void gl_context::initPixelFormatsSupport()
{
	// set any existing entries to false
	for (size_t target = 0; target < gfx_api::PIXEL_FORMAT_TARGET_COUNT; target++)
	{
		for (size_t i = 0; i < textureFormatsSupport[target].size(); i++)
		{
			textureFormatsSupport[target][i] = gfx_api::pixel_format_usage::none;
		}
		textureFormatsSupport[target].resize(static_cast<size_t>(gfx_api::MAX_PIXEL_FORMAT) + 1, gfx_api::pixel_format_usage::none);
	}

	// check if 2D texture array support is available
	has2DTextureArraySupport = true; // Texture arrays are supported in OpenGL ES 3.0+ / WebGL 2.0

	#define PIXEL_2D_FORMAT_SUPPORT_SET(x) \
	textureFormatsSupport[static_cast<size_t>(gfx_api::pixel_format_target::texture_2d)][static_cast<size_t>(x)] = getPixelFormatUsageSupport_gl(GL_TEXTURE_2D, x, gles);

	#define PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(x) \
	if (has2DTextureArraySupport) { textureFormatsSupport[static_cast<size_t>(gfx_api::pixel_format_target::texture_2d_array)][static_cast<size_t>(x)] = getPixelFormatUsageSupport_gl(GL_TEXTURE_2D_ARRAY, x, gles); }

	// The following are always guaranteed to be supported
	PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8)
	PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8)
	PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R8_UNORM)

	PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8)
	PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8)
	PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R8_UNORM)

	// RG8
	// WebGL: WebGL 2.0
	PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG8_UNORM)
	PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG8_UNORM)

	// S3TC
	// WebGL: WEBGL_compressed_texture_s3tc
	if (enabledWebGLExtensions.count("WEBGL_compressed_texture_s3tc") > 0)
	{
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM) // DXT1
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM) // DXT3
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM) // DXT5

		PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM) // DXT1
		PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM) // DXT3
		PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM) // DXT5
	}

	// RGTC
	// WebGL: EXT_texture_compression_rgtc
	if (enabledWebGLExtensions.count("EXT_texture_compression_rgtc") > 0)
	{
		// Note: EXT_texture_compression_rgtc does *NOT* support glCompressedTex*Image3D (even for 2d texture arrays)?
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R_BC4_UNORM)
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG_BC5_UNORM)
	}

	// BPTC
	// WebGL: EXT_texture_compression_bptc
	if (enabledWebGLExtensions.count("EXT_texture_compression_bptc") > 0)
	{
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM)
		PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM)
	}

	// ETC1

	// ETC2
	// WebGL: WEBGL_compressed_texture_etc
	if (enabledWebGLExtensions.count("WEBGL_compressed_texture_etc") > 0)
	{
		// NOTES:
		// WebGL 2.0 claims it is supported for 2d texture arrays
		bool canSupport2DTextureArrays = true;

		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB8_ETC2)
		if (canSupport2DTextureArrays)
		{
			PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB8_ETC2)
		}

		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC)
		if (canSupport2DTextureArrays)
		{
			PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC)
		}

		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R11_EAC)
		if (canSupport2DTextureArrays)
		{
			PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R11_EAC)
		}

		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG11_EAC)
		if (canSupport2DTextureArrays)
		{
			PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG11_EAC)
		}
	}

	// ASTC (LDR)
	// WebGL: WEBGL_compressed_texture_astc
	if (enabledWebGLExtensions.count("WEBGL_compressed_texture_astc") > 0)
	{
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM)
		PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM)
	}
}

#else // !defined(__EMSCRIPTEN__) // Regular OpenGL / OpenGL ES implementation

static gfx_api::pixel_format_usage::flags getPixelFormatUsageSupport_gl(GLenum target, gfx_api::pixel_format format, bool gles)
{
	gfx_api::pixel_format_usage::flags retVal = gfx_api::pixel_format_usage::none;

	auto internal_format = to_gl_internalformat(format, gles);
	if (internal_format == GL_INVALID_ENUM || internal_format == GL_FALSE)
	{
		return retVal;
	}

	if (gles || !GLAD_GL_ARB_internalformat_query2 || !glGetInternalformativ)
	{
		// Special cases:
		switch (format)
		{
			// ASTC:
			case gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM:
				//   - While WZ currently only supports ASTC on GLES, some desktop platforms support GLES *and* advertise ASTC support, but may encounter performance issues (in practice)
				//   - In OpenGL mode, glGetInternalformativ could be used to query "full support", but that functionality isn't currently available (and isn't even possible in OpenGL ES?)
				//   - So restrict this to specific platforms for now:
				//     - Android, Apple platforms (which should only advertise this if actually supported)
			#if defined(__ANDROID__) || defined(__APPLE__)
				break; // assume the format is available
			#else
				return gfx_api::pixel_format_usage::none; // immediately return - not available
			#endif
			// All other formats:
			default:
				// the query function is not available, so just assume that the format is available
				break;
		}

		// for now, mark it as available for sampled_image - TODO: investigate whether we can safely assume it's available for other usages
		retVal |= gfx_api::pixel_format_usage::sampled_image;
		return retVal;
	}

	GLint supported = GL_FALSE;
	glGetInternalformativ(target, internal_format, GL_INTERNALFORMAT_SUPPORTED, 1, &supported);
	if (!supported)
	{
		return retVal;
	}

	supported = GL_NONE;
	glGetInternalformativ(target, internal_format, GL_FRAGMENT_TEXTURE, 1, &supported);
	if (supported == GL_FULL_SUPPORT) // ignore caveat support for now
	{
		retVal |= gfx_api::pixel_format_usage::sampled_image;
	}

	GLint shader_image_load = GL_NONE;
	GLint shader_image_store = GL_NONE;
	glGetInternalformativ(target, internal_format, GL_SHADER_IMAGE_LOAD, 1, &shader_image_load);
	glGetInternalformativ(target, internal_format, GL_SHADER_IMAGE_STORE, 1, &shader_image_store);
	if (shader_image_load == GL_FULL_SUPPORT && shader_image_store == GL_FULL_SUPPORT) // ignore caveat support for now
	{
		retVal |= gfx_api::pixel_format_usage::storage_image;
	}

	GLint depth_renderable = GL_FALSE;
	GLint stencil_renderable = GL_FALSE;
	glGetInternalformativ(target, internal_format, GL_DEPTH_RENDERABLE, 1, &depth_renderable);
	glGetInternalformativ(target, internal_format, GL_STENCIL_RENDERABLE, 1, &stencil_renderable);
	if (depth_renderable && stencil_renderable)
	{
		retVal |= gfx_api::pixel_format_usage::depth_stencil_attachment;
	}

	return retVal;
}

void gl_context::initPixelFormatsSupport()
{
	// set any existing entries to false
	for (size_t target = 0; target < gfx_api::PIXEL_FORMAT_TARGET_COUNT; target++)
	{
		for (size_t i = 0; i < textureFormatsSupport[target].size(); i++)
		{
			textureFormatsSupport[target][i] = gfx_api::pixel_format_usage::none;
		}
		textureFormatsSupport[target].resize(static_cast<size_t>(gfx_api::MAX_PIXEL_FORMAT) + 1, gfx_api::pixel_format_usage::none);
	}

	// check if 2D texture array support is available
	has2DTextureArraySupport =
		(!gles && (GLAD_GL_VERSION_3_0 || GLAD_GL_EXT_texture_array)) // Texture arrays are supported in core OpenGL 3.0+, and earlier with EXT_texture_array
		|| (gles && GLAD_GL_ES_VERSION_3_0); // Texture arrays are supported in OpenGL ES 3.0+

	#define PIXEL_2D_FORMAT_SUPPORT_SET(x) \
	textureFormatsSupport[static_cast<size_t>(gfx_api::pixel_format_target::texture_2d)][static_cast<size_t>(x)] = getPixelFormatUsageSupport_gl(GL_TEXTURE_2D, x, gles);

	#define PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(x) \
	if (has2DTextureArraySupport) { textureFormatsSupport[static_cast<size_t>(gfx_api::pixel_format_target::texture_2d_array)][static_cast<size_t>(x)] = getPixelFormatUsageSupport_gl(GL_TEXTURE_2D_ARRAY, x, gles); }

	// The following are always guaranteed to be supported
//	texture2DFormatsSupport[static_cast<size_t>(gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8)] = true;
//	texture2DFormatsSupport[static_cast<size_t>(gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8)] = true;
//	texture2DFormatsSupport[static_cast<size_t>(gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8)] = true;
//	texture2DFormatsSupport[static_cast<size_t>(gfx_api::pixel_format::FORMAT_R8_UNORM)] = true;
	PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8)
	PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8)
	PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8)
	PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R8_UNORM)

	PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8)
	PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8)
	PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8)
	PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R8_UNORM)

	// RG8
	// Desktop OpenGL: OpenGL 3.0+, or GL_ARB_texture_rg
	// OpenGL ES: GL_EXT_texture_rg
	if ((!gles && (GLAD_GL_VERSION_3_0 || GLAD_GL_ARB_texture_rg)) || (gles && GLAD_GL_EXT_texture_rg))
	{
		// NOTES:
		// GL_ARB_texture_rg (and OpenGL 3.0+) supports this format for glTex*Image3D
		// GL_EXT_texture_rg does *NOT* specify support for glTex*Image3D?
		bool canSupport2DTextureArrays = (!gles && (GLAD_GL_VERSION_3_0 || GLAD_GL_ARB_texture_rg));

		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG8_UNORM)
		if (canSupport2DTextureArrays)
		{
			PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG8_UNORM)
		}
	}

	// S3TC
	// Desktop OpenGL: GL_EXT_texture_compression_s3tc
	// OpenGL ES: (May be supported by the same GL_EXT_texture_compression_s3tc, other extensions)
	if (GLAD_GL_EXT_texture_compression_s3tc)
	{
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM) // DXT1
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM) // DXT3
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM) // DXT5
		// NOTES:
		// On Desktop OpenGL, the EXT_texture_array extension interacts with the s3tc extension to provide 2d texture array support for these formats
		// The EXT_texture_compression_s3tc docs appear to say that OpenGL ES 3.0.2(?)+ supports passing these formats to glCompressedTex*Image3D for TEXTURE_2D_ARRAY support
		if ((!gles && (GLAD_GL_VERSION_3_0 || GLAD_GL_EXT_texture_array)) // EXT_texture_array is core in OpenGL 3.0+
			|| (gles && GLAD_GL_ES_VERSION_3_0))
		{
			PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM) // DXT1
			PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM) // DXT3
			PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM) // DXT5
		}
	}
	else if (gles)
	{
		if (GLAD_GL_ANGLE_texture_compression_dxt3)
		{
			textureFormatsSupport[static_cast<size_t>(gfx_api::pixel_format_target::texture_2d)][static_cast<size_t>(gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM)] = gfx_api::pixel_format_usage::sampled_image;
		}
		if (GLAD_GL_ANGLE_texture_compression_dxt5)
		{
			textureFormatsSupport[static_cast<size_t>(gfx_api::pixel_format_target::texture_2d)][static_cast<size_t>(gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM)] = gfx_api::pixel_format_usage::sampled_image;
		}
		if ((GLAD_GL_ANGLE_texture_compression_dxt3 || GLAD_GL_ANGLE_texture_compression_dxt5) && GLAD_GL_EXT_texture_compression_dxt1)
		{
			PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM) // DXT1
		}
	}

	// RGTC
	// Desktop OpenGL: GL_ARB_texture_compression_rgtc (or OpenGL 3.0+ Core?)
	// OpenGL ES: TODO: Could check for GL_EXT_texture_compression_rgtc, but support seems rare
	if (!gles && GLAD_GL_ARB_texture_compression_rgtc)
	{
		// Note: GL_ARB_texture_compression_rgtc does *NOT* support glCompressedTex*Image3D (even for 2d texture arrays)?
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R_BC4_UNORM)
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG_BC5_UNORM)
	}

	// BPTC
	// Desktop OpenGL: GL_ARB_texture_compression_bptc
	// OpenGL ES: GL_EXT_texture_compression_bptc
	if ((!gles && GLAD_GL_ARB_texture_compression_bptc) || (gles && GLAD_GL_EXT_texture_compression_bptc))
	{
		// NOTES:
		// GL_ARB_texture_compression_bptc claims it is supported in glCompressedTex*Image2DARB, glCompressedTex*Image3DARB, but these were folded into core without the ARB suffix in OpenGL 1.3?)
		// GL_EXT_texture_compression_bptc claims it is supported in glCompressedTex*Image2D, glCompressedTex*Image3D
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM)
		PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM)
	}

	// ETC1
	if (gles && GLAD_GL_OES_compressed_ETC1_RGB8_texture)
	{
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB8_ETC1)
	}

	// ETC2
	// Desktop OpenGL: Mandatory in OpenGL 4.3+, or with GL_ARB_ES3_compatibility
	//	- NOTES:
	//		- Desktop OpenGL (especially 4.3+ where it's required) may claim support for ETC2/EAC,
	//		  but may actually not have hardware support (so it'll just decompress in memory)
	//		- While theoretically the checks in getPixelFormatUsageSupport_gl should detect this,
	//		  it seems GL_FULL_SUPPORT is sometimes returned instead of GL_CAVEAT_SUPPORT (even when
	//		  there is no hardware support and the textures are decompressed by the driver in software)
	//		- To handle this, really restrict when it's permitted on desktop - only permit if S3TC is *not* supported
	// OpenGL ES: Mandatory in OpenGL ES 3.0+
	// IMPORTANT: **MUST** come after S3TC/DXT & RGTC is checked !!
	if ((gles && GLAD_GL_ES_VERSION_3_0) || (!gles && GLAD_GL_ARB_ES3_compatibility))
	{
		// NOTES:
		// OpenGL ES 3.0+ claims it is supported for 2d texture arrays
		// GL_ARB_ES3_compatibility makes no claims about support in glCompressedTex*Image3D?
		bool canSupport2DTextureArrays = (gles && GLAD_GL_ES_VERSION_3_0);

		if (gles || !textureFormatIsSupported(gfx_api::pixel_format_target::texture_2d, gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM, gfx_api::pixel_format_usage::sampled_image))
		{
			PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB8_ETC2)
			if (canSupport2DTextureArrays)
			{
				PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGB8_ETC2)
			}
		}
		if (gles || !textureFormatIsSupported(gfx_api::pixel_format_target::texture_2d, gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM, gfx_api::pixel_format_usage::sampled_image))
		{
			PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC)
			if (canSupport2DTextureArrays)
			{
				PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC)
			}
		}
		if (gles || !textureFormatIsSupported(gfx_api::pixel_format_target::texture_2d, gfx_api::pixel_format::FORMAT_R_BC4_UNORM, gfx_api::pixel_format_usage::sampled_image))
		{
			PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R11_EAC)
			if (canSupport2DTextureArrays)
			{
				PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_R11_EAC)
			}
		}
		if (gles || !textureFormatIsSupported(gfx_api::pixel_format_target::texture_2d, gfx_api::pixel_format::FORMAT_RG_BC5_UNORM, gfx_api::pixel_format_usage::sampled_image))
		{
			PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG11_EAC)
			if (canSupport2DTextureArrays)
			{
				PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_RG11_EAC)
			}
		}
	}

	// ASTC (LDR)
	// Either OpenGL: GL_KHR_texture_compression_astc_ldr
	// However, for now - until more testing is complete - only enable on OpenGL ES
	if (gles && GLAD_GL_KHR_texture_compression_astc_ldr)
	{
		// NOTE: GL_KHR_texture_compression_astc_ldr claims it is supported in glCompressedTex*Image3D
		PIXEL_2D_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM)
		PIXEL_2D_TEXTURE_ARRAY_FORMAT_SUPPORT_SET(gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM)
	}

	// Determine multiSampledBufferInternalFormat
	if (!gles)
	{
		// OpenGL
		// - Required formats for render buffer include: GL_RGB10_A2, GL_RGBA8
		// - Notably: GL_RGB8 is *not* a required format, so support may vary
		multiSampledBufferInternalFormat = GL_RGBA8;
		multiSampledBufferBaseFormat = GL_RGBA;
		maxMultiSampleBufferFormatSamples = 0;
		glGetIntegerv(GL_MAX_SAMPLES, &maxMultiSampleBufferFormatSamples);
		if (GLAD_GL_ARB_internalformat_query2 && glGetInternalformativ)
		{
			glGetInternalformativ(GL_RENDERBUFFER, multiSampledBufferInternalFormat, GL_SAMPLES, 1, &maxMultiSampleBufferFormatSamples);
		}
	}
	else
	{
		// OpenGL ES (3.0+)
		// - Required formats for render buffer include: GL_RGB10_A2, GL_RGBA8
		// - Notably: GL_RGB8 is *not* a required format, so support may vary
		multiSampledBufferInternalFormat = GL_RGBA8;
		multiSampledBufferBaseFormat = GL_RGBA;
		maxMultiSampleBufferFormatSamples = 0;
		glGetInternalformativ(GL_RENDERBUFFER, multiSampledBufferInternalFormat, GL_SAMPLES, 1, &maxMultiSampleBufferFormatSamples);
	}

	// Cap maxMultiSampleBufferFormatSamples at 16 (some drivers report supporting 32 but then crash)
	maxMultiSampleBufferFormatSamples = std::min<GLint>(maxMultiSampleBufferFormatSamples, 16);
	maxMultiSampleBufferFormatSamples = std::max<GLint>(maxMultiSampleBufferFormatSamples, 0);
}

#endif

bool gl_context::initInstancedFunctions()
{
#if !defined(WZ_STATIC_GL_BINDINGS)
	wz_dyn_glDrawArraysInstanced = nullptr;
	wz_dyn_glDrawElementsInstanced = nullptr;
	wz_dyn_glVertexAttribDivisor = nullptr;

	if (!gles)
	{
		if (GLAD_GL_VERSION_3_1)
		{
			wz_dyn_glDrawArraysInstanced = glDrawArraysInstanced;
			wz_dyn_glDrawElementsInstanced = glDrawElementsInstanced;
		}
		else if (GLAD_GL_ARB_draw_instanced)
		{
			wz_dyn_glDrawArraysInstanced = glDrawArraysInstancedARB;
			wz_dyn_glDrawElementsInstanced = glDrawElementsInstancedARB;
		}

		if (GLAD_GL_VERSION_3_3)
		{
			wz_dyn_glVertexAttribDivisor = glVertexAttribDivisor;
		}
		else if (GLAD_GL_ARB_instanced_arrays)
		{
			wz_dyn_glVertexAttribDivisor = glVertexAttribDivisorARB;
		}
	}
	else
	{
		if (GLAD_GL_ES_VERSION_3_0)
		{
			wz_dyn_glDrawArraysInstanced = glDrawArraysInstanced;
			wz_dyn_glDrawElementsInstanced = glDrawElementsInstanced;
			wz_dyn_glVertexAttribDivisor = glVertexAttribDivisor;
		}
		else if (GLAD_GL_EXT_instanced_arrays)
		{
			wz_dyn_glDrawArraysInstanced = glDrawArraysInstancedEXT;
			wz_dyn_glDrawElementsInstanced = glDrawElementsInstancedEXT;
			wz_dyn_glVertexAttribDivisor = glVertexAttribDivisorEXT;
		}
		else if (GLAD_GL_ANGLE_instanced_arrays)
		{
			wz_dyn_glDrawArraysInstanced = glDrawArraysInstancedANGLE;
			wz_dyn_glDrawElementsInstanced = glDrawElementsInstancedANGLE;
			wz_dyn_glVertexAttribDivisor = glVertexAttribDivisorANGLE;
		}
	}

	if (!wz_dyn_glDrawArraysInstanced || !wz_dyn_glDrawElementsInstanced || !wz_dyn_glVertexAttribDivisor)
	{
		wz_dyn_glDrawArraysInstanced = nullptr;
		wz_dyn_glDrawElementsInstanced = nullptr;
		wz_dyn_glVertexAttribDivisor = nullptr;
		return false;
	}
#endif
	return true;
}

bool gl_context::initCheckBorderClampSupport()
{
	// GL_CLAMP_TO_BORDER is supported on:
	// - OpenGL (any version we support)
	if (!gles)
	{
		return true;
	}
	// - OpenGL ES (3.2+, or with extensions)
	else
	{
#if !defined(WZ_STATIC_GL_BINDINGS) && !defined(__EMSCRIPTEN__)
		return GLAD_GL_ES_VERSION_3_2 || GLAD_GL_EXT_texture_border_clamp || GLAD_GL_OES_texture_border_clamp || GLAD_GL_NV_texture_border_clamp;
#else
		// - WebGL has no support
		return false;
#endif
	}
}

void gl_context::handleWindowSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	// Update the viewport to use the new *drawable* size (which may be greater than the new window size
	// if SDL's built-in high-DPI support is enabled and functioning).
	int drawableWidth = 0, drawableHeight = 0;
	backend_impl->getDrawableSize(&drawableWidth, &drawableHeight);
	debug(LOG_WZ, "Logical Size: %d x %d; Drawable Size: %d x %d", screenWidth, screenHeight, drawableWidth, drawableHeight);

	int width = std::max<int>(drawableWidth, 0);
	int height = std::max<int>(drawableHeight, 0);

	glViewport(0, 0, width, height);
	viewportWidth = static_cast<uint32_t>(width);
	viewportHeight = static_cast<uint32_t>(height);
	glCullFace(GL_FRONT);
	//	glEnable(GL_CULL_FACE);

	// Re-create scene FBOs using new size (if drawable size is of reasonable dimensions)
	if (viewportWidth > 0 && viewportHeight > 0)
	{
		uint32_t newSceneFramebufferWidth = std::max<uint32_t>(viewportWidth, 2);
		uint32_t newSceneFramebufferHeight = std::max<uint32_t>(viewportHeight, 2);

		if (sceneFramebufferWidth != newSceneFramebufferWidth || sceneFramebufferHeight != newSceneFramebufferHeight)
		{
			sceneFramebufferWidth = newSceneFramebufferWidth;
			sceneFramebufferHeight = newSceneFramebufferHeight;
			createSceneRenderpass();
		}
	}
	else
	{
		// Some drivers seem to like to occasionally return a drawableSize that has a 0 dimension (ex. when minimized?)
		// In this case, don't bother recreating the scene framebuffer (until it changes to something sensible)
		debug(LOG_INFO, "Delaying scene framebuffer recreation (current Drawable Size: %d x %d)", drawableWidth, drawableHeight);
	}
}

std::pair<uint32_t, uint32_t> gl_context::getDrawableDimensions()
{
	return {viewportWidth, viewportHeight};
}

bool gl_context::shouldDraw()
{
	return viewportWidth > 0 && viewportHeight > 0;
}

void gl_context::shutdown()
{
#if !defined(WZ_STATIC_GL_BINDINGS)
	if (glClear)
#endif
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	deleteSceneRenderpass();

#if !defined(WZ_STATIC_GL_BINDINGS)
	if (glDeleteFramebuffers)
#endif
	{
		if (depthFBO.size() > 0)
		{
			glDeleteFramebuffers(static_cast<GLsizei>(depthFBO.size()), depthFBO.data());
			depthFBO.clear();
		}
	}

	if (depthTexture)
	{
		delete depthTexture;
		depthTexture = nullptr;
	}

	if (pDefaultTexture)
	{
		delete pDefaultTexture;
		pDefaultTexture = nullptr;
	}

	if (pDefaultArrayTexture)
	{
		delete pDefaultArrayTexture;
		pDefaultArrayTexture = nullptr;
	}

	if (pDefaultDepthTexture)
	{
		delete pDefaultDepthTexture;
		pDefaultDepthTexture = nullptr;
	}

#if !defined(WZ_STATIC_GL_BINDINGS)
	if (glDeleteBuffers) // glDeleteBuffers might be NULL (if initializing the OpenGL loader library fails)
#endif
	{
		glDeleteBuffers(1, &scratchbuffer);
		scratchbuffer = 0;
	}

#if defined(WZ_GL_TIMER_QUERY_SUPPORTED)
	if (GLAD_GL_ARB_timer_query && glDeleteQueries)
	{
		glDeleteQueries(PERF_COUNT, perfpos);
	}
#endif

#if !defined(__EMSCRIPTEN__)
	if (GLAD_GL_VERSION_3_0) // if context is OpenGL 3.0+
	{
		// Cleanup from very simple VAO code (just bind a single global VAO)
		if (vaoId != 0)
		{
			if (glDeleteVertexArrays != nullptr)
			{
				glDeleteVertexArrays(1, &vaoId);
			}
			vaoId = 0;
		}
	}
#endif

	for (auto& pipelineInfo : createdPipelines)
	{
		if (pipelineInfo.pso)
		{
			delete pipelineInfo.pso;
		}
	}
	createdPipelines.clear();

#if defined(WZ_DEBUG_GFX_API_LEAKS)
	if (!debugLiveTextures.empty())
	{
		// Some textures were not properly freed before OpenGL context shutdown
		for (auto texture : debugLiveTextures)
		{
			debug(LOG_ERROR, "Texture resource not cleaned up: %p (name: %s)", (const void*)texture, texture->debugName.c_str());
		}
		ASSERT(debugLiveTextures.empty(), "There are %zu textures that were not cleaned up.", debugLiveTextures.size());
	}
#endif

	if (backend_impl)
	{
		backend_impl->destroyGLContext();
	}
}

const size_t& gl_context::current_FrameNum() const
{
	return frameNum;
}

bool gl_context::setSwapIntervalInternal(gfx_api::context::swap_interval_mode mode)
{
	return backend_impl->setSwapInterval(mode);
}

bool gl_context::setSwapInterval(gfx_api::context::swap_interval_mode mode, const SetSwapIntervalCompletionHandler& completionHandler)
{
	auto success = setSwapIntervalInternal(mode);
	if (success && completionHandler)
	{
		completionHandler();
	}
	return success;
}

gfx_api::context::swap_interval_mode gl_context::getSwapInterval() const
{
	return backend_impl->getSwapInterval();
}

bool gl_context::supportsMipLodBias() const
{
#if !defined(__EMSCRIPTEN__)
	if (!gles)
	{
		if (GLAD_GL_VERSION_2_1)
		{
			// glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, ...) should be available in OpenGL 3.0+ (OpenGL 3.3+?)
			// But also can be supported by providing bias to texture() / texture2d() sampling call in shader (so just do that)
			return true;
		}
		return false;
	}
	else
	{
		if (GLAD_GL_ES_VERSION_2_0)
		{
			// Can support on OpenGL ES 2.0+
			// By providing bias to texture() (OpenGL ES 3.0+) or texture2d() (OpenGL ES 2.0) sampling call in shader
			return true;
		}
		return false;
	}
#else
	// Can support on OpenGL ES 2.0+
	// By providing bias to texture() (OpenGL ES 3.0+) or texture2d() (OpenGL ES 2.0) sampling call in shader
	return true;
#endif
}

bool gl_context::supports2DTextureArrays() const
{
	return has2DTextureArraySupport;
}

bool gl_context::supportsIntVertexAttributes() const
{
#if !defined(__EMSCRIPTEN__)
	// glVertexAttribIPointer requires: OpenGL 3.0+ or OpenGL ES 3.0+
	bool hasRequiredVersion = (!gles && GLAD_GL_VERSION_3_0) || (gles && GLAD_GL_ES_VERSION_3_0);
	bool hasRequiredFunction = glVertexAttribIPointer != nullptr;
	return hasRequiredVersion && hasRequiredFunction;
#else
	// Always available in WebGL 2.0
	return true;
#endif
}

size_t gl_context::maxFramesInFlight() const
{
	return 2;
}

gfx_api::lighting_constants gl_context::getShadowConstants()
{
	return shadowConstants;
}

bool gl_context::setShadowConstants(gfx_api::lighting_constants newValues)
{
	if (shadowConstants == newValues)
	{
		return true;
	}

	shadowConstants = newValues;

	// Must recompile any shaders that used this value
	bool patchFragmentShaderMipLodBias = true; // provide the constant to the shader directly
	for (auto& pipelineInfo : createdPipelines)
	{
		if (pipelineInfo.pso &&
			(pipelineInfo.pso->hasSpecializationConstant_ShadowConstants || pipelineInfo.pso->hasSpecializationConstants_PointLights))
		{
			delete pipelineInfo.pso;
			pipelineInfo.pso = new gl_pipeline_state_object(*this, fragmentHighpFloatAvailable, fragmentHighpIntAvailable, patchFragmentShaderMipLodBias, pipelineInfo.createInfo, mipLodBias, shadowConstants);
		}
	}

	return true;
}

bool gl_context::debugRecompileAllPipelines()
{
	bool patchFragmentShaderMipLodBias = true; // provide the constant to the shader directly
	for (auto& pipelineInfo : createdPipelines)
	{
		if (pipelineInfo.pso)
		{
			delete pipelineInfo.pso;
			pipelineInfo.pso = new gl_pipeline_state_object(*this, fragmentHighpFloatAvailable, fragmentHighpIntAvailable, patchFragmentShaderMipLodBias, pipelineInfo.createInfo, mipLodBias, shadowConstants);
		}
	}
	return true;
}

static const char *cbframebuffererror(GLenum err)
{
	switch (err)
	{
		case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
		case GL_FRAMEBUFFER_UNDEFINED: return "GL_FRAMEBUFFER_UNDEFINED";
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
#endif
		case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
		default: return "Other";
	}
}

size_t gl_context::initDepthPasses(size_t resolution)
{
	depthPassCount = std::min<size_t>(depthPassCount, WZ_MAX_SHADOW_CASCADES);

#if !defined(__EMSCRIPTEN__)
	if (depthPassCount > 1)
	{
		if ((!gles && !GLAD_GL_VERSION_3_0) || (gles && !GLAD_GL_ES_VERSION_3_0))
		{
			// glFramebufferTextureLayer requires OpenGL 3.0+ / ES 3.0+
			debug(LOG_ERROR, "Cannot create depth texture array - requires OpenGL 3.0+ / OpenGL ES 3.0+ - this will fail");
		}
	}
#endif

	// delete prior depth texture & FBOs (if present)
#if !defined(WZ_STATIC_GL_BINDINGS)
	if (glDeleteFramebuffers)
#endif
	{
		if (depthFBO.size() > 0)
		{
			glDeleteFramebuffers(static_cast<GLsizei>(depthFBO.size()), depthFBO.data());
			depthFBO.clear();
		}
	}
	if (depthTexture)
	{
		delete depthTexture;
		depthTexture = nullptr;
	}

	if (depthPassCount == 0)
	{
		return 0;
	}

	auto pNewDepthTexture = create_depthmap_texture(depthPassCount, resolution, resolution, "<depth map>");
	if (!pNewDepthTexture)
	{
		debug(LOG_ERROR, "Failed to create depth texture");
		return 0;
	}
	depthTexture = pNewDepthTexture;

	GLenum target = depthTexture->target();
	depthTexture->bind();
	glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	depthTexture->unbind();

	for (auto i = 0; i < depthPassCount; ++i)
	{
		GLuint newFBO = 0;
		glGenFramebuffers(1, &newFBO);
		depthFBO.push_back(newFBO);

		glBindFramebuffer(GL_FRAMEBUFFER, newFBO);
		if (depthTexture->isArray())
		{
#if !defined(WZ_STATIC_GL_BINDINGS)
			ASSERT(glFramebufferTextureLayer != nullptr, "glFramebufferTextureLayer is not available?");
#endif
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture->id(), 0, static_cast<GLint>(i)); // OpenGL 3.0+ / ES 3.0+ only
		}
		else
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture->id(), 0);
		}
		GLenum buf = GL_NONE;
		glDrawBuffers(1, &buf);
		glReadBuffer(GL_NONE);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			debug(LOG_ERROR, "Failed to create framebuffer with error: %s", cbframebuffererror(status));
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	return depthPassCount;
}

void gl_context::deleteSceneRenderpass()
{
	// delete prior scene texture & FBOs (if present)
#if !defined(WZ_STATIC_GL_BINDINGS)
	if (glDeleteFramebuffers)
#endif
	{
		if (sceneFBO.size() > 0)
		{
			glDeleteFramebuffers(static_cast<GLsizei>(sceneFBO.size()), sceneFBO.data());
			sceneFBO.clear();
		}
		if (sceneResolveFBO.size() > 0)
		{
			glDeleteFramebuffers(static_cast<GLsizei>(sceneResolveFBO.size()), sceneResolveFBO.data());
			sceneResolveFBO.clear();
		}
	}
	if (sceneMsaaRBO)
	{
		glDeleteRenderbuffers(1, &sceneMsaaRBO);
		sceneMsaaRBO = 0;
	}
	if (sceneTexture)
	{
		delete sceneTexture;
		sceneTexture = nullptr;
	}
	if (sceneDepthStencilRBO)
	{
		glDeleteRenderbuffers(1, &sceneDepthStencilRBO);
		sceneDepthStencilRBO = 0;
	}
}

bool gl_context::createSceneRenderpass()
{
	deleteSceneRenderpass();

#if !defined(__EMSCRIPTEN__)
	if ( ! ((!gles && GLAD_GL_VERSION_3_0) || (gles && GLAD_GL_ES_VERSION_3_0)) )
	{
		// The following requires OpenGL 3.0+ or OpenGL ES 3.0+
		debug(LOG_ERROR, "Unsupported version of OpenGL / OpenGL ES.");
		return false;
	}
#endif

	wzGLClearErrors(); // clear OpenGL error states

	bool encounteredError = false;
	GLsizei samples = std::min<GLsizei>(multisamples, maxMultiSampleBufferFormatSamples);

	if (samples > 0)
	{
		// If MSAA is enabled, use glRenderbufferStorageMultisample to create an intermediate buffer with MSAA enabled
		glGenRenderbuffers(1, &sceneMsaaRBO);
		ASSERT_GL_NOERRORS_OR_RETURN(false);
		glBindRenderbuffer(GL_RENDERBUFFER, sceneMsaaRBO);
		ASSERT_GL_NOERRORS_OR_RETURN(false);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, multiSampledBufferInternalFormat, sceneFramebufferWidth, sceneFramebufferHeight); // OpenGL 3.0+, OpenGL ES 3.0+
		ASSERT_GL_NOERRORS_OR_RETURN(false);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		ASSERT_GL_NOERRORS_OR_RETURN(false);
	}

	// Always create a standard color texture (for the resolved color values)
	// NOTE:
	// - OpenGL ES: color texture format must *MATCH* the format used for the multisampled color render buffer
	GLenum colorInternalFormat = (samples > 0 && gles) ? multiSampledBufferInternalFormat : GL_RGB8;
	GLenum colorBaseFormat = (samples > 0 && gles) ? multiSampledBufferBaseFormat : GL_RGB;
	auto pNewSceneTexture = create_framebuffer_color_texture(colorInternalFormat, colorBaseFormat, GL_UNSIGNED_BYTE, sceneFramebufferWidth, sceneFramebufferHeight, "<scene texture>");
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	if (!pNewSceneTexture)
	{
		debug(LOG_ERROR, "Failed to create scene color texture (%" PRIu32 " x %" PRIu32 ")", sceneFramebufferWidth, sceneFramebufferHeight);
		return false;
	}
	sceneTexture = pNewSceneTexture;
	sceneTexture->bind();
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	sceneTexture->unbind();
	ASSERT_GL_NOERRORS_OR_RETURN(false);

	// Create a matching depth/stencil texture
	sceneDepthStencilRBO = 0;
	glGenRenderbuffers(1, &sceneDepthStencilRBO);
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthStencilRBO);
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, sceneFramebufferWidth, sceneFramebufferHeight); // OpenGL 3.0+, OpenGL ES 3.0+
	ASSERT_GL_NOERRORS_OR_RETURN(false);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	ASSERT_GL_NOERRORS_OR_RETURN(false);

	const size_t numSceneFBOs = 2;
	for (auto i = 0; i < numSceneFBOs; ++i)
	{
		GLuint newFBO = 0;
		glGenFramebuffers(1, &newFBO);
		ASSERT_GL_NOERRORS_OR_RETURN(false);
		sceneFBO.push_back(newFBO);

		glBindFramebuffer(GL_FRAMEBUFFER, newFBO);
		ASSERT_GL_NOERRORS_OR_RETURN(false);
		if (samples > 0)
		{
			// use the MSAA renderbuffer as the color attachment
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, sceneMsaaRBO);
			ASSERT_GL_NOERRORS_OR_RETURN(false);
		}
		else
		{
			// just directly use the sceneTexture as the color attachment (since no MSAA resolving needs to occur)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture->id(), 0);
			ASSERT_GL_NOERRORS_OR_RETURN(false);
		}
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneDepthStencilRBO);
		ASSERT_GL_NOERRORS_OR_RETURN(false);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			debug(LOG_ERROR, "Failed to create scene framebuffer[%d] (%" PRIu32 " x %" PRIu32 ", samples: %d) with error: %s", i, sceneFramebufferWidth, sceneFramebufferHeight, static_cast<int>(samples), cbframebuffererror(status));
			encounteredError = true;
		}
		ASSERT_GL_NOERRORS_OR_RETURN(false);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		ASSERT_GL_NOERRORS_OR_RETURN(false);

		if (samples > 0)
		{
			// Must also create a sceneResolveFBO for resolving MSAA
			GLuint newResolveRBO = 0;
			glGenFramebuffers(1, &newResolveRBO);
			ASSERT_GL_NOERRORS_OR_RETURN(false);
			sceneResolveFBO.push_back(newResolveRBO);

			glBindFramebuffer(GL_FRAMEBUFFER, newResolveRBO);
			ASSERT_GL_NOERRORS_OR_RETURN(false);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture->id(), 0);
			ASSERT_GL_NOERRORS_OR_RETURN(false);
			// shouldn't need a depth/stencil buffer

			status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				debug(LOG_ERROR, "Failed to create scene resolve framebuffer[%d] (%" PRIu32 " x %" PRIu32 ", samples: %d) with error: %s", i, sceneFramebufferWidth, sceneFramebufferHeight, static_cast<int>(samples), cbframebuffererror(status));
				encounteredError = true;
			}
			ASSERT_GL_NOERRORS_OR_RETURN(false);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			ASSERT_GL_NOERRORS_OR_RETURN(false);
		}
	}

	ASSERT_GL_NOERRORS_OR_RETURN(false);

	return !encounteredError;
}

void gl_context::beginSceneRenderPass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO[sceneFBOIdx]);
	glViewport(0, 0, sceneFramebufferWidth, sceneFramebufferHeight);
	GLbitfield clearFlags = 0;
	glDepthMask(GL_TRUE);
	clearFlags = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	glClear(clearFlags);
}

void gl_context::endSceneRenderPass()
{
	// Performance optimization:
	//
	// Before switching the draw framebuffer, call glInvalidateFramebuffer on any parts of the scene framebuffer(s) that we can
	// NOTE: The only one we need to keep around by the end is sceneTexture, which is bound as GL_COLOR_ATTACHMENT0 on one of the FBOs
	// However: If using the sceneMsaaRBO, we have to keep that around initially, and only invalidate it after resolving with glBlitFramebuffer
	//
	// Support:
	// - OpenGL:
	//		- ARB_invalidate_subdata extension provides glInvalidateFramebuffer
	//			- NOTE: ARB_invalidate_subdata's API does *not* accept GL_DEPTH_STENCIL_ATTACHMENT
	//		- core in OpenGL 4.3+
	// - OpenGL ES:
	//		- EXT_discard_framebuffer provides glDiscardFramebufferEXT
	//			- NOTE: glDiscardFramebufferEXT does *not* accept GL_DEPTH_STENCIL_ATTACHMENT
	//			- NOTE: glDiscardFramebufferEXT *only* supports GL_FRAMEBUFFER as a target
	//		- core in OpenGL ES 3.0+

	// invalidate depth_stencil on sceneFBO[sceneFBOIdx]
	GLenum invalid_ap[2];
	if (/*(!gles && GLAD_GL_VERSION_4_3) || */ (gles && GLAD_GL_ES_VERSION_3_0))
	{
		invalid_ap[0] = GL_DEPTH_STENCIL_ATTACHMENT;
		glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, invalid_ap);
	}
#if !defined(__EMSCRIPTEN__)
	else
	{
		invalid_ap[0] = GL_DEPTH_ATTACHMENT;
		invalid_ap[1] = GL_STENCIL_ATTACHMENT;
		if (!gles && GLAD_GL_ARB_invalidate_subdata)
		{
		#if !defined(WZ_STATIC_GL_BINDINGS)
			if (glInvalidateFramebuffer)
		#endif
			{
				glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, invalid_ap);
			}
		}
		else if (gles && GLAD_GL_EXT_discard_framebuffer)
		{
		#if !defined(WZ_STATIC_GL_BINDINGS)
			if (glDiscardFramebufferEXT)
		#endif
			{
				glDiscardFramebufferEXT(GL_FRAMEBUFFER, 2, invalid_ap);
			}
		}
	}
#endif

	// If MSAA is enabled, use glBiltFramebuffer from the intermediate MSAA-enabled renderbuffer storage to a standard texture (resolving MSAA)
	bool usingMSAAIntermediate = (sceneMsaaRBO != 0);
	if (usingMSAAIntermediate)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFBO[sceneFBOIdx]);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sceneResolveFBO[sceneFBOIdx]);
		glBlitFramebuffer(0,0, sceneFramebufferWidth, sceneFramebufferHeight,
						  0,0, sceneFramebufferWidth, sceneFramebufferHeight,
						  GL_COLOR_BUFFER_BIT,
						  GL_LINEAR);
	}

	// after this, sceneTexture should be the (msaa-resolved) color texture of the scene

	if (usingMSAAIntermediate)
	{
		// invalidate color0 (sceneMsaaRBO) in sceneFBO[sceneFBOIdx] (which is GL_READ_FRAMEBUFFER at this point)
		GLenum invalid_msaarbo_ap[1];
		invalid_msaarbo_ap[0] = GL_COLOR_ATTACHMENT0;
		if (/*(!gles && GLAD_GL_VERSION_4_3) || */ (gles && GLAD_GL_ES_VERSION_3_0))
		{
			glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 1, invalid_msaarbo_ap);
		}
		else
		{
#if defined(GL_ARB_invalidate_subdata)
			if (!gles && GLAD_GL_ARB_invalidate_subdata && glInvalidateFramebuffer)
			{
				glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 1, invalid_msaarbo_ap);
			}
#endif
//			else if (gles && GLAD_GL_EXT_discard_framebuffer && glDiscardFramebufferEXT)
//			{
//				// NOTE: glDiscardFramebufferEXT only supports GL_FRAMEBUFFER... but that doesn't work here
//				glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, invalid_msaarbo_ap);
//			}
		}
	}

	sceneFBOIdx++;
	if (sceneFBOIdx >= sceneFBO.size())
	{
		sceneFBOIdx = 0;
	}

	// switch back to default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Because the scene uses the same viewport, a call to glViewport should not be needed (NOTE: viewport is *not* part of the framebuffer state)
	if (sceneFramebufferWidth != viewportWidth || sceneFramebufferHeight != viewportHeight)
	{
		glViewport(0, 0, viewportWidth, viewportHeight);
	}
}

gfx_api::abstract_texture* gl_context::getSceneTexture()
{
	return sceneTexture;
}

#if defined(__EMSCRIPTEN__)

static std::vector<std::string> getEmscriptenSupportedGLExtensions()
{
	// Use the GL_NUM_EXTENSIONS / GL_EXTENSIONS implementation to get the list of extensions that *Emscripten* natively supports
	// This may be a subset of all the extensions that the browser supports
	// (WebGL extensions must be enabled to be available)
	auto extensions = wzGLGetStringi_GL_EXTENSIONS_Impl();

	// Remove the "GL_" prefix that Emscripten may add
	const std::string GL_prefix = "GL_";
	for (auto& extensionStr : extensions)
	{
		if (extensionStr.rfind(GL_prefix, 0) == 0)
		{
			extensionStr = extensionStr.substr(GL_prefix.length());
		}
	}

	return extensions;
}

static bool initWebGLExtensions()
{
	// Get list of _supported_ WebGL extensions (which may be a superset of the Emscripten-default-enabled ones)
	std::unordered_set<std::string> supportedWebGLExtensions;
	char* spaceSeparatedExtensions = emscripten_webgl_get_supported_extensions();
	if (!spaceSeparatedExtensions)
	{
		return false;
	}

	debug(LOG_INFO, "Supported WebGL extensions: %s", spaceSeparatedExtensions);

	std::vector<std::string> strings;
	std::istringstream str_stream(spaceSeparatedExtensions);
	std::string s;
	while (getline(str_stream, s, ' ')) {
		supportedWebGLExtensions.insert(s);
	}

	free(spaceSeparatedExtensions);

	// Get the list of Emscripten-enabled / default-supported WebGL extensions
	auto glExtensionsResult = getEmscriptenSupportedGLExtensions();
	std::unordered_set<std::string> emscriptenSupportedExtensionsList(glExtensionsResult.begin(), glExtensionsResult.end());

	// Enable WebGL extensions
	// NOTE: It is possible to compile for Emscripten without auto-enabling of the default set of extensions
	// So we *MUST* always call emscripten_webgl_enable_extension() for each desired extension
	enabledWebGLExtensions.clear();
	auto ctx = emscripten_webgl_get_current_context();
	auto tryEnableWebGLExtension = [&](const char* extensionName, bool bypassEmscriptenSupportedCheck = false) {
		if (supportedWebGLExtensions.count(extensionName) == 0)
		{
			debug(LOG_3D, "Extension not available: %s", extensionName);
			return;
		}
		if (!bypassEmscriptenSupportedCheck && emscriptenSupportedExtensionsList.count(extensionName) == 0)
		{
			debug(LOG_3D, "Skipping due to lack of Emscripten-advertised support: %s", extensionName);
			return;
		}
		if (!emscripten_webgl_enable_extension(ctx, extensionName))
		{
			debug(LOG_3D, "Failed to enable extension: %s", extensionName);
			return;
		}

		debug(LOG_3D, "Enabled extension: %s", extensionName);
		enabledWebGLExtensions.insert(extensionName);
	};

	// NOTE: WebGL 2 includes some features by default (that used to be in extensions)
	// See: https://webgl2fundamentals.org/webgl/lessons/webgl1-to-webgl2.html#features-you-can-take-for-granted

	// general
	tryEnableWebGLExtension("EXT_color_buffer_half_float");
	tryEnableWebGLExtension("EXT_color_buffer_float");
	tryEnableWebGLExtension("EXT_float_blend");
	tryEnableWebGLExtension("EXT_texture_filter_anisotropic", true);
	tryEnableWebGLExtension("OES_texture_float_linear");
	tryEnableWebGLExtension("WEBGL_blend_func_extended");

	// compressed texture format extensions
	tryEnableWebGLExtension("WEBGL_compressed_texture_s3tc", true);
	tryEnableWebGLExtension("WEBGL_compressed_texture_s3tc_srgb", true);
	tryEnableWebGLExtension("EXT_texture_compression_rgtc", true);
	tryEnableWebGLExtension("EXT_texture_compression_bptc", true);
	tryEnableWebGLExtension("WEBGL_compressed_texture_etc", true);
	tryEnableWebGLExtension("WEBGL_compressed_texture_astc", true);

	return true;
}
#endif
