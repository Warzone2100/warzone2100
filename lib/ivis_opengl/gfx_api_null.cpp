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
#include "gfx_api_null.h"
#include "lib/exceptionhandler/dumpinfo.h"

// MARK: null_texture

null_texture::null_texture()
{
	// no-op
}

null_texture::~null_texture()
{
	// no-op
}

void null_texture::bind()
{
	// no-op
}

bool null_texture::upload(const size_t& mip_level, const iV_BaseImage& image)
{
	ASSERT_OR_RETURN(false, image.data() != nullptr, "Attempt to upload image without data");
	ASSERT_OR_RETURN(false, image.pixel_format() == internal_format, "Uploading image to texture with different format");
	size_t width = image.width();
	size_t height = image.height();
	ASSERT(width > 0 && height > 0, "Attempt to upload texture with width or height of 0 (width: %zu, height: %zu)", width, height);
	// no-op
	return true;
}

bool null_texture::upload_sub(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const iV_Image& image)
{
	ASSERT_OR_RETURN(false, image.data() != nullptr, "Attempt to upload image without data");
	ASSERT_OR_RETURN(false, image.pixel_format() == internal_format, "Uploading image to texture with different format");
	size_t width = image.width();
	size_t height = image.height();
	ASSERT(width > 0 && height > 0, "Attempt to upload texture with width or height of 0 (width: %zu, height: %zu)", width, height);
	// no-op
	return true;
}

unsigned null_texture::id()
{
	return 0;
}

// MARK: null_buffer

null_buffer::null_buffer(const gfx_api::buffer::usage& usage, const gfx_api::context::buffer_storage_hint& hint)
: usage(usage)
, hint(hint)
{
	// no-op
}

null_buffer::~null_buffer()
{
	// no-op
}

void null_buffer::bind()
{
	// no-op
}

void null_buffer::upload(const size_t & size, const void * data)
{
	size_t current_FrameNum = gfx_api::context::get().current_FrameNum();
#if defined(DEBUG)
	ASSERT(lastUploaded_FrameNum != current_FrameNum, "Attempt to upload to buffer more than once per frame");
#endif
	lastUploaded_FrameNum = current_FrameNum;

	ASSERT(size > 0, "Attempt to upload buffer of size 0");
	buffer_size = size;
	// no-op
}

void null_buffer::update(const size_t & start, const size_t & size, const void * data, const update_flag flag)
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
	// no-op
}

// MARK: null_pipeline_state_object

null_pipeline_state_object::null_pipeline_state_object(const gfx_api::state_description& _desc, const std::vector<gfx_api::vertex_buffer>& _vertex_buffer_desc)
: desc(_desc), vertex_buffer_desc(_vertex_buffer_desc)
{
	// no-op
}

// MARK: null_context

null_context::~null_context()
{
	// nothing - any cleanup should occur in null_context::shutdown()
}

gfx_api::texture* null_context::create_texture(const size_t& mipmap_count, const size_t & width, const size_t & height, const gfx_api::pixel_format & internal_format, const std::string& filename)
{
	auto* new_texture = new null_texture();
	new_texture->internal_format = internal_format;
	return new_texture;
}

gfx_api::buffer * null_context::create_buffer_object(const gfx_api::buffer::usage &usage, const buffer_storage_hint& hint /*= buffer_storage_hint::static_draw*/)
{
	return new null_buffer(usage, hint);
}

gfx_api::pipeline_state_object * null_context::build_pipeline(const gfx_api::state_description &state_desc,
															const SHADER_MODE& shader_mode,
															const gfx_api::primitive_type& primitive,
															const std::vector<std::type_index>& uniform_blocks,
															const std::vector<gfx_api::texture_input>& texture_desc,
															const std::vector<gfx_api::vertex_buffer>& attribute_descriptions)
{
	return new null_pipeline_state_object(state_desc, attribute_descriptions);
}

void null_context::bind_pipeline(gfx_api::pipeline_state_object* pso, bool notextures)
{
	null_pipeline_state_object* new_program = static_cast<null_pipeline_state_object*>(pso);
	if (current_program != new_program)
	{
		current_program = new_program;
	}
}

void null_context::bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	for (size_t i = 0, e = vertex_buffers_offset.size(); i < e && (first + i) < current_program->vertex_buffer_desc.size(); ++i)
	{
		auto* buffer = static_cast<null_buffer*>(std::get<0>(vertex_buffers_offset[i]));
		if (buffer == nullptr)
		{
			continue;
		}
		ASSERT(buffer->usage == gfx_api::buffer::usage::vertex_buffer, "bind_vertex_buffers called with non-vertex-buffer");
		// no-op
	}
}

void null_context::unbind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	// no-op
}

void null_context::disable_all_vertex_buffers()
{
	// no-op
}

void null_context::bind_streamed_vertex_buffers(const void* data, const std::size_t size)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	ASSERT(size > 0, "bind_streamed_vertex_buffers called with size 0");
	// no-op
}

void null_context::bind_index_buffer(gfx_api::buffer& _buffer, const gfx_api::index_type&)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	auto& buffer = static_cast<null_buffer&>(_buffer);
	ASSERT(buffer.usage == gfx_api::buffer::usage::index_buffer, "Passed gfx_api::buffer is not an index buffer");
	// no-op
}

void null_context::unbind_index_buffer(gfx_api::buffer&)
{
	// no-op
}

void null_context::bind_textures(const std::vector<gfx_api::texture_input>& texture_descriptions, const std::vector<gfx_api::abstract_texture*>& textures)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	ASSERT(textures.size() <= texture_descriptions.size(), "Received more textures than expected");
	// no-op
}

void null_context::set_constants(const void* buffer, const size_t& size)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	// no-op
}

void null_context::set_uniforms(const size_t& first, const std::vector<std::tuple<const void*, size_t>>& uniform_blocks)
{
	ASSERT_OR_RETURN(, current_program != nullptr, "current_program == NULL");
	// no-op
}

void null_context::draw(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive)
{
	// no-op
}

void null_context::draw_instanced(const std::size_t& offset, const std::size_t &count, const gfx_api::primitive_type &primitive, std::size_t instance_count)
{
	// no-op
}

void null_context::draw_elements(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive, const gfx_api::index_type& index)
{
	// no-op
}

void null_context::draw_elements_instanced(const std::size_t& offset, const std::size_t &count, const gfx_api::primitive_type &primitive, const gfx_api::index_type& index, std::size_t instance_count)
{
	// no-op
}

void null_context::set_polygon_offset(const float& offset, const float& slope)
{
	// no-op
}

void null_context::set_depth_range(const float& min, const float& max)
{
	// no-op
}

int32_t null_context::get_context_value(const context_value property)
{
	// provide some fake, large-enough values to avoid issues
	switch(property)
	{
		case gfx_api::context::context_value::MAX_ELEMENTS_VERTICES:
			return 32000;
		case gfx_api::context::context_value::MAX_ELEMENTS_INDICES:
			return 32000;
		case gfx_api::context::context_value::MAX_TEXTURE_SIZE:
			return 4096;
		case gfx_api::context::context_value::MAX_SAMPLES:
			return 0;
		case gfx_api::context::context_value::MAX_ARRAY_TEXTURE_LAYERS:
			return 2048;
		case gfx_api::context::context_value::MAX_VERTEX_ATTRIBS:
			return 16;
		case gfx_api::context::context_value::MAX_VERTEX_OUTPUT_COMPONENTS:
			return 64;
	}
	debug(LOG_FATAL, "Unsupported property");
	return 0;
}

// MARK: null_context - debug

void null_context::debugStringMarker(const char *str)
{
	// no-op
}

void null_context::debugSceneBegin(const char *descr)
{
	// no-op
}

void null_context::debugSceneEnd(const char *descr)
{
	// no-op
}

bool null_context::debugPerfAvailable()
{
	return false;
}

bool null_context::debugPerfStart(size_t sample)
{
	return false;
}

void null_context::debugPerfStop()
{
	// no-op
}

void null_context::debugPerfBegin(PERF_POINT pp, const char *descr)
{
	// no-op
}

void null_context::debugPerfEnd(PERF_POINT pp)
{
	// no-op
}

uint64_t null_context::debugGetPerfValue(PERF_POINT pp)
{
	return 0;
}

std::map<std::string, std::string> null_context::getBackendGameInfo()
{
	std::map<std::string, std::string> backendGameInfo;
	backendGameInfo["null_gfx_backend"] = true;
	return backendGameInfo;
}

const std::string& null_context::getFormattedRendererInfoString() const
{
	return formattedRendererInfoString;
}

bool null_context::getScreenshot(std::function<void (std::unique_ptr<iV_Image>)> callback)
{
	return false;
}

bool null_context::_initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode mode, optional<float> mipLodBias)
{
	// obtain backend_Null_Impl from impl
	backend_impl = impl.createNullBackendImpl();
	if (!backend_impl)
	{
		debug(LOG_ERROR, "Failed to get Null / headless backend implementation");
		return false;
	}

	if (!setSwapInterval(mode))
	{
		// default to vsync on
		debug(LOG_3D, "Failed to set swap interval: %d; defaulting to vsync on", to_int(mode));
		setSwapInterval(gfx_api::context::swap_interval_mode::vsync);
	}

	return true;
}

void null_context::beginRenderPass()
{
	// no-op
}

void null_context::endRenderPass()
{
	frameNum = std::max<size_t>(frameNum + 1, 1);

	// Backend is expected to handle throttling / sleeping
	backend_impl->swapWindow();

	current_program = nullptr;
}

void null_context::handleWindowSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	// no-op
}

std::pair<uint32_t, uint32_t> null_context::getDrawableDimensions()
{
	return {0,0};
}

bool null_context::shouldDraw()
{
	return false;
}

void null_context::shutdown()
{
	// no-op
}

const size_t& null_context::current_FrameNum() const
{
	return frameNum;
}

bool null_context::setSwapInterval(gfx_api::context::swap_interval_mode mode)
{
	return backend_impl->setSwapInterval(mode);
}

gfx_api::context::swap_interval_mode null_context::getSwapInterval() const
{
	return backend_impl->getSwapInterval();
}

bool null_context::textureFormatIsSupported(gfx_api::pixel_format_target target, gfx_api::pixel_format format, gfx_api::pixel_format_usage::flags usage)
{
	// no matter what the input is, return true (since this null backend doesn't care and no-ops whatever it gets)
	return true;
}

bool null_context::supportsMipLodBias() const
{
	return true;
}

size_t null_context::maxFramesInFlight() const
{
	return 1;
}

bool null_context::supportsInstancedRendering()
{
	return false;
}
