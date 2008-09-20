/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

#ifdef WIN32
//#include <windows.h>
//#include <time.h>
#else
#include <sys/time.h>
#endif
#include "timer.h"
#include "lib/framework/frame.h"

static double startTimeInMicroSec = 0;			// starting time in microseconds
static double endTimeInMicroSec = 0;			// ending time in microseconds
static BOOL   stopped = false;					// stop flag 
#ifdef WIN32
static double freq = 0;
static LARGE_INTEGER frequency = {0};			// ticks per second
static LARGE_INTEGER startCount = {0};
static LARGE_INTEGER endCount = {0};  
#else
static struct timeval startCount;
static struct timeval endCount;
#endif

// Uses the highest resolution timers avail on windows & linux
void Timer_Init(void)
{
#ifdef WIN32
	// Note, may have to enable this for QPC!
	//SetThreadAffinityMask() 
	QueryPerformanceFrequency(&frequency);
	startCount.QuadPart = 0;
	endCount.QuadPart = 0;
	freq = 1000000.0f / frequency.QuadPart;
#else
	startCount.tv_sec = startCount.tv_usec = 0;
	endCount.tv_sec = endCount.tv_usec = 0;
#endif

	stopped = false;
	startTimeInMicroSec = 0;
	endTimeInMicroSec = 0;
}

//
void Timer_start(void)
{
	stopped = false; // reset stop flag
#ifdef WIN32
	QueryPerformanceCounter(&startCount);
#else
	gettimeofday(&startCount, NULL);
#endif
}
void Timer_stop(void)
{
	stopped = true; // set timer stopped flag

#ifdef WIN32
	QueryPerformanceCounter(&endCount);
#else
	gettimeofday(&endCount, NULL);
#endif
}
double Timer_getElapsedMicroSecs(void)
{
#ifdef WIN32
	if(!stopped)
	{
		QueryPerformanceCounter(&endCount);
	}
	startTimeInMicroSec = (double)startCount.QuadPart * freq;
	endTimeInMicroSec = (double)endCount.QuadPart * freq;
#else
	if(!stopped)
	{
		gettimeofday(&endCount, NULL);
	}
	startTimeInMicroSec = (startCount.tv_sec * 1000000.0) + startCount.tv_usec;
	endTimeInMicroSec = (endCount.tv_sec * 1000000.0) + endCount.tv_usec;
#endif

	return endTimeInMicroSec - startTimeInMicroSec;
}
double Timer_getElapsedMilliSecs(void)
{
	return Timer_getElapsedMicroSecs() * 0.001;// (or) / 1000.0f; * is faster?
}
double Timer_getElapsedSecs(void)
{
	return Timer_getElapsedMicroSecs()* 0.000001;	// (or) /1000000.0f; 
}
