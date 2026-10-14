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

#ifndef __INCLUDED_SRC_HUD_INTEL_INTELNOTICES_H__
#define __INCLUDED_SRC_HUD_INTEL_INTELNOTICES_H__

struct MESSAGE;

/// What the HUD's intelligence screen tells as things arrive, in the campaign: a toast for each research report or
/// briefing that doesn't play at once (with the Notices option, while the screen isn't up), with a View button that
/// opens the screen on it, and the count of unread items on the reticule's intelligence button.

void hudIntelAnnounce(const MESSAGE *message);
/// The count the reticule's intelligence button shows: 0 unless the button opens the HUD's screen
unsigned hudIntelButtonCount();

#endif // __INCLUDED_SRC_HUD_INTEL_INTELNOTICES_H__
