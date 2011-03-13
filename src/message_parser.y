/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Giel van Schijndel
	Copyright (C) 2008-2011  Warzone 2100 Project

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
 *  Parser for message data
 */

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/frameresource.h"
#include "src/message.h"
#include "src/messagedef.h"
#include "src/messagely.h"
#include "src/text.h"

extern void yyerror(const char* msg);
void yyerror(const char* msg)
{
	debug(LOG_ERROR, "SMSG file parse error:\n%s at line %d\nText: '%s'", msg, message_get_lineno(), message_get_text());
}

struct TEXT_MESSAGE
{
	char * str;
	TEXT_MESSAGE *psNext;
};

static void freeTextMessageList(TEXT_MESSAGE* list)
{
	while (list)
	{
		TEXT_MESSAGE* const toDelete = list;
		list = list->psNext;
		free(toDelete->str);
		free(toDelete);
	}
}

struct VIEWDATAMESSAGE
{
	VIEWDATA view;
	struct VIEWDATAMESSAGE* psNext;
};

static void freeViewDataMessageList(VIEWDATAMESSAGE* list)
{
	while (list)
	{
		VIEWDATAMESSAGE* const toDelete = list;
		list = list->psNext;
		free(toDelete->view.pName);
		free(toDelete->view.ppTextMsg);
		switch (toDelete->view.type)
		{
			case VIEW_RES:
			{
				VIEW_RESEARCH* const psViewRes = (VIEW_RESEARCH *)toDelete->view.pData;
				if (psViewRes->pAudio)
					free(psViewRes->pAudio);
				free(psViewRes);
				break;
			}
			default:
				ASSERT(!"Unhandled view data type", "Unhandled view data type %u", toDelete->view.type);
		}
		free(toDelete);
	}
}

#define YYPARSE_PARAM ppsViewData

%}

%name-prefix="message_"

%union {
	char*                   sval;
	struct VIEWDATAMESSAGE* viewdatamsg;
	struct TEXT_MESSAGE*    txtmsg;
	VIEW_TYPE               viewtype;
	VIEW_RESEARCH*          researchdata;
	struct
	{
		const char**    stringArray;
		unsigned int    count;
	}                       msg_list;
}

	/* value tokens */
%token <sval> TEXT_T
%token <sval> QTEXT_T NULL_T		/* Text with double quotes surrounding it */
%token <viewtype> VIEW_T_RES VIEW_T_RPL VIEW_T_PROX VIEW_T_RPLX VIEW_T_BEACON
%token IMD_NAME_T IMD_NAME2_T SEQUENCE_NAME_T AUDIO_NAME_T

// Rule types
%type <txtmsg> text_message text_messages
%type <msg_list> message_list
%type <sval> optional_string imd_name imd_name2 sequence_name audio_name
%type <researchdata> research_message
%type <viewdatamsg> all_messages message

%destructor {
#ifndef WZ_OS_WIN
	// Force type checking by the compiler
	char * const s = $$;

	if (s)
		free(s);
#endif
} TEXT_T QTEXT_T optional_string imd_name imd_name2 sequence_name audio_name

%destructor {
	freeTextMessageList($$);
} text_message text_messages;

%destructor {
	// Force type checking by the compiler
	VIEW_RESEARCH* const r = $$;

	if (r)
	{
		if (r->pAudio)
			free(r->pAudio);

		free(r);
	}
} research_message

%destructor {
	freeViewDataMessageList($$);
} all_messages message

%%

file:			all_messages
				{
					unsigned int numData = 0, i;
					VIEWDATAMESSAGE* curMsg;
					VIEWDATA* psViewData;

					for (curMsg = $1; curMsg != NULL; curMsg = curMsg->psNext)
					{
						++numData;
					}

					ASSERT(numData <= UBYTE_MAX, "loadViewData: Didn't expect %d (or more) viewData messages (got %u)!", UBYTE_MAX, numData);
					if (numData > UBYTE_MAX)
					{
						freeViewDataMessageList($1);
						YYABORT;
					}

					psViewData = (VIEWDATA *)malloc(numData * sizeof(*psViewData));
					if (psViewData == NULL)
					{
						debug(LOG_ERROR, "Out of memory");
						abort();
						freeViewDataMessageList($1);
						YYABORT;
					}

					curMsg = $1;
					for (i = 0; i < numData; ++i)
					{
						VIEWDATAMESSAGE* const toMove = curMsg;
						assert(toMove != NULL);
						curMsg = curMsg->psNext;
						memcpy(&psViewData[i], &toMove->view, sizeof(psViewData[i]));
						free(toMove);
					}

					addToViewDataList(psViewData, numData);
					*(VIEWDATA**)ppsViewData = psViewData;
				}
			;

all_messages:		message
			|	message all_messages
				{
					$1->psNext = $2;
					$$ = $1;
				}
			;

message:		TEXT_T '{' message_list ',' research_message '}' ';'
				{
					$$ = (VIEWDATAMESSAGE *)malloc(sizeof(*$$));
					if ($$ == NULL)
					{
						debug(LOG_ERROR, "Out of memory");
						abort();
						free($1);
						free($3.stringArray);
						if ($5)
							free($5->pAudio);
						free($5);
						YYABORT;
					}

					$$->view.pName = $1;
					$$->view.numText = $3.count;
					$$->view.ppTextMsg = $3.stringArray;
					$$->view.type = VIEW_RES;
					$$->view.pData = $5;
					$$->psNext = NULL;
				}
			;

research_message:	imd_name ',' imd_name2 ',' sequence_name ',' audio_name ','
				{
					$$ = (VIEW_RESEARCH *)malloc(sizeof(*$$));
					if ($$ == NULL)
					{
						debug(LOG_ERROR, "Out of memory");
						abort();
						free($1);
						free($3);
						free($5);
						free($7);
						YYABORT;
					}

					$$->pAudio = $7;
					sstrcpy($$->sequenceName, $5);
					// Get rid of our tokens ASAP (so that the free() lists on errors become shorter)
					free($5);

					$$->pIMD = (iIMDShape *) resGetData("IMD", $1);
					if ($$->pIMD == NULL)
					{
						ASSERT(LOG_ERROR, "Cannot find PIE \"%s\"", $1);
						free($1);
						free($3);
						YYABORT;
					}
					free($1);

					if ($3)
					{
						$$->pIMD2 = (iIMDShape *) resGetData("IMD", $3);
						if ($$->pIMD2 == NULL)
						{
							ASSERT(false, "Cannot find 2nd PIE \"%s\"", $3);
							free($3);
							YYABORT;
						}
						free($3);
					}
					else
					{
						$$->pIMD2 = NULL;
					}
				}
			;

imd_name:		IMD_NAME_T '=' QTEXT_T              { $$ = $3; }
			| QTEXT_T
			;

imd_name2:		IMD_NAME2_T '=' optional_string     { $$ = $3; }
			| optional_string
			;

sequence_name:		SEQUENCE_NAME_T '=' QTEXT_T         { $$ = $3; }
			| QTEXT_T
			;

audio_name:		AUDIO_NAME_T '=' optional_string    { $$ = $3; }
			| optional_string
			;

optional_string:	QTEXT_T
			| NULL_T
			;

message_list:		'{' text_messages '}'
				{
					size_t bytes = 0;
					unsigned int i;
					TEXT_MESSAGE* psCur;
					char* stringStart;

					$$.count = 0;

					// Compute the required space for all strings and an array of pointers to hold it
					for (psCur = $2; psCur != NULL; psCur = psCur->psNext)
					{
						++$$.count;
						bytes += sizeof(char*) + strlen(psCur->str) + 1;
					}

					ASSERT($$.count <= MAX_DATA, "Too many text strings (%u) provided, with %u as maximum", $$.count, (unsigned int)MAX_DATA);
					if ($$.count > MAX_DATA)
					{
						YYABORT;
					}

					if ($$.count)
					{
						$$.stringArray = (char const **)malloc(bytes);
						if ($$.stringArray == NULL)
						{
							debug(LOG_ERROR, "Out of memory");
							abort();
							freeTextMessageList($2);
							YYABORT;
						}

						stringStart = (char*)&$$.stringArray[$$.count];
						for (psCur = $2, i = 0;
                                                     stringStart && psCur != NULL && i < $$.count;
						     psCur = psCur->psNext, ++i)
						{
							assert(&stringStart[strlen(psCur->str)] - (char*)$$.stringArray < bytes);
							$$.stringArray[i] = strcpy(stringStart, psCur->str);
							stringStart = &stringStart[strlen(psCur->str) + 1];
						}
					}
					else
					{
						$$.stringArray = NULL;
					}

					// Clean up our tokens
					freeTextMessageList($2);
				}
			;

text_messages:		text_message
				/* Allow trailing commas */
			| text_message ','
			| text_message ',' text_messages
				{
					$1->psNext = $3;
					$$ = $1;
				}
			;

text_message: 		TEXT_T
				{
					const char * const msg = strresGetString(psStringRes, $1);
					if (!msg)
					{
						ASSERT(!"Cannot find string resource", "Cannot find the view data string with id \"%s\"", $1);
						free($1);
						YYABORT;
					}

					$$ = (TEXT_MESSAGE *)malloc(sizeof(*$$));
					if ($$ == NULL)
					{
						debug(LOG_ERROR, "Out of memory");
						abort();
						free($1);
						YYABORT;
					}

					$$->str = $1;
					$$->psNext = NULL;
				}
			| QTEXT_T
				{
					$$ = (TEXT_MESSAGE *)malloc(sizeof(*$$));
					if ($$ == NULL)
					{
						debug(LOG_ERROR, "Out of memory");
						abort();
						free($1);
						YYABORT;
					}

					$$->str = $1;
					$$->psNext = NULL;
				}
			| '_' '(' QTEXT_T ')'
				{
					$$ = (TEXT_MESSAGE *)malloc(sizeof(*$$));
					if ($$ == NULL)
					{
						debug(LOG_ERROR, "Out of memory");
						abort();
						free($3);
						YYABORT;
					}

					$$->str = $3;
					$$->psNext = NULL;
				}
			;
