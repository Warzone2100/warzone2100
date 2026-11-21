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
 *  Quick chat in the Modern skin: the sections in a rail, the chosen section's messages in two columns of five.
 *  Two number keys send a message, as in the Classic panel (src/hci/quickchat.cpp): the section's, then the message's.
 *  The messages and recipients are the Classic panel's (quickChatInGameSections()), with Team / All instead of tabs,
 *  and End-Game a section of All. Under the console's chat bar it's a strip instead.
 */

#ifndef __INCLUDED_SRC_HUD_HUDQUICKCHAT_H__
#define __INCLUDED_SRC_HUD_HUDQUICKCHAT_H__

#include "../hci/quickchat.h"
#include "../screens/chatscreen.h"

#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/form.h"

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class HudChannelSwitch;
namespace intelkit { class IntelButton; }

/// Whether the skin draws quick chat
bool hudQuickChatEnabled();

class HudQuickChat : public W_FORM
{
public:
	/// On Team (when there are teammates) or All. Compact: a narrower rail
	static std::shared_ptr<HudQuickChat> make(bool team, bool compact);
	/// As a strip under the console's chat bar: the sections in a row, the open one a tab over its messages. The bar's
	/// switch sets its channel, and typing keeps the keys until Tab
	static std::shared_ptr<HudQuickChat> makeStrip(bool team, bool compact);
	bool isStrip() const { return strip; }
	/// The strip's height at a width (its open section's messages included)
	int stripHeight(int w);
	/// Tab from typing: the sections take the keys
	void takeFocusFromTyping();
	/// The strip's height may have changed (a section opened or closed, a page turned, the channel changed)
	std::function<void ()> onResized;

	/// Takes the keys (the screen's focus)
	void takeFocus();
	WzChatMode mode() const { return chatMode; }
	void setMode(WzChatMode newMode);

	/// A message sent: close it
	std::function<void ()> onSent;
	/// Enter: type instead
	std::function<void ()> onTypeRequested;
	/// Team / All chosen here (Ctrl+Tab, the switch)
	std::function<void (WzChatMode)> onModeChanged;

	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void geometryChanged() override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key = WKEY_PRIMARY) override;
	void run(W_CONTEXT *psContext) override;
	void display(int xOffset, int yOffset) override;

	struct Section
	{
		std::string title;
		std::vector<WzQuickChatMessage> messages;
		bool everyone = false;   ///< sent to all whatever the channel (End of game)
	};
	/// Whether its number keys apply: the section's (false), or the messages' (true)
	bool choosingMessage() const { return messageLevel; }
	bool hasKeys() const;
	size_t sectionCount() const { return sections.size(); }
	const Section &sectionAt(size_t index) const;
	size_t currentSection() const { return section; }
	const Section &shownSection() const { return sections[section]; }
	size_t currentPage() const { return page; }
	size_t pageCount() const;
	/// The message in a slot of the shown page, if any
	const WzQuickChatMessage *messageAt(size_t slot) const;
	/// The slot just sent, flashing
	int flashingSlot() const;

	void chooseSection(size_t index);
	/// A click on a section: opens it, or closes the strip's open one
	void clickSection(size_t index);
	/// The strip shows its open section's messages
	bool sectionShown() const;
	void send(size_t slot);
	void setPage(size_t newPage);

private:
	void initialize(bool team, bool compactLayout);
	void rebuildSections();
	int layoutStrip(int w, bool apply);
	void displayStrip(const WzRect &rect);
	void notifyResized();
	void openRecipients();
	void updateRecipientsLabel();
	WzQuickChatTargeting teamTargeting() const;
	void handleKeys();

	bool compact = false;
	bool strip = false;
	bool teamAvailable = false;
	/// The strip, on the bar's Team without teammates: nothing to send
	bool noTeammates = false;
	WzChatMode chatMode = WzChatMode::Glob;
	std::vector<Section> sections;
	size_t section = 0;
	size_t page = 0;
	bool messageLevel = false;
	int sentSlot = -1;
	uint32_t sentAt = 0;
	bool closing = false;

	std::vector<uint32_t> humans;
	std::vector<uint32_t> bots;
	std::unordered_set<uint32_t> chosen;

	std::shared_ptr<HudChannelSwitch> channelSwitch;
	std::shared_ptr<intelkit::IntelButton> toButton;
	std::vector<std::shared_ptr<WIDGET>> sectionRows;
	std::array<std::shared_ptr<WIDGET>, 10> messageCells;
	std::shared_ptr<WIDGET> pagePrevious;
	std::shared_ptr<WIDGET> pageNext;
	WzRect trayRect;
	WzRect pagerRect;
	WzText pageText;
	WzText measureKey;
	WzText noTeammatesText;
	WzText titleText;
	std::vector<WzText> hintTexts;
	WzText endText;
};

#endif // __INCLUDED_SRC_HUD_HUDQUICKCHAT_H__
