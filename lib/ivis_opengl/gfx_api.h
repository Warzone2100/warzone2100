#pragma once

#include <memory>
#include <string>

namespace gfx_api
{
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

	struct context
	{
		virtual ~context() {};
		virtual texture* create_texture(const size_t& width, const size_t& height, const pixel_format& internal_format, const std::string& filename = "") = 0;
		static context& get();
	};
}

