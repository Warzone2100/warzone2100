/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
#include "parsetest.h"
#include "levels.h"

#define ANSI_COLOUR_DEFAULT "\x1B[m"
#define ANSI_COLOUR_RED     "\x1B[31m"
#define ANSI_COLOUR_YELLOW  "\x1B[33m"

#define TEST(TEST_NAME, valid_files, invalid_files, doParseTest) \
static inline bool TEST_NAME(void) \
{ \
	bool retval = true; \
	unsigned int i; \
\
	/* \
	 * Test the valid files, failure to parse is a unit test failure. \
	 */ \
	for (i = 0; i < ARRAY_SIZE(valid_files); ++i) \
	{ \
		fprintf(stdout, "\t\t[%s] Testing parsing of valid file \"%s\"\n", __FUNCTION__, valid_files[i].testname); \
		fflush(stdout); \
		if (!doParseTest(valid_files[i].content, strlen(valid_files[i].content))) \
		{ \
			fprintf(stdout, "\t\t\t[%s] " ANSI_COLOUR_RED "Failure while parsing valid test file \"" ANSI_COLOUR_YELLOW "%s" ANSI_COLOUR_RED "\"." ANSI_COLOUR_DEFAULT "\n", __FUNCTION__, valid_files[i].testname); \
			fflush(stdout); \
			retval = false; \
		} \
	} \
\
	/* \
	 * Test the invalid files, successfully parsing these is a unit test failure. \
	 */ \
	for (i = 0; i < ARRAY_SIZE(invalid_files); ++i) \
	{ \
		fprintf(stdout, "\t\t[%s] Testing parsing of invalid file \"%s\" (expect error debug-messages)\n", __FUNCTION__, invalid_files[i].testname); \
		fflush(stdout); \
		if (doParseTest(invalid_files[i].content, strlen(invalid_files[i].content))) \
		{ \
			fprintf(stdout, "\t\t\t[%s] " ANSI_COLOUR_RED "Parsing of invalid test file \"" ANSI_COLOUR_YELLOW "%s" ANSI_COLOUR_RED "\" was successful, it shouldn't have been!" ANSI_COLOUR_DEFAULT "\n", __FUNCTION__, invalid_files[i].testname); \
			fflush(stdout); \
			retval = false; \
		} \
	} \
\
	return retval; \
}


static inline bool level_parse_test(const char* content, size_t size)
{
	bool retval = levParse(content, size, mod_clean, false);
	return retval;
}

static const test_files valid_level_files[] =
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

static const test_files invalid_level_files[] =
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

TEST(t1000, valid_level_files, invalid_level_files, level_parse_test)

void parseTest()
{
	bool success = true;

	fprintf(stdout, "\tParsing self-test...\n");

	success = success && t1000();

	fprintf(stdout, "\tParsing self-test: %s\n", success ? "PASSED" : "FAILED");
}
