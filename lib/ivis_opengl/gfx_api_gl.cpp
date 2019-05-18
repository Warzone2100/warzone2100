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

#include <vector>
#include <string>

// for compatibility with older versions of GLEW
#ifndef GLEW_ARB_timer_query
#define GLEW_ARB_timer_query false
#endif
#ifndef GLEW_KHR_debug
#define GLEW_KHR_debug false
#define GL_DEBUG_SOURCE_APPLICATION 0x824A
static void glPopDebugGroup() {}
static void glPushDebugGroup(int, unsigned, unsigned, const char *) {}
#else
#ifndef glPopDebugGroup // hack to workaround a glew 1.9 bug
static void glPopDebugGroup() {}
#endif
#endif

struct OPENGL_DATA
{
	char vendor[256];
	char renderer[256];
	char version[256];
	char GLEWversion[256];
	char GLSLversion[256];
};
OPENGL_DATA opengl;

static GLuint perfpos[PERF_COUNT];
static bool perfStarted = false;

static GLenum to_gl(const gfx_api::pixel_format& format)
{
	switch (format)
	{
	case gfx_api::pixel_format::rgba:
		return GL_RGBA;
	case gfx_api::pixel_format::rgb:
		return GL_RGB;
	case gfx_api::pixel_format::compressed_rgb:
		return GL_RGB_S3TC;
	case gfx_api::pixel_format::compressed_rgba:
		return GL_RGBA_S3TC;
	default:
		debug(LOG_FATAL, "Unrecognised pixel format");
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

void gl_texture::upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t & width, const size_t & height, const gfx_api::pixel_format & buffer_format, const void * data, bool generate_mip_levels /*= false*/)
{
	bind();
	if(generate_mip_levels && !glGenerateMipmap)
	{
		// fallback for if glGenerateMipmap is unavailable
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	}
	glTexSubImage2D(GL_TEXTURE_2D, mip_level, offset_x, offset_y, width, height, to_gl(buffer_format), GL_UNSIGNED_BYTE, data);
	if(generate_mip_levels && glGenerateMipmap)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}
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

void gl_buffer::upload(const size_t & size, const void * data)
{
	glBindBuffer(to_gl(usage), buffer);
	glBufferData(to_gl(usage), size, data, to_gl(hint));
	buffer_size = size;
}

void gl_buffer::update(const size_t & start, const size_t & size, const void * data)
{
	ASSERT(start < buffer_size, "Starting offset (%zu) is past end of buffer (length: %zu)", start, buffer_size);
	ASSERT(start + size <= buffer_size, "Attempt to write past end of buffer");
	if (size == 0)
	{
		debug(LOG_WARNING, "Attempt to update buffer with 0 bytes of new data");
		return;
	}
	glBindBuffer(to_gl(usage), buffer);
	glBufferSubData(to_gl(usage), start, size, data);
}

// MARK: gl_context

gl_context::~gl_context()
{
}

gfx_api::texture* gl_context::create_texture(const size_t & width, const size_t & height, const gfx_api::pixel_format & internal_format, const std::string& filename)
{
	auto* new_texture = new gl_texture();
	new_texture->bind();
	if (!filename.empty() && (GLEW_VERSION_4_3 || GLEW_KHR_debug))
	{
		glObjectLabel(GL_TEXTURE, new_texture->id(), -1, filename.c_str());
	}
	for (unsigned i = 0; i < floor(log(std::max(width, height))) + 1; ++i)
	{
		glTexImage2D(GL_TEXTURE_2D, i, to_gl(internal_format), width >> i, height >> i, 0, to_gl(internal_format), GL_UNSIGNED_BYTE, nullptr);
	}
	return new_texture;
}

gfx_api::buffer * gl_context::create_buffer_object(const gfx_api::buffer::usage &usage, const buffer_storage_hint& hint /*= buffer_storage_hint::static_draw*/)
{
	return new gl_buffer(usage, hint);
}

gfx_api::context& gfx_api::context::get()
{
	static gl_context ctx;
	return ctx;
}

void gl_context::debugStringMarker(const char *str)
{
	if (GLEW_GREMEDY_string_marker)
	{
		glStringMarkerGREMEDY(0, str);
	}
}

void gl_context::debugSceneBegin(const char *descr)
{
	if (khr_debug)
	{
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, descr);
	}
}

void gl_context::debugSceneEnd(const char *descr)
{
	if (khr_debug)
	{
		glPopDebugGroup();
	}
}

bool gl_context::debugPerfAvailable()
{
	return GLEW_ARB_timer_query;
}

bool gl_context::debugPerfStart(size_t sample)
{
	if (GLEW_ARB_timer_query)
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
	if (khr_debug)
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
	if (khr_debug)
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
	backendGameInfo["openGL_GLEW_version"] = opengl.GLEWversion;
	backendGameInfo["openGL_GLSL_version"] = opengl.GLSLversion;
	// NOTE: deprecated for GL 3+. Needed this to check what extensions some chipsets support for the openGL hacks
	std::string extensions = (const char *) glGetString(GL_EXTENSIONS);
	backendGameInfo["GL_EXTENSIONS"] = extensions;
	return backendGameInfo;
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

bool gl_context::initGLContext()
{
	GLint glMaxTUs;
	GLenum err;

#if defined(WZ_USE_OPENGL_3_2_CORE_PROFILE)
	const char * _glewMajorVersionString = (const char*)glewGetString(GLEW_VERSION_MAJOR);
	const char * _glewMinorVersionString = (const char*)glewGetString(GLEW_VERSION_MINOR);
	int _glewMajorVersion = std::stoi(_glewMajorVersionString);
	int _glewMinorVersion = std::stoi(_glewMinorVersionString);

	if ((_glewMajorVersion < 1) || (_glewMajorVersion == 1 && _glewMinorVersion <= 13))
	{
		// GLEW <= 1.13 has a problem with OpenGL Core Contexts
		// It calls glGetString(GL_EXTENSIONS), which causes GL_INVALID_ENUM as soon as glewInit() is called.
		// It also doesn't fetch the function pointers.
		// (GLEW 2.0.0+ properly uses glGetStringi() instead.)
		// The only fix for GLEW <= 1.13 is to use glewExperimental
		glewExperimental = GL_TRUE;
	}
#endif
	err = glewInit();
	if (GLEW_OK != err)
	{
		debug(LOG_FATAL, "Error: %s", glewGetErrorString(err));
		exit(1);
	}
#if defined(WZ_USE_OPENGL_3_2_CORE_PROFILE)
	if ((_glewMajorVersion < 1) || (_glewMajorVersion == 1 && _glewMinorVersion <= 13))
	{
		// Swallow the gl error generated by glewInit() on GLEW <= 1.13 when using a Core Context
		err = glGetError();
		err = GLEW_OK;
	}
#endif

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
	ssprintf(opengl.GLEWversion, "GLEW Version: %s", glewGetString(GLEW_VERSION));
	if (strncmp(opengl.GLEWversion, "1.9.", 4) == 0) // work around known bug with KHR_debug extension support in this release
	{
		debug(LOG_WARNING, "Your version of GLEW is old and buggy, please upgrade to at least version 1.10.");
		khr_debug = false;
	}
	else
	{
		khr_debug = GLEW_KHR_debug;
	}
	addDumpInfo(opengl.GLEWversion);
	debug(LOG_3D, "%s", opengl.GLEWversion);

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
	debug(LOG_3D, "Notable OpenGL features:");
	debug(LOG_3D, "  * OpenGL 1.2 %s supported!", GLEW_VERSION_1_2 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.3 %s supported!", GLEW_VERSION_1_3 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.4 %s supported!", GLEW_VERSION_1_4 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.5 %s supported!", GLEW_VERSION_1_5 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 2.0 %s supported!", GLEW_VERSION_2_0 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 2.1 %s supported!", GLEW_VERSION_2_1 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 3.0 %s supported!", GLEW_VERSION_3_0 ? "is" : "is NOT");
#ifdef GLEW_VERSION_3_1
	debug(LOG_3D, "  * OpenGL 3.1 %s supported!", GLEW_VERSION_3_1 ? "is" : "is NOT");
#endif
#ifdef GLEW_VERSION_3_2
	debug(LOG_3D, "  * OpenGL 3.2 %s supported!", GLEW_VERSION_3_2 ? "is" : "is NOT");
#endif
#ifdef GLEW_VERSION_3_3
	debug(LOG_3D, "  * OpenGL 3.3 %s supported!", GLEW_VERSION_3_3 ? "is" : "is NOT");
#endif
#ifdef GLEW_VERSION_4_0
	debug(LOG_3D, "  * OpenGL 4.0 %s supported!", GLEW_VERSION_4_0 ? "is" : "is NOT");
#endif
#ifdef GLEW_VERSION_4_1
	debug(LOG_3D, "  * OpenGL 4.1 %s supported!", GLEW_VERSION_4_1 ? "is" : "is NOT");
#endif
	debug(LOG_3D, "  * Texture compression %s supported.", GLEW_ARB_texture_compression ? "is" : "is NOT");
	debug(LOG_3D, "  * Two side stencil %s supported.", GLEW_EXT_stencil_two_side ? "is" : "is NOT");
	debug(LOG_3D, "  * ATI separate stencil is%s supported.", GLEW_ATI_separate_stencil ? "" : " NOT");
	debug(LOG_3D, "  * Stencil wrap %s supported.", GLEW_EXT_stencil_wrap ? "is" : "is NOT");
	debug(LOG_3D, "  * Anisotropic filtering %s supported.", GLEW_EXT_texture_filter_anisotropic ? "is" : "is NOT");
	debug(LOG_3D, "  * Rectangular texture %s supported.", GLEW_ARB_texture_rectangle ? "is" : "is NOT");
	debug(LOG_3D, "  * FrameBuffer Object (FBO) %s supported.", GLEW_EXT_framebuffer_object ? "is" : "is NOT");
	debug(LOG_3D, "  * ARB Vertex Buffer Object (VBO) %s supported.", GLEW_ARB_vertex_buffer_object ? "is" : "is NOT");
	debug(LOG_3D, "  * NPOT %s supported.", GLEW_ARB_texture_non_power_of_two ? "is" : "is NOT");
	debug(LOG_3D, "  * texture cube_map %s supported.", GLEW_ARB_texture_cube_map ? "is" : "is NOT");
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &glMaxTUs);
	debug(LOG_3D, "  * Total number of Texture Units (TUs) supported is %d.", (int) glMaxTUs);
	debug(LOG_3D, "  * GL_ARB_timer_query %s supported!", GLEW_ARB_timer_query ? "is" : "is NOT");
	debug(LOG_3D, "  * KHR_DEBUG support %s detected", khr_debug ? "was" : "was NOT");
	debug(LOG_3D, "  * glGenerateMipmap support %s detected", glGenerateMipmap ? "was" : "was NOT");

	if (!GLEW_VERSION_2_0)
	{
		debug(LOG_FATAL, "OpenGL 2.0 not supported! Please upgrade your drivers.");
		return false;
	}

	std::pair<int, int> glslVersion(0, 0);
	sscanf((char const *)glGetString(GL_SHADING_LANGUAGE_VERSION), "%d.%d", &glslVersion.first, &glslVersion.second);

	/* Dump information about OpenGL 2.0+ implementation to the console and the dump file */
	GLint glMaxTIUs, glMaxTCs, glMaxTIUAs, glmaxSamples, glmaxSamplesbuf;

	debug(LOG_3D, "  * OpenGL GLSL Version : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	ssprintf(opengl.GLSLversion, "OpenGL GLSL Version : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	addDumpInfo(opengl.GLSLversion);

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &glMaxTIUs);
	debug(LOG_3D, "  * Total number of Texture Image Units (TIUs) supported is %d.", (int) glMaxTIUs);
	glGetIntegerv(GL_MAX_TEXTURE_COORDS, &glMaxTCs);
	debug(LOG_3D, "  * Total number of Texture Coords (TCs) supported is %d.", (int) glMaxTCs);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB, &glMaxTIUAs);
	debug(LOG_3D, "  * Total number of Texture Image Units ARB(TIUAs) supported is %d.", (int) glMaxTIUAs);
	glGetIntegerv(GL_SAMPLE_BUFFERS, &glmaxSamplesbuf);
	debug(LOG_3D, "  * (current) Max Sample buffer is %d.", (int) glmaxSamplesbuf);
	glGetIntegerv(GL_SAMPLES, &glmaxSamples);
	debug(LOG_3D, "  * (current) Max Sample level is %d.", (int) glmaxSamples);

#if defined(WZ_USE_OPENGL_3_2_CORE_PROFILE)
	// Very simple VAO code - just bind a single global VAO (this gets things working, but is not optimal)
	static GLuint vaoId = 0;
	if (glGenVertexArrays == nullptr)
	{
		debug(LOG_FATAL, "glGenVertexArray is not available, but core profile was specified");
		exit(1);
		return false;
	}
	glGenVertexArrays(1, &vaoId);
	glBindVertexArray(vaoId);
#endif

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	if (GLEW_ARB_timer_query)
	{
		glGenQueries(PERF_COUNT, perfpos);
	}

	if (khr_debug)
	{
		glDebugMessageCallback(khr_callback, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
		// Do not want to output notifications. Some drivers spam them too much.
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		debug(LOG_3D, "Enabling KHR_debug message callback");
	}

	return true;
}

