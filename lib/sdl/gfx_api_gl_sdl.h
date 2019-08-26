/*
	This file is part of Warzone 2100.
	Copyright (C) 2019  Warzone 2100 Project

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

#include "lib/ivis_opengl/gfx_api_gl.h"
#include <SDL_video.h>

class sdl_OpenGL_Impl final : public gfx_api::backend_OpenGL_Impl
{
public:
	sdl_OpenGL_Impl(SDL_Window* window, bool useOpenGLES);

	virtual GLADloadproc getGLGetProcAddress() override;
	virtual bool createGLContext() override;
	virtual void swapWindow() override;
	virtual void getDrawableSize(int* w, int* h) override;

	virtual bool isOpenGLES() override;

private:
	SDL_Window* window;
	bool useOpenglES = false;

	enum GLContextRequests {
		// Desktop OpenGL Context Requests
		OpenGLCore_HighestAvailable,
		OpenGL21Compat,
		// OpenGL ES Context Requests
		OpenGLES30,
		OpenGLES20,
		//
		MAX_CONTEXT_REQUESTS
	};

	GLContextRequests contextRequest = OpenGLCore_HighestAvailable;

private:
	bool configureNextOpenGLContextRequest();
	std::string to_string(const GLContextRequests& request) const;
};
