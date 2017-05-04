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
#ifndef _RAND_H_
#define _RAND_H_

#include "lib/framework/types.h"

// Pseudorandom number generator with 19937 bit state and period of 2**19937 - 1. Equidistributed in up to 623 dimensions.
// See http://en.wikipedia.org/wiki/Mersenne_twister
// If all clients use the same seed, they will generate the same pseudorandom number sequence.
class MersenneTwister
{
public:
	MersenneTwister(uint32_t seed = 42);
	uint32_t u32();  ///< Generates a random number in the interval [0...UINT32_MAX].

private:
	void generate();  ///< Generates more random numbers.

	int offset;  ///< Index in state.
	uint32_t state[624];  ///< 19937 bit state, rounded up to next 32 bits.
};

/// Seeds the random number generator. The seed is sent over the network, such that all clients generate the same number sequence, without the number sequence being the same each game.
void gameSRand(uint32_t seed);

/// Generates a random number in the interval [0...UINT32_MAX].
/// Must not be called from graphics routines, only for making game decisions.
uint32_t gameRandU32();

/// Generates a random number in the interval [0...limit - 1]. Not equidistributed for large non-powers of 2.
/// Must not be called from graphics routines, only for making game decisions.
int32_t gameRand(uint32_t limit);

#endif //_RAND_H_
