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
 *  Detail popovers: rich tooltips for HUD elements, shown as an overlay beside the element (taking no input).
 *  With touch, a long-press opens one instead (the peek), pinned once the finger lifts, with the element's
 *  secondary actions as buttons.
 */

#ifndef __INCLUDED_SRC_HUD_HUDDETAIL_H__
#define __INCLUDED_SRC_HUD_HUDDETAIL_H__

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "lib/framework/geometry.h"
#include "lib/framework/wzstring.h"
#include <memory>

class WIDGET;

enum class HudDetailPlacement : uint8_t
{
	Right,
	Above,
};

/// A secondary action offered in a pinned popover (the touch long-press): what the element's right-click does
struct HudDetailAction
{
	WzString label;
	std::string icon;   ///< (a named icon, images/hud/icons/<icon>.svg)
	WzString note;      ///< (after the label, ex. a count)
	bool destructive = false;
	bool enabled = true;
	std::function<void ()> run;
};

/// A HUD widget that can describe itself in a detail popover (instead of its simple tooltip)
class HudDetailSource
{
public:
	virtual ~HudDetailSource() = default;
	/// The popover's contents, or none (the simple tooltip shows then)
	virtual std::shared_ptr<WIDGET> makeHudDetail() = 0;
	/// The touch long-press's popover, when it can say something even where the hover's can't (ex. an empty group)
	virtual std::shared_ptr<WIDGET> makeHudDetailPeek() { return makeHudDetail(); }
	/// The actions a long-press's popover offers
	virtual std::vector<HudDetailAction> hudDetailActions() { return {}; }
	virtual bool hasHudDetail() { return true; }
	virtual HudDetailPlacement hudDetailPlacement() const { return HudDetailPlacement::Above; }
	/// The ID of the panel the popover goes beside (rather than beside the widget itself), ex. the object row
	virtual uint32_t hudDetailPanelId() const { return 0; }
};

/// For widgets that aren't HudDetailSources: describe them all the same
struct HudDetailProvider
{
	std::function<std::shared_ptr<WIDGET> ()> make;
	HudDetailPlacement placement = HudDetailPlacement::Above;
	uint32_t panelId = 0;
	std::function<std::vector<HudDetailAction> ()> actions;
};
void hudDetailRegister(const std::shared_ptr<WIDGET> &widget, const HudDetailProvider &provider);

/// Each frame, before the widgets run: show, swap, refresh or close the popover
void hudDetailUpdate();
void hudDetailClose();
bool hudDetailShowing();
/// Whether a long-press's popover is open (held or pinned)
bool hudDetailPeeking();
/// Whether a long-press's popover stays open after the finger lifted
bool hudDetailPinned();
/// (For tests) Where a pinned popover's action buttons are, in order
std::vector<WzRect> hudDetailPeekActionRectsForTests();

/// Contents width the popovers aim for, and the most they may take of the screen's height
int hudDetailWidth();
int hudDetailMaxHeight();

#endif // __INCLUDED_SRC_HUD_HUDDETAIL_H__
