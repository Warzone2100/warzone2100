// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
/** @file scene_prepass.h
 * Combined scene depth + view-space normals prepass for SSAO.
 */

#pragma once

#include "lib/ivis_opengl/gfx_api.h"

void recordScenePrepass(const gfx_api::RenderPassContext& passCtx);
