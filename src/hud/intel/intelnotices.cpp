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

#include "intelnotices.h"
#include "intelkit.h"
#include "intelmodel.h"
#include "intelpanel.h"

#include "../hudsettings.h"
#include "../hudtoast.h"

#include "lib/framework/frame.h"

#include "../../messagedef.h"
#include "../../multiplay.h"

void hudIntelAnnounce(const MESSAGE *message)
{
	// (Campaign only: in a skirmish or multiplayer game, reports come too often to announce)
	if (bMultiPlayer || !hudSettings().toasts || !hudIntelScreenEnabled() || hudIntelScreenIsOpen())
	{
		return;
	}
	const nonstd::optional<HudIntelEntry> entry = hudIntelEntryFor(message);
	if (!entry.has_value())
	{
		return;
	}
	HudToast toast;
	toast.title = entry->title;
	const HudIntelKind kind = entry->kind;
	const std::string key = entry->key;
	if (kind == HudIntelKind::Research)
	{
		toast.kind = _("Research complete");
		toast.group = "research";
		toast.groupTitle = [](size_t count) {
			return astringf(ngettext("%u research report", "%u research reports", static_cast<unsigned>(count)), static_cast<unsigned>(count));
		};
		const RESEARCH *research = entry->research;
		toast.drawPicture = [research](const WzRect &rect) {
			intelkit::drawResearchModel(research, rect);
		};
	}
	else
	{
		toast.kind = kind == HudIntelKind::Briefing ? _("New briefing") : _("New update");
		toast.group = "briefing";
		toast.groupTitle = [](size_t count) {
			return astringf(ngettext("%u new message", "%u new messages", static_cast<unsigned>(count)), static_cast<unsigned>(count));
		};
		toast.drawPicture = [](const WzRect &rect) {
			const int s = std::min(16, rect.height() - 8);
			intelkit::skin().drawNamedIcon("replay-play", WzRect(rect.x() + (rect.width() - s) / 2, rect.y() + (rect.height() - s) / 2, s, s),
			                               intelkit::skin().colors().accent);
		};
	}
	toast.action = _("View");
	toast.onAction = [kind, key]() {
		hudIntelScreenShowEntry(static_cast<int>(kind), key);
	};
	hudToastShow(std::move(toast));
}

unsigned hudIntelButtonCount()
{
	// (Campaign only, like the notices)
	if (bMultiPlayer || !hudIntelScreenEnabled() || selectedPlayer >= MAX_PLAYERS)
	{
		return 0;
	}
	return static_cast<unsigned>(hudIntelUnreadCount());
}
