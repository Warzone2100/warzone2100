/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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
 *  Lobby Player Row Widget (and associated display functions)
 */

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"

class WzMultiplayerOptionsTitleUI;

class WzPlayerRow : public WIDGET
{
protected:
	WzPlayerRow(uint32_t playerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent);

public:
	static std::shared_ptr<WzPlayerRow> make(uint32_t playerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent);

	void geometryChanged() override;

	void updateState();

private:
	void updateReadyButton();

private:
	std::weak_ptr<WzMultiplayerOptionsTitleUI> parentTitleUI;
	unsigned playerIdx = 0;
	std::shared_ptr<W_BUTTON> teamButton;
	std::shared_ptr<W_BUTTON> colorButton;
	std::shared_ptr<W_BUTTON> playerInfo;
	std::shared_ptr<WIDGET> readyButtonContainer;
	std::shared_ptr<W_BUTTON> difficultyChooserButton;
	std::shared_ptr<W_BUTTON> readyButton;
	std::shared_ptr<W_LABEL> readyTextLabel;
};
