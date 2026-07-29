// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2008-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#ifndef __INCLUDED_LIB_SEQUENCE_SEQUENCE_H__
#define __INCLUDED_LIB_SEQUENCE_SEQUENCE_H__

#include "lib/framework/types.h"
#include "video_provider.h"
#include <memory>

typedef enum
{
	SCANLINES_OFF,
	SCANLINES_50,
	SCANLINES_BLACK
} SCANLINE_MODE;

bool seq_Play(std::shared_ptr<VideoProvider> video);
bool seq_Playing();
/** Set the preferred FMV audio-track language (a WZ locale code, e.g. "de").
 * Empty / nullptr (the default) = automatic: follow the game language.
 * Takes effect from the next seq_Play(). Videos without a matching track
 * fall back to their English track. */
void seq_SetPreferredAudioLanguage(const char *languageCode);
bool seq_Update();
void seq_Shutdown();
int seq_GetFrameNumber();
void seq_SetDisplaySize(int sizeX, int sizeY, int posX, int posY);
void seq_setScanlinesDisabled(bool flag);
bool seq_getScanlinesDisabled();
void seq_setScanlineMode(SCANLINE_MODE mode);
SCANLINE_MODE seq_getScanlineMode();
double seq_GetFrameTime();

#endif // __INCLUDED_LIB_SEQUENCE_SEQUENCE_H__
