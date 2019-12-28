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
#include "screen.h"
#include "gfx_api_gl.h"
#include "lib/exceptionhandler/dumpinfo.h"
#include "lib/framework/physfs_ext.h"
#include "piemode.h"

#include <vector>
#include <string>
#include <algorithm>

#include <glm/gtc/type_ptr.hpp>

#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP 0x8191
#endif

struct OPENGL_DATA
{
	char vendor[256];
	char renderer[256];
	char version[256];
	char GLSLversion[256];
};
OPENGL_DATA opengl;

static GLuint perfpos[PERF_COUNT];
static bool perfStarted = false;

static std::pair<GLenum, GLenum> to_gl(const gfx_api::pixel_format& format)
{
	switch (format)
	{
		case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
			return std::make_pair(GL_RGBA8, GL_RGBA);
		case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
			return std::make_pair(GL_RGBA8, GL_BGRA);
		case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
			return std::make_pair(GL_RGB8, GL_RGB);
		default:
			debug(LOG_FATAL, "Unrecognised pixel format");
	}
	return std::make_pair(GL_INVALID_ENUM, GL_INVALID_ENUM);
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
		default:
			debug(LOG_FATAL, "Unrecognised property type");
	}
	return GL_INVALID_ENUM;
}

// MARK: gl_texture

gl_texture::gl_texture()
{
	glGenTextures(1, &_id);
}

gl_texture::~gl_texture()
{
	glDeleteTextures(1, &_id);
}

void gl_texture::bind()
{
	glBindTexture(GL_TEXTURE_2D, _id);
}

void gl_texture::unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void gl_texture::upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t & width, const size_t & height, const gfx_api::pixel_format & buffer_format, const void * data)
{
	ASSERT(width > 0 && height > 0, "Attempt to upload texture with width or height of 0 (width: %zu, height: %zu)", width, height);
	bind();
	glTexSubImage2D(GL_TEXTURE_2D, mip_level, offset_x, offset_y, width, height, std::get<1>(to_gl(buffer_format)), GL_UNSIGNED_BYTE, data);
	unbind();
}

void gl_texture::upload_and_generate_mipmaps(const size_t& offset_x, const size_t& offset_y, const size_t& width, const size_t& height, const  gfx_api::pixel_format& buffer_format, const void* data)
{
	bind();
	if(!glGenerateMipmap)
	{
		// fallback for if glGenerateMipmap is unavailable
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	}
	glTexSubImage2D(GL_TEXTURE_2D, 0, offset_x, offset_y, width, height, std::get<1>(to_gl(buffer_format)), GL_UNSIGNED_BYTE, data);
	if(glGenerateMipmap)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	unbind();
}

unsigned gl_texture::id()
{
	return _id;
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
	ASSERT(lastUploaded_FrameNum != current_FrameNum, "Attempt to upload to buffer more than once per frame");
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
	ASSERT(flag == update_flag::non_overlapping_updates_promise || (lastUploaded_FrameNum != current_FrameNum), "Attempt to upload to buffer more than once per frame");
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

// MARK: gl_pipeline_state_object

struct program_data
{
	std::string friendly_name;
	std::string vertex_file;
	std::string fragment_file;
	std::vector<std::string> uniform_names;
};

static const std::map<SHADER_MODE, program_data> shader_to_file_table =
{
	std::make_pair(SHADER_COMPONENT, program_data{ "Component program", "shaders/tcmask.vert", "shaders/tcmask.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
			"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
			"fogEnd", "fogStart", "fogColor" } }),
	std::make_pair(SHADER_BUTTON, program_data{ "Button program", "shaders/button.vert", "shaders/button.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
			"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
			"fogEnd", "fogStart", "fogColor" } }),
	std::make_pair(SHADER_NOLIGHT, program_data{ "Plain program", "shaders/nolight.vert", "shaders/nolight.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
			"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
			"fogEnd", "fogStart", "fogColor" } }),
	std::make_pair(SHADER_TERRAIN, program_data{ "terrain program", "shaders/terrain_water.vert", "shaders/terrain.frag",
		{ "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "tex", "lightmap_tex", "textureMatrix1", "textureMatrix2",
			"fogEnabled", "fogEnd", "fogStart", "fogColor" } }),
	std::make_pair(SHADER_TERRAIN_DEPTH, program_data{ "terrain_depth program", "shaders/terrain_water.vert", "shaders/terraindepth.frag",
		{ "ModelViewProjectionMatrix", "paramx2", "paramy2", "lightmap_tex", "paramx2", "paramy2" } }),
	std::make_pair(SHADER_DECALS, program_data{ "decals program", "shaders/decals.vert", "shaders/decals.frag",
		{ "ModelViewProjectionMatrix", "paramxlight", "paramylight", "lightTextureMatrix",
			"fogEnabled", "fogEnd", "fogStart", "fogColor", "tex", "lightmap_tex" } }),
	std::make_pair(SHADER_WATER, program_data{ "water program", "shaders/terrain_water.vert", "shaders/water.frag",
		{ "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "tex1", "tex2", "textureMatrix1", "textureMatrix2",
			"fogEnabled", "fogEnd", "fogStart" } }),
	std::make_pair(SHADER_RECT, program_data{ "Rect program", "shaders/rect.vert", "shaders/rect.frag",
		{ "transformationMatrix", "color" } }),
	std::make_pair(SHADER_TEXRECT, program_data{ "Textured rect program", "shaders/rect.vert", "shaders/texturedrect.frag",
		{ "transformationMatrix", "tuv_offset", "tuv_scale", "color", "texture" } }),
	std::make_pair(SHADER_GFX_COLOUR, program_data{ "gfx_color program", "shaders/gfx.vert", "shaders/gfx.frag",
		{ "posMatrix" } }),
	std::make_pair(SHADER_GFX_TEXT, program_data{ "gfx_text program", "shaders/gfx.vert", "shaders/texturedrect.frag",
		{ "posMatrix", "color", "texture" } }),
	std::make_pair(SHADER_GENERIC_COLOR, program_data{ "generic color program", "shaders/generic.vert", "shaders/rect.frag",{ "ModelViewProjectionMatrix", "color" } }),
	std::make_pair(SHADER_LINE, program_data{ "line program", "shaders/line.vert", "shaders/rect.frag",{ "from", "to", "color", "ModelViewProjectionMatrix" } }),
	std::make_pair(SHADER_TEXT, program_data{ "Text program", "shaders/rect.vert", "shaders/text.frag",
		{ "transformationMatrix", "tuv_offset", "tuv_scale", "color", "texture" } })
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
	while(glGetError() != GL_NO_ERROR) { } // clear the OpenGL error queue
	glGetIntegerv(pname, &retVal);
	GLenum err = glGetError();
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
typename std::pair<SHADER_MODE, std::function<void(const void*)>> gl_pipeline_state_object::uniform_binding_entry()
{
	return std::make_pair(shader, [this](const void* buffer) { this->set_constants(*reinterpret_cast<const gfx_api::constant_buffer_type<shader>*>(buffer)); });
}

gl_pipeline_state_object::gl_pipeline_state_object(bool gles, const gfx_api::state_description& _desc, const SHADER_MODE& shader, const std::vector<gfx_api::vertex_buffer>& _vertex_buffer_desc) :
desc(_desc), vertex_buffer_desc(_vertex_buffer_desc)
{
	std::string vertexShaderHeader;
	std::string fragmentShaderHeader;

	if (!gles)
	{
		// Determine the shader version directive we should use by examining the current OpenGL context
		// (The built-in shaders support (and have been tested with) VERSION_120 and VERSION_150_CORE)
		const char *shaderVersionStr = shaderVersionString(getMaximumShaderVersionForCurrentGLContext(VERSION_120, VERSION_150_CORE));

		vertexShaderHeader = shaderVersionStr;
		fragmentShaderHeader = shaderVersionStr;
	}
	else
	{
		// Determine the shader version directive we should use by examining the current OpenGL ES context
		// (The built-in shaders support (and have been tested with) VERSION_ES_100 and VERSION_ES_300)
		const char *shaderVersionStr = shaderVersionString(getMaximumShaderVersionForCurrentGLESContext(VERSION_ES_100, VERSION_ES_300));

		vertexShaderHeader = shaderVersionStr;
		// OpenGL ES Shading Language - 4. Variables and Types - pp. 35-36
		// https://www.khronos.org/registry/gles/specs/2.0/GLSL_ES_Specification_1.0.17.pdf?#page=41
		// 
		// > The fragment language has no default precision qualifier for floating point types.
		// > Hence for float, floating point vector and matrix variable declarations, either the
		// > declaration must include a precision qualifier or the default float precision must
		// > have been previously declared.
		fragmentShaderHeader = std::string(shaderVersionStr) + "#if GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\n";
	}

	build_program(
				  shader_to_file_table.at(shader).friendly_name,
				  vertexShaderHeader.c_str(),
				  shader_to_file_table.at(shader).vertex_file,
				  fragmentShaderHeader.c_str(),
				  shader_to_file_table.at(shader).fragment_file,
				  shader_to_file_table.at(shader).uniform_names);

	const std::map < SHADER_MODE, std::function<void(const void*)>> uniforms_bind_table =
	{
		uniform_binding_entry<SHADER_COMPONENT>(),
		uniform_binding_entry<SHADER_BUTTON>(),
		uniform_binding_entry<SHADER_NOLIGHT>(),
		uniform_binding_entry<SHADER_TERRAIN>(),
		uniform_binding_entry<SHADER_TERRAIN_DEPTH>(),
		uniform_binding_entry<SHADER_DECALS>(),
		uniform_binding_entry<SHADER_WATER>(),
		uniform_binding_entry<SHADER_RECT>(),
		uniform_binding_entry<SHADER_TEXRECT>(),
		uniform_binding_entry<SHADER_GFX_COLOUR>(),
		uniform_binding_entry<SHADER_GFX_TEXT>(),
		uniform_binding_entry<SHADER_GENERIC_COLOR>(),
		uniform_binding_entry<SHADER_LINE>(),
		uniform_binding_entry<SHADER_TEXT>()
	};

	uniform_bind_function = uniforms_bind_table.at(shader);
}

void gl_pipeline_state_object::set_constants(const void* buffer)
{
	uniform_bind_function(buffer);
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
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
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
			if (GLAD_GL_VERSION_2_0 || GLAD_GL_ES_VERSION_2_0)
			{
				glStencilMask(~0);
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
				glStencilFunc(GL_ALWAYS, 0, ~0);
			}
			else if (GLAD_GL_EXT_stencil_two_side)
			{
				glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
				glStencilMask(~0);
				glActiveStencilFaceEXT(GL_BACK);
				glStencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP);
				glStencilFunc(GL_ALWAYS, 0, ~0);
				glActiveStencilFaceEXT(GL_FRONT);
				glStencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP);
				glStencilFunc(GL_ALWAYS, 0, ~0);
			}
			else if (GLAD_GL_ATI_separate_stencil)
			{
				glStencilMask(~0);
				glStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
				glStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
				glStencilFunc(GL_ALWAYS, 0, ~0);
			}

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
			break;
		case gfx_api::cull_mode::none:
			glDisable(GL_CULL_FACE);
			break;
	}
}

// Read shader into text buffer
char *gl_pipeline_state_object::readShaderBuf(const std::string& name)
{
	PHYSFS_file	*fp;
	int	filesize;
	char *buffer;

	fp = PHYSFS_openRead(name.c_str());
	debug(LOG_3D, "Reading...[directory: %s] %s", PHYSFS_getRealDir(name.c_str()), name.c_str());
	ASSERT_OR_RETURN(nullptr, fp != nullptr, "Could not open %s", name.c_str());
	filesize = PHYSFS_fileLength(fp);
	buffer = (char *)malloc(filesize + 1);
	if (buffer)
	{
		WZ_PHYSFS_readBytes(fp, buffer, filesize);
		buffer[filesize] = '\0';
	}
	PHYSFS_close(fp);

	return buffer;
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

void gl_pipeline_state_object::getLocs()
{
	glUseProgram(program);

	// Uniforms, these never change.
	GLint locTex0 = glGetUniformLocation(program, "Texture");
	GLint locTex1 = glGetUniformLocation(program, "TextureTcmask");
	GLint locTex2 = glGetUniformLocation(program, "TextureNormal");
	GLint locTex3 = glGetUniformLocation(program, "TextureSpecular");
	glUniform1i(locTex0, 0);
	glUniform1i(locTex1, 1);
	glUniform1i(locTex2, 2);
	glUniform1i(locTex3, 3);
}

void gl_pipeline_state_object::build_program(const std::string& programName,
											 const char * vertex_header, const std::string& vertexPath,
											 const char * fragment_header, const std::string& fragmentPath,
											 const std::vector<std::string> &uniformNames)
{
	GLint status;
	bool success = true; // Assume overall success

	program = glCreateProgram();
	glBindAttribLocation(program, 0, "vertex");
	glBindAttribLocation(program, 1, "vertexTexCoord");
	glBindAttribLocation(program, 2, "vertexColor");
	glBindAttribLocation(program, 3, "vertexNormal");
	ASSERT_OR_RETURN(, program, "Could not create shader program!");

	char* shaderContents = nullptr;

	if (!vertexPath.empty())
	{
		success = false; // Assume failure before reading shader file

		if ((shaderContents = readShaderBuf(vertexPath)))
		{
			GLuint shader = glCreateShader(GL_VERTEX_SHADER);

			const char* ShaderStrings[2] = { vertex_header, shaderContents };

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
			if ((/*GLEW_VERSION_4_3 ||*/ GLAD_GL_KHR_debug) && glObjectLabel)
			{
				glObjectLabel(GL_SHADER, shader, -1, vertexPath.c_str());
			}
			free(shaderContents);
			shaderContents = nullptr;
		}
	}


	if (success && !fragmentPath.empty())
	{
		success = false; // Assume failure before reading shader file

		if ((shaderContents = readShaderBuf(fragmentPath)))
		{
			GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

			const char* ShaderStrings[2] = { fragment_header, shaderContents };

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
			if ((/*GLEW_VERSION_4_3 ||*/ GLAD_GL_KHR_debug) && glObjectLabel)
			{
				glObjectLabel(GL_SHADER, shader, -1, fragmentPath.c_str());
			}
			free(shaderContents);
			shaderContents = nullptr;
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
		if ((/*GLEW_VERSION_4_3 ||*/ GLAD_GL_KHR_debug) && glObjectLabel)
		{
			glObjectLabel(GL_PROGRAM, program, -1, programName.c_str());
		}
	}
	fetch_uniforms(uniformNames);
	getLocs();
}

void gl_pipeline_state_object::fetch_uniforms(const std::vector<std::string>& uniformNames)
{
	std::transform(uniformNames.begin(), uniformNames.end(),
				   std::back_inserter(locations),
				   [&](const std::string& name) { return glGetUniformLocation(program, name.data()); });
}

void gl_pipeline_state_object::setUniforms(GLint location, const ::glm::vec4 &v)
{
	glUniform4f(location, v.x, v.y, v.z, v.w);
}

void gl_pipeline_state_object::setUniforms(GLint location, const ::glm::mat4 &m)
{
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
}

void gl_pipeline_state_object::setUniforms(GLint location, const Vector2i &v)
{
	glUniform2i(location, v.x, v.y);
}

void gl_pipeline_state_object::setUniforms(GLint location, const Vector2f &v)
{
	glUniform2f(location, v.x, v.y);
}

void gl_pipeline_state_object::setUniforms(GLint location, const int32_t &v)
{
	glUniform1i(location, v);
}

void gl_pipeline_state_object::setUniforms(GLint location, const float &v)
{
	glUniform1f(location, v);
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

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_BUTTON>& cbuf)
{
	set_constants_for_component(cbuf);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_COMPONENT>& cbuf)
{
	set_constants_for_component(cbuf);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_NOLIGHT>& cbuf)
{
	set_constants_for_component(cbuf);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_TERRAIN>& cbuf)
{
	setUniforms(locations[0], cbuf.transform_matrix);
	setUniforms(locations[1], cbuf.paramX);
	setUniforms(locations[2], cbuf.paramY);
	setUniforms(locations[3], cbuf.paramXLight);
	setUniforms(locations[4], cbuf.paramYLight);
	setUniforms(locations[5], cbuf.texture0);
	setUniforms(locations[6], cbuf.texture1);
	setUniforms(locations[7], cbuf.unused);
	setUniforms(locations[8], cbuf.texture_matrix);
	setUniforms(locations[9], cbuf.fog_enabled);
	setUniforms(locations[10], cbuf.fog_begin);
	setUniforms(locations[11], cbuf.fog_end);
	setUniforms(locations[12], cbuf.fog_colour);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_TERRAIN_DEPTH>& cbuf)
{
	setUniforms(locations[0], cbuf.transform_matrix);
	setUniforms(locations[1], cbuf.paramX);
	setUniforms(locations[2], cbuf.paramY);
	setUniforms(locations[3], cbuf.texture0);
	setUniforms(locations[4], cbuf.paramXLight);
	setUniforms(locations[5], cbuf.paramYLight);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_DECALS>& cbuf)
{
	setUniforms(locations[0], cbuf.transform_matrix);
	setUniforms(locations[1], cbuf.param1);
	setUniforms(locations[2], cbuf.param2);
	setUniforms(locations[3], cbuf.texture_matrix);
	setUniforms(locations[4], cbuf.fog_enabled);
	setUniforms(locations[5], cbuf.fog_begin);
	setUniforms(locations[6], cbuf.fog_end);
	setUniforms(locations[7], cbuf.fog_colour);
	setUniforms(locations[8], cbuf.texture0);
	setUniforms(locations[9], cbuf.texture1);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_WATER>& cbuf)
{
	setUniforms(locations[0], cbuf.transform_matrix);
	setUniforms(locations[1], cbuf.param1);
	setUniforms(locations[2], cbuf.param2);
	setUniforms(locations[3], cbuf.param3);
	setUniforms(locations[4], cbuf.param4);
	setUniforms(locations[5], cbuf.texture0);
	setUniforms(locations[6], cbuf.texture1);
	setUniforms(locations[7], cbuf.translation);
	setUniforms(locations[8], cbuf.texture_matrix);
	setUniforms(locations[9], cbuf.fog_enabled);
	setUniforms(locations[10], cbuf.fog_begin);
	setUniforms(locations[11], cbuf.fog_end);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_RECT>& cbuf)
{
	setUniforms(locations[0], cbuf.transform_matrix);
	setUniforms(locations[1], cbuf.colour);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_TEXRECT>& cbuf)
{
	setUniforms(locations[0], cbuf.transform_matrix);
	setUniforms(locations[1], cbuf.offset);
	setUniforms(locations[2], cbuf.size);
	setUniforms(locations[3], cbuf.color);
	setUniforms(locations[4], cbuf.texture);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_GFX_COLOUR>& cbuf)
{
	setUniforms(locations[0], cbuf.transform_matrix);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_GFX_TEXT>& cbuf)
{
	setUniforms(locations[0], cbuf.transform_matrix);
	setUniforms(locations[1], cbuf.color);
	setUniforms(locations[2], cbuf.texture);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_GENERIC_COLOR>& cbuf)
{
	setUniforms(locations[0], cbuf.transform_matrix);
	setUniforms(locations[1], cbuf.colour);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_LINE>& cbuf)
{
	setUniforms(locations[0], cbuf.p0);
	setUniforms(locations[1], cbuf.p1);
	setUniforms(locations[2], cbuf.colour);
	setUniforms(locations[3], cbuf.mat);
}

void gl_pipeline_state_object::set_constants(const gfx_api::constant_buffer_type<SHADER_TEXT>& cbuf)
{
	setUniforms(locations[0], cbuf.transform_matrix);
	setUniforms(locations[1], cbuf.offset);
	setUniforms(locations[2], cbuf.size);
	setUniforms(locations[3], cbuf.color);
	setUniforms(locations[4], cbuf.texture);
}

size_t get_size(const gfx_api::vertex_attribute_type& type)
{
	switch (type)
	{
		case gfx_api::vertex_attribute_type::float2:
			return 2;
		case gfx_api::vertex_attribute_type::float3:
			return 3;
		case gfx_api::vertex_attribute_type::float4:
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
		case gfx_api::vertex_attribute_type::u8x4_norm:
			return GL_UNSIGNED_BYTE;
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
	if (glDeleteBuffers) // glDeleteBuffers might be NULL (if initializing the OpenGL loader library fails)
	{
		glDeleteBuffers(1, &scratchbuffer);
	}
}

gfx_api::texture* gl_context::create_texture(const size_t& mipmap_count, const size_t & width, const size_t & height, const gfx_api::pixel_format & internal_format, const std::string& filename)
{
	auto* new_texture = new gl_texture();
	new_texture->mip_count = mipmap_count;
	new_texture->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmap_count - 1);
	if (!filename.empty() && ((/*GLEW_VERSION_4_3 ||*/ GLAD_GL_KHR_debug) && glObjectLabel))
	{
		glObjectLabel(GL_TEXTURE, new_texture->id(), -1, filename.c_str());
	}
	for (unsigned i = 0, e = mipmap_count; i < e; ++i)
	{
		glTexImage2D(GL_TEXTURE_2D, i, std::get<0>(to_gl(internal_format)), width >> i, height >> i, 0, std::get<1>(to_gl(internal_format)), GL_UNSIGNED_BYTE, nullptr);
	}
	return new_texture;
}

gfx_api::buffer * gl_context::create_buffer_object(const gfx_api::buffer::usage &usage, const buffer_storage_hint& hint /*= buffer_storage_hint::static_draw*/)
{
	return new gl_buffer(usage, hint);
}

gfx_api::pipeline_state_object * gl_context::build_pipeline(const gfx_api::state_description &state_desc,
															const SHADER_MODE& shader_mode,
															const gfx_api::primitive_type& primitive,
															const std::vector<gfx_api::texture_input>& texture_desc,
															const std::vector<gfx_api::vertex_buffer>& attribute_descriptions)
{
	return new gl_pipeline_state_object(gles, state_desc, shader_mode, attribute_descriptions);
}

void gl_context::bind_pipeline(gfx_api::pipeline_state_object* pso, bool notextures)
{
	gl_pipeline_state_object* new_program = static_cast<gl_pipeline_state_object*>(pso);
	if (current_program != new_program)
	{
		current_program = new_program;
		current_program->bind();
		if (notextures)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
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
	for (size_t i = 0, e = vertex_buffers_offset.size(); i < e && (first + i) < current_program->vertex_buffer_desc.size(); ++i)
	{
		const auto& buffer_desc = current_program->vertex_buffer_desc[first + i];
		auto* buffer = static_cast<gl_buffer*>(std::get<0>(vertex_buffers_offset[i]));
		ASSERT(buffer->usage == gfx_api::buffer::usage::vertex_buffer, "bind_vertex_buffers called with non-vertex-buffer");
		buffer->bind();
		for (const auto& attribute : buffer_desc.attributes)
		{
			enableVertexAttribArray(attribute.id);
			glVertexAttribPointer(attribute.id, get_size(attribute.type), get_type(attribute.type), get_normalisation(attribute.type), buffer_desc.stride, reinterpret_cast<void*>(attribute.offset + std::get<1>(vertex_buffers_offset[i])));
		}
	}
}

void gl_context::unbind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset)
{
	for (size_t i = 0, e = vertex_buffers_offset.size(); i < e && (first + i) < current_program->vertex_buffer_desc.size(); ++i)
	{
		const auto& buffer_desc = current_program->vertex_buffer_desc[first + i];
		for (const auto& attribute : buffer_desc.attributes)
		{
			disableVertexAttribArray(attribute.id);
		}
	}
	glBindBuffer(to_gl(gfx_api::buffer::usage::vertex_buffer), 0);
}

void gl_context::disable_all_vertex_buffers()
{
	for (size_t index = 0; index < enabledVertexAttribIndexes.size(); ++index)
	{
		if(enabledVertexAttribIndexes[index])
		{
			disableVertexAttribArray(index);
		}
	}
	glBindBuffer(to_gl(gfx_api::buffer::usage::vertex_buffer), 0);
}

void gl_context::bind_streamed_vertex_buffers(const void* data, const std::size_t size)
{
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
		enableVertexAttribArray(attribute.id);
		glVertexAttribPointer(attribute.id, get_size(attribute.type), get_type(attribute.type), get_normalisation(attribute.type), buffer_desc.stride, nullptr);
	}
}

void gl_context::bind_index_buffer(gfx_api::buffer& _buffer, const gfx_api::index_type&)
{
	auto& buffer = static_cast<gl_buffer&>(_buffer);
	ASSERT(buffer.usage == gfx_api::buffer::usage::index_buffer, "Passed gfx_api::buffer is not an index buffer");
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.buffer);
}

void gl_context::unbind_index_buffer(gfx_api::buffer&)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void gl_context::bind_textures(const std::vector<gfx_api::texture_input>& texture_descriptions, const std::vector<gfx_api::texture*>& textures)
{
	ASSERT(textures.size() <= texture_descriptions.size(), "Received more textures than expected");
	for (size_t i = 0; i < texture_descriptions.size() && i < textures.size(); ++i)
	{
		const auto& desc = texture_descriptions[i];
		glActiveTexture(GL_TEXTURE0 + desc.id);
		if (textures[i] == nullptr)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			continue;
		}
		textures[i]->bind();
		switch (desc.sampler)
		{
			case gfx_api::sampler_type::nearest_clamped:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			case gfx_api::sampler_type::bilinear:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			case gfx_api::sampler_type::bilinear_repeat:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				break;
			case gfx_api::sampler_type::anisotropic_repeat:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				if (GLAD_GL_EXT_texture_filter_anisotropic)
				{
					GLfloat max;
					glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MIN(4.0f, max));
				}
				break;
			case gfx_api::sampler_type::anisotropic:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				if (GLAD_GL_EXT_texture_filter_anisotropic)
				{
					GLfloat max;
					glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MIN(4.0f, max));
				}
				break;
		}
	}
}

void gl_context::set_constants(const void* buffer, const size_t& size)
{
	current_program->set_constants(buffer);
}

void gl_context::draw(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive)
{
	glDrawArrays(to_gl(primitive), offset, count);
}

void gl_context::draw_elements(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive, const gfx_api::index_type& index)
{
	glDrawElements(to_gl(primitive), count, to_gl(index), reinterpret_cast<void*>(offset));
}

void gl_context::set_polygon_offset(const float& offset, const float& slope)
{
	glPolygonOffset(offset, slope);
}

void gl_context::set_depth_range(const float& min, const float& max)
{
	if (!gles)
	{
		glDepthRange(min, max);
	}
	else
	{
		glDepthRangef(min, max);
	}
}

int32_t gl_context::get_context_value(const context_value property)
{
	GLint value;
	glGetIntegerv(to_gl(property), &value);
	return value;
}

// MARK: gl_context - debug

void gl_context::debugStringMarker(const char *str)
{
	if (GLAD_GL_GREMEDY_string_marker)
	{
		glStringMarkerGREMEDY(0, str);
	}
}

void gl_context::debugSceneBegin(const char *descr)
{
	if (khr_debug && glPushDebugGroup)
	{
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, descr);
	}
}

void gl_context::debugSceneEnd(const char *descr)
{
	if (khr_debug && glPopDebugGroup)
	{
		glPopDebugGroup();
	}
}

bool gl_context::debugPerfAvailable()
{
	return GLAD_GL_ARB_timer_query;
}

bool gl_context::debugPerfStart(size_t sample)
{
	if (GLAD_GL_ARB_timer_query)
	{
		char text[80];
		ssprintf(text, "Starting performance sample %02d", sample);
		debugStringMarker(text);
		perfStarted = true;
		return true;
	}
	return false;
}

void gl_context::debugPerfStop()
{
	perfStarted = false;
}

void gl_context::debugPerfBegin(PERF_POINT pp, const char *descr)
{
	if (khr_debug && glPushDebugGroup)
	{
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, pp, -1, descr);
	}
	if (!perfStarted)
	{
		return;
	}
	glBeginQuery(GL_TIME_ELAPSED, perfpos[pp]);
}

void gl_context::debugPerfEnd(PERF_POINT pp)
{
	if (khr_debug && glPopDebugGroup)
	{
		glPopDebugGroup();
	}
	if (!perfStarted)
	{
		return;
	}
	glEndQuery(GL_TIME_ELAPSED);
}

uint64_t gl_context::debugGetPerfValue(PERF_POINT pp)
{
	GLuint64 count;
	glGetQueryObjectui64v(perfpos[pp], GL_QUERY_RESULT, &count);
	return count;
}

std::map<std::string, std::string> gl_context::getBackendGameInfo()
{
	std::map<std::string, std::string> backendGameInfo;
	backendGameInfo["openGL_vendor"] = opengl.vendor;
	backendGameInfo["openGL_renderer"] = opengl.renderer;
	backendGameInfo["openGL_version"] = opengl.version;
	backendGameInfo["openGL_GLSL_version"] = opengl.GLSLversion;
	// NOTE: deprecated for GL 3+. Needed this to check what extensions some chipsets support for the openGL hacks
	std::string extensions = (const char *) glGetString(GL_EXTENSIONS);
	backendGameInfo["GL_EXTENSIONS"] = extensions;
	return backendGameInfo;
}

static const unsigned int channelsPerPixel = 3;

bool gl_context::getScreenshot(iV_Image &image)
{
	// IMPORTANT: Must get the size of the viewport directly from the viewport, to account for
	//            high-DPI / display scaling factors (or only a sub-rect of the full viewport
	//            will be captured, as the logical screenWidth/Height may differ from the
	//            underlying viewport pixel dimensions).
	GLint m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	image.width = m_viewport[2];
	image.height = m_viewport[3];
	image.depth = 8;
	image.bmp = (unsigned char *)malloc((size_t)channelsPerPixel * (size_t)image.width * (size_t)image.height);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.bmp);

	return true;
}

//

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
	debug(LOG_ERROR, "GL::%s(%s:%s) : %s", cbsource(source), cbtype(type), cbseverity(severity), message);
}

bool gl_context::_initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing)
{
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
		return false;
	}

	int width, height = 0;
	backend_impl->getDrawableSize(&width, &height);
	debug(LOG_WZ, "Drawable Size: %d x %d", width, height);

	glViewport(0, 0, width, height);
	glCullFace(GL_FRONT);
	//	glEnable(GL_CULL_FACE);

	return true;
}

bool gl_context::initGLContext()
{
	frameNum = 1;

	GLADloadproc func_GLGetProcAddress = backend_impl->getGLGetProcAddress();
	if (!func_GLGetProcAddress)
	{
		debug(LOG_FATAL, "backend_impl->getGLGetProcAddress() returned NULL");
		exit(1);
	}
	gles = backend_impl->isOpenGLES();
	if (!gles)
	{
		if (!gladLoadGLLoader(func_GLGetProcAddress))
		{
			debug(LOG_FATAL, "gladLoadGLLoader failed");
			exit(1);
		}
	}
	else
	{
		if (!gladLoadGLES2Loader(func_GLGetProcAddress))
		{
			debug(LOG_FATAL, "gladLoadGLLoader failed");
			exit(1);
		}
	}

	/* Dump general information about OpenGL implementation to the console and the dump file */
	ssprintf(opengl.vendor, "OpenGL Vendor: %s", glGetString(GL_VENDOR));
	addDumpInfo(opengl.vendor);
	debug(LOG_3D, "%s", opengl.vendor);
	ssprintf(opengl.renderer, "OpenGL Renderer: %s", glGetString(GL_RENDERER));
	addDumpInfo(opengl.renderer);
	debug(LOG_3D, "%s", opengl.renderer);
	ssprintf(opengl.version, "OpenGL Version: %s", glGetString(GL_VERSION));
	addDumpInfo(opengl.version);
	debug(LOG_3D, "%s", opengl.version);

	khr_debug = GLAD_GL_KHR_debug;

	GLubyte const *extensionsBegin = glGetString(GL_EXTENSIONS);
	if (extensionsBegin == nullptr)
	{
		static GLubyte const emptyString[] = "";
		extensionsBegin = emptyString;
	}
	GLubyte const *extensionsEnd = extensionsBegin + strlen((char const *)extensionsBegin);
	std::vector<std::string> glExtensions;
	for (GLubyte const *i = extensionsBegin; i < extensionsEnd;)
	{
		GLubyte const *j = std::find(i, extensionsEnd, ' ');
		glExtensions.push_back(std::string(i, j));
		i = j + 1;
	}

	/* Dump extended information about OpenGL implementation to the console */

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
		debug(LOG_3D, "  * OpenGL 1.2 %s supported!", GLAD_GL_VERSION_1_2 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL 1.3 %s supported!", GLAD_GL_VERSION_1_3 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL 1.4 %s supported!", GLAD_GL_VERSION_1_4 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL 1.5 %s supported!", GLAD_GL_VERSION_1_5 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL 2.0 %s supported!", GLAD_GL_VERSION_2_0 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL 2.1 %s supported!", GLAD_GL_VERSION_2_1 ? "is" : "is NOT");
		debug(LOG_3D, "  * OpenGL 3.0 %s supported!", GLAD_GL_VERSION_3_0 ? "is" : "is NOT");
	#ifdef GLAD_GL_VERSION_3_1
		debug(LOG_3D, "  * OpenGL 3.1 %s supported!", GLAD_GL_VERSION_3_1 ? "is" : "is NOT");
	#endif
	#ifdef GLAD_GL_VERSION_3_2
		debug(LOG_3D, "  * OpenGL 3.2 %s supported!", GLAD_GL_VERSION_3_2 ? "is" : "is NOT");
	#endif
	#ifdef GLAD_GL_VERSION_3_3
		debug(LOG_3D, "  * OpenGL 3.3 %s supported!", GLAD_GL_VERSION_3_3 ? "is" : "is NOT");
	#endif
	#ifdef GLAD_GL_VERSION_4_0
		debug(LOG_3D, "  * OpenGL 4.0 %s supported!", GLAD_GL_VERSION_4_0 ? "is" : "is NOT");
	#endif
	#ifdef GLAD_GL_VERSION_4_1
		debug(LOG_3D, "  * OpenGL 4.1 %s supported!", GLAD_GL_VERSION_4_1 ? "is" : "is NOT");
	#endif

		debug(LOG_3D, "  * Texture compression %s supported.", GLAD_GL_ARB_texture_compression ? "is" : "is NOT");
		debug(LOG_3D, "  * Two side stencil %s supported.", GLAD_GL_EXT_stencil_two_side ? "is" : "is NOT");
		debug(LOG_3D, "  * ATI separate stencil is%s supported.", GLAD_GL_ATI_separate_stencil ? "" : " NOT");
		debug(LOG_3D, "  * Stencil wrap %s supported.", GLAD_GL_EXT_stencil_wrap ? "is" : "is NOT");
		debug(LOG_3D, "  * Rectangular texture %s supported.", GLAD_GL_ARB_texture_rectangle ? "is" : "is NOT");
		debug(LOG_3D, "  * FrameBuffer Object (FBO) %s supported.", GLAD_GL_EXT_framebuffer_object ? "is" : "is NOT");
		debug(LOG_3D, "  * ARB Vertex Buffer Object (VBO) %s supported.", GLAD_GL_ARB_vertex_buffer_object ? "is" : "is NOT");
		debug(LOG_3D, "  * NPOT %s supported.", GLAD_GL_ARB_texture_non_power_of_two ? "is" : "is NOT");
		debug(LOG_3D, "  * texture cube_map %s supported.", GLAD_GL_ARB_texture_cube_map ? "is" : "is NOT");
		debug(LOG_3D, "  * GL_ARB_timer_query %s supported!", GLAD_GL_ARB_timer_query ? "is" : "is NOT");
	}
	else
	{
		debug(LOG_3D, "  * OpenGL ES 2.0 %s supported!", GLAD_GL_ES_VERSION_2_0 ? "is" : "is NOT");
	#ifdef GLAD_GL_ES_VERSION_3_0
		debug(LOG_3D, "  * OpenGL ES 3.0 %s supported!", GLAD_GL_ES_VERSION_3_0 ? "is" : "is NOT");
	#endif
	}
	debug(LOG_3D, "  * Anisotropic filtering %s supported.", GLAD_GL_EXT_texture_filter_anisotropic ? "is" : "is NOT");
	debug(LOG_3D, "  * KHR_DEBUG support %s detected", khr_debug ? "was" : "was NOT");
	debug(LOG_3D, "  * glGenerateMipmap support %s detected", glGenerateMipmap ? "was" : "was NOT");

	if (!GLAD_GL_VERSION_2_0 && !GLAD_GL_ES_VERSION_2_0)
	{
		debug(LOG_FATAL, "OpenGL 2.0 / OpenGL ES 2.0 not supported! Please upgrade your drivers.");
		return false;
	}
	if (gles)
	{
		GLboolean bShaderCompilerSupported = GL_FALSE;
		glGetBooleanv(GL_SHADER_COMPILER, &bShaderCompilerSupported);
		if (bShaderCompilerSupported != GL_TRUE)
		{
			debug(LOG_FATAL, "Shader compiler is not supported! (GL_SHADER_COMPILER == GL_FALSE)");
			return false;
		}
	}

	std::pair<int, int> glslVersion(0, 0);
	sscanf((char const *)glGetString(GL_SHADING_LANGUAGE_VERSION), "%d.%d", &glslVersion.first, &glslVersion.second);

	/* Dump information about OpenGL 2.0+ implementation to the console and the dump file */
	GLint glMaxTIUs, glMaxTIUAs, glmaxSamples, glmaxSamplesbuf, glmaxVertexAttribs;

	debug(LOG_3D, "  * OpenGL GLSL Version : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	ssprintf(opengl.GLSLversion, "OpenGL GLSL Version : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
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

	// IMPORTANT: Reserve enough slots in enabledVertexAttribIndexes based on glmaxVertexAttribs
	enabledVertexAttribIndexes.resize(static_cast<size_t>(glmaxVertexAttribs), false);

	if (GLAD_GL_VERSION_3_0) // if context is OpenGL 3.0+
	{
		// Very simple VAO code - just bind a single global VAO (this gets things working, but is not optimal)
		static GLuint vaoId = 0;
		if (glGenVertexArrays == nullptr)
		{
			debug(LOG_FATAL, "glGenVertexArrays is not available, but context is OpenGL 3.0+");
			exit(1);
			return false;
		}
		glGenVertexArrays(1, &vaoId);
		glBindVertexArray(vaoId);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	if (GLAD_GL_ARB_timer_query)
	{
		glGenQueries(PERF_COUNT, perfpos);
	}

	if (khr_debug)
	{
		if (glDebugMessageCallback && glDebugMessageControl)
		{
			glDebugMessageCallback(khr_callback, nullptr);
			glEnable(GL_DEBUG_OUTPUT);
			// Do not want to output notifications. Some drivers spam them too much.
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
			debug(LOG_3D, "Enabling KHR_debug message callback");
		}
		else
		{
			debug(LOG_3D, "Failed to enable KHR_debug message callback");
		}
	}

	glGenBuffers(1, &scratchbuffer);

	return true;
}

void gl_context::flip(int clearMode)
{
	frameNum = std::max<size_t>(frameNum + 1, 1);
	backend_impl->swapWindow();
	glUseProgram(0);
	current_program = nullptr;

	if (clearMode & CLEAR_OFF_AND_NO_BUFFER_DOWNLOAD)
	{
		return;
	}

	GLbitfield clearFlags = 0;
	glDepthMask(GL_TRUE);
	clearFlags = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
	if (clearMode & CLEAR_SHADOW)
	{
		clearFlags |= GL_STENCIL_BUFFER_BIT;
	}
	glClear(clearFlags);
}

void gl_context::handleWindowSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	// Update the viewport to use the new *drawable* size (which may be greater than the new window size
	// if SDL's built-in high-DPI support is enabled and functioning).
	int drawableWidth = 0, drawableHeight = 0;
	backend_impl->getDrawableSize(&drawableWidth, &drawableHeight);
	debug(LOG_WZ, "Logical Size: %d x %d; Drawable Size: %d x %d", screenWidth, screenHeight, drawableWidth, drawableHeight);

	glViewport(0, 0, drawableWidth, drawableHeight);
	glCullFace(GL_FRONT);
	//	glEnable(GL_CULL_FACE);
}

void gl_context::shutdown()
{
	// move any other cleanup here?
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

const size_t& gl_context::current_FrameNum() const
{
	return frameNum;
}

