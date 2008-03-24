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
%{
/*
 * ScriptVals.y
 *
 * yacc grammar for loading script variable values
 *
 */
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"

#include "lib/gamelib/gtime.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"

#include "src/scripttabs.h"
#include "src/scriptvals.h"
#include "src/objects.h"
#include "src/droid.h"
#include "src/structure.h"
#include "src/message.h"
#include "src/levels.h"
#include "src/research.h"

extern int scrv_lex(void);

// The current script code
static SCRIPT_CODE		*psCurrScript;

// The current script context
static SCRIPT_CONTEXT	*psCurrContext;

// the current array indexes
static ARRAY_INDEXES	sCurrArrayIndexes;

// check that an array index is valid
BOOL scrvCheckArrayIndex(SDWORD base, ARRAY_INDEXES *psIndexes, UDWORD *pIndex)
{
	SDWORD	i, size;

	if (!psCurrScript || psCurrScript->psDebug == NULL)
	{
		return false;
	}

	if (base < 0 || base >= psCurrScript->numArrays)
	{
		scrv_error("Array index out of range");
		return false;
	}

	if (psIndexes->dimensions != psCurrScript->psArrayInfo[base].dimensions)
	{
		scrv_error("Invalid number of dimensions for array initialiser");
		return false;
	}

	for(i=0; i<psCurrScript->psArrayInfo[base].dimensions; i++)
	{
		if ((psIndexes->elements[i] < 0) ||
			(psIndexes->elements[i] >= psCurrScript->psArrayInfo[base].elements[i]))
		{
			scrv_error("Invalid index for dimension %d", i);
			return false;
		}
	}

	*pIndex = 0;
	size = 1;
	for(i = psCurrScript->psArrayInfo[base].dimensions-1; i >= 0; i--)
	{
		*pIndex += psIndexes->elements[i] * size;
		size *= psCurrScript->psArrayInfo[base].elements[i];
	}

	*pIndex += psCurrScript->psArrayInfo[base].base;

	return true;
}

%}

%name-prefix="scrv_"

%union {
	BOOL			bval;
	INTERP_TYPE		tval;
	char			*sval;
	UDWORD			vindex;
	SDWORD			ival;
	VAR_INIT		sInit;
	ARRAY_INDEXES	*arrayIndex;
}

	/* value tokens */
%token <bval> BOOLEAN_T
%token <ival> INTEGER
%token <sval> IDENT
%token <sval> QTEXT			/* Text with double quotes surrounding it */
%token <tval> TYPE
%token <vindex> VAR
%token <vindex> ARRAY

	/* keywords */
%token SCRIPT
%token STORE
%token RUN

	/* rule types */
%type <sval> script_name
%type <sInit> var_value
%type <vindex> var_entry
%type <arrayIndex> array_index
%type <arrayIndex> array_index_list

%%

val_file:		script_entry
			|	val_file script_entry
			;

script_entry:	script_name RUN
				{
					if (!eventNewContext(psCurrScript, CR_RELEASE, &psCurrContext))
					{
						scrv_error("Couldn't create context");
						YYABORT;
					}
					if (!scrvAddContext($1, psCurrContext, SCRV_EXEC))
					{
						scrv_error("Couldn't store context");
						YYABORT;
					}
				}
							'{' var_init_list '}'
				{
					if (!eventRunContext(psCurrContext, gameTime/SCR_TICKRATE))
					{
						YYABORT;
					}
				}
			|	script_name STORE QTEXT
				{
					if (!eventNewContext(psCurrScript, CR_NORELEASE, &psCurrContext))
					{
						scrv_error("Couldn't create context");
						YYABORT;
					}
					if (!scrvAddContext($3, psCurrContext, SCRV_NOEXEC))
					{
						scrv_error("Couldn't store context");
						YYABORT;
					}
				}
										 '{' var_init_list '}'
			;

script_name:	SCRIPT QTEXT
				{

					int namelen,extpos;
					char *stringname;

					stringname=$2;

					namelen=strlen( stringname);
					extpos=namelen-3;
					if (strncmp(&stringname[extpos],"blo",3)==0)
					{
						if (resPresent("BLO",stringname)==true)
						{
							psCurrScript = (SCRIPT_CODE*)resGetData("BLO",stringname);
						}
						else
						{
							// change extension to "slo"
							stringname[extpos]='s';
							psCurrScript = (SCRIPT_CODE*)resGetData("SCRIPT",stringname);
						}
					}
					else if (strncmp(&stringname[extpos],"slo",3)==0)
					{
						if (resPresent("SCRIPT",stringname)==true)
						{
							psCurrScript = (SCRIPT_CODE*)resGetData("SCRIPT",stringname);
						}
					}

					if (!psCurrScript)
					{
						scrv_error("Script file %s not found", stringname);
						YYABORT;
					}

					$$ = $2;
				}
			;

var_init_list:	/* NULL token */
			|	var_init
			|	var_init_list var_init
			;

var_init:		var_entry TYPE var_value
				{
					INTERP_VAL		data;	/* structure to to hold all types */
					BASE_OBJECT *psObj;
					char			*pString;
					SDWORD   compIndex;

					/* set type */
					data.type = $2;

					switch ($2)
					{
					case VAL_INT:
						data.v.ival = $3.index;	//index = integer value of the variable, not var index
						if ($3.type != IT_INDEX ||
							!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						break;
					case ST_DROID:
						if ($3.type != IT_INDEX)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						psObj = getBaseObjFromId((UDWORD)$3.index);
						if (!psObj)
						{
							scrv_error("Droid id %d not found", (UDWORD)$3.index);
							YYABORT;
						}

						data.v.oval = psObj;	/* store as pointer */

						if (psObj->type != OBJ_DROID)
						{
							scrv_error("Object id %d is not a droid", (UDWORD)$3.index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, $1, &data))
							{
								scrv_error("Set Value Failed for %s", $1);
								YYABORT;
							}
						}
						break;

					case ST_STRUCTURE:
						if ($3.type != IT_INDEX)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						psObj = getBaseObjFromId((UDWORD)$3.index);
						if (!psObj)
						{
							scrv_error("Structure id %d not found", (UDWORD)$3.index);
							YYABORT;
						}

						data.v.oval = psObj;

						if (psObj->type != OBJ_STRUCTURE)
						{
							scrv_error("Object id %d is not a structure", (UDWORD)$3.index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, $1, &data))
							{
								scrv_error("Set Value Failed for %s", $1);
								YYABORT;
							}
						}
						break;
					case ST_FEATURE:
						if ($3.type != IT_INDEX)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						psObj = getBaseObjFromId((UDWORD)$3.index);
						if (!psObj)
						{
							scrv_error("Feature id %d not found", (UDWORD)$3.index);
							YYABORT;
						}

						data.v.oval = psObj;

						if (psObj->type != OBJ_FEATURE)
						{
							scrv_error("Object id %d is not a feature", (UDWORD)$3.index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, $1, &data))
							{
								scrv_error("Set Value Failed for %s", $1);
								YYABORT;
							}
						}
						break;
					case ST_FEATURESTAT:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}

						data.v.ival = getFeatureStatFromName($3.pString);

						if (data.v.ival == -1)
						{
							scrv_error("Feature Stat %s not found", $3.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case VAL_BOOL:
						data.v.bval = $3.index;	//index = boolean value, not var index
						if ($3.type != IT_BOOL ||
							!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						break;
					case ST_BODY:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.ival = getCompFromResName(COMP_BODY, $3.pString);
						if (data.v.ival == -1)
						{
							scrv_error("body component %s not found", $3.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_PROPULSION:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.ival = getCompFromResName(COMP_PROPULSION, $3.pString);
						if (data.v.ival == -1)
						{
							scrv_error("Propulsion component %s not found", $3.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_ECM:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.ival = getCompFromResName(COMP_ECM, $3.pString);
						if (data.v.ival == -1)
						{
							scrv_error("ECM component %s not found", $3.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_SENSOR:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.ival = getCompFromResName(COMP_SENSOR, $3.pString);
						if (data.v.ival == -1)
						{
							scrv_error("Sensor component %s not found", $3.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_CONSTRUCT:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.ival = getCompFromResName(COMP_CONSTRUCT, $3.pString);
						if (data.v.ival == -1)
						{
							scrv_error("Construct component %s not found",	$3.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_REPAIR:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.ival = getCompFromResName(COMP_REPAIRUNIT, $3.pString);
						if (data.v.ival == -1)
						{
							scrv_error("Repair component %s not found",	$3.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_BRAIN:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.ival = getCompFromResName(COMP_BRAIN, $3.pString);
						if (data.v.ival == -1)
						{
							scrv_error("Brain component %s not found",	$3.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_WEAPON:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.ival = getCompFromResName(COMP_WEAPON, $3.pString);
						if (data.v.ival == -1)
						{
							scrv_error("Weapon component %s not found", $3.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_TEMPLATE:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.oval = getTemplateFromTranslatedNameNoPlayer($3.pString);	/* store pointer to the template */
						if (data.v.oval == NULL)
						{
							scrv_error("Template %s not found", $3.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_STRUCTURESTAT:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.ival = getStructStatFromName($3.pString);
						if (data.v.ival == -1)
						{
							scrv_error("Structure Stat %s not found", $3.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_STRUCTUREID:
						if ($3.type != IT_INDEX)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						psObj = getBaseObjFromId((UDWORD)$3.index);
						if (!psObj)
						{
							scrv_error("Structure id %d not found", (UDWORD)$3.index);
							YYABORT;
						}
						data.v.ival = $3.index;	/* store structure id */
						if (psObj->type != OBJ_STRUCTURE)
						{
							scrv_error("Object id %d is not a structure", (UDWORD)$3.index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, $1, &data))
							{
								scrv_error("Set Value Failed for %s", $1);
								YYABORT;
							}
						}
						break;
					case ST_DROIDID:
						if ($3.type != IT_INDEX)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						psObj = getBaseObjFromId((UDWORD)$3.index);
						if (!psObj)
						{
							scrv_error("Droid id %d not found", (UDWORD)$3.index);
							YYABORT;
						}
						data.v.ival = $3.index;	/* store id*/
						if (psObj->type != OBJ_DROID)
						{
							scrv_error("Object id %d is not a droid", (UDWORD)$3.index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, $1, &data))
							{
								scrv_error("Set Value Failed for %s", $1);
								YYABORT;
							}
						}
						break;
					case ST_INTMESSAGE:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.oval = getViewData($3.pString);	/* store pointer to the intelligence message */
						if (data.v.oval == NULL)
						{
							scrv_error("Message %s not found", $3.pString);
							YYABORT;
						}
						if(!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_TEXTSTRING:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						if (!scrvGetString($3.pString, &pString))
						{
							scrv_error("String %s not found", $3.pString);
							YYABORT;
						}
						data.v.sval = pString;
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_LEVEL:
						{
							LEVEL_DATASET	*psLevel;

							if ($3.type != IT_STRING)
							{
								scrv_error("Typemismatch for variable %d", $1);
								YYABORT;
							}
							// just check the level exists
							if (!levFindDataSet($3.pString, &psLevel))
							{
								scrv_error("Level %s not found", $3.pString);
								YYABORT;
							}
							data.v.sval = psLevel->pName; /* store string pointer */
							if (!eventSetContextVar(psCurrContext, $1, &data))
							{
								scrv_error("Set Value Failed for %s", $1);
								YYABORT;
							}
						}
						break;
					case ST_SOUND:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						/* find audio id */
						compIndex = audio_GetTrackID( $3.pString );
						if (compIndex == SAMPLE_NOT_FOUND)
						{
							/* set track vals */
							compIndex = audio_SetTrackVals($3.pString, false, 100, 1800);
						}
						/* save track ID */
						data.v.ival = compIndex;
						if (!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					case ST_RESEARCH:
						if ($3.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", $1);
							YYABORT;
						}
						data.v.oval = getResearch($3.pString, true);	/* store pointer */
						if (data.v.oval == NULL)
						{
							scrv_error("Research %s not found", $3.pString);
							YYABORT;
						}
						if(!eventSetContextVar(psCurrContext, $1, &data))
						{
							scrv_error("Set Value Failed for %s", $1);
							YYABORT;
						}
						break;
					default:
						scrv_error("Unknown type: %s", asTypeTable[$2].pIdent);
						YYABORT;
						break;
					}
				}
			;

array_index:	'[' INTEGER ']'
				{
					sCurrArrayIndexes.dimensions = 1;
					sCurrArrayIndexes.elements[0] = $2;

					$$ = &sCurrArrayIndexes;
				}
			;

array_index_list:	array_index
				{
					$$ = $1;
				}
			|		array_index_list '[' INTEGER ']'
				{
					if ($1->dimensions >= VAR_MAX_DIMENSIONS)
					{
						scrv_error("Too many dimensions for array");
						YYABORT;
					}
					$1->elements[$1->dimensions] = $3;
					$1->dimensions += 1;
				}
			;


var_entry:		VAR
				{
					$$ = $1;
				}
			|	ARRAY array_index_list
				{
					UDWORD	index;

					if (!scrvCheckArrayIndex($1, $2, &index))
					{
						YYABORT;
					}

					$$ = index;
				}
			;

var_value:		BOOLEAN_T
				{
					$$.type = IT_BOOL;
					$$.index = $1;
				}
			|	INTEGER
				{
					$$.type = IT_INDEX;
					$$.index = $1;
				}
			|	QTEXT
				{
					$$.type = IT_STRING;
					$$.pString = $1;
				}
			;

%%

// Lookup a type
BOOL scrvLookUpType(const char *pIdent, INTERP_TYPE *pType)
{
	TYPE_SYMBOL		*psCurr;

	for(psCurr = asTypeTable; psCurr->typeID != 0; psCurr++)
	{
		if (strcmp(psCurr->pIdent, pIdent) == 0)
		{
			*pType = psCurr->typeID;
			return true;
		}
	}

	return false;
}


// Lookup a variable identifier
BOOL scrvLookUpVar(const char *pIdent, UDWORD *pIndex)
{
	UDWORD	i;

	if (!psCurrScript || psCurrScript->psDebug == NULL)
	{
		return false;
	}

	for(i=0; i<psCurrScript->numGlobals; i++)
	{
		if (psCurrScript->psVarDebug[i].pIdent != NULL &&
			strcmp(psCurrScript->psVarDebug[i].pIdent, pIdent) == 0)
		{
			*pIndex = i;
			return true;
		}
	}

	return false;
}


// Lookup an array identifier
BOOL scrvLookUpArray(const char *pIdent, UDWORD *pIndex)
{
	UDWORD	i;

	if (!psCurrScript || psCurrScript->psDebug == NULL)
	{
		return false;
	}

	for(i=0; i<psCurrScript->numArrays; i++)
	{
		if (psCurrScript->psArrayDebug[i].pIdent != NULL &&
			strcmp(psCurrScript->psArrayDebug[i].pIdent, pIdent) == 0)
		{
			*pIndex = i;
			return true;
		}
	}

	return false;
}


// Load a script value file
BOOL scrvLoad(PHYSFS_file* fileHandle)
{
	scrvSetInputFile(fileHandle);

	if (scrv_parse() != 0)
	{
		return false;
	}

	return true;
}

/* A simple error reporting routine */
void scrv_error(const char *pMessage,...)
{
	int		line;
	char	*pText;
	char	aTxtBuf[1024];
	va_list	args;

	va_start(args, pMessage);

	vsprintf(aTxtBuf, pMessage, args);
	scrvGetErrorData(&line, &pText);
	debug(LOG_ERROR, "VLO parse error: %s at line %d, token: %d, text: '%s'",
	      aTxtBuf, line, scrv_char, pText);

	va_end(args);
}
