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

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

struct PerspectiveCache {
	glm::mat4 currentPerspectiveMatrix;
	float _width;
	float _height;
	int _rendSurface_xcentre;
	int _rendSurface_ycentre;
};
// Since computing the perspective matrix is a semi-costly operation, store a cache here
static PerspectiveCache perspectiveCache;

/*!
 * 3D vector perspective projection
 * Projects 3D vector into 2D screen space
 * \param v3d       3D vector to project
 * \param[out] v2d  resulting 2D vector
 * \return projected z component of v2d
 */
int32_t pie_RotateProject(const Vector3i *v3d, const glm::mat4& matrix, Vector2i *v2d)
{
	float hackScaleFactor = 1.0f / (3 * 330);  // HACK: This seems to work by experimentation, not sure why.

	/*
	 * v = curMatrix . v3d
	 */
	glm::vec4 v(pie_PerspectiveGet() * matrix * glm::vec4(*v3d, 1.f));

	const float xx = v.x / v.w;
	const float yy = v.y / v.w;

	if (v.w < 256 * hackScaleFactor)
	{
		v2d->x = LONG_WAY; //just along way off screen
		v2d->y = LONG_WAY;
	}
	else
	{
		v2d->x = (.5 + .5 * xx) * pie_GetVideoBufferWidth();
		v2d->y = (.5 - .5 * yy) * pie_GetVideoBufferHeight();
	}

	return v.w;
}

const glm::mat4& pie_PerspectiveGet()
{
	const float width = std::max(pie_GetVideoBufferWidth(), 1);  // Require width > 0 && height > 0, to avoid glScalef(1, 1, -1) crashing in some graphics drivers.
	const float height = std::max(pie_GetVideoBufferHeight(), 1);

	if (width != perspectiveCache._width || height != perspectiveCache._height || rendSurface.xcentre != perspectiveCache._rendSurface_xcentre || rendSurface.ycentre != perspectiveCache._rendSurface_ycentre)
	{
		// update the current perspective matrix (can be a semi-costly operation)
		const float xangle = width / 6.0f;
		const float yangle = height / 6.0f;
		perspectiveCache.currentPerspectiveMatrix = glm::translate(glm::vec3((2.f * rendSurface.xcentre - width) / width, (height - 2.f * rendSurface.ycentre) / height, 0.f))
		* glm::frustum(-xangle, xangle, -yangle, yangle, 330.f, 100000.f) * glm::scale(glm::vec3(1.f, 1.f, -1.f));
		perspectiveCache._width = width;
		perspectiveCache._height = height;
		perspectiveCache._rendSurface_xcentre = rendSurface.xcentre;
		perspectiveCache._rendSurface_ycentre = rendSurface.ycentre;
	}

	return perspectiveCache.currentPerspectiveMatrix;
}


void pie_Begin3DScene()
{
	gfx_api::context::get().set_depth_range(0.1f, 1.f);
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
