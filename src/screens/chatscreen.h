/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/form.h"
#include "lib/framework/wzstring.h"

#include <functional>

enum class WzChatMode
{
	Team,
	Glob
};

/// Sends what the local player typed, as the chat screen's box does: cheat codes, and in All the "." (allies) and number
/// (players) prefixes. Team goes to allies. Nothing is sent when free chat is off (the console says why).
void chatSendTypedText(const WzString &text, WzChatMode mode);
/// Whether the local player may type messages: the host's permission and the "Quick Chat Only" option
bool chatFreeChatEnabled();
bool chatFreeChatDisabledByHost();
/// Whether quick chat is there for the local player (skirmish and multiplayer, players but not spectators)
bool chatQuickChatAvailable();

/// The chat screen closes on a click outside its widgets, except inside the cutout, which it leaves to the screens below
std::shared_ptr<W_SCREEN> createChatScreen(std::function<void ()> onCloseFunc, WzChatMode initialChatMode, bool startWithQuickChatFocused, const std::shared_ptr<WIDGET> &cutout = nullptr);
void shutdownChatScreen();
