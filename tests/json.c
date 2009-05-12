/*
	This file is part of Warzone 2100.
	Copyright (C) 2008-2009  Giel van Schijndel
	Copyright (C) 2009       Warzone Resurrection Project

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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "lib/framework/json_value.h"
#include "lib/framework/json_parser.tab.h"
#include "lib/framework/lexer_input.h"

extern int json_parse(json_value** jsonAST);
extern int json_debug;
extern void json_set_debug(int bdebug);
extern void json_set_extra(YY_EXTRA_TYPE user_defined);
int json_lex_destroy(void);

static unsigned int parse_and_report(const char* filename)
{
	FILE* f;
	if (filename == NULL
	 || strcmp(filename, "-") == 0)
	{
		f = stdin;
		filename = "stdin";
	}
	else
	{
		f = fopen(filename, "r");
		if (f == NULL)
		{
			perror(filename);
			return EXIT_FAILURE;
		}
	}

	lexerinput_t input;
	input.type = LEXINPUT_STDIO;
	input.input.stdiofile = f;

	json_set_extra(&input);

#ifdef DEBUG
	json_set_debug(1);
#endif
	unsigned int err = EXIT_SUCCESS;
	json_debug = 0;
	json_value* jsonAST;
	switch (json_parse(&jsonAST))
	{
		case 0:
			fprintf(stderr, "%s: valid JSON\n", filename);
			char* serializedJson = serializeJsonValue(jsonAST);
			fprintf(stdout, "%s\n", serializedJson);
			free(serializedJson);
			freeJsonValue(jsonAST);
			break;
		case 1:
			fprintf(stderr, "%s: parsing failed\n", filename);
			err = EXIT_FAILURE;
			break;
		case 2:
			fprintf(stderr, "Out of memory!\n");
			err = EXIT_FAILURE;
			exit(EXIT_FAILURE);
			break;
	}

	json_lex_destroy();
	if (f != stdin)
		fclose(f);

	return err;
}

int main(int argc, char** argv)
{
	if (argc == 1)
		return parse_and_report(NULL);

	unsigned int err = EXIT_SUCCESS;
	for (int i = 1; i < argc; ++i)
	{
		if (parse_and_report(argv[i]) != EXIT_SUCCESS)
			err = EXIT_FAILURE;
	}

	return err;
}
