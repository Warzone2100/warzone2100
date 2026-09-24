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

#ifndef __INCLUDED_SRC_HUD_HUDMODEL_H__
#define __INCLUDED_SRC_HUD_HUDMODEL_H__

#include <cstdint>
#include <deque>

#include <nonstd/optional.hpp>

/// A small snapshot of the local player's game data that HUD modules read, refreshed a few times a second while the
/// HUD runs (never headless). It keeps counts and object IDs only, never object pointers.
class HudModel
{
public:
	static constexpr uint32_t REFRESH_INTERVAL_MS = 250;
	static constexpr uint32_t INCOME_WINDOW_MS = 60 * 1000;

	struct Power
	{
		int current = 0;
		int afterQueued = 0;
		int queued = 0;
		int maxStorage = 0;
		/// Power extracted per minute, averaged over up to the last INCOME_WINDOW_MS of game time
		/// (including what is wasted at the storage cap). None until a second of game time was sampled
		nonstd::optional<int> incomePerMinute;
		uint32_t incomeWindowMs = 0;
		unsigned derricks = 0;
		unsigned unusedDerricks = 0;
		unsigned generators = 0;
	};

	/// Called by the HUD every REFRESH_INTERVAL_MS of real time
	void update();
	/// Forget everything time-based: a new game, an in-process restore or a replay seek
	void reset();

	const Power &power() const { return powerInfo; }

private:
	struct PowerSample
	{
		uint32_t gameTime;
		int64_t extracted;  ///< power extracted so far in the game (getExtractedPower)
	};

	void updatePower(unsigned player);

	Power powerInfo;
	std::deque<PowerSample> powerSamples;
	nonstd::optional<uint32_t> lastUpdateGameTime;
};

HudModel &hudModel();

#endif // __INCLUDED_SRC_HUD_HUDMODEL_H__
