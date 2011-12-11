/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

#define  SEQUENCE_PLAY 0//play once and exit
#define  SEQUENCE_LOOP 1//loop till stopped externally
#define  SEQUENCE_PAUSE 2//pause time
#define  SEQUENCE_KILL 3//stop
#define  SEQUENCE_HOLD 4//play once and hold last frame

enum SEQ_TEXT_POSITIONING
{
	/**
	 * Position text.
	 */
	SEQ_TEXT_POSITION,

	/**
	 * Justify if less than 3/4 length.
	 */
	SEQ_TEXT_FOLLOW_ON,

	/**
	 * Justify if less than 520/600 length.
	 */
	SEQ_TEXT_JUSTIFY,
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
//buffer render
extern bool seq_RenderVideoToBuffer(const char* sequenceName, int seqCommand);

extern bool seq_UpdateFullScreenVideo(int *bClear);

extern bool seq_StopFullScreenVideo(void);
//control
extern bool seq_GetVideoSize(SDWORD* pWidth, SDWORD* pHeight);
//text
extern bool seq_AddTextForVideo(const char* pText, SDWORD xOffset, SDWORD yOffset, double startTime, double endTime, SEQ_TEXT_POSITIONING textJustification);
//clear the sequence list
extern void seq_ClearSeqList(void);
//add a sequence to the list to be played
extern void seq_AddSeqToList(const char *pSeqName, const char *pAudioName, const char *pTextName, bool bLoop);
/*checks to see if there are any sequences left in the list to play*/
extern bool seq_AnySeqLeft(void);

//set and check subtitle mode, true subtitles on
extern void seq_SetSubtitles(bool bNewState);
extern bool seq_GetSubtitles(void);

/*returns the next sequence in the list to play*/
extern void seq_StartNextFullScreenVideo(void);

#endif	// __INCLUDED_SRC_SEQDISP_H__
