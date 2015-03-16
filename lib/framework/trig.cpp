/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
/*
 * Trig.c
 *
 * Trig lookup tables
 *
 */

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "types.h"
#include "trig.h"
#include "crc.h"

#include <assert.h>
#include <stdlib.h>
// for M_PI
#if defined(WZ_CC_MSVC)
#define _USE_MATH_DEFINES
#endif

#include <math.h>

static uint16_t trigSinTable[0x4001];
static uint16_t trigAtanTable[0x2001];

/* Initialise the Trig tables */
bool trigInitialise(void)
{
	uint64_t test;
	uint32_t crc;
	uint32_t i;

	// Generate tables.
	STATIC_ASSERT(sizeof(trigSinTable) / sizeof(*trigSinTable) == 0x4001);
	for (i = 0; i != 0x4001; ++i)
	{
		trigSinTable[i] = (int)(0x10000 * sin(i * (M_PI / 0x8000)) + 0.5) - !!i; // -!!i = subtract 1, unless i == 0.
	}
	STATIC_ASSERT(sizeof(trigAtanTable) / sizeof(*trigAtanTable) == 0x2001);
	for (i = 0; i != 0x2001; ++i)
	{
		trigAtanTable[i] = (int)(0x8000 / M_PI * atan((double)i / 0x2000) + 0.5);
	}

	// Check tables are correct.
	crc = ~crcSumU16(0, trigSinTable, sizeof(trigSinTable) / sizeof(*trigSinTable));
	ASSERT(crc == 0x6D3C8DB5, "Bad trigSinTable CRC = 0x%08X, sin function is broken.", crc);
	crc = ~crcSumU16(0, trigAtanTable, sizeof(trigAtanTable) / sizeof(*trigAtanTable));
	ASSERT(crc == 0xD2797F85, "Bad trigAtanTable CRC = 0x%08X, atan function is broken.", crc);

	// Test large and small square roots.
	for (test = 0x0000; test != 0x10000; ++test)
	{
		uint64_t lower = test * test;
		uint64_t upper = (test + 1) * (test + 1) - 1;
		ASSERT((uint32_t)iSqrt(lower) == test, "Sanity check failed, sqrt(%" PRIu64") gave %" PRIu32" instead of %" PRIu64"!", lower, i64Sqrt(lower), test);
		ASSERT((uint32_t)iSqrt(upper) == test, "Sanity check failed, sqrt(%" PRIu64") gave %" PRIu32" instead of %" PRIu64"!", upper, i64Sqrt(upper), test);
	}
	for (test = 0xFFFE0000; test != 0x00020000; test = (test + 1) & 0xFFFFFFFF)
	{
		uint64_t lower = test * test;
		uint64_t upper = (test + 1) * (test + 1) - 1;
		ASSERT((uint32_t)i64Sqrt(lower) == test, "Sanity check failed, sqrt(%" PRIu64") gave %" PRIu32" instead of %" PRIu64"!", lower, i64Sqrt(lower), test);
		ASSERT((uint32_t)i64Sqrt(upper) == test, "Sanity check failed, sqrt(%" PRIu64") gave %" PRIu32" instead of %" PRIu64"!", upper, i64Sqrt(upper), test);
	}

	return true;
}

int32_t iSin(uint16_t a)
{
	int sign[4] = {1, 1, -1, -1};
	bool reverse[4] = {false, true, false, true};
	int      q = a >> 14;
	uint16_t r = a & 0x3FFF;
	uint16_t rvr = reverse[q] ? 0x4000 - r : r;
	return sign[q] * (trigSinTable[rvr] + !!rvr); // +!!rvr = add 1, unless rvr == 0.
}

int32_t iCos(uint16_t a)
{
	int sign[4] = {1, -1, -1, 1};
	bool reverse[4] = {true, false, true, false};
	int      q = a >> 14;
	uint16_t r = a & 0x3FFF;
	uint16_t rvr = reverse[q] ? 0x4000 - r : r;
	return sign[q] * (trigSinTable[rvr] + !!rvr); // +!!rvr = add 1, unless rvr == 0.
}

int32_t iSinR(uint16_t a, int32_t r)
{
	return ((int64_t)r * iSin(a)) / 65536;
}

int32_t iCosR(uint16_t a, int32_t r)
{
	return ((int64_t)r * iCos(a)) / 65536;
}

int32_t iSinSR(int32_t a, int32_t s, int32_t r)
{
	return ((int64_t)r * iSin(((int64_t)a << 16) / s)) / 65536;
}

int32_t iCosSR(int32_t a, int32_t s, int32_t r)
{
	return ((int64_t)r * iCos(((int64_t)a << 16) / s)) / 65536;
}

uint16_t iAtan2(int32_t s, int32_t c)
{
	uint16_t d = 0;         // Dummy initialisation.
	uint32_t j = 0, k = 0;  // Dummy initialisations.

	if (s == 0 && c == 0)
	{
		return 0;       // All return values equally valid, don't divide by 0.
	}

	switch (((s < 0) << 1) + (c < 0))
	{
	case 0: j =  s; k =  c; d = 0x0000; break;
	case 1: j = -c; k =  s; d = 0x4000; break;
	case 2: j =  c; k = -s; d = 0xC000; break;
	case 3: j = -s; k = -c; d = 0x8000; break;
	}
	return j < k ? d +          trigAtanTable[((int64_t)j * 0x2000 + k / 2) / k]
	       : d + 0x4000 - trigAtanTable[((int64_t)k * 0x2000 + j / 2) / j];
}

int32_t iSqrt(uint32_t n)
{
	uint32_t r = sqrt((double)n);          // Calculate square root, rounded down. Excess precision does not change the result.

	// Check that we got the right result.
	ASSERT((int32_t)(r * r - n) <= 0 && (int32_t)((r + 1) * (r + 1) - n) > 0, "Too badly broken sqrt function, iSqrt(%u) = %u.", (unsigned)n, (unsigned)r);

	return r;
}

int32_t i64Sqrt(uint64_t n)
{
	uint64_t r;
	if (sizeof(void *) > 4)
	{
		r = sqrt((double)n);          // Calculate square root, usually rounded down. Excess precision may result in rounding down instead of up, which is fine.
	}
	else
	{
		// Bad compiler workaround. On some compilers, sqrt() seems to have somehow been taking 64-bit doubles and returning 80-bit doubles, breaking expected rounding behaviour.
		r = sqrtl(n);         // Calculate square root, usually rounded down. Excess precision may result in rounding down instead of up, which is fine.
	}

	r -= (int64_t)(r * r - n) > 0; // If we rounded up, subtract 1.

	// Check that we got the right result.
	ASSERT((int64_t)(r * r - n) <= 0 && (int64_t)((r + 1) * (r + 1) - n) > 0, "Too badly broken sqrt function, i64Sqrt(%" PRIu64") = %" PRIu64".", n, r);

	return r;
}

int32_t iHypot(int32_t x, int32_t y)
{
	return i64Sqrt((uint64_t)x * x + (uint64_t)y * y); // Casting from int32_t to uint64_t does sign extend.
}

int32_t iHypot3(int32_t x, int32_t y, int32_t z)
{
	return i64Sqrt((uint64_t)x * x + (uint64_t)y * y + (uint64_t)z * z); // Casting from int32_t to uint64_t does sign extend.
}

// For testing above functions.
#if 0
int main()
{
	srand(time(NULL));
	for (unsigned i = 0; i != 1000000; ++i)
	{
		int32_t s = rand() % 2 ? rand() + rand() + rand() + rand() : rand() % 101 - 50;
		int32_t c = rand() % 2 ? rand() + rand() + rand() + rand() : rand() % 101 - 50;
		int a1 = uint16_t(round(32768 / M_PI * atan2(s, c))), a2 = iAtan2(s, c);
		if (a1 + 1 < a2 || a1 > a2 + 1)
		{
			std::printf("32768/π atan2(%d, %d) = %d, iAtan2() = %d\n", s, c, a1, a2);
		}
	}
	for (unsigned a = 0; a < 65536; ++a)
	{
		int s1 = round(65536 * sin(a * M_PI / 32768)), s2 = iSin(a);
		int c1 = round(65536 * cos(a * M_PI / 32768)), c2 = iCos(a);
		if (s1 != s2 || c1 != c2)
		{
			std::printf("a = %d, sin = %d, %d, cos = %d, %d\n", a, s1, s2, c1, c2);
		}
	}
}
#endif
