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
 *  The chat bar: a Team / All switch, the field, and a button for the quick chat panel, which opens above the bar.
 *  It sends as the in-game chat screen does (chatSendTypedText()), with its rules.
 *  Embedded, it's a card's bottom row (the in-game console's), and quick chat is a strip under it.
 */

#ifndef __INCLUDED_SRC_HUD_HUDCHATBAR_H__
#define __INCLUDED_SRC_HUD_HUDCHATBAR_H__

#include "../screens/chatscreen.h"

#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/widget.h"

#include <functional>
#include <memory>

class HudQuickChat;
class W_EDITBOX;

/// Below this screen width, the chat bar is compact
constexpr int HUD_CHAT_BAR_COMPACT_WIDTH = 680;
namespace intelkit { class IntelButton; }

/// The Team / All switch (the chat bar's and quick chat's)
class HudChannelSwitch : public WIDGET
{
public:
	/// `mode` reads the channel and `onChosen` sets it. Without `teamEnabled`, Team is shown off
	static std::shared_ptr<HudChannelSwitch> make(bool compact, std::function<WzChatMode ()> mode, std::function<void (WzChatMode)> onChosen,
	                                              bool teamEnabled = true);
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;

private:
	bool compact = false;
	std::shared_ptr<WIDGET> teamButton;
	std::shared_ptr<WIDGET> allButton;
};

class HudChatBar : public WIDGET
{
public:
	/// Compact: shorter, the quick chat button an icon
	static std::shared_ptr<HudChatBar> make(bool compact, WzChatMode mode);
	static int barHeight(bool compact);
	~HudChatBar() override;

	/// As a card's bottom row: no card of its own (the field is framed while typing), with quick chat under it
	void setEmbedded(bool value);
	/// Shows the bar without taking typing: the field (or Enter's binding) and the quick chat button call these instead
	/// (ex. to open the chat screen, whose own bar takes this one's place)
	void setIdle(std::function<void ()> onTypeRequested, std::function<void ()> onQuickChatRequested);

	void startTyping();
	void stopTyping();
	bool isTyping() const;
	WzChatMode mode() const { return chatMode; }
	void setMode(WzChatMode newMode);

	/// Quick chat as a strip under the bar, for as long as the bar is there (the chat screen's, in the console)
	void showQuickChatStrip();
	/// The strip's height, when it changes
	std::function<void (int)> onStripResized;

	void toggleQuickChat();
	bool quickChatOpen() const { return quickChat != nullptr; }
	void closeQuickChat();

	/// A message sent, or the typing stopped
	std::function<void ()> onFinished;
	/// The channel changed (the switch, Ctrl+Tab, or quick chat's own switch)
	std::function<void (WzChatMode)> onModeChanged;

	int32_t idealHeight() override;
	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;

private:
	void initialize(bool compactLayout, WzChatMode initialMode);

	bool compact = false;
	bool embedded = false;
	bool stripShown = false;
	bool spectator = false;
	std::function<void ()> quickChatRequested;
	WzChatMode chatMode = WzChatMode::Glob;
	std::shared_ptr<HudChannelSwitch> channelSwitch;
	std::shared_ptr<WIDGET> field;
	std::shared_ptr<W_EDITBOX> box;
	std::shared_ptr<intelkit::IntelButton> quickButton;
	std::shared_ptr<HudQuickChat> quickChat;
	WzText hintKey;
	WzText hintText;
	WzText spectatorsText;
};

#endif // __INCLUDED_SRC_HUD_HUDCHATBAR_H__
