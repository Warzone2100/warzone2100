/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#ifndef __INCLUDED_LIB_SOUND_CDAUDIO_H__
#define __INCLUDED_LIB_SOUND_CDAUDIO_H__

enum SONG_CONTEXT
{
	SONG_FRONTEND,
	SONG_INGAME,
};

bool cdAudio_Open(const char *user_musicdir);
void cdAudio_Close(void);
bool cdAudio_PlayTrack(SONG_CONTEXT context);
void cdAudio_Stop(void);
void cdAudio_Pause(void);
void cdAudio_Resume(void);

#endif // __INCLUDED_LIB_SOUND_CDAUDIO_H__
