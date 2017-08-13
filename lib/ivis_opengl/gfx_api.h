#pragma once

#include <memory>
#include <string>

namespace gfx_api
{
#ifdef GL_ONLY
	using gfxFloat = GLfloat;
#else
	using gfxFloat = float;
#endif

	enum class pixel_format
	{
		invalid,
		rgb,
		rgba,
		compressed_rgb,
		compressed_rgba,
	};

	struct texture
	{
		virtual ~texture() {};
		virtual void bind() = 0;
		virtual void upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t& width, const size_t& height, const pixel_format& buffer_format, const void* data) = 0;
		virtual void generate_mip_levels() = 0;
		virtual unsigned id() = 0;
	};

	struct buffer
	{
		enum class usage
		{
			vertex_buffer,
			index_buffer,
		};

		virtual void upload(const size_t& start, const size_t& size, const void* data) = 0;
		virtual void bind() = 0;
		virtual ~buffer() {};

	};

	struct context
	{
		enum class buffer_storage_hint
		{
			static_draw,
			stream_draw,
			dynamic_draw,
		};

		virtual ~context() {};
		virtual texture* create_texture(const size_t& width, const size_t& height, const pixel_format& internal_format, const std::string& filename = "") = 0;
		virtual buffer* create_buffer(const buffer::usage&, const size_t& width, const buffer_storage_hint& = buffer_storage_hint::static_draw) = 0;
		static context& get();
	};
}

