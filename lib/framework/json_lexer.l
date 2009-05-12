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
/**
 *  @file
 *  Lexical analyser for JSON (JavaScript Object Notation)
 */

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "json_value.h"
#include "json_parser.tab.h"
#include "lib/framework/debug.h"
#include "lib/framework/lexer_input.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/utf.h"

#ifndef yyextra
# define yyextra yyget_extra()
#endif

#define yylval (&json_lval)

/* Older GNU Flex versions don't define yyget_extra(), yyset_extra(),
 * yyget_text() and yyget_lineno().
 * (and neither define a subminor version)
 */
#if !defined(YY_FLEX_SUBMINOR_VERSION) || (YY_FLEX_SUBMINOR_VERSION < 9)
# define yyget_extra  json_get_extra
# define yyset_extra  json_set_extra
# define yyget_lineno json_get_lineno
# define yyget_text   json_get_text
extern void yyset_extra(YY_EXTRA_TYPE user_defined);
extern YY_EXTRA_TYPE yyget_extra(void);
extern int yyget_lineno(void);
int yyget_lineno()
{
	return yylineno;
}

extern char* yyget_text(void);
char* yyget_text()
{
	return yytext;
}
#elif defined(YY_FLEX_SUBMINOR_VERSION) && YY_FLEX_SUBMINOR_VERSION == 33
extern YY_EXTRA_TYPE yyget_extra(void);
extern int json_get_lineno(void);
extern FILE *json_get_in(void);
extern FILE *json_get_out(void);
extern int json_get_leng(void);
extern char *json_get_text(void);
extern void json_set_lineno(int line_number);
extern void json_set_in(FILE* in_str);
extern void json_set_out(FILE* out_str);
extern int json_get_debug(void);
extern void json_set_debug(int bdebug);
#endif

static inline uint8_t HexCharToInt(const char c)
{
	if (c >= '0'
	 && c <= '9')
		return c - '0';
	else if (c >= 'A'
	      && c <= 'F')
		return c - 'A';
	else if (c >= 'a'
	      && c <= 'f')
		return c - 'a';
	else
		assert(!"Non hexadecimal character passed to HexCharToInt");
}

static inline uint16_t HexSeqToInt16(const char seq[4])
{
	const uint16_t d1 = HexCharToInt(seq[0]);
	const uint16_t d2 = HexCharToInt(seq[1]);
	const uint16_t d3 = HexCharToInt(seq[2]);
	const uint16_t d4 = HexCharToInt(seq[3]);

	return (d1 << 12) | (d2 << 8) | (d3 << 4) | (d4 << 0);
}

static inline uint32_t HexSeqToInt32(const char seq[4])
{
	const uint32_t d1 = HexSeqToInt16(&seq[0]);
	const uint32_t d2 = HexSeqToInt16(&seq[4]);

	return (d1 << 16) | (d2 << 0);
}
%}

%option 8bit
%option yylineno noyywrap nounput
%option warn nodefault
%option prefix="json_"

%x MLCOMMENT
%x QUOTE

%%

	/* Whitespace */
	/* (horizontal and vertical tab, formfeed and a space character) */
[\t\v\n\f ]                       /* No action assigned: skip these. */

	/* Strings */
\"                              {
					yylval->sval = NULL;
					BEGIN QUOTE;
				}
	/* Match everything except for \ and " or control characters */
<QUOTE>[^\\\"]+                 {
					yylval->sval = strrealloccat(yylval->sval, yytext);
				}
	/* Unicode sequence (16bit) */
<QUOTE>\\u[0-9A-Fa-f]{4}        {
					char utf8Encoded[MAX_UTF8_LEN];

					yylval->sval = strrealloccat(yylval->sval, UTF8EncodeChar(utf8Encoded, HexSeqToInt16(&yytext[2])));
				}
	/* Unicode sequence (32bit) */
<QUOTE>\\U[0-9A-Fa-f]{8}        {
					char utf8Encoded[MAX_UTF8_LEN];

					yylval->sval = strrealloccat(yylval->sval, UTF8EncodeChar(utf8Encoded, HexSeqToInt32(&yytext[2])));
				}
	/* Escape sequences for certain ASCII and string control characters */
<QUOTE>\\\"                     { yylval->sval = strrealloccat(yylval->sval, "\""); /* quotation mark                 */ }
<QUOTE>\\\\                     { yylval->sval = strrealloccat(yylval->sval, "\\"); /* reverse solidus, aka backslash */ }
<QUOTE>\\\/                     { yylval->sval = strrealloccat(yylval->sval, "/" ); /* solidus, aka slash             */ }
<QUOTE>\\b                      { yylval->sval = strrealloccat(yylval->sval, "\b"); /* backspace                      */ }
<QUOTE>\\f                      { yylval->sval = strrealloccat(yylval->sval, "\f"); /* formfeed                       */ }
<QUOTE>\\n                      { yylval->sval = strrealloccat(yylval->sval, "\n"); /* newline                        */ }
<QUOTE>\\r                      { yylval->sval = strrealloccat(yylval->sval, "\r"); /* carriage return                */ }
<QUOTE>\\t                      { yylval->sval = strrealloccat(yylval->sval, "\t"); /* horizontal tab                 */ }

<QUOTE>\"                       {
					BEGIN INITIAL;
					return STRING;
				}
	/* Match everything else (which is erroneous data). */
<QUOTE>.|\n                     {
					return TINVALID;
				}

	/* Punctuation */
[\{\}:,\[\]]                    {
					return yytext[0];
				}

	/* Numbers */
	/* Match floating point numbers with an exponent in them */
-?([1-9][0-9]*(\.[0-9]+)?|0\.[0-9]+)([Ee][+-]?[0-9]+) {
					char* endptr;
					yylval->fval = strtod(yytext, &endptr);
					if (yytext == endptr)
					{
						/// @TODO Handle this conversion error
					}
					return FLOAT;
				}
	/* Match floating point numbers without an exponent in them */
-?([1-9][0-9]*|0)\.[0-9]+       {
					char* endptr;
					yylval->fval = strtod(yytext, &endptr);
					if (yytext == endptr)
					{
						/// @TODO Handle this conversion error
					}
					return FLOAT;
				}
	/* Match integer numbers */
0|-?[1-9][0-9]*                 { yylval->ival = atol(yytext); return INTEGER; }

	/* Keywords */
true                            return TTRUE;
false                           return TFALSE;
null                            return TNULL;

"/*"                            {
					BEGIN MLCOMMENT;
				}

<MLCOMMENT>"*/"                 {
					BEGIN INITIAL;
				}

<MLCOMMENT>(\*[^/]|[^*])+ |
<MLCOMMENT>\n

\/\/.*\n                        /* No action */

	/* Everything not matched by the above rules is erroneous data. */
.                               return TINVALID;

%%

static YY_EXTRA_TYPE pBuffer = NULL;

void yyset_extra(YY_EXTRA_TYPE user_defined)
{
	pBuffer = user_defined;
}

YY_EXTRA_TYPE yyget_extra()
{
	return pBuffer;
}

/* Older GNU Flex versions don't define yylex_destroy()
 * (and neither define a subminor version)
 */
#if !defined(YY_FLEX_SUBMINOR_VERSION) || (YY_FLEX_SUBMINOR_VERSION < 9)
int json_lex_destroy(void)
{
	/* For non-reentrant C scanner only. */
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_init = 1;
}
#endif
