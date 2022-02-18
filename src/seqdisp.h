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

/** @file
 *  Functions for the display of the Escape Sequences
 */

#ifndef __INCLUDED_SRC_SEQDISP_H__
#define __INCLUDED_SRC_SEQDISP_H__

#include "lib/framework/types.h"
#include <string>

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

#define  SEQUENCE_KILL 3 //stop
#define  SEQUENCE_HOLD 4 //play once and hold last frame

#define  SEQUENCE_MIN_SKIP_DELAY 75 //amount of loops before skipping is allowed

// POSITION is for ordinary text, such as the name of the mission.
// JUSTIFy is for subtitles. It is more like center.
enum SEQ_TEXT_POSITIONING
{
	/**
	 * Position text.
	 */
	SEQ_TEXT_POSITION,

	/**
	 * Center if less than 520/600 length.
	 */
	SEQ_TEXT_CENTER,
};

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/

bool seq_RenderVideoToBuffer(const WzString &sequenceName, int seqCommand);

bool seq_UpdateFullScreenVideo(int *bClear);

bool seq_StopFullScreenVideo();

bool seq_GetVideoSize(SDWORD *pWidth, SDWORD *pHeight);

void seq_AddLineForVideo(const std::string& text, SDWORD xOffset, SDWORD yOffset, double startTime, double endTime, SEQ_TEXT_POSITIONING textJustification);

void seq_ClearSeqList();

void seq_AddSeqToList(const WzString &seqName, const WzString &audioName, const std::string& textFileName, bool loop);

bool seq_AnySeqLeft();

// set and check subtitle mode, true subtitles on
void seq_SetSubtitles(bool bNewState);
bool seq_GetSubtitles();

/* start the next sequence in the list to play */
void seq_StartNextFullScreenVideo();

void seqReleaseAll();

#endif	// __INCLUDED_SRC_SEQDISP_H__
