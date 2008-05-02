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

#ifndef TEST_NAME
# error You must #define TEST_NAME for this unit test lib.
#endif

#define ANSI_COLOUR_DEFAULT "\x1B[m"
#define ANSI_COLOUR_RED     "\x1B[31m"
#define ANSI_COLOUR_YELLOW  "\x1B[33m"

static int TEST_NAME(void)
{
	int retval = EXIT_SUCCESS;
	unsigned int i;

	/*
	 * Test the valid files, failure to parse is a unit test failure.
	 */
	for (i = 0; i < ARRAY_SIZE(valid_files); ++i)
	{
		debug(LOG_NEVER, "Testing parsing of valid file \"%s\"", valid_files[i].testname);
		if (!levParse(valid_files[i].content, strlen(valid_files[i].content), mod_clean))
		{
			debug(LOG_ERROR, ANSI_COLOUR_RED "Failure while parsing valid test file \"" ANSI_COLOUR_YELLOW "%s" ANSI_COLOUR_RED "\"." ANSI_COLOUR_DEFAULT, valid_files[i].testname);
			retval = EXIT_FAILURE;
		}
		doCleanUp();
	}

	/*
	 * Test the invalid files, succesfully parsing these is a unit test failure.
	 */
	for (i = 0; i < ARRAY_SIZE(invalid_files); ++i)
	{
		debug(LOG_NEVER, "Testing parsing of invalid file \"%s\" (expect error messages)", invalid_files[i].testname);
		if (levParse(invalid_files[i].content, strlen(invalid_files[i].content), mod_clean))
		{
			debug(LOG_ERROR, ANSI_COLOUR_RED "Parsing of invalid test file \"" ANSI_COLOUR_YELLOW "%s" ANSI_COLOUR_RED "\" was succesfull, it shouldn't have been!" ANSI_COLOUR_DEFAULT, invalid_files[i].testname);
			retval = EXIT_FAILURE;
		}
		doCleanUp();
	}

	return retval;
}

#ifdef main
# undef main
#endif

extern int main(void);
int main()
{
	debug_init();

	debug_register_callback( debug_callback_stderr, NULL, NULL, NULL );
#if defined(WZ_OS_WIN) && defined(DEBUG_INSANE)
	debug_register_callback( debug_callback_win32debug, NULL, NULL, NULL );
#endif // WZ_OS_WIN && DEBUG_INSANE

	return TEST_NAME();
}
