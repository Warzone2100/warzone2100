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

#pragma once

#include "gfx_api.h"

namespace gfx_api
{
	class backend_Null_Impl
	{
	public:
		backend_Null_Impl() {};
		virtual ~backend_Null_Impl() {};

		virtual void swapWindow() = 0;

		virtual bool setSwapInterval(gfx_api::context::swap_interval_mode mode) = 0;
		virtual gfx_api::context::swap_interval_mode getSwapInterval() const = 0;
	};
}

struct null_texture final : public gfx_api::texture
{
private:
	friend struct null_context;
	null_texture();
	virtual ~null_texture();
public:
	virtual void bind() override;
	virtual bool upload(const size_t& mip_level, const iV_BaseImage& image) override;
	virtual bool upload_sub(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const iV_Image& image) override;
	virtual unsigned id() override;
protected:
	gfx_api::pixel_format internal_format = gfx_api::pixel_format::invalid;
};

struct null_buffer final : public gfx_api::buffer
{
	gfx_api::buffer::usage usage;
	gfx_api::context::buffer_storage_hint hint;
	size_t buffer_size = 0;
	size_t lastUploaded_FrameNum = 0;

public:
	null_buffer(const gfx_api::buffer::usage& usage, const gfx_api::context::buffer_storage_hint& hint);
	virtual ~null_buffer() override;

	void bind() override;
	virtual void upload(const size_t & size, const void * data) override;
	virtual void update(const size_t & start, const size_t & size, const void * data, const update_flag flag = update_flag::none) override;
};

struct null_pipeline_state_object final : public gfx_api::pipeline_state_object
{
	gfx_api::state_description desc;
	std::vector<gfx_api::vertex_buffer> vertex_buffer_desc;

	null_pipeline_state_object(const gfx_api::state_description& _desc, const std::vector<gfx_api::vertex_buffer>& vertex_buffer_desc);
};

struct null_context final : public gfx_api::context
{
private:
	std::unique_ptr<gfx_api::backend_Null_Impl> backend_impl;

	null_pipeline_state_object* current_program = nullptr;
	std::string formattedRendererInfoString = "Null backend (Headless mode)";

public:
	null_context(bool _debug) {}
	~null_context();

	virtual gfx_api::texture* create_texture(const size_t& mipmap_count, const size_t & width, const size_t & height, const gfx_api::pixel_format & internal_format, const std::string& filename) override;
	virtual gfx_api::buffer * create_buffer_object(const gfx_api::buffer::usage &usage, const buffer_storage_hint& hint = buffer_storage_hint::static_draw) override;

	virtual gfx_api::pipeline_state_object * build_pipeline(const gfx_api::state_description &state_desc,
															const SHADER_MODE& shader_mode,
															const gfx_api::primitive_type& primitive,
															const std::vector<std::type_index>& uniform_blocks,
															const std::vector<gfx_api::texture_input>& texture_desc,
															const std::vector<gfx_api::vertex_buffer>& attribute_descriptions) override;
	virtual void bind_pipeline(gfx_api::pipeline_state_object* pso, bool notextures) override;
	virtual void bind_index_buffer(gfx_api::buffer&, const gfx_api::index_type&) override;
	virtual void unbind_index_buffer(gfx_api::buffer&) override;
	virtual void bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) override;
	virtual void unbind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) override;
	virtual void disable_all_vertex_buffers() override;
	virtual void bind_streamed_vertex_buffers(const void* data, const std::size_t size) override;
	virtual void bind_textures(const std::vector<gfx_api::texture_input>& texture_descriptions, const std::vector<gfx_api::abstract_texture*>& textures) override;
	virtual void set_constants(const void* buffer, const size_t& size) override;
	virtual void set_uniforms(const size_t& first, const std::vector<std::tuple<const void*, size_t>>& uniform_blocks) override;
	virtual void draw(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive) override;
	virtual void draw_elements(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive, const gfx_api::index_type& index) override;
	virtual void set_polygon_offset(const float& offset, const float& slope) override;
	virtual void set_depth_range(const float& min, const float& max) override;
	virtual int32_t get_context_value(const context_value property) override;

	virtual void beginRenderPass() override;
	virtual void endRenderPass() override;
	virtual void debugStringMarker(const char *str) override;
	virtual void debugSceneBegin(const char *descr) override;
	virtual void debugSceneEnd(const char *descr) override;
	virtual bool debugPerfAvailable() override;
	virtual bool debugPerfStart(size_t sample) override;
	virtual void debugPerfStop() override;
	virtual void debugPerfBegin(PERF_POINT pp, const char *descr) override;
	virtual void debugPerfEnd(PERF_POINT pp) override;
	virtual uint64_t debugGetPerfValue(PERF_POINT pp) override;
	virtual std::map<std::string, std::string> getBackendGameInfo() override;
	virtual const std::string& getFormattedRendererInfoString() const override;
	virtual bool getScreenshot(std::function<void (std::unique_ptr<iV_Image>)> callback) override;
	virtual void handleWindowSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight) override;
	virtual std::pair<uint32_t, uint32_t> getDrawableDimensions() override;
	virtual bool shouldDraw() override;
	virtual void shutdown() override;
	virtual const size_t& current_FrameNum() const override;
	virtual bool setSwapInterval(gfx_api::context::swap_interval_mode mode) override;
	virtual gfx_api::context::swap_interval_mode getSwapInterval() const override;
	virtual bool textureFormatIsSupported(gfx_api::pixel_format_target target, gfx_api::pixel_format format, gfx_api::pixel_format_usage::flags usage) override;
	virtual bool supportsMipLodBias() const override;
	virtual size_t maxFramesInFlight() const override;
	// instanced rendering APIs
	virtual bool supportsInstancedRendering() override;
	virtual void draw_instanced(const std::size_t& offset, const std::size_t &count, const gfx_api::primitive_type &primitive, std::size_t instance_count) override;
	virtual void draw_elements_instanced(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type& primitive, const gfx_api::index_type& index, std::size_t instance_count) override;
private:
	virtual bool _initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode mode, optional<float> mipLodBias) override;
private:

	size_t frameNum = 0;
};
