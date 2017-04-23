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
#ifndef _WAVE_CAST_H_
#define _WAVE_CAST_H_

struct WavecastTile
{
	int16_t dx, dy;            ///< Tile coordinates.
	int32_t invRadius;         ///< Arbitrary constant divided by radius.
	int16_t angBegin, angEnd;  ///< Start and finish angles for obstruction of view. Non-linear units, for comparison purposes only.
};


// Not thread safe if someone calls with a new radius. Thread safe, otherwise.
const WavecastTile *getWavecastTable(unsigned radius, size_t *size);

#endif //_WAVE_CAST_H_
