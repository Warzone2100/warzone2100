/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Giel van Schijndel
	Copyright (C) 2008  Warzone Resurrection Project

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
#include <stdlib.h>
#include "src/levels.h"

// Copied from levels.c to make linking easier
LEVEL_DATASET* psLevels = NULL;
LEVEL_DATASET* levFindDataSet(const char* name)
{
	LEVEL_DATASET* psNewLevel;

	for (psNewLevel = psLevels; psNewLevel; psNewLevel = psNewLevel->psNext)
	{
		if (psNewLevel->pName != NULL
		 && strcmp(psNewLevel->pName, name) == 0)
		{
			return psNewLevel;
		}
	}

	return NULL;
}

static inline void doCleanUp(void)
{
	while (psLevels != NULL)
	{
		unsigned int i;
		LEVEL_DATASET * const toDelete = psLevels;
		psLevels = psLevels->psNext;
		for (i = 0; i < ARRAY_SIZE(toDelete->apDataFiles); ++i)
		{
			if (toDelete->apDataFiles[i])
				free(toDelete->apDataFiles[i]);
		}
		free(toDelete->pName);
		free(toDelete);
	}
}

static inline BOOL doParseTest(const char* content, size_t size)
{
	BOOL retval = levParse(content, size, mod_clean);
	doCleanUp();
	return retval;
}

#include "parse_test.h"

static const test_files valid_files[] =
{
	{
		"test1: empty file",
		""
	},
	{
		"test2: fully commented out",
		"/*\n"
		"level PROG\n"
		"data \"wrf/somewrf.wrf\"\n"
		"data \"wrf/yetanother.wrf\"\n"
		"game \"maps/somemap.gam\"\n"
		"*/"
	},
	{
		"test3: single level entry",
		"level PROG\n"
		"data \"wrf/somewrf.wrf\"\n"
		"data \"wrf/yetanother.wrf\"\n"
		"game \"maps/somemap.gam\"\n"
	},
	{
		"test4: Custom type number",
		"level PROG\n"
		/* NOTE: This type number will need a change when LEVEL_TYPE in src/levels.h changes (it should be at least LDS_MULTI_TYPE_START) */
		"type 11\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test5: Custom player count",
		"level PROG\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test6: Custom player count and level type (1)",
		"level PROG\n"
		"players 4\n"
		"type 1000\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test7: Custom player count and level type (2)",
		"level PROG\n"
		"type 1000\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test8: Assign a game to a non-`campaign` dataset",
		"level PROG\n"
		"game \"maps/somemap.gam\"\n"
	},
};

static const test_files invalid_files[] =
{
	{
		"test1: wrongly ordered level_entry (1)",
		"level PROG\n"
		"data \"wrf/somewrf.wrf\"\n"
		"players 4\n"
	},
	{
		"test2: wrongly ordered level_entry (2)",
		"level PROG\n"
		"data \"wrf/somewrf.wrf\"\n"
		"type 20\n"
	},
	{
		"test3: wrongly ordered level_entry (3)",
		"level PROG\n"
		"type 20\n"
		"data \"wrf/somewrf.wrf\"\n"
		"players 4\n"
	},
	{
		"test4: wrongly ordered level_entry (4)",
		"level PROG\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
		"type 20\n"
	},
	{
		"test5: Bad type number",
		"level PROG\n"
		/* NOTE: This type number will need a change when LEVEL_TYPE in src/levels.h changes (it should be lower than LDS_MULTI_TYPE_START) */
		"type 10\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test6: Changing type of non-`level` dataset  (1)",
		"campaign PROG\n"
		"type 1000\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test7: Changing type of non-`level` dataset  (2)",
		"camstart PROG\n"
		"type 1000\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test8: Changing type of non-`level` dataset  (3)",
		"camchange PROG\n"
		"type 1000\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test9: Changing type of non-`level` dataset  (4)",
		"expand PROG\n"
		"type 1000\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test10: Changing type of non-`level` dataset (5)",
		"expand_limbo PROG\n"
		"type 1000\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test11: Changing type of non-`level` dataset (6)",
		"between PROG\n"
		"type 1000\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test12: Changing type of non-`level` dataset (7)",
		"miss_keep PROG\n"
		"type 1000\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test13: Changing type of non-`level` dataset (8)",
		"miss_keep_limbo PROG\n"
		"type 1000\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test14: Changing type of non-`level` dataset (9)",
		"miss_clear PROG\n"
		"type 1000\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test15: Changing player count of non-`level` dataset (1)",
		"campaign PROG\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test16: Changing player count of non-`level` dataset (2)",
		"camstart PROG\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test17: Changing player count of non-`level` dataset (3)",
		"camchange PROG\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test18: Changing player count of non-`level` dataset (4)",
		"expand PROG\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test19: Changing player count of non-`level` dataset (5)",
		"expand_limbo PROG\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test20: Changing player count of non-`level` dataset (6)",
		"between PROG\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test21: Changing player count of non-`level` dataset (7)",
		"miss_keep PROG\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test22: Changing player count of non-`level` dataset (8)",
		"miss_keep_limbo PROG\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test23: Changing player count of non-`level` dataset (9)",
		"miss_clear PROG\n"
		"players 4\n"
		"data \"wrf/somewrf.wrf\"\n"
	},
	{
		"test24: Assign a game to a `campaign` dataset",
		"campaign PROG\n"
		"game \"maps/somemap.gam\"\n"
	},
	{
		"test25: Assign multiple games to a dataset",
		"level PROG\n"
		"game \"maps/somemap.gam\"\n"
		"game \"maps/yetanothermap.gam\"\n"
	},
};

#define TEST_NAME t1000
#include "parse_test.c"
