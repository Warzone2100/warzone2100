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

struct iIMDShape;

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
bool pie_Draw3DShape(iIMDShape *shape, int frame, int team, PIELIGHT colour, int pieFlag, int pieFlagData, const glm::mat4 &modelView, float stretchDepth = 0.f, bool onlySingleLevel = false);
void pie_Draw3DButton(iIMDShape *shape, PIELIGHT teamcolour, const glm::mat4 &matrix);

void pie_GetResetCounts(size_t *pPieCount, size_t *pPolyCount);

/** Setup stencil shadows and OpenGL lighting. */
void pie_BeginLighting(const Vector3f &light);
void pie_setShadows(bool drawShadows);

/** Set light parameters */
void pie_InitLighting();
void pie_Lighting0(LIGHTING_TYPE entry, const float value[4]);

void pie_RemainingPasses(uint64_t currentGameFrame);

void pie_InitializeInstancedRenderer();
void pie_CleanUp();

#endif // _piedef_h
