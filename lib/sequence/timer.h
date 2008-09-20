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
#ifndef _timer_h
#define _timer_h

	void   Timer_Init(void);
	void   Timer_start(void);					// start timer
	void   Timer_stop(void);					// stop the timer
	double Timer_getElapsedSecs(void);			// get elapsed time in seconds
	double Timer_getElapsedMilliSecs(void);		// get elapsed time in milliseconds
	double Timer_getElapsedMicroSecs(void);		// get elapsed time in microseconds

#endif
