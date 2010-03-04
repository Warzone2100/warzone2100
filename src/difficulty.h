/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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

#ifndef __INCLUDED_SRC_DIFFICULTY_H__
#define __INCLUDED_SRC_DIFFICULTY_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

typedef enum _difficulty_level
{
	DL_EASY,
	DL_NORMAL,
	DL_HARD,
	DL_TOUGH,
	DL_KILLER
} DIFFICULTY_LEVEL;

void setDifficultyLevel(DIFFICULTY_LEVEL lev);
DIFFICULTY_LEVEL getDifficultyLevel(void);
int modifyForDifficultyLevel(int basicVal, bool IsPlayer);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_SRC_DIFFICULTY_H__
