/*
	This file is part of Warzone 2100.
	Copyright (C) 2021  Warzone 2100 Project

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
/**
 * @file
 * Definitions for scrollable table functions.
 */

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/form.h"

bool specLayerInit(bool showButton = true);
void specToggleOverlays();
bool specStatsViewShutdown();
void specStatsViewCreate();
void specStatsViewClose();
bool specStatsViewShown();
/// Built again when open in the skin it no longer belongs to
void specStatsViewSkinChanged();
// Hides the stats button for as long as something else opens the stats (the HUD's spectator strip)
void specSetStatsButtonHidden(bool hidden);
// The overlay the spectator widgets live on, so anything opened over them can say
// which screen to sit above. Null when there is none.
std::shared_ptr<W_SCREEN> specOverlayScreen();
