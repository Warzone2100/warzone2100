/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2008  Warzone Resurrection Project
	Copyright (C) 2008  Giel van Schijndel

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
/** \file
 *  Database stat loading functions
 */

#ifndef __INCLUDED_STATS_DB_H__
#define __INCLUDED_STATS_DB_H__

#include "lib/framework/frame.h"

// Forward declaration so that we can use pointers to sqlite3 databases
struct sqlite3;

extern bool loadWeaponStatsFromDB(struct sqlite3* db, const char* tableName);
extern bool loadBodyStatsFromDB(struct sqlite3* db, const char* tableName);
extern bool loadBrainStatsFromDB(struct sqlite3* db, const char* tableName);
extern bool loadPropulsionStatsFromDB(struct sqlite3* db, const char* tableName);
extern bool loadSensorStatsFromDB(struct sqlite3* db, const char* tableName);
extern bool loadECMStatsFromDB(struct sqlite3* db, const char* tableName);

#endif // __INCLUDED_STATS_DB_H__
