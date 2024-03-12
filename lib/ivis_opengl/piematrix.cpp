/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/** \file
 *  Matrix manipulation functions.
 */

#include "lib/framework/frame.h"

#include "lib/framework/fixedpoint.h"
#include "lib/ivis_opengl/pieclip.h"
#include "piematrix.h"
#include "lib/ivis_opengl/piemode.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

static float perspectiveZClose = 330.f;
static float perspectiveZFar = 45000.f;

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

enum class PerspectiveModes
{
	Camera,
	Skybox,
	ScreenUI
};
constexpr size_t Num_Perspective_Modes = static_cast<size_t>(PerspectiveModes::ScreenUI) + 1;

struct PerspectiveCache {
	glm::mat4 currentPerspectiveMatrix = glm::mat4();
	float _width = 0.f;
	float _height = 0.f;
	int _rendSurface_xcentre = 0;
	int _rendSurface_ycentre = 0;
	float _zfar = 0.f;
};
// Since computing the perspective matrix is a semi-costly operation, store a cache here
static PerspectiveCache perspectiveCaches[Num_Perspective_Modes] = {};

/*!
 * 3D vector perspective projection
 * Projects 3D vector into 2D screen space
 * \param v3d       3D vector to project
 * \param perspectiveViewMatrix		precomputed perspective multipled by view matrix
 * \param[out] v2d  resulting 2D vector
 * \return projected z component of v2d
 */
int32_t pie_RotateProjectWithPerspective(const Vector3i *v3d, const glm::mat4 &perspectiveViewMatrix, Vector2i *v2d)
{
	float hackScaleFactor = 1.0f / (3 * 330);  // HACK: This seems to work by experimentation, not sure why.

	/*
	 * v = curMatrix . v3d
	 */
	glm::vec4 v(perspectiveViewMatrix * glm::vec4(*v3d, 1.f));

	const float xx = v.x / v.w;
	const float yy = v.y / v.w;

	if (v.w < 256 * hackScaleFactor)
	{
		v2d->x = LONG_WAY; //just along way off screen
		v2d->y = LONG_WAY;
	}
	else
	{
		v2d->x = static_cast<int>((.5 + .5 * xx) * pie_GetVideoBufferWidth());
		v2d->y = static_cast<int>((.5 - .5 * yy) * pie_GetVideoBufferHeight());
	}

	return static_cast<int32_t>(v.w);
}

glm::mat4 pie_PerspectiveGetConstrained(float zNear, float zFar)
{
	const float width = std::max(pie_GetVideoBufferWidth(), 1);  // Require width > 0 && height > 0, to avoid glScalef(1, 1, -1) crashing in some graphics drivers.
	const float height = std::max(pie_GetVideoBufferHeight(), 1);

	// can be a semi-costly operation
	const float xangle = width / 6.0f;
	const float yangle = height / 6.0f;
	return
//	glm::translate(glm::vec3((2.f * rendSurface.xcentre - width) / width, (height - 2.f * rendSurface.ycentre) / height, 0.f)) *
	glm::frustum(-xangle, xangle, -yangle, yangle, zNear, zFar) * glm::scale(glm::vec3(1.f, 1.f, -1.f));
}

static const glm::mat4& pie_PerspectiveGetCache(PerspectiveModes mode, float zNear, float zFar, const std::function<void (glm::mat4&, float width, float height)>& modifyGeneratedMatrix = nullptr)
{
	const float width = std::max(pie_GetVideoBufferWidth(), 1);  // Require width > 0 && height > 0, to avoid glScalef(1, 1, -1) crashing in some graphics drivers.
	const float height = std::max(pie_GetVideoBufferHeight(), 1);

	PerspectiveCache& perspectiveCache = perspectiveCaches[static_cast<size_t>(mode)];

	if (width != perspectiveCache._width || height != perspectiveCache._height || rendSurface.xcentre != perspectiveCache._rendSurface_xcentre || rendSurface.ycentre != perspectiveCache._rendSurface_ycentre || zFar != perspectiveCache._zfar)
	{
		// update the current perspective matrix (can be a semi-costly operation)
		perspectiveCache.currentPerspectiveMatrix = pie_PerspectiveGetConstrained(zNear, zFar);
		if (modifyGeneratedMatrix)
		{
			modifyGeneratedMatrix(perspectiveCache.currentPerspectiveMatrix, width, height);
		}
		perspectiveCache._width = width;
		perspectiveCache._height = height;
		perspectiveCache._rendSurface_xcentre = rendSurface.xcentre;
		perspectiveCache._rendSurface_ycentre = rendSurface.ycentre;
		perspectiveCache._zfar = perspectiveZFar;
	}

	return perspectiveCache.currentPerspectiveMatrix;
}

const glm::mat4& pie_SkyboxPerspectiveGet()
{
	return pie_PerspectiveGetCache(PerspectiveModes::Skybox, 330.f, 100000.f, [](glm::mat4& matrix, float width, float height) {
		matrix = glm::translate(glm::vec3((2.f * rendSurface.xcentre - width) / width, (height - 2.f * rendSurface.ycentre) / height, 0.f)) * matrix;
	});
}

const glm::mat4& pie_PerspectiveGet()
{
	return pie_PerspectiveGetCache(PerspectiveModes::Camera, perspectiveZClose, perspectiveZFar, [](glm::mat4& matrix, float width, float height) {
		matrix = glm::translate(glm::vec3((2.f * rendSurface.xcentre - width) / width, (height - 2.f * rendSurface.ycentre) / height, 0.f)) * matrix;
	});
}

const glm::mat4& pie_UIPerspectiveGet()
{
	return pie_PerspectiveGetCache(PerspectiveModes::ScreenUI, 330.f, 100000.f, [](glm::mat4& matrix, float width, float height) {
		matrix = glm::translate(glm::vec3((2.f * rendSurface.xcentre - width) / width, (height - 2.f * rendSurface.ycentre) / height, 0.f)) * matrix;
	});
}

float pie_getPerspectiveZFar()
{
	return perspectiveZFar;
}

void pie_Begin3DScene()
{
	gfx_api::context::get().set_depth_range(0.f, 1.f);
}

void pie_BeginInterface()
{
	gfx_api::context::get().set_depth_range(0.f, 0.1f);
}

void pie_SetGeometricOffset(int x, int y)
{
	rendSurface.xcentre = x;
	rendSurface.ycentre = y;
}
