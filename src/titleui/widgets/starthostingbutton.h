// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2025  Warzone 2100 Project

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
 *  Start Hosting Button
 */

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/button.h"
#include "../multiplayer.h"

class WzStartHostingButton : public W_BUTTON
{
protected:
	WzStartHostingButton();
	void initialize();
public:
	~WzStartHostingButton();
	static std::shared_ptr<WzStartHostingButton> make(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parentTitleUI);
protected:
	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
protected:
	friend class HostingAdvancedOptionsForm;
	std::shared_ptr<WzMultiplayerOptionsTitleUI> getMultiOptionsTitleUI();
private:
	std::shared_ptr<WIDGET> createOptionsPopoverForm();
	void displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent);
	void closeOptionsOverlay();
private:
	std::weak_ptr<WzMultiplayerOptionsTitleUI> multiOptionsTitleUI;
	WzText wzText;
	std::shared_ptr<W_SCREEN> optionsOverlayScreen;
	std::shared_ptr<W_BUTTON> optionsButton;
};
