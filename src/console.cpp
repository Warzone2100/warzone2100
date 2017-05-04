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
/* Alex McLean, Pumpkin Studios, EIDOS Interactive */
/** \file
	Functions for the in-game console.
*/

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "loadsave.h"
#include "ai.h"
#include "console.h"
#include "main.h"
#include "radar.h"
#include "hci.h"
#include <string>
#include <istream>
#include <sstream>
#include <deque>

// FIXME: When we switch over to full JS, use class version of this file

#define	DEFAULT_MESSAGE_DURATION	GAME_TICKS_PER_SEC * 4
// Chat/history "window"
#define CON_BORDER_WIDTH			4
#define CON_BORDER_HEIGHT			4
#define HISTORYBOX_X 				RET_X
#define HISTORYBOX_Y 				RET_Y - 80
#define NumDisplayLines 			4

struct CONSOLE
{
	UDWORD	topX;
	UDWORD	topY;
	UDWORD	width;
	UDWORD	textDepth;
	bool	permanent;
};

struct CONSOLE_MESSAGE
{
	WzText display;
	UDWORD	timeAdded;		// When was it added to our list?
	CONSOLE_TEXT_JUSTIFICATION JustifyType;	// text justification
	int		player;			// Player who sent this message or SYSTEM_MESSAGE
	CONSOLE_MESSAGE(const std::string &text, iV_fonts fontID, UDWORD time, CONSOLE_TEXT_JUSTIFICATION justify, int plr)
	                : display(text, fontID), timeAdded(time), JustifyType(justify), player(plr) {}
};

static std::deque<CONSOLE_MESSAGE> ActiveMessages;		// we add all messages to this container
static std::deque<CONSOLE_MESSAGE> TeamMessages;		// history of team/private communications
static std::deque<CONSOLE_MESSAGE> HistoryMessages;	// history of all other communications
static std::deque<CONSOLE_MESSAGE> InfoMessages;
static bool	bConsoleDropped = false;			// Is the console history on or off?
static bool HistoryMode = false;				// toggle between team & global history
static int updatepos = 0;						// if user wants to scroll back the history log
static int linePitch = 0;						// the pitch of a line
static bool showBackgroundColor = false;		// if user wants to add more contrast to the history display
static CONSOLE mainConsole;						// Stores the console dimensions and states
static CONSOLE historyConsole;					// Stores the History console dimensions and states
static UDWORD	messageDuration;				/** How long do messages last for? */
static bool	bTextBoxActive = false;				/** Is there a box under the console text? */
static bool	bConsoleDisplayEnabled = false;		/** Is the console being displayed? */
static UDWORD	consoleVisibleLines;			/** How many lines are displayed? */
static int allowNewMessages;					/** Whether new messages are allowed to be added */

char ConsoleString[MAX_CONSOLE_TMP_STRING_LENGTH];	/// Globally available string for new console messages.

/**
	Specify how long messages will stay on screen.
*/
static void	setConsoleMessageDuration(UDWORD time)
{
	messageDuration = time;
}

/** Sets the system up */
void	initConsoleMessages()
{
	linePitch = iV_GetTextLineSize(font_regular);
	bConsoleDropped = false;
	setConsoleMessageDuration(DEFAULT_MESSAGE_DURATION);	// Setup how long messages are displayed for
	setConsoleBackdropStatus(true);							// No box under the text
	enableConsoleDisplay(true);								// Turn on the console display

	//	Set up the main console size and position x,y,width
	setConsoleSizePos(16, 32, pie_GetVideoBufferWidth() - 32);
	historyConsole.topX = HISTORYBOX_X;
	historyConsole.topY = HISTORYBOX_Y;
	historyConsole.width = pie_GetVideoBufferWidth() - 32;
	setConsoleLineInfo(MAX_CONSOLE_MESSAGES / 4 + 4);
	setConsolePermanence(false, true);						// We're not initially having permanent messages
	permitNewConsoleMessages(true);							// Allow new messages
}

// toggle between team & global history
void setHistoryMode(bool mode)
{
	HistoryMode = mode;
}

/** Open the console when it's closed and close it when it's open. */
void	toggleConsoleDrop()
{
	if (!bConsoleDropped)
	{	// it was closed, so play open sound
		bConsoleDropped = true;
		audio_PlayTrack(ID_SOUND_WINDOWOPEN);
	}
	else
	{
		// play closing sound
		audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
		bConsoleDropped = false;
	}
}

/** Add a string to the console. */
bool addConsoleMessage(const char *Text, CONSOLE_TEXT_JUSTIFICATION jusType, SDWORD player, bool team)
{
	if (!allowNewMessages)
	{
		return false;	// Don't allow it to be added if we've disabled adding of new messages
	}

	std::istringstream stream(Text);
	std::string lines;

	while (std::getline(stream, lines))
	{
		// We got one "line" from the total string, now we must check
		// to see if it fits, if not, we truncate it. (Full text is in the logs)
		// NOTE: may want to break up line into multi-line so it matches the log
		std::string FitText(lines);
		while (!FitText.empty())
		{
			int pixelWidth = iV_GetTextWidth(FitText.c_str(), font_regular);
			if (pixelWidth <= mainConsole.width)
			{
				break;
			}
			FitText.resize(FitText.length() - 1);	// Erase last char.
		}

		debug(LOG_CONSOLE, "(to player %d): %s", (int)player, FitText.c_str());

		if (player == INFO_MESSAGE)
		{
			InfoMessages.emplace_back(FitText, font_regular, realTime, jusType, player);
		}
		else
		{
			ActiveMessages.emplace_back(FitText, font_regular, realTime, jusType, player);	// everything gets logged here for a specific period of time
			if (team)
			{
				TeamMessages.emplace_back(FitText, font_regular, realTime, jusType, player);	// persistent team specific logs
			}
			HistoryMessages.emplace_back(FitText, font_regular, realTime, jusType, player);	// persistent messages (all types)
		}
	}
	return true;
}

/// \return The number of active console messages
int	getNumberConsoleMessages()
{
	return (ActiveMessages.size());
}

/** Update the console messages.
	This function will remove messages that are overdue.
*/
void	updateConsoleMessages()
{
	// If there are no messages or we're on permanent (usually for scripts) then exit
	if ((!getNumberConsoleMessages() && !InfoMessages.size()) || mainConsole.permanent)
	{
		return;
	}
	for (auto i = InfoMessages.begin(); i != InfoMessages.end();)
	{
		if (realTime - i->timeAdded > messageDuration)
		{
			i = InfoMessages.erase(i);
		}
		else
		{
			++i;
		}
	}
	// Time to kill all expired ones
	for (auto i = ActiveMessages.begin(); i != ActiveMessages.end();)
	{
		if (realTime - i->timeAdded > messageDuration)
		{
			i = ActiveMessages.erase(i);
		}
		else
		{
			++i;
		}
	}
}

/**
	Remove the top message on screen.
	This and setConsoleMessageDuration should be sufficient to allow
	us to put up messages that stay there until we remove them
	ourselves - be sure and reset message duration afterwards
*/
void	removeTopConsoleMessage()
{
	if (getNumberConsoleMessages())
	{
		ActiveMessages.pop_front();
	}
}

/** Clears just Active console messages */
void clearActiveConsole()
{
	ActiveMessages.clear();
}

/** Clears all console messages */
void	flushConsoleMessages()
{
	ActiveMessages.clear();
	TeamMessages.clear();
	HistoryMessages.clear();
}

/** Sets console text color depending on message type */
static PIELIGHT getConsoleTextColor(int player)
{
	// System messages
	if (player == SYSTEM_MESSAGE)
	{
		return WZCOL_CONS_TEXT_SYSTEM;
	}
	else if (player == NOTIFY_MESSAGE)
	{
		return WZCOL_YELLOW;
	}
	else if (player == INFO_MESSAGE)
	{
		return WZCOL_CONS_TEXT_INFO;
	}
	else
	{
		// Don't use friend-foe colors in the lobby
		if (bEnemyAllyRadarColor && (GetGameMode() == GS_NORMAL))
		{
			if (aiCheckAlliances(player, selectedPlayer))
			{
				return WZCOL_CONS_TEXT_USER_ALLY;
			}
			else
			{
				return WZCOL_CONS_TEXT_USER_ENEMY;
			}
		}
		// Friend-foe is off
		return WZCOL_TEXT_BRIGHT;
	}
}

static void console_drawtext(WzText &display, PIELIGHT colour, int x, int y, CONSOLE_TEXT_JUSTIFICATION justify, int width)
{
	switch (justify)
	{
	case LEFT_JUSTIFY:
		break; // do nothing
	case RIGHT_JUSTIFY:
		x = x + width - display.width();
		break;
	case CENTRE_JUSTIFY:
		x = x + (width - display.width()) / 2;
		break;
	}
	display.render(x, y, colour);
}

// Show global (mode=true) or team (mode=false) history messages
void displayOldMessages(bool mode)
{
	int startpos = 0;
	std::deque<CONSOLE_MESSAGE> *WhichMessages;

	if (mode)
	{
		WhichMessages = &TeamMessages;
	}
	else
	{
		WhichMessages = &HistoryMessages;
	}
	if (!WhichMessages->empty())
	{
		unsigned int count = WhichMessages->size();	// total number of messages
		if (count > NumDisplayLines)	// if we have more than we can display
		{
			startpos = count - NumDisplayLines;	// show last X lines
			startpos += updatepos;	// unless user wants to start at something else
			if (startpos < 0)		// don't underflow
			{
				startpos = 0;
				updatepos = (count - NumDisplayLines) * -1; // reset back to first entry
				count = NumDisplayLines;
			}
			else if (count + updatepos <= count)
			{
				count += updatepos;		// user may want something different
			}
			else
			{
				// reset all, we got overflow
				count = WhichMessages->size();
				updatepos = 0;
				startpos = count - NumDisplayLines;
			}
		}

		int nudgeright = 0;
		int TextYpos = historyConsole.topY + linePitch - 2;

		if (isSecondaryWindowUp())	// see if (build/research/...)window is up
		{
			nudgeright = RET_FORMWIDTH + 2; // move text over
		}
		// if user wants to add a bit more contrast to the text
		if (showBackgroundColor)
		{
			iV_TransBoxFill(historyConsole.topX + nudgeright - CON_BORDER_WIDTH, historyConsole.topY - historyConsole.textDepth - CON_BORDER_HEIGHT,
			                historyConsole.topX + historyConsole.width, historyConsole.topY + (NumDisplayLines * linePitch) + CON_BORDER_HEIGHT);
		}
		for (int i = startpos; i < count; ++i)
		{
			PIELIGHT colour = mode ? WZCOL_CONS_TEXT_USER_ALLY : getConsoleTextColor((*WhichMessages)[i].player);
			console_drawtext((*WhichMessages)[i].display, colour, historyConsole.topX + nudgeright, TextYpos, (*WhichMessages)[i].JustifyType, historyConsole.width);
			TextYpos += (*WhichMessages)[i].display.lineSize();
		}
	}
}

/** Displays all the console messages */
void	displayConsoleMessages()
{
	// Check if we have any messages we want to show
	if (!getNumberConsoleMessages() && !bConsoleDropped && !InfoMessages.size())
	{
		return;
	}

	// scripts can disable the console
	if (!bConsoleDisplayEnabled && !InfoMessages.size())
	{
		return;
	}

	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(false);

	if (bConsoleDropped)
	{
		displayOldMessages(HistoryMode);
	}

	if (InfoMessages.size())
	{
		auto i = InfoMessages.end() - 1;		// we can only show the last one...
		int tmp = pie_GetVideoBufferWidth();
		drawBlueBox(0, 0,tmp, 18);
		tmp -= i->display.width();
		console_drawtext(i->display, getConsoleTextColor(i->player), tmp - 6, linePitch - 2, i->JustifyType, i->display.width());
	}
	int TextYpos = mainConsole.topY;
	// Draw the blue background for the text (only in game, not lobby)
	if (bTextBoxActive && GetGameMode() == GS_NORMAL)
	{
		iV_TransBoxFill(mainConsole.topX - CON_BORDER_WIDTH, mainConsole.topY - mainConsole.textDepth - CON_BORDER_HEIGHT,
						mainConsole.topX + mainConsole.width, mainConsole.topY + (getNumberConsoleMessages() * linePitch) + CON_BORDER_HEIGHT - linePitch);
	}
	for (auto & ActiveMessage : ActiveMessages)
	{
		console_drawtext(ActiveMessage.display, getConsoleTextColor(ActiveMessage.player), mainConsole.topX, TextYpos, ActiveMessage.JustifyType, mainConsole.width);
		TextYpos += ActiveMessage.display.lineSize();
	}
}

/** Allows toggling of the box under the console text */
void	setConsoleBackdropStatus(bool state)
{
	bTextBoxActive = state;
}

/**
	Turns on and off display of console. It's worth
	noting that this is just the display so if you want
	to make sure that when it's turned back on again, there
	are no messages, the call flushConsoleMessages first.
*/
void	enableConsoleDisplay(bool state)
{
	bConsoleDisplayEnabled = state;
}

/** Allows positioning of the console on screen */
void	setConsoleSizePos(UDWORD x, UDWORD y, UDWORD width)
{
	mainConsole.topX = x;
	mainConsole.topY = y;
	mainConsole.width = width;

	/* Should be done below */
	mainConsole.textDepth = 8;
	flushConsoleMessages();
}

/**	Establishes whether the console messages stay there */
void	setConsolePermanence(bool state, bool bClearOld)
{
	if (mainConsole.permanent == true && state == false)
	{
		if (bClearOld)
		{
			flushConsoleMessages();
		}
		mainConsole.permanent = false;
	}
	else
	{
		if (bClearOld)
		{
			flushConsoleMessages();
		}
		mainConsole.permanent = state;
	}
}

/** Check if mouse is over the Active console 'window' area */
bool mouseOverConsoleBox()
{
	int gotMessages = getNumberConsoleMessages();
	if (gotMessages &&
		((UDWORD)mouseX() > mainConsole.topX)
		&& ((UDWORD)mouseY() > mainConsole.topY)
		&& ((UDWORD)mouseX() < mainConsole.topX + mainConsole.width)
		&& ((UDWORD)mouseY() < (mainConsole.topY + 4 + linePitch * gotMessages)))
	{
		return true;
	}
	return false;
}

/** Check if mouse is over the History console 'window' area */
bool	mouseOverHistoryConsoleBox()
{
	int nudgeright = 0;
	if (isSecondaryWindowUp())
	{	// if a build/research/... is up, we need to move text over by this much
		nudgeright = RET_FORMWIDTH;
	}
	// enable below to see the hitbox of the history console window
#if 0
	if (isSecondaryWindowUp())
	{
		iV_Box2(historyConsole.topX + nudgeright, historyConsole.topY, historyConsole.topX + historyConsole.width, (historyConsole.topY + 4 + linePitch * NumDisplayLines), WZCOL_RED, WZCOL_GREEN);

	}
	else
	{
		iV_Box2(historyConsole.topX, historyConsole.topY, historyConsole.topX + historyConsole.width, (historyConsole.topY + 4 + linePitch * NumDisplayLines), WZCOL_GREY, WZCOL_GREY);
	}
#endif
	// check to see if mouse is in the area when console is enabled
	if	(bConsoleDropped &&
	     ((UDWORD)mouseX() > historyConsole.topX + nudgeright)
	     && ((UDWORD)mouseY() > historyConsole.topY)
	     && ((UDWORD)mouseX() < historyConsole.topX + historyConsole.width)
	     && ((UDWORD)mouseY() < (historyConsole.topY + 4 + linePitch * NumDisplayLines)))
	{
		if (mousePressed(MOUSE_WUP))
		{
			updatepos--;
		}
		else if (mousePressed(MOUSE_WDN))
		{
			updatepos++;
		}
		if (keyDown(KEY_LCTRL))
		{
			showBackgroundColor = true;
		}
		else
		{
			showBackgroundColor = false;
		}
		return (true);
	}
	else
	{
		return (false);
	}
}

/** Sets up how many lines are allowed and how many are visible */
void	setConsoleLineInfo(UDWORD vis)
{
	ASSERT(vis <= MAX_CONSOLE_MESSAGES, "Request for more visible lines in the console than exist");
	consoleVisibleLines = vis;
}

/** get how many lines are allowed and how many are visible */
UDWORD getConsoleLineInfo()
{
	return consoleVisibleLines;
}

/// Set if new messages may be added to the console
void	permitNewConsoleMessages(bool allow)
{
	allowNewMessages = allow;
}

/// \return the visibility of the console
bool	getConsoleDisplayStatus()
{
	return (bConsoleDisplayEnabled);
}

/** Use console() for when you want to display a console message,
    and keep it in the history logs.

	Use the macro CONPRINTF if you don't want it to be in the history logs.
**/
void console(const char *pFormat, ...)
{
	char		aBuffer[500];	// Output string buffer
	va_list		pArgs;			// Format arguments

	/* Print out the string */
	va_start(pArgs, pFormat);
	vsnprintf(aBuffer, sizeof(aBuffer), pFormat, pArgs);
	va_end(pArgs);

	/* Output it */
	addConsoleMessage(aBuffer, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
}
