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
/** \file
	The in-game console: the feed of recent messages at the top of the screen.
*/

#ifndef __INCLUDED_SRC_HCI_INGAMECONSOLE_H__
#define __INCLUDED_SRC_HCI_INGAMECONSOLE_H__

#include "lib/widget/widget.h"

#include <memory>

/// Attaches the console to the root form of the in-game screen
void inGameConsoleAdd(const std::shared_ptr<W_SCREEN> &screen);
/// Moves the console back above any form attached after it (ex. the HUD's)
void inGameConsoleKeepOnTop();

#endif // __INCLUDED_SRC_HCI_INGAMECONSOLE_H__
