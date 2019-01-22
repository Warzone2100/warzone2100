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

#include "lib/framework/frame.h"
#include "gfx_api_gl.h"

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

void gl_texture::upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t & width, const size_t & height, const gfx_api::pixel_format & buffer_format, const void * data)
{
	bind();
	glTexSubImage2D(GL_TEXTURE_2D, mip_level, offset_x, offset_y, width, height, to_gl(buffer_format), GL_UNSIGNED_BYTE, data);
}

unsigned gl_texture::id()
{
	return _id;
}

void gl_texture::generate_mip_levels()
{
	glGenerateMipmap(GL_TEXTURE_2D);
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
