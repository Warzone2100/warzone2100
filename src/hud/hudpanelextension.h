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
 *  Panel extensions: a panel grows away from its anchor to hold a module's controls, while everything it already had
 *  keeps its screen position, inside a content anchor.
 */

#ifndef __INCLUDED_SRC_HUD_HUDPANELEXTENSION_H__
#define __INCLUDED_SRC_HUD_HUDPANELEXTENSION_H__

#include "lib/widget/widget.h"

#include <functional>
#include <memory>

/// How far a panel grows for its extension
struct HudExtensionFit
{
	int top = 0;
	int right = 0;
	/// The room to the right is a tab this high at the top, not a strip down the panel's side
	int rightTabHeight = 0;
};

/// What a module puts in a panel extension
class HudExtensionContent : public WIDGET
{
public:
	/// Its size, given the panel's Classic size and the room above and to the right of it.
	/// The content folds (or hides, returning nothing) when it doesn't fit.
	virtual HudExtensionFit fit(int panelWidth, int panelHeight, int availableHeightAbove, int availableWidthRight) = 0;
};

struct HudPanelExtensionSpec
{
	uint32_t contentId = 0;
	std::function<WzRect ()> classicRect;
	std::function<int (const WzRect &classic)> availableHeightAbove;
	std::function<int (const WzRect &classic)> availableWidthRight;
	/// Moved to the extension's top-right corner, which the content leaves free (hudCloseButtonGutter)
	std::shared_ptr<WIDGET> closeButton;
	/// How much of the top of the panel's contents a band may cover: rows the content takes over (ex. page tabs)
	int bandOverlap = 0;
	/// Rows at the top of the panel's contents it no longer shows: the panel's top edge moves down by as much (or up,
	/// when negative)
	std::function<int ()> topInset;
	/// After each layout of the panel
	std::function<void ()> afterLayout;
};

/// Wraps the panel's children in a content anchor, and grows the panel for `content` (if any) on every layout,
/// and up by the skin's panelTopPadding() when nothing else takes the room above the contents
void hudExtendPanel(const std::shared_ptr<WIDGET> &panel, const std::shared_ptr<HudExtensionContent> &content, const HudPanelExtensionSpec &spec);
/// The width a top extension leaves free at its right end, and the height a side one leaves free at its top
int hudCloseButtonGutter();
/// A panel's close button that its extension moved
bool hudIsMovedCloseButton(const WIDGET &widget);
/// One of a panel's own controls that its extension moved, inside the panel
void hudSetMovedPanelControl(const std::shared_ptr<WIDGET> &widget);
bool hudIsMovedPanelControl(const WIDGET &widget);
/// A Classic widget that a skin may place a little differently in its row, for what it draws beside it (the mission
/// timer's time, after the Modern skin's stopwatch)
void hudSetSkinPlacedWidget(const std::shared_ptr<WIDGET> &widget);
bool hudIsSkinPlacedWidget(const WIDGET &widget);
/// The rows at the top of an extended panel's contents that it no longer shows (HudPanelExtensionSpec::topInset)
int hudPanelTopInset(const WIDGET &panel);
/// A list's page tabs, which an extension hides while it shows them itself
void hudSetReplacedPageTabs(const std::shared_ptr<WIDGET> &tabs);
bool hudIsReplacedPageTabs(const WIDGET &widget);
/// The content's size may have changed
void hudRelayoutPanelOf(WIDGET &content);

/// The band under the screen's top edge kept for the console, mission timer and transporter button
int hudTopBand();

#endif // __INCLUDED_SRC_HUD_HUDPANELEXTENSION_H__
