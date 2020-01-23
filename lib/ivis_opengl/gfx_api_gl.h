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

#include <glad/glad.h>
#include <algorithm>
#include <cmath>
#include <functional>

namespace gfx_api
{
	class backend_OpenGL_Impl
	{
	public:
		backend_OpenGL_Impl() {};
		virtual ~backend_OpenGL_Impl() {};

		virtual GLADloadproc getGLGetProcAddress() = 0;

		// Creates an OpenGL context (double-buffered)
		virtual bool createGLContext() = 0;
		virtual void swapWindow() = 0;

		// Use this function to get the size of a window's underlying drawable in pixels (for use with glViewport).
		virtual void getDrawableSize(int* w, int* h) = 0;

		virtual bool isOpenGLES() const = 0;

		virtual bool setSwapInterval(gfx_api::context::swap_interval_mode mode) = 0;
		virtual gfx_api::context::swap_interval_mode getSwapInterval() const = 0;
	};
}

struct gl_texture final : public gfx_api::texture
{
private:
	friend struct gl_context;
	GLuint _id;
	size_t mip_count;

	gl_texture();
	virtual ~gl_texture();
public:
	virtual void bind() override;
	void unbind();
	virtual void upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t & width, const size_t & height, const gfx_api::pixel_format & buffer_format, const void * data) override;
	virtual void upload_and_generate_mipmaps(const size_t& offset_x, const size_t& offset_y, const size_t& width, const size_t& height, const  gfx_api::pixel_format& buffer_format, const void* data) override;
	virtual unsigned id() override;
};

struct gl_buffer final : public gfx_api::buffer
{
	gfx_api::buffer::usage usage;
	gfx_api::context::buffer_storage_hint hint;
	GLuint buffer = 0;
	size_t buffer_size = 0;
	size_t lastUploaded_FrameNum = 0;

public:
	gl_buffer(const gfx_api::buffer::usage& usage, const gfx_api::context::buffer_storage_hint& hint);
	virtual ~gl_buffer() override;

	void bind() override;
	void unbind();
	virtual void upload(const size_t & size, const void * data) override;
	virtual void update(const size_t & start, const size_t & size, const void * data, const update_flag flag = update_flag::none) override;
};

struct gl_pipeline_state_object final : public gfx_api::pipeline_state_object
{
	gfx_api::state_description desc;
	GLuint program;
	std::vector<gfx_api::vertex_buffer> vertex_buffer_desc;
	std::vector<GLint> locations;

	std::function<void(const void*)> uniform_bind_function;

	template<SHADER_MODE shader>
	typename std::pair<SHADER_MODE, std::function<void(const void*)>> uniform_binding_entry();

	gl_pipeline_state_object(bool gles, const gfx_api::state_description& _desc, const SHADER_MODE& shader, const std::vector<gfx_api::vertex_buffer>& vertex_buffer_desc);
	void set_constants(const void* buffer);

	void bind();

private:
	// Read shader into text buffer
	static char *readShaderBuf(const std::string& name);

	// Retrieve shader compilation errors
	static void printShaderInfoLog(code_part part, GLuint shader);

	// Retrieve shader linkage errors
	static void printProgramInfoLog(code_part part, GLuint program);

	void getLocs();

	void build_program(const std::string& programName,
					   const char * vertex_header, const std::string& vertexPath,
					   const char * fragment_header, const std::string& fragmentPath,
					   const std::vector<std::string> &uniformNames);

	void fetch_uniforms(const std::vector<std::string>& uniformNames);

	/**
	 * setUniforms is an overloaded wrapper around glUniform* functions
	 * accepting glm structures.
	 * Please do not use directly, use pie_ActivateShader below.
	 */
	void setUniforms(GLint location, const ::glm::vec4 &v);
	void setUniforms(GLint location, const ::glm::mat4 &m);
	void setUniforms(GLint location, const Vector2i &v);
	void setUniforms(GLint location, const Vector2f &v);
	void setUniforms(GLint location, const int32_t &v);
	void setUniforms(GLint location, const float &v);


	// Wish there was static reflection in C++...
	template<typename T>
	void set_constants_for_component(const T& cbuf)
	{
		setUniforms(locations[0], cbuf.colour);
		setUniforms(locations[1], cbuf.teamcolour);
		setUniforms(locations[2], cbuf.shaderStretch);
		setUniforms(locations[3], cbuf.tcmask);
		setUniforms(locations[4], cbuf.fogEnabled);
		setUniforms(locations[5], cbuf.normalMap);
		setUniforms(locations[6], cbuf.specularMap);
		setUniforms(locations[7], cbuf.ecmState);
		setUniforms(locations[8], cbuf.alphaTest);
		setUniforms(locations[9], cbuf.timeState);
		setUniforms(locations[10], cbuf.ModelViewMatrix);
		setUniforms(locations[11], cbuf.ModelViewProjectionMatrix);
		setUniforms(locations[12], cbuf.NormalMatrix);
		setUniforms(locations[13], cbuf.sunPos);
		setUniforms(locations[14], cbuf.sceneColor);
		setUniforms(locations[15], cbuf.ambient);
		setUniforms(locations[16], cbuf.diffuse);
		setUniforms(locations[17], cbuf.specular);
		setUniforms(locations[18], cbuf.fogColour);
		setUniforms(locations[19], cbuf.fogEnd);
		setUniforms(locations[20], cbuf.fogBegin);
		setUniforms(locations[21], cbuf.hasTangents);
	}

	void set_constants(const gfx_api::constant_buffer_type<SHADER_BUTTON>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_COMPONENT>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_NOLIGHT>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_TERRAIN>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_TERRAIN_DEPTH>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_DECALS>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_WATER>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_RECT>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_TEXRECT>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_GFX_COLOUR>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_GFX_TEXT>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_GENERIC_COLOR>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_LINE>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_TEXT>& cbuf);
};

struct gl_context final : public gfx_api::context
{
	std::unique_ptr<gfx_api::backend_OpenGL_Impl> backend_impl;

	gl_pipeline_state_object* current_program = nullptr;
	GLuint scratchbuffer = 0;
	size_t scratchbuffer_size = 0;
	bool khr_debug = false;

	bool gles = false;
	bool fragmentHighpFloatAvailable = true;
	bool fragmentHighpIntAvailable = true;

	gl_context(bool _debug) : khr_debug(_debug) {}
	~gl_context();

	virtual gfx_api::texture* create_texture(const size_t& mipmap_count, const size_t & width, const size_t & height, const gfx_api::pixel_format & internal_format, const std::string& filename) override;
	virtual gfx_api::buffer * create_buffer_object(const gfx_api::buffer::usage &usage, const buffer_storage_hint& hint = buffer_storage_hint::static_draw) override;

	virtual gfx_api::pipeline_state_object * build_pipeline(const gfx_api::state_description &state_desc,
															const SHADER_MODE& shader_mode,
															const gfx_api::primitive_type& primitive,
															const std::vector<gfx_api::texture_input>& texture_desc,
															const std::vector<gfx_api::vertex_buffer>& attribute_descriptions) override;
	virtual void bind_pipeline(gfx_api::pipeline_state_object* pso, bool notextures) override;
	virtual void bind_index_buffer(gfx_api::buffer&, const gfx_api::index_type&) override;
	virtual void unbind_index_buffer(gfx_api::buffer&) override;
	virtual void bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) override;
	virtual void unbind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) override;
	virtual void disable_all_vertex_buffers() override;
	virtual void bind_streamed_vertex_buffers(const void* data, const std::size_t size) override;
	virtual void bind_textures(const std::vector<gfx_api::texture_input>& texture_descriptions, const std::vector<gfx_api::texture*>& textures) override;
	virtual void set_constants(const void* buffer, const size_t& size) override;
	virtual void draw(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive) override;
	virtual void draw_elements(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive, const gfx_api::index_type& index) override;
	virtual void set_polygon_offset(const float& offset, const float& slope) override;
	virtual void set_depth_range(const float& min, const float& max) override;
	virtual int32_t get_context_value(const context_value property) override;

	virtual void flip(int clearMode) override;
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
	virtual bool getScreenshot(iV_Image &output) override;
	virtual void handleWindowSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight) override;
	virtual void shutdown() override;
	virtual const size_t& current_FrameNum() const override;
	virtual bool setSwapInterval(gfx_api::context::swap_interval_mode mode) override;
	virtual gfx_api::context::swap_interval_mode getSwapInterval() const override;
private:
	virtual bool _initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode mode) override;
private:
	bool initGLContext();
	void enableVertexAttribArray(GLuint index);
	void disableVertexAttribArray(GLuint index);
	std::string calculateFormattedRendererInfoString() const;

	std::vector<bool> enabledVertexAttribIndexes;
	size_t frameNum = 0;
	std::string formattedRendererInfoString;
};
