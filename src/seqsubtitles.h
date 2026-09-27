// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#ifndef __INCLUDED_SRC_SEQSUBTITLES_H__
#define __INCLUDED_SRC_SEQSUBTITLES_H__

#include "lib/framework/wzstring.h"
#include <memory>

// Video text placed in areas of the video ("wz2100.subtitles.v1" files, see doc/SequenceSubtitlesFormat.md)
struct SeqSubtitles;

// Loads sequenceaudio/<fileName>, with the areas of its "areasFile" (nullptr if it can't be used)
std::shared_ptr<SeqSubtitles> seqSubtitles_Load(const WzString &fileName);
// Draws the lines shown at a video time, in the video's rectangle on screen
void seqSubtitles_Draw(SeqSubtitles &subtitles, double time, bool showSubtitles, int videoX, int videoY, int videoWidth, int videoHeight);
// Frees the drawn text and the video fonts (drawing lays it out again when needed)
void seqSubtitles_ReleaseLayout();

#endif // __INCLUDED_SRC_SEQSUBTITLES_H__
