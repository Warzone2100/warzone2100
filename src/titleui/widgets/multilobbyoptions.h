// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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
/** \file
 *  Multiplayer lobby options
 */

#pragma once

#include "lib/widget/widget.h"
#include <array>

class WzMultiplayerOptionsTitleUI;

struct LimitIcon
{
	char const *stat;
	char const *desc;
	int         icon;
};

extern std::array<LimitIcon, 6> limitIcons;

class WzMultiLobbyOptionsWidgetBase : public WIDGET
{
public:
	virtual void refreshData() = 0;
};

std::shared_ptr<WzMultiLobbyOptionsWidgetBase> makeWzMultiLobbyOptionsForm(bool isChallenge, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI);
