/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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

#include <memory>

// tMode
#include "../frontend.h"
// TITLECODE
#include "../wrappers.h"

// Regarding construction vs. start():
// This allows a reference to the parent to be held for a stack-like effect.
class WzTitleUI
{
public:
	virtual ~WzTitleUI();
	virtual void start() = 0;
	// NOTE! When porting, add screen_disableMapPreview(); if relevant!
	virtual TITLECODE run() = 0;
	virtual void screenSizeDidChange();
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
	virtual void screenSizeDidChange() override;
private:
	tMode mode;
};

// - multiint.cpp -
class WzMultiOptionTitleUI: public WzTitleUI
{
public:
	WzMultiOptionTitleUI();
	virtual void start() override;
	virtual TITLECODE run() override;
private:
	bool performedFirstStart = false;
};

// WzMultiLimitTitleUI: multilimit.cpp/h

#endif
