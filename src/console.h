/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_CONSOLE_H__
#define __INCLUDED_SRC_CONSOLE_H__

#define MAX_CONSOLE_MESSAGES			(64)
#define MAX_CONSOLE_STRING_LENGTH		(255)
#define MAX_CONSOLE_TMP_STRING_LENGTH	(255)

enum CONSOLE_TEXT_JUSTIFICATION
{
	LEFT_JUSTIFY,
	DEFAULT_JUSTIFY = LEFT_JUSTIFY,
	RIGHT_JUSTIFY,
	CENTRE_JUSTIFY
};

/* ID to use for addConsoleMessage() in case of a system message */
#define	SYSTEM_MESSAGE				(-1)
#define NOTIFY_MESSAGE				(-2)	// mainly used for lobby & error messages
#define INFO_MESSAGE				(-3)	// This type is not stored, it is used for simple messages

extern char ConsoleString[MAX_CONSOLE_TMP_STRING_LENGTH];

bool addConsoleMessage(const char *Text, CONSOLE_TEXT_JUSTIFICATION jusType, SDWORD player, bool team = false);
void updateConsoleMessages();
void initConsoleMessages();
void removeTopConsoleMessage();
void displayConsoleMessages();
void displayOldMessages();
void flushConsoleMessages();
void setConsoleBackdropStatus(bool state);
void enableConsoleDisplay(bool state);
bool getConsoleDisplayStatus();
void setConsoleSizePos(UDWORD x, UDWORD y, UDWORD width);
void setConsolePermanence(bool state, bool bClearOld);
void clearActiveConsole();
bool mouseOverConsoleBox();
bool mouseOverHistoryConsoleBox();
int getNumberConsoleMessages();
void setConsoleLineInfo(UDWORD vis);
UDWORD getConsoleLineInfo();
void permitNewConsoleMessages(bool allow);
void toggleConsoleDrop();
void setHistoryMode(bool mode);
void clearInfoMessages();

#if defined(DEBUG)
# define debug_console(...) \
	console(__VA_ARGS__)
#else // defined(DEBUG)
# define debug_console(...) (void)0
#endif // !defined(DEBUG)

void console(const char *pFormat, ...); /// Print always to the ingame console

/**
 Usage:
	CONPRINTF(StringPointer,(StringPointer,"format",data));
	StringPointer should always be ConsoleString.
	NOTE: This class of messages are NOT saved in the history
	logs.  These are "one shot" type of messages.

 eg.
	CONPRINTF(ConsoleString,(ConsoleString,"Hello %d",123));
*/
#define CONPRINTF(s,x) \
	sprintf x; \
	addConsoleMessage(s, DEFAULT_JUSTIFY, INFO_MESSAGE)

#endif // __INCLUDED_SRC_CONSOLE_H__
