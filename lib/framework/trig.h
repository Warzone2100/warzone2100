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
/*! \file trig.h
 *  * \brief Interface to trig lookup tables
 */
#ifndef _trig_h
#define _trig_h

/* The number of units around a full circle */
#define TRIG_DEGREES	360

/* conversion macros */
#define DEG_TO_RAD(x)	(x * M_PI / 180.0)
#define RAD_TO_DEG(x)	(x * 180.0 / M_PI)

#endif
