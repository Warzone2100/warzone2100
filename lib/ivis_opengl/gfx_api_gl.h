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

#pragma once

#include "gfx_api.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>

struct gl_texture final : public gfx_api::texture
{
private:
	friend struct gl_context;
	GLuint _id;

	gl_texture();
	virtual ~gl_texture();
public:
	virtual void bind() override;
	virtual void upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t & width, const size_t & height, const gfx_api::pixel_format & buffer_format, const void * data) override;
	virtual unsigned id() override;
	virtual void generate_mip_levels() override;
};

struct gl_buffer final : public gfx_api::buffer
{
	gfx_api::buffer::usage usage;
	gfx_api::context::buffer_storage_hint hint;
	GLuint buffer = 0;
	size_t buffer_size = 0;

	gl_buffer(const gfx_api::buffer::usage& usage, const gfx_api::context::buffer_storage_hint& hint);
	virtual ~gl_buffer() override;

	void bind() override;
	virtual void upload(const size_t & size, const void * data) override;
	virtual void update(const size_t & start, const size_t & size, const void * data) override;
};

struct gl_context final : public gfx_api::context
{
	gl_context() {}
	~gl_context();

	virtual gfx_api::texture* create_texture(const size_t & width, const size_t & height, const gfx_api::pixel_format & internal_format, const std::string& filename) override;
	virtual gfx_api::buffer * create_buffer_object(const gfx_api::buffer::usage &usage, const buffer_storage_hint& hint = buffer_storage_hint::static_draw) override;
};
