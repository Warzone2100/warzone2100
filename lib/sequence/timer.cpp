/*
	This file is part of Warzone 2100.
	Copyright (C) 2008-2017  Warzone 2100 Project

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


#include "lib/framework/frame.h"
#include "timer.h"

#if defined(WZ_OS_UNIX)
# include <sys/time.h>
#endif

static double startTimeInMicroSec = 0;			// starting time in microseconds
static double endTimeInMicroSec = 0;			// ending time in microseconds
static bool   stopped = false;					// stop flag

static struct timeval startCount;
static struct timeval endCount;

#if defined(WZ_OS_WIN)
/**
 * The difference between the FAT32 and Unix epoch.
 *
 * The FAT32 epoch starts at 1 January 1601 while the Unix epoch starts at 1
 * January 1970. And apparently we gained 3.25 days in that time period.
 *
 * Thus the amount of micro seconds passed between these dates can be computed
 * as follows:
 * \f[((1970 - 1601) \cdot 365.25 + 3.25) \cdot 86400 \cdot 1000000\f]
 *
 * Use 1461 and 13 instead of 365.25 and 3.25 respectively because we can't use
 * floating point math here.
 */
static const uint64_t usecs_between_fat32_and_unix_epoch = (uint64_t)((1970 - 1601) * 1461 + 13) * (uint64_t)86400 / (uint64_t)4 * (uint64_t)1000000;

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	ASSERT(tz == NULL, "This gettimeofday implementation doesn't provide timezone info.");

	if (tv)
	{
		FILETIME ft;
		uint64_t systime, usec;

		/* Retrieve the current time expressed as 100 nano-second
		 * intervals since 1 January 1601 (UTC).
		 */
		GetSystemTimeAsFileTime(&ft);
		systime = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;

		// Convert to micro seconds since 1 January 1970 (UTC).
		usec = systime / 10 - usecs_between_fat32_and_unix_epoch;

		tv->tv_sec  = usec / (uint64_t)1000000;
		tv->tv_usec = usec % (uint64_t)1000000;
	}

	return 0;
}
#endif

// Uses the highest resolution timers avail on windows & linux
void Timer_Init(void)
{
	startCount.tv_sec = startCount.tv_usec = 0;
	endCount.tv_sec = endCount.tv_usec = 0;

	stopped = false;
	startTimeInMicroSec = 0;
	endTimeInMicroSec = 0;
}

void Timer_start(void)
{
	stopped = false; // reset stop flag
	gettimeofday(&startCount, nullptr);
}

void Timer_stop(void)
{
	stopped = true; // set timer stopped flag

	gettimeofday(&endCount, nullptr);
}

double Timer_getElapsedMicroSecs(void)
{
	if (!stopped)
	{
		gettimeofday(&endCount, nullptr);
	}
	startTimeInMicroSec = (startCount.tv_sec * 1000000.0) + startCount.tv_usec;
	endTimeInMicroSec = (endCount.tv_sec * 1000000.0) + endCount.tv_usec;

	return endTimeInMicroSec - startTimeInMicroSec;
}

double Timer_getElapsedMilliSecs(void)
{
	return Timer_getElapsedMicroSecs() * 0.001;
}
