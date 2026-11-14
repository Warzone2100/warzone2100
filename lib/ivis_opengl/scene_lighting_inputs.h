// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/** @file scene_lighting_inputs.h
 * Forward-pass lighting textures (CSM + optional contact-shadow mask).
 */

#pragma once

#include "gfx_api.h"

#include <glm/vec4.hpp>

namespace gfx_api
{

inline bool isContactShadowMaskRead(const ResolvedRead& read)
{
	return read.pipelineSurfaceId.has_value()
		&& (*read.pipelineSurfaceId == PipelineSurfaceId::ContactShadowRaw
			|| *read.pipelineSurfaceId == PipelineSurfaceId::ContactShadowBlurred);
}

struct SceneLightingInputs
{
	abstract_texture* shadowMap = nullptr;
	abstract_texture* contactShadow = nullptr;
	abstract_texture* contactShadowNeutral = nullptr;
	glm::vec4 contactUvScaleClamp{1.f, 1.f, 1.f, 1.f};

	static SceneLightingInputs dummy(abstract_texture* shadowMap, abstract_texture* dummyLit)
	{
		SceneLightingInputs lighting;
		lighting.shadowMap = shadowMap;
		lighting.contactShadow = dummyLit;
		lighting.contactShadowNeutral = dummyLit;
		lighting.contactUvScaleClamp = {1.f, 1.f, 1.f, 1.f};
		return lighting;
	}

	SceneLightingInputs withoutContact() const
	{
		SceneLightingInputs lighting = *this;
		lighting.contactShadow = contactShadowNeutral;
		return lighting;
	}
};

} // namespace gfx_api
