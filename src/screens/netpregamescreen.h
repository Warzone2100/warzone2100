// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2023-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/form.h"

#include <cstdint>

std::shared_ptr<W_SCREEN> createGameStartScreen(std::function<void ()> onCloseFunc);
void shutdownGameStartScreen();

/// The team planning stage's length, and the final countdown's
constexpr int WZ_TEAMPLANNING_COUNTDOWN_MS = 20000;
constexpr int WZ_FINAL_COUNTDOWN_MS = 3000;

/// The host kicks a player who is still loading, or starts a vote to (with more than two players, unless a spectator)
void pregameKickLoadingPlayer(uint32_t player);
/// Whether the local player can do that to `player` (the host, a human player still loading)
bool pregameCanKickLoadingPlayer(uint32_t player);
/// Whether it would start a vote rather than kick
bool pregameKickStartsVote(uint32_t player);
