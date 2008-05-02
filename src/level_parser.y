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
/** @file
 *  yacc grammar for loading level files.
 */

%{
#include "lib/framework/frame.h"
#include "lib/framework/listmacs.h"
#include "levels.h"
#include "levelint.h"

typedef struct
{
	searchPathMode datadir;
	LEVEL_DATASET* dataset;
	unsigned int   currData;
	bool           dataLoaded;
} parser_context;

/** Error reporting function for the level parser.
 *  @msg Error message to display.
 */
void yyerror(const char* msg)
{
	debug(LOG_ERROR, "Level File parse error: `%s` at line `%d` text `%s`", msg, levGetErrorLine(), levGetErrorText());
}

#define YYPARSE_PARAM yyparse_param
#define yycontext     ((parser_context*)yyparse_param)

%}

%name-prefix="lev_"

%union
{
	char*                   sval;
	int                     ival;
	LEVEL_TYPE              ltype;
	LEVEL_DATASET*          dataset;
}

	/* value tokens */
%token <sval> IDENTIFIER
%token <sval> QTEXT
%token <ival> INTEGER

%destructor { free($$); } IDENTIFIER QTEXT

	/* keywords */
%token LEVEL
%token PLAYERS
%token TYPE
%token DATA
%token GAME
%token CAMPAIGN
%token CAMSTART
%token CAMCHANGE
%token DATASET
%token EXPAND
%token EXPAND_LIMBO
%token BETWEEN
%token MISS_KEEP
%token MISS_KEEP_LIMBO
%token MISS_CLEAR

	/* rule types */
%type <dataset> level_rule
%type <ltype>   level_type

%start lev_file

%%

lev_file:               /* Empty to allow empty files */
					{
						debug(LOG_WARNING, "Warning level file is empty (this could be intentional though).");
					}
                      | level_entry
                      | lev_file level_entry
                        ;

level_entry:            level_line level_directives_1st dataload_directives
                        ;

level_line:             level_rule IDENTIFIER
				{
					if ($1->type == LDS_CAMCHANGE)
					{
						// This is a campaign change dataset, we need to find the full data set.
						LEVEL_DATASET * const psFoundData = levFindDataSet($2);

						if (psFoundData == NULL)
						{
							yyerror("Cannot find full data set for `camchange'");
							YYABORT;
						}

						if (psFoundData->type != LDS_CAMSTART)
						{
							yyerror("Invalid data set name for `camchange'");
							YYABORT;
						}
						psFoundData->psChange = $1;
					}

					// Store the level name
					$1->pName = strdup($2);
					if ($1->pName == NULL)
					{
						debug(LOG_ERROR, "Out of memory!");
						abort();
						YYABORT;
					}

					// Make this dataset current to our parsing context
					yycontext->dataset = $1;
				}
                        ;

level_rule:            level_type
				{
					$$ = (LEVEL_DATASET*)malloc(sizeof(LEVEL_DATASET));
					if ($$ == NULL)
					{
						debug(LOG_ERROR, "Out of memory!");
						abort();
						YYABORT;
					}
					memset($$, 0, sizeof(*$$));

					$$->players = 1;
					$$->game = -1;
					$$->dataDir = yycontext->datadir;

					LIST_ADDEND(psLevels, $$, LEVEL_DATASET);
					yycontext->currData = 0;
					yycontext->dataLoaded = false;

					$$->type = $1;
				}
                        ;

level_type:             LEVEL           { $$ = LDS_COMPLETE;            }
                      | CAMPAIGN        { $$ = LDS_CAMPAIGN;            }
                      | CAMSTART        { $$ = LDS_CAMSTART;            }
                      | CAMCHANGE       { $$ = LDS_CAMCHANGE;           }
                      | EXPAND          { $$ = LDS_EXPAND;              }
                      | EXPAND_LIMBO    { $$ = LDS_EXPAND_LIMBO;        }
                      | BETWEEN         { $$ = LDS_BETWEEN;             }
                      | MISS_KEEP       { $$ = LDS_MKEEP;               }
                      | MISS_KEEP_LIMBO { $$ = LDS_MKEEP_LIMBO;         }
                      | MISS_CLEAR      { $$ = LDS_MCLEAR;              }
                        ;

// These directives _must_ appear _before_ all other level_directive's, they can
// appear in any order or not at all.
level_directives_1st:   /* empty: match nothing */
                      | player_directive
                      | type_directive
                      | player_directive type_directive
                      | type_directive player_directive
                        ;

player_directive:       PLAYERS INTEGER
				{
					if (yycontext->dataset->type != LDS_COMPLETE
					 && yycontext->dataset->type <  LDS_MULTI_TYPE_START)
					{
						yyerror("a `players` directive is only allowed for datasets that are initiated with the `level` directive or have been assigned a custom type with the `type` directive.");
						YYABORT;
					}

					yycontext->dataset->players = $2;
				}
                        ;

type_directive:         TYPE INTEGER
				{
					if (yycontext->dataset->type != LDS_COMPLETE)
					{
						yyerror("a `type` directive is only allowed for datasets that are initiated with the `level` directive and have not yet been assigned a custom type.");
						YYABORT;
					}

					if ($2 < LDS_MULTI_TYPE_START)
					{
						char* str;
						sasprintf(&str, "type number to low: must be at least LDS_MULTI_TYPE_START (%u)", (unsigned int)LDS_MULTI_TYPE_START);
						yyerror(str);
						YYABORT;
					}

					yycontext->dataset->type = $2;
				}
                        ;

dataload_directives:    dataload_directive
                      | dataload_directives dataload_directive
                        ;

dataload_directive:     dataset_directive
                      | data_directive
                        ;

dataset_directive:      DATASET IDENTIFIER
				{
					if (yycontext->dataset->type == LDS_COMPLETE)
					{
						yyerror("a `dataset` directive is not allowed for datasets that are initiated with the `level` directive or have been assigned a custom type.");
						YYABORT;
					}

					// Find the dataset with the given identifier
					yycontext->dataset->psBaseData = levFindDataSet($2);
					if (yycontext->dataset->psBaseData == NULL)
					{
						yyerror("unknown dataset");
						YYABORT;
					}

					yycontext->dataLoaded = true;
				}
                        ;

data_directive:         data_directives QTEXT
				{
					if (yycontext->currData >= LEVEL_MAXFILES)
					{
						yyerror("Too many data files");
						YYABORT;
					}

					// store the data name
					yycontext->dataset->apDataFiles[yycontext->currData] = strdup($2);
					if (yycontext->dataset->apDataFiles[yycontext->currData] == NULL)
					{
						debug(LOG_ERROR, "Out of memory!");
						abort();
						YYABORT;
					}

					++yycontext->currData;
					yycontext->dataLoaded = true;
				}
                        ;

data_directives:        DATA
				{
					if (!yycontext->dataLoaded
					 && (yycontext->dataset->type == LDS_CAMSTART
					  || yycontext->dataset->type == LDS_MKEEP
					  || yycontext->dataset->type == LDS_CAMCHANGE
					  || yycontext->dataset->type == LDS_EXPAND
					  || yycontext->dataset->type == LDS_MCLEAR
					  || yycontext->dataset->type == LDS_EXPAND_LIMBO
					  || yycontext->dataset->type == LDS_MKEEP_LIMBO))
					{
						yyerror("This `data` directive must be preceded by a `dataset` or `game` directive.");
					}
				}
                      | GAME
				{
					if (yycontext->dataset->type == LDS_CAMPAIGN)
					{
						yyerror("Cannot use the `game` directive with `campaign` datasets.");
						YYABORT;
					}

					if (yycontext->dataset->game != -1)
					{
						yyerror("Cannot assign multiple games to a dataset.");
						YYABORT;
					}

					// note the game's index
					yycontext->dataset->game = yycontext->currData;
				}
                        ;

%%

BOOL levParse(const char* buffer, size_t size, searchPathMode datadir)
{
	BOOL retval;
	parser_context context;

	context.datadir = datadir;
	context.dataset = NULL;

	levSetInputBuffer(buffer, size);

	retval = (yyparse(&context) == 0);

	lev_lex_destroy();

	return retval;
}
