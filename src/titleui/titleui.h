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
 *  The various title screen UIs go here.
 */

#ifndef __INCLUDED_SRC_TITLEUI_TITLEUI_H__
#define __INCLUDED_SRC_TITLEUI_TITLEUI_H__

// tMode
#include "../frontend.h"
// TITLECODE
#include "../wrappers.h"

#include <memory>
#include <functional>

// Regarding construction vs. start():
// This allows a reference to the parent to be held for a stack-like effect.
class WzTitleUI : public std::enable_shared_from_this<WzTitleUI>
{
public:
	virtual ~WzTitleUI();
	virtual void start() = 0;
	// NOTE! When porting, add screen_disableMapPreview(); if relevant!
	virtual TITLECODE run() = 0;
	virtual void screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight);
};

// Pointer to the current UI. Dynamic allocation helps with the encapsulation.
extern std::shared_ptr<WzTitleUI> wzTitleUICurrent;

extern char serverName[128];

void changeTitleUI(std::shared_ptr<WzTitleUI> ui);

// - old.cpp -
class WzOldTitleUI: public WzTitleUI
{
public:
	WzOldTitleUI(tMode mode);
	virtual void start() override;
	virtual TITLECODE run() override;
	virtual void screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight) override;
private:
	tMode mode;
};

// - protocol.cpp -
class WzProtocolTitleUI: public WzTitleUI
{
public:
	WzProtocolTitleUI();
	virtual void start() override;
	virtual TITLECODE run() override;
	virtual void screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight) override;
private:
	void openIPDialog();
	void closeIPDialog();
	// Not-null: the settings screen is up
	std::shared_ptr<W_SCREEN> psSettingsScreen = nullptr;
	// If true, there's an IP address waiting to be used.
	bool hasWaitingIP = false;
};

// - multiint.cpp (defined in titleui/multiplayer.h) -
class WzMultiplayerOptionsTitleUI;

// - multilimit.cpp -
class WzMultiLimitTitleUI: public WzTitleUI
{
public:
	WzMultiLimitTitleUI(std::shared_ptr<WzMultiplayerOptionsTitleUI> parent);
	virtual void start() override;
	virtual TITLECODE run() override;
private:
	// The parent WzMultiplayerOptionsTitleUI to return to.
	std::shared_ptr<WzMultiplayerOptionsTitleUI> parent;
};

// - msgbox.cpp -
class WzMsgBoxTitleUI: public WzTitleUI
{
public:
	WzMsgBoxTitleUI(WzString title, WzString text, std::shared_ptr<WzTitleUI> next);
	virtual void start() override;
	virtual TITLECODE run() override;
private:
	WzString title;
	WzString text;
	// Where to go after the user has acknowledged.
	std::shared_ptr<WzTitleUI> next;
};

// - passbox.cpp -
class WzPassBoxTitleUI: public WzTitleUI
{
public:
	// The callback receives nullptr for cancellation, or a widgGetString result otherwise.
	// The callback is expected to change current UI.
	WzPassBoxTitleUI(std::function<void(const char *)> next);
	virtual void start() override;
	virtual TITLECODE run() override;
private:
	// Where to go after the user has acknowledged.
	std::function<void(const char *)> next;
};

// - gamefind.cpp -
class WzGameFindTitleUI: public WzTitleUI
{
public:
	WzGameFindTitleUI();
	~WzGameFindTitleUI();
	virtual void start() override;
	virtual TITLECODE run() override;
private:
	void addGames();
	void addConsoleBox();
	bool safeSearch = false; // allow auto game finding.
	bool toggleFilter = true; // Used to show all games or only games that are of the same version
	bool queuedRefreshOfGamesList = false;
};

#define WZ_MSGBOX_TUI_LEAVE 4597000

void mpSetServerName(const char *hostname);
const char *mpGetServerName();

#endif
