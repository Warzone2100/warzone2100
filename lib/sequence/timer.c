/*
	This file is part of Warzone 2100.
	Copyright (C) 2008-2010  Warzone 2100 Project

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

#include "timer.h"
#include "lib/framework/frame.h"

#if defined(WZ_OS_UNIX)
# include <sys/time.h>
#endif

static double startTimeInMicroSec = 0;			// starting time in microseconds
static double endTimeInMicroSec = 0;			// ending time in microseconds
static BOOL   stopped = false;					// stop flag

static struct timeval startCount;
static struct timeval endCount;

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
	gettimeofday(&startCount, NULL);
}

void Timer_stop(void)
{
	stopped = true; // set timer stopped flag

	gettimeofday(&endCount, NULL);
}

double Timer_getElapsedMicroSecs(void)
{
	if (!stopped)
	{
		gettimeofday(&endCount, NULL);
	}
	startTimeInMicroSec = (startCount.tv_sec * 1000000.0) + startCount.tv_usec;
	endTimeInMicroSec = (endCount.tv_sec * 1000000.0) + endCount.tv_usec;

	return endTimeInMicroSec - startTimeInMicroSec;
}

double Timer_getElapsedMilliSecs(void)
{
	return Timer_getElapsedMicroSecs() * 0.001;
}
