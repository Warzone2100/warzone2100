#include "gfx_api.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include "lib/framework/frame.h"

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
		debug(LOG_FATAL, "unsupported buffer hint");
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

struct gl_texture : public gfx_api::texture
{
private:
	friend struct gl_context;
	GLuint _id;

	gl_texture()
	{
		glGenTextures(1, &_id);
	}

	~gl_texture()
	{
		glDeleteTextures(1, &_id);
	}
public:
	virtual void bind() override
	{
		glBindTexture(GL_TEXTURE_2D, _id);
	}

	virtual void upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t & width, const size_t & height, const gfx_api::pixel_format & buffer_format, const void * data) override
	{
		bind();
		glTexSubImage2D(GL_TEXTURE_2D, mip_level, offset_x, offset_y, width, height, to_gl(buffer_format), GL_UNSIGNED_BYTE, data);
	}

	virtual unsigned id() override
	{
		return _id;
	}

	virtual void generate_mip_levels() override
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

};

struct gl_buffer : public gfx_api::buffer
{
	gfx_api::buffer::usage usage;
	GLuint buffer;

	virtual ~gl_buffer() override
	{
		glDeleteBuffers(1, &buffer);
	}

	gl_buffer(const gfx_api::buffer::usage& usage, const size_t& size, const gfx_api::context::buffer_storage_hint& hint)
		: usage(usage)
	{
		glGenBuffers(1, &buffer);
		glBindBuffer(to_gl(usage), buffer);
		glBufferData(to_gl(usage), size, nullptr, to_gl(hint));
	}

	void bind() override
	{
		glBindBuffer(to_gl(usage), buffer);
	}


	virtual void upload(const size_t & start, const size_t & size, const void * data) override
	{
		glBindBuffer(to_gl(usage), buffer);
		glBufferSubData(to_gl(usage), start, size, data);
	}

};

struct gl_context : public gfx_api::context
{
	virtual gfx_api::texture* create_texture(const size_t & width, const size_t & height, const gfx_api::pixel_format & internal_format, const std::string& filename) override
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

	virtual gfx_api::buffer * create_buffer(const gfx_api::buffer::usage &usage, const size_t & width, const buffer_storage_hint& hint = buffer_storage_hint::static_draw) override
	{
		return new gl_buffer(usage, width, hint);
	}
};

gfx_api::context& gfx_api::context::get()
{
	static gl_context ctx;
	return ctx;
}
