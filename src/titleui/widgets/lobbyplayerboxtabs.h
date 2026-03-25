/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2026  Warzone 2100 Project

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
 *  Lobby Player Box Tabs Widget (and associated display functions)
 */

#pragma once

#include "lib/widget/widget.h"

class WzMultiplayerOptionsTitleUI;
class WzPlayerBoxTabButton;
class WzPlayerBoxOptionsButton;
class PopoverMenuWidget;

class WzPlayerBoxTabs : public WIDGET
{
public:
	enum class PlayerDisplayView
	{
		Players,
		Spectators
	};

protected:
	WzPlayerBoxTabs(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI);
	~WzPlayerBoxTabs();

public:
	static std::shared_ptr<WzPlayerBoxTabs> make(bool displayHostOptions, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI);

	void geometryChanged() override;
	void run(W_CONTEXT *psContext) override;

	PlayerDisplayView getDisplayView() const;
	void setDisplayView(PlayerDisplayView view);

private:
	void setSelectedTab(size_t index);

	void refreshData();

	void recalculateTabLayout();

	void displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent);
	std::shared_ptr<PopoverMenuWidget> createOptionsPopoverForm();

private:
	std::weak_ptr<WzMultiplayerOptionsTitleUI> weakTitleUI;
	std::vector<std::shared_ptr<WzPlayerBoxTabButton>> tabButtons;
	std::shared_ptr<WzPlayerBoxOptionsButton> optionsButton;
	std::shared_ptr<PopoverMenuWidget> currentPopoverMenu;
	PlayerDisplayView playerDisplayView = PlayerDisplayView::Players;
};
