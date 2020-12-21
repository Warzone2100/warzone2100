/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
 *  Multiplayer Options Title UI. Used for skirmish, challenge and online skirmish options UIs.
 */

#ifndef __INCLUDED_SRC_TITLEUI_MULTIPLAYER_H__
#define __INCLUDED_SRC_TITLEUI_MULTIPLAYER_H__

#include "../wrappers.h"
#include "titleui.h"

class WzMultiplayerOptionsTitleUI : public WzTitleUI
{
public:
	WzMultiplayerOptionsTitleUI(std::shared_ptr<WzTitleUI> parent);
	virtual void start() override;
	virtual TITLECODE run() override;
	void frontendMultiMessages(bool running);

	void openDifficultyChooser(uint32_t playerIndex);
	void closeDifficultyChooser();

	void openAiChooser(uint32_t playerIndex);
	void closeAiChooser();

	void openPositionChooser(uint32_t playerIndex);
	void closePositionChooser();

	void openTeamChooser(uint32_t playerIndex);
	void closeTeamChooser();

	void openColourChooser(uint32_t playerIndex);
	void closeColourChooser();

	void closeAllChoosers();

	void screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight) override;
private:
	/**
	 * Initializes a chooser, preparing to add it on a single line on player list. This involves removing
	 * any widgets that may have been positioned on that player's row.
	 */
	void initInlineChooser(uint32_t playerIndex);

	/**
	 * Initializes the right side box which usually contains the list of players. Handles opening difficulty
	 * and ai choosers etc.
	 */
	void addPlayerBox(bool addPlayerEntries);

	/**
	 * Starts hosting. Used as an action for "Host Game" button. Another place where this is used is for
	 * activating the host immediately when entering the multiplayer menu with a challenge active.
	 */
	bool startHost();

	void processMultiopWidgets(UDWORD button);

	std::shared_ptr<W_SCREEN> psInlineChooserOverlayScreen = nullptr;
	std::shared_ptr<WzTitleUI> parent;
	bool performedFirstStart = false;

	int8_t inlineChooserUp;
	int8_t aiChooserUp;
	int8_t difficultyChooserUp;
	int8_t positionChooserUp;
};

#endif
