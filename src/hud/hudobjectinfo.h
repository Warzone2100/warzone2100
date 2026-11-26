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

/** @file
 *  What a factory or research facility is doing, read the same way by every HUD addition.
 */

#ifndef __INCLUDED_SRC_HUD_HUDOBJECTINFO_H__
#define __INCLUDED_SRC_HUD_HUDOBJECTINFO_H__

#include <cstdint>
#include <vector>

#include <nonstd/optional.hpp>

struct BASE_STATS;
struct DROID_TEMPLATE;
struct STRUCTURE;

struct HudJob
{
	BASE_STATS *subject = nullptr;  ///< the design being made or the topic researched (none when idle)
	unsigned totalPoints = 0;
	unsigned donePoints = 0;
	unsigned pointsPerSecond = 0;
	unsigned queued = 0;            ///< (factories) units still to make in the production run, this one included
	unsigned queuedOfSubject = 0;   ///< (factories) units of the current design still to make, this one included
	int loops = 0;                  ///< (factories) the production loops left, or INFINITE_PRODUCTION
	bool onHold = false;
	nonstd::optional<int32_t> powerNeeded;  ///< waiting for this much power

	unsigned percentDone() const;
	unsigned secondsLeft() const;
};

/// For the local player's factories and research facilities (with the UI's pending state). None for anything else
nonstd::optional<HudJob> hudStructureJob(STRUCTURE *psStruct);

struct HudQueueEntry
{
	DROID_TEMPLATE *psTemplate = nullptr;
	unsigned remaining = 0;
};

/// What a factory still has to make, in the order it will make it: the current design first
std::vector<HudQueueEntry> hudFactoryQueue(STRUCTURE *psStruct);

#endif // __INCLUDED_SRC_HUD_HUDOBJECTINFO_H__
