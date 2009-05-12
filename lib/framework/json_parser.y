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
%{
/** @file
 *
 *  Parser for JSON (JavaScript Object Notation)
 */

extern int json_lex(void);
extern int json_get_lineno(void);
extern char* json_get_text(void);

#include "frame.h"
#include "debug.h"
#include "types.h"
#include <stdlib.h>
#include <string.h>
#include "json_value.h"
#include "json_parser.tab.h"

#ifdef DEBUG
# define YYDEBUG 1
#endif

static void yyerror(json_value** jsonAST, const char* error)
{
	debug(LOG_ERROR, "%s at line %d (while scanning '%s')", error, json_get_lineno(), json_get_text());
}

typedef struct __json_element
{
	struct __json_value*            value;
	struct __json_element*          next;
} json_element;
%}

%name-prefix="json_"

/* The Abstract Syntax Tree resulting from our parsing process */
%parse-param {json_value** jsonAST}

%union
{
	char*                   sval;
	long int                ival;
	double                  fval;
	uint8_t                 u8val;
	uint32_t                u32val;
	json_value*             jval;
	json_value_pair*        jvalpair;
	struct __json_element*  jvalelem;
}

/* String tokens */
%token          STRING_START    // Marks the start of a string
%token <u8val>  ASCII_CHAR      // A single ASCII character
%token <sval>   STRING          // Non-value string termination token
/* String rules */
%type <jval>    json_string

/* Number tokens */
%token <ival>   INTEGER         // A number that can be represented as an integer without loss of precision
%token <fval>   FLOAT           // A number that can only be represented as a floating point
/* Number rules */
%type <jval>    json_number

/* Boolean tokens */
%token          TTRUE
%token          TFALSE
/* Boolean rules */
%type <jval>    json_bool

/* NULL tokens */
%token          TNULL
/* NULL rules */
%type <jval>    json_null

	/* Special tokens */
%token TINVALID  /* Represents erroneous tokens which shouldn't be where they are. */

	/* rule types */
%type <jval> json_object
%type <jvalpair> json_object_members member
%type <jval> json_value
%type <jval> json_array
%type <jvalelem> json_array_elements

%destructor {
	// Force type checking by the compiler
	char * s = $$;

	if (s != NULL)
		free(s);
} STRING

%destructor {
	// Force type checking by the compiler
	json_value* v = $$;

	if (v != NULL)
		freeJsonValue(v);
} json_value json_string json_number json_object json_array json_bool json_null

%destructor {
	// Force type checking by the compiler
	json_value_pair* v = $$;

	if (v != NULL)
		freeJsonValuePair(v);
} json_object_members member
%%

json_file:              json_value
				{
					*jsonAST = $1;
				}
                        ;

json_object:            '{' '}'
				{
					$$ = malloc(sizeof(*$$));
					if ($$ == NULL)
					{
						debug(LOG_ERROR, "Out of memory!");
						abort();
						YYABORT;
					}

					$$->type = JSON_OBJECT;
					$$->value.members = NULL;
				}
                      | '{' json_object_members '}'
				{
					$$ = malloc(sizeof(*$$));
					if ($$ == NULL)
					{
						debug(LOG_ERROR, "Out of memory!");
						abort();
						YYABORT;
					}

					$$->type = JSON_OBJECT;
					$$->value.members = $2;
				}
                        ;

json_object_members:    member
				{
					$$ = $1;
					$$->next = NULL;
				}
                      | member ',' json_object_members
				{
					$$ = $1;
					$$->next            = $3;
				}
                        ;

member:                 STRING ':' json_value
				{
					$$ = createJsonValuePair($1, $3);

					// Clean up our string parameter (as we own it)
					free($1);
#ifdef DEBUG
					$1 = NULL;
#endif
				}
                        ;

json_value:             json_string
                      | json_number
                      | json_object
                      | json_array
                      | json_bool
                      | json_null
                        ;

json_array:             '[' ']'
				{
					$$ = createJsonArray(0);
					if ($$ == NULL)
						YYABORT;
				}
                      | '[' json_array_elements ']'
				{
					size_t i, count;
					json_element* cur;

					// Determine the amount of elements
					count = 0;
					for (cur = $2; cur != NULL; cur = cur->next)
					{
						count += 1;
					}

					// Allocate memory for the elements
					$$ = createJsonArray(count);
					if ($$ == NULL)
						YYABORT;

					// Copy all elements into the array and delete the originals
					cur = $2;
					for (i = 0; assert(i <= $$->value.array.size), cur != NULL; ++i)
					{
						json_element * const ele = cur;
						cur = cur->next;

						// Copy the element
						$$->value.array.a[i] = ele->value;

						// Delete the original
						free(ele);
					}
				}
                        ;

json_array_elements:    json_value
				{
					$$ = malloc(sizeof(*$$));
					if ($$ == NULL)
					{
						debug(LOG_ERROR, "Out of memory!");
						abort();
						YYABORT;
					}

					$$->value = $1;
					$$->next = NULL;
				}
                      | json_value ',' json_array_elements
				{
					$$ = malloc(sizeof(*$$));
					if ($$ == NULL)
					{
						debug(LOG_ERROR, "Out of memory!");
						abort();
						YYABORT;
					}

					$$->value = $1;
					$$->next = $3;
				}
                        ;

json_number:            INTEGER
				{
					$$ = createJsonInteger($1);
					if ($$ == NULL)
						YYABORT;
				}
                      | FLOAT
				{
					$$ = createJsonFloat($1);
					if ($$ == NULL)
						YYABORT;
				}
                        ;

json_bool:              TTRUE
				{
					$$ = createJsonBool(true);
					if ($$ == NULL)
						YYABORT;
				}
                      | TFALSE
				{
					$$ = createJsonBool(false);
					if ($$ == NULL)
						YYABORT;
				}
                        ;

json_null:              TNULL
				{
					$$ = malloc(sizeof(*$$));
					if ($$ == NULL)
						YYABORT;

					$$->type = JSON_NULL;
				}
                        ;

json_string:            STRING
				{
					$$ = createJsonString($1);
					if ($$ == NULL)
						YYABORT;

					// Clean up our string parameter (as we own it)
					free($1);
#ifdef DEBUG
					$1 = NULL;
#endif
				}
                        ;
%%
