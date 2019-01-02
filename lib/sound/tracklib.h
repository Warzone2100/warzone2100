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
 *  Library-specific sound library functions;
 *  these need to be re-written for each library.
 */

#ifndef __INCLUDED_LIB_SOUND_TRACKLIB_H__
#define __INCLUDED_LIB_SOUND_TRACKLIB_H__

#include "track.h"
#include "lib/framework/vector.h"

bool	sound_InitLibrary();
void	sound_ShutdownLibrary();

void	sound_FreeTrack(TRACK *psTrack);

bool	sound_Play2DSample(TRACK *psTrack, AUDIO_SAMPLE *psSample,
                           bool bQueued);
bool	sound_Play3DSample(TRACK *psTrack, AUDIO_SAMPLE *psSample);
void	sound_StopSample(AUDIO_SAMPLE *psSample);
void	sound_PauseSample(AUDIO_SAMPLE *psSample);
void	sound_ResumeSample(AUDIO_SAMPLE *psSample);

AUDIO_STREAM *sound_PlayStream(PHYSFS_file *PHYSFS_fileHandle, float volume, void (*onFinished)(const void *), const void *user_data);

void	sound_SetSampleFreq(AUDIO_SAMPLE *psSample, SDWORD iFreq);
void	sound_SetSampleVol(AUDIO_SAMPLE *psSample, SDWORD iVol,
                           bool bScale3D);

int		sound_GetNumSamples();
bool	sound_SampleIsFinished(AUDIO_SAMPLE *psSample);
bool	sound_QueueSamplePlaying();

void	sound_SetPlayerPos(Vector3f pos);
void sound_SetPlayerOrientationVector(Vector3f forward, Vector3f up);
void sound_SetPlayerOrientation(float angle);
void	sound_SetObjectPosition(AUDIO_SAMPLE *psSample);

void 	*sound_GetObject(SDWORD iSample);
void	sound_SetObject(SDWORD iSample, void *pObj);

void	sound_SetCallback(SDWORD iSample, AUDIO_CALLBACK pCallBack);

void	sound_PauseAll();
void	sound_ResumeAll();
void	sound_StopAll();
void	sound_Update();
unsigned int sound_GetActiveSamplesCount();

UDWORD	sound_GetGameTime();

#endif	// __INCLUDED_LIB_SOUND_TRACKLIB_H__
