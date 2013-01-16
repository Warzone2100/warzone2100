/*
	This file is part of Warzone 2100.
	Copyright (C) 2008-2013  Warzone 2100 Project

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

typedef enum {
    SCANLINES_OFF,
    SCANLINES_50,
    SCANLINES_BLACK
} SCANLINE_MODE;

extern bool seq_Play(const char* filename);
extern bool seq_Playing(void);
extern bool seq_Update(void);
extern void seq_Shutdown(void);
extern int seq_GetFrameNumber(void);
extern void seq_SetDisplaySize(int sizeX, int sizeY, int posX, int posY);
extern void seq_setScanlineMode(SCANLINE_MODE mode);
extern SCANLINE_MODE seq_getScanlineMode(void);
double seq_GetFrameTime();

#endif // __INCLUDED_LIB_SEQUENCE_SEQUENCE_H__
