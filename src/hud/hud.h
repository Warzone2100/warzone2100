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
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/** @file
 *  The optional HUD layer: skins, modules and their lifecycle (see tests/hud for the geometry contract they keep).
 */

#ifndef __INCLUDED_SRC_HUD_HUD_H__
#define __INCLUDED_SRC_HUD_HUD_H__

#include "hudsettings.h"

#include "lib/framework/geometry.h"

#include <memory>

class W_FORM;

/// A HUD addition shown on the HUD screen (a free zone), created while its option is on
class HudModule
{
public:
	virtual ~HudModule() = default;
	virtual HudModuleId moduleId() const = 0;
	virtual void create(const std::shared_ptr<W_FORM> &root) = 0;
	virtual void destroy() = 0;
	/// At the HUD model's refresh rate
	virtual void update() {}
	virtual void setVisible(bool visible) = 0;
};

/// The in-game interface's lifecycle (src/hci.cpp)
void hudInitialize();
void hudShutdown();
void hudUpdate();
/// Each frame, before the widgets are drawn
void hudFrame();
/// The scripts showed or hid the interface (showInterface(), hideInterface())
void hudInterfaceShown(bool shown);

/// Rebuild what the settings changed (modules, panel additions): at once if it is safe, otherwise once the design or
/// intelligence screen, the in-game menu or the mission results are closed. The open panel is opened again.
void hudRequestRebuild();
bool hudRebuildPending();
/// The skin (or the tooltip delay) changed: applies what the skin sets globally. A skin change also rebuilds the
/// panels, whose size depends on panelTopPadding()
void hudSkinChanged();

/// The free zones (logical screen coordinates) where modules may go. Empty when the zone has no room.
/// Z1: the column between the reticule and the object row, at the bottom
WzRect hudStatusZone();
/// Z2: under the top edge, clear of the buttons in its corners
WzRect hudTopZone();
/// Z3: above the radar, right-aligned
WzRect hudRadarZone();

#endif // __INCLUDED_SRC_HUD_HUD_H__
