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
 *  The HUD's toasts: a short notice at the top center of the screen (under the spectator strip), one at a time, with an
 *  action ("View") and a close button. Toasts of the same group that come together make one ("3 research reports").
 */

#ifndef __INCLUDED_SRC_HUD_HUDTOAST_H__
#define __INCLUDED_SRC_HUD_HUDTOAST_H__

#include "lib/framework/geometry.h"

#include <functional>
#include <string>

struct HudToast
{
	std::string kind;                                       ///< over the title, in the accent's color ("Research complete")
	std::string title;
	/// Toasts of a group that come while one of them shows (or is waiting) make one, titled `groupTitle(count)`
	std::string group;
	std::function<std::string (size_t count)> groupTitle;
	std::function<void (const WzRect &rect)> drawPicture;   ///< in a square well at the toast's left
	std::string action;                                     ///< the action's button ("View"), when there's one
	std::function<void ()> onAction;
};

void hudToastShow(HudToast toast);
/// Takes the toast down and forgets the ones waiting (ex. the screen they'd point to opened)
void hudToastClearAll();
bool hudToastShown();

#endif // __INCLUDED_SRC_HUD_HUDTOAST_H__
