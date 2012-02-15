/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
#include "random.h"
#include "lib/netplay/netplay.h"

static MersenneTwister gamePseudorandomNumberGenerator;

MersenneTwister::MersenneTwister(uint32_t seed)
	: offset(624)
{
	// Set state to something pseudorandom. Exact constants aren't important here.
	state[0] = seed;
	for (unsigned i = 1; i != 624; ++i)
	{
		state[i] = 0x6C078965*(state[i - 1] ^ state[i - 1]>>30) + i;
	}
}

uint32_t MersenneTwister::u32()
{
	if (offset == 624)
	{
		generate();
	}

	uint32_t ret = state[offset++];

	// Give better than 624 dimensional equidistribution for lower bits.
	// Don't change the constants unless you know what you're doing.
	// Can skip this step if it noticably affects performance (which it probably doesn't), since we aren't doing Monte-Carlo simulations.
	ret ^= ret>>11;
	ret ^= ret<<7 & 0x9D2C5680;
	ret ^= ret<<15 & 0xEFC60000;
	ret ^= ret>>18;

	return ret;
}

void MersenneTwister::generate()
{
	offset = 0;

	// Don't change these constants, unless you have verified that the period is still 2**19937 - 1.
	// Discards the lower 31 bits of state[0], since the state is 19937 = 624*32 - 31 bits.
	// Loop tripled, to avoid using %624 everywhere.
	for (unsigned i = 0; i != 227; ++i)
	{
		int v = (state[i]&0x80000000) | (state[i + 1      ]&0x7FFFFFFF);
		state[i] = state[i + 397     ] ^ v>>1 ^ ((v&0x00000001)*0x9908B0DF);
	}
	for (unsigned i = 227; i != 623; ++i)
	{
		int v = (state[i]&0x80000000) | (state[i + 1      ]&0x7FFFFFFF);
		state[i] = state[i + 397 - 624] ^ v>>1 ^ ((v&0x00000001)*0x9908B0DF);
	}
	for (unsigned i = 623; i != 624; ++i)  // Very short loop.
	{
		int v = (state[i]&0x80000000) | (state[i + 1 - 624]&0x7FFFFFFF);
		state[i] = state[i + 397 - 624] ^ v>>1 ^ ((v&0x00000001)*0x9908B0DF);
	}
}

void gameSRand(uint32_t seed)
{
	gamePseudorandomNumberGenerator = MersenneTwister(seed);
}

uint32_t gameRandU32()
{
	return gamePseudorandomNumberGenerator.u32();
}

int32_t gameRand(uint32_t limit)
{
	syncDebug("Used a random number.");
	return gamePseudorandomNumberGenerator.u32()%limit;
}
