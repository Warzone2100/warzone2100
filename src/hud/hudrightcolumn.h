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

#ifndef __INCLUDED_SRC_HUD_HUDRIGHTCOLUMN_H__
#define __INCLUDED_SRC_HUD_HUDRIGHTCOLUMN_H__

#include <memory>

class WIDGET;

/// The right column: under the in-game options button, right-aligned with its card, the HUD's panels on the right
/// stack top to bottom in this order, a gap apart, so none overlaps another and each keeps its place in the order.
enum class HudRightColumnSlot
{
	Connection,   ///< the connection warnings (hudconnection.h)
	Count
};

/// Puts a widget in its slot (it keeps its own size and visibility, the column only places it)
void hudRightColumnSet(HudRightColumnSlot slot, const std::shared_ptr<WIDGET> &widget);
/// Places the column's visible widgets, each frame after they are updated
void hudLayoutRightColumn();

#endif // __INCLUDED_SRC_HUD_HUDRIGHTCOLUMN_H__
