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
 */

#ifndef __INCLUDED_SRC_HUD_HUDCHATBAR_H__
#define __INCLUDED_SRC_HUD_HUDCHATBAR_H__

#include "../screens/chatscreen.h"

#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/widget.h"

#include <functional>
#include <memory>

class W_EDITBOX;
class W_FORM;
namespace intelkit { class IntelButton; }

class HudChatBar : public WIDGET
{
public:
	/// Compact: shorter, the quick chat button an icon
	static std::shared_ptr<HudChatBar> make(bool compact, WzChatMode mode);
	~HudChatBar() override;

	void startTyping();
	void stopTyping();
	bool isTyping() const;
	WzChatMode mode() const { return chatMode; }
	void setMode(WzChatMode newMode);

	void toggleQuickChat();
	bool quickChatOpen() const { return quickChat != nullptr; }
	void closeQuickChat();

	/// A message sent, or the typing stopped
	std::function<void ()> onFinished;

	int32_t idealHeight() override;
	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;

private:
	void initialize(bool compactLayout, WzChatMode initialMode);

	bool compact = false;
	bool spectator = false;
	WzChatMode chatMode = WzChatMode::Glob;
	std::shared_ptr<WIDGET> teamButton;
	std::shared_ptr<WIDGET> allButton;
	std::shared_ptr<WIDGET> field;
	std::shared_ptr<W_EDITBOX> box;
	std::shared_ptr<intelkit::IntelButton> quickButton;
	std::shared_ptr<W_FORM> quickChat;
	WzText hintKey;
	WzText hintText;
	WzText spectatorsText;
};

#endif // __INCLUDED_SRC_HUD_HUDCHATBAR_H__
