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
/***************************************************************************/
/*
 * piedef.h
 *
 * type defines for all pumpkin image library functions.
 *
 */
/***************************************************************************/

#ifndef _piedef_h
#define _piedef_h

/***************************************************************************/

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include <glm/fwd.hpp>
#include "pietypes.h"

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

struct iIMDShape;

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
bool pie_Draw3DShape(const iIMDShape *shape, int frame, int team, PIELIGHT colour, int pieFlag, int pieFlagData, const glm::mat4 &modelMatrix, const glm::mat4 &viewMatrix, float stretchDepth = 0.f, bool onlySingleLevel = false);
void pie_Draw3DButton(const iIMDShape *shape, PIELIGHT teamcolour, const glm::mat4 &modelMatrix, const glm::mat4 &viewMatrix);

void pie_GetResetCounts(size_t *pPieCount, size_t *pPolyCount);

/** Setup shadows and OpenGL lighting. */
void pie_BeginLighting(const Vector3f &light);
void pie_setShadows(bool drawShadows);

/** Set light parameters */
void pie_InitLighting();
void pie_Lighting0(LIGHTING_TYPE entry, const float value[4]);
glm::vec4 pie_GetLighting0(LIGHTING_TYPE entry);

void pie_InitializeInstancedRenderer();
void pie_CleanUp();

enum class ShadowMode
{
	Fallback_Stencil_Shadows,
	Shadow_Mapping
};
bool pie_setShadowMode(ShadowMode mode);
ShadowMode pie_getShadowMode();
bool pie_setShadowCascades(uint32_t newValue);
uint32_t pie_getShadowCascades();

optional<bool> pie_supportsShadowMapping();
bool pie_setShadowMapResolution(uint32_t resolution);
uint32_t pie_getShadowMapResolution();

#endif // _piedef_h
