/*
	This file is part of Warzone 2100.
	Copyright (C) 2008-2020  Warzone 2100 Project

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

#ifndef __INCLUDED_LIB_SEQUENCE_TIMER_H__
#define __INCLUDED_LIB_SEQUENCE_TIMER_H__

void   Timer_Init();
void   Timer_start();					// start timer
void   Timer_stop();					// stop the timer
double Timer_getElapsedMilliSecs();		// get elapsed time in milliseconds
double Timer_getElapsedMicroSecs();		// get elapsed time in microseconds

#if defined(WZ_OS_WIN)
# include <winsock2.h> /* for struct timeval */

struct timezone;
int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

#endif // __INCLUDED_LIB_SEQUENCE_TIMER_H__
