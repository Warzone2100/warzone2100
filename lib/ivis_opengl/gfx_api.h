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

	// An abstract base that manages a single gfx buffer
	struct buffer
	{
		enum class usage
		{
			 vertex_buffer,
			 index_buffer,
		};

		// Create a new data store for the buffer. Any existing data store will be deleted.
		// The new data store is created with the specified `size` in bytes.
		// If `data` is not NULL, the data store is initialized with `size` bytes of data from the pointer.
		// - If data is NULL, a data store of the specified `size` is still created, but its contents may be uninitialized (and thus undefined).
		virtual void upload(const size_t& size, const void* data) = 0;

		// Update `size` bytes in the existing data store, from the `start` offset, using the data at `data`.
		// - The `start` offset must be within the range of the existing data store (0 to buffer_size - 1, inclusive).
		// - The `size` of the data specified (+ the offset) must not attempt to write past the end of the existing data store.
		// - A data store must be allocated before it can be updated - call `upload` on a `buffer` instance before `update`.
		virtual void update(const size_t& start, const size_t& size, const void* data) = 0;

		virtual void bind() = 0;

		virtual ~buffer() {};

		buffer( const buffer& other ) = delete; // non construction-copyable
		buffer& operator=( const buffer& ) = delete; // non copyable
		buffer() {};
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
		virtual buffer* create_buffer_object(const buffer::usage&, const buffer_storage_hint& = buffer_storage_hint::static_draw) = 0;
		static context& get();
	};
}

