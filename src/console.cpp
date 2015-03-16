/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

#include "ai.h"
#include "console.h"
#include "main.h"
#include "radar.h"

#include <string>
#include <istream>

/* Alex McLean, Pumpkin Studios, EIDOS Interactive */

#define	DEFAULT_MESSAGE_DURATION		GAME_TICKS_PER_SEC * 8

#define CON_BORDER_WIDTH				4
#define CON_BORDER_HEIGHT				4

struct CONSOLE
{
	UDWORD	topX;
	UDWORD	topY;
	UDWORD	width;
	UDWORD	textDepth;
	bool	permanent;
};

/* Definition of a message */
struct CONSOLE_MESSAGE
{
	char				text[MAX_CONSOLE_STRING_LENGTH];		// Text of the message
	UDWORD				timeAdded;								// When was it added to our list?
	UDWORD				JustifyType;
	UDWORD				id;
	SDWORD				player;						// Player who sent this message or SYSTEM_MESSAGE
	CONSOLE_MESSAGE                *psNext;
};

/** Is the console history on or off? */
static bool	bConsoleDropped = false;

/** Stores the console dimensions and states */
static CONSOLE mainConsole;

/** Static storage for the maximum possible number of console messages */
static CONSOLE_MESSAGE	consoleStorage[MAX_CONSOLE_MESSAGES];

/** Maximum drop */
#define	MAX_DROP	32
static UDWORD	history[MAX_DROP];

/** Pointer to linked list of active messages - points to elements of the array history */
static CONSOLE_MESSAGE	*consoleMessages;

/** Where in the array are we - it's cyclic */
static UDWORD	messageIndex;

/** How many lines in the console history */
static UDWORD	consoleDrop = MAX_DROP;

static	UDWORD	maxDrop;

#define DROP_DROPPING	1
#define DROP_CLOSING	2
#define DROP_STATIC		3
#define DROP_CLOSED		4
#define DROP_STEP_INTERVAL	(15)

/** Console history state */
static UDWORD	dropState;

/** How many messages are presently active? */
static UDWORD	numActiveMessages;

/** How long do messages last for? */
static UDWORD	messageDuration;

static UDWORD	lastDropChange = 0;

/** Is there a box under the console text? */
static bool		bTextBoxActive;

/** Is the console being displayed? */
static bool		bConsoleDisplayEnabled;

/** How many lines are displayed? */
static UDWORD	consoleVisibleLines;

/** Whether new messages are allowed to be added */
static int allowNewMessages;

static UDWORD	messageId;	// unique ID

/// Global string for new console messages.
char ConsoleString[MAX_CONSOLE_TMP_STRING_LENGTH];


/**
	Specify how long messages will stay on screen.
*/
static void	setConsoleMessageDuration(UDWORD time)
{
	messageDuration = time;
}

/** Sets the system up */
void	initConsoleMessages(void)
{
	int TextLineSize = iV_GetTextLineSize();
	messageIndex = 0;

	/* Console can extend to half screen height */
	if (TextLineSize)
	{
		maxDrop = ((pie_GetVideoBufferHeight() / TextLineSize) / 2);
	}
	else
	{
		debug(LOG_FATAL, "Something is wrong with the fonts? Aborting.");
		abort();
	}

	if (maxDrop > 32)
	{
		maxDrop = 32;
	}

	consoleDrop = maxDrop;//MAX_DROP;

	dropState = DROP_CLOSED;

	/* No active messages to begin with */
	numActiveMessages = 0;

	lastDropChange = 0;

	bConsoleDropped = false;

	/* Linked list is empty */
	consoleMessages = NULL;

	/* Setup how long messages are displayed for... */
	setConsoleMessageDuration(DEFAULT_MESSAGE_DURATION);

	/* No box under the text */
	setConsoleBackdropStatus(true);

	/* Turn on the console display */
	enableConsoleDisplay(true);

	/*	Set up the console size and postion
		x,y,width */
	setConsoleSizePos(16, 16, pie_GetVideoBufferWidth() - 32);

	setConsoleLineInfo(MAX_CONSOLE_MESSAGES / 4 + 4);

	/* We're not initially having permanent messages */
	setConsolePermanence(false, true);

	/* Allow new messages */
	permitNewConsoleMessages(true);
}

/** Open the console when it's closed and close it when it's open. */
void	toggleConsoleDrop(void)
{
	/* If it's closed ... */
	if (bConsoleDropped == false)
	{
		dropState = DROP_DROPPING;
		consoleDrop = 0;
		bConsoleDropped = true;

		audio_PlayTrack(ID_SOUND_WINDOWOPEN);
	}
	else
	{
		/* It's already open (or opening) */
		dropState = DROP_CLOSING;
		audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
	}
}

/** Add a string to the console. */
bool addConsoleMessage(const char *Text, CONSOLE_TEXT_JUSTIFICATION jusType, SDWORD player)
{
	int textLength;
	CONSOLE_MESSAGE	*psMessage;

	/* Just don't add it if there's too many already */
	if (numActiveMessages >= MAX_CONSOLE_MESSAGES - 1)
	{
		return false;
	}

	/* Don't allow it to be added if we've disabled adding of new messages */
	if (!allowNewMessages)
	{
		return false ;
	}

	std::istringstream stream(Text);
	std::string lines;
	char messageText[MAX_CONSOLE_STRING_LENGTH];

	while (std::getline(stream, lines))
	{
		sstrcpy(messageText, lines.c_str());

		/* Is the string too long? */
		textLength = strlen(messageText);

		ASSERT(textLength < MAX_CONSOLE_STRING_LENGTH, "Attempt to add a message to the console that exceeds MAX_CONSOLE_STRING_LENGTH");

		debug(LOG_CONSOLE, "(to player %d): %s", (int)player, messageText);

		consoleStorage[messageIndex].player = player;

		/* Precalculate and store (quicker!) the indent for justified text */
		switch (jusType)
		{
		/* Allign to left edge of screen */
		case LEFT_JUSTIFY:
			consoleStorage[messageIndex].JustifyType = FTEXT_LEFTJUSTIFY;
			break;

		/* Allign to right edge of screen */
		case RIGHT_JUSTIFY:
			consoleStorage[messageIndex].JustifyType = FTEXT_RIGHTJUSTIFY;
			break;

		/* Allign to centre of the screen,NOT TO CENTRE OF CONSOLE!!!!!! */
		case CENTRE_JUSTIFY:
			consoleStorage[messageIndex].JustifyType = FTEXT_CENTRE;
			break;
		/* Gone tits up by the looks of it */
		default:
			debug(LOG_FATAL, "Weirdy type of text justification for console print");
			abort();
			break;
		}

		/* Copy over the text of the message */
		sstrcpy(consoleStorage[messageIndex].text, messageText);

		/* Set the time when it was added - this might not be needed */
		consoleStorage[messageIndex].timeAdded = realTime;

		/* This is the present newest message */
		consoleStorage[messageIndex].psNext = NULL;

		consoleStorage[messageIndex].id = 0;

		/* Are there no messages? */
		if (consoleMessages == NULL)
		{
			consoleMessages = &consoleStorage[messageIndex];
		}
		else
		{
			/* Get to the last element in our message list */
			for (psMessage = consoleMessages; psMessage->psNext; psMessage = psMessage->psNext)
			{
				/* NOP */
				;
			}
			/* Add it to the end */
			psMessage->psNext = &consoleStorage[messageIndex];
		}

		/* Move on in our array */
		if (messageIndex++ >= MAX_CONSOLE_MESSAGES - 1)
		{
			/* Reset */
			messageIndex = 0;
		}

		/* There's one more active console message */
		numActiveMessages++;
	}
	return true;
}


/// \return The number of console messages currently active
UDWORD	getNumberConsoleMessages(void)
{
	return (numActiveMessages);
}

/** Update the console messages.
	This function will remove messages that are overdue.
*/
void	updateConsoleMessages(void)
{
	if (dropState == DROP_DROPPING)
	{
		if (gameTime - lastDropChange > DROP_STEP_INTERVAL)
		{
			lastDropChange = gameTime;
			if (++consoleDrop > maxDrop)
			{
				consoleDrop = maxDrop;
				dropState = DROP_STATIC;
			}
		}
	}
	else if (dropState == DROP_CLOSING)
	{
		if (gameTime - lastDropChange > DROP_STEP_INTERVAL)
		{
			lastDropChange = gameTime;
			if (consoleDrop)
			{
				consoleDrop--;
			}
			else
			{
				dropState = DROP_CLOSED;
				bConsoleDropped = false;
			}
		}
	}

	/* Don't do anything for DROP_STATIC */

	/* If there are no messages or we're on permanent then exit */
	if (consoleMessages == NULL || mainConsole.permanent)
	{
		return;
	}

	/* Time to kill the top one ?*/
	if (realTime - consoleMessages->timeAdded > messageDuration)
	{
		consoleMessages->id = messageId++;
		/* Is this the only message? */
		if (consoleMessages->psNext == NULL)
		{
			/* Then list is now empty */
			consoleMessages = NULL;
		}
		else
		{
			/* Otherwise point it at the next one */
			consoleMessages = consoleMessages->psNext;
		}
		/* There's one less active console message */
		numActiveMessages--;
	}
}

/**
	Remove the top message on screen.
	This and setConsoleMessageDuration should be sufficient to allow
	us to put up messages that stay there until we remove them
	ourselves - be sure and reset message duration afterwards
*/
void	removeTopConsoleMessage(void)
{
	/* No point unless there is at least one */
	if (consoleMessages != NULL)
	{
		/* Is this the only message? */
		if (consoleMessages->psNext == NULL)
		{
			/* Then list is now empty */
			consoleMessages = NULL;
		}
		else
		{
			/* Otherwise point it at the next one */
			consoleMessages = consoleMessages->psNext;
		}
		/* There's one less active console message */
		numActiveMessages--;
	}
}

/** Clears all console messages */
void	flushConsoleMessages(void)
{
	consoleMessages = NULL;
	numActiveMessages = 0;
	messageId = 0;
}

/** Sets console text color depending on message type */
static void setConsoleTextColor(SDWORD player)
{
	// System messages
	if (player == SYSTEM_MESSAGE)
	{
		iV_SetTextColour(WZCOL_CONS_TEXT_SYSTEM);
	}
	else if (player == NOTIFY_MESSAGE)
	{
		iV_SetTextColour(WZCOL_YELLOW);
	}
	else
	{
		// Don't use friend-foe colors in the lobby
		if (bEnemyAllyRadarColor && (GetGameMode() == GS_NORMAL))
		{
			if (aiCheckAlliances(player, selectedPlayer))
			{
				iV_SetTextColour(WZCOL_CONS_TEXT_USER_ALLY);
			}
			else
			{
				iV_SetTextColour(WZCOL_CONS_TEXT_USER_ENEMY);
			}
		}
		else
		{
			// Friend-foe is off
			iV_SetTextColour(WZCOL_CONS_TEXT_USER);
		}
	}
}


/** Display up to the last 8 messages.
	\return The number of messages actually shown */
static int displayOldMessages(void)
{
	int i;
	bool bGotIt;
	bool bQuit;
	int marker = 0;
	int linePitch;
	int MesY;
	unsigned int count = 0;

	/* Check there actually are any messages */
	int thisIndex = messageId;

	if (thisIndex)
	{
		bQuit = false;
		while (!bQuit)
		{
			for (i = 0, bGotIt = false; i < MAX_CONSOLE_MESSAGES && !bGotIt; i++)
			{
				if (consoleStorage[i].id == thisIndex - 1)
				{
					bGotIt = true;
					marker = i;
				}
			}
			/* We found an older one */
			if (bGotIt)
			{
				history[count++] = marker;
			}
			else
			{
				bQuit = true;	// count holds how many we got
			}
			if (thisIndex)
			{
				/* Look for an older one */
				thisIndex--;
			}
			else
			{
				bQuit = true;	// We've reached the big bang - there is nothing older...
			}
			/* History can only hold so many */
			if (count >= consoleDrop)
			{
				bQuit = true;
			}
		}
	}

	if (!count)
	{
		/* there are messages - just no old ones yet */
		return (0);
	}
	else
	{
		/* Get the line pitch */
		linePitch = iV_GetTextLineSize();

		/* How big a box is necessary? */
		/* GET RID OF THE MAGIC NUMBERS BELOW */
		iV_TransBoxFill(mainConsole.topX - CON_BORDER_WIDTH, mainConsole.topY - mainConsole.textDepth - CON_BORDER_HEIGHT,
		                mainConsole.topX + mainConsole.width , mainConsole.topY + ((count)*linePitch) + CON_BORDER_HEIGHT - linePitch);
	}
	MesY = mainConsole.topY;
	/* Render what we found */
	for (i = count - 1; i > 0; i--)
	{
		/* Set text color depending on message type */
		setConsoleTextColor(consoleStorage[history[i]].player);

		/* Draw the text string */
		MesY = iV_DrawFormattedText(consoleStorage[history[i]].text,
		                            mainConsole.topX,
		                            MesY,
		                            mainConsole.width,
		                            consoleStorage[history[i]].JustifyType);
	}

	/* Set text color depending on message type */
	setConsoleTextColor(consoleStorage[history[0]].player);

	/* Draw the top one */
	iV_DrawFormattedText(consoleStorage[history[0]].text,
	                     mainConsole.topX,
	                     MesY,
	                     mainConsole.width,
	                     consoleStorage[history[0]].JustifyType);

	/* Return how much to drop the existing console by... Fix this for lines>screenWIDTH */
	return count;
}


/** Displays all the console messages */
void	displayConsoleMessages(void)
{
	CONSOLE_MESSAGE *psMessage;
	int linePitch;
	int boxDepth;
	int drop;
	int MesY;
	int clipDepth;
	unsigned int exceed, numProcessed;

	/* Are there any to display? */
	if (consoleMessages == NULL && !bConsoleDropped)
	{
		/* No point - so get out */
		return;
	}

	/* Return if it's disabled */
	if (!bConsoleDisplayEnabled)
	{
		return;
	}

	iV_SetFont(font_regular);

	/* Get the travel to the next line */
	linePitch = iV_GetTextLineSize();

	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(false);

	drop = 0;
	if (bConsoleDropped)
	{
		drop = displayOldMessages();
	}
	if (consoleMessages == NULL)
	{
		return;
	}

	/* Do we want a box under it? */
	if (bTextBoxActive)
	{
		for (psMessage = consoleMessages, exceed = 0;
		     psMessage && consoleVisibleLines > 0 && exceed < 4; // ho ho ho!!!
		     psMessage = psMessage->psNext)
		{
			if (iV_GetTextWidth(psMessage->text) > mainConsole.width)
			{
				++exceed;
			}
		}

		/* How big a box is necessary? */
		boxDepth = (numActiveMessages > consoleVisibleLines ? consoleVisibleLines - 1 : numActiveMessages - 1);

		/* Add on the extra - hope it doesn't exceed two lines! */
		boxDepth += exceed;

		/* GET RID OF THE MAGIC NUMBERS BELOW */
		clipDepth = (mainConsole.topY + (boxDepth * linePitch) + CON_BORDER_HEIGHT + drop);
		if (clipDepth > pie_GetVideoBufferHeight() - linePitch)
		{
			clipDepth = (pie_GetVideoBufferHeight() - linePitch);
		}

		iV_TransBoxFill(mainConsole.topX - CON_BORDER_WIDTH, mainConsole.topY - mainConsole.textDepth - CON_BORDER_HEIGHT + drop + 1,
		                mainConsole.topX + mainConsole.width , clipDepth);
	}

	/* Stop when we've drawn enough or we're at the end */
	MesY = mainConsole.topY + drop;

	for (psMessage = consoleMessages, numProcessed = 0;
	     psMessage && numProcessed < consoleVisibleLines && MesY < pie_GetVideoBufferHeight() - linePitch;
	     psMessage = psMessage->psNext)
	{

		/* Set text color depending on message type */
		setConsoleTextColor(psMessage->player);

		/* Draw the text string */
		MesY = iV_DrawFormattedText(psMessage->text, mainConsole.topX, MesY,
		                            mainConsole.width, psMessage->JustifyType);

		/* Move on */
		++numProcessed;
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

/** true or false as to whether the mouse is presently over the console window */
bool	mouseOverConsoleBox(void)
{
	if	(
	    ((UDWORD)mouseX() > mainConsole.topX)	// condition 1
	    && ((UDWORD)mouseY() > mainConsole.topY)	// condition 2
	    && ((UDWORD)mouseX() < mainConsole.topX + mainConsole.width)	//condition 3
	    && ((UDWORD)mouseY() < (mainConsole.topY + iV_GetTextLineSize()*numActiveMessages))	//condition 4
	)
	{
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
UDWORD getConsoleLineInfo(void)
{
	return consoleVisibleLines;
}

/// Set if new messages may be added to the console
void	permitNewConsoleMessages(bool allow)
{
	allowNewMessages = allow;
}

/// \return the visibility of the console
bool	getConsoleDisplayStatus(void)
{
	return (bConsoleDisplayEnabled);
}

/** like debug_console, but for release */
void console(const char *pFormat, ...)
{
	char		aBuffer[500];   // Output string buffer
	va_list		pArgs;					  // Format arguments

	/* Print out the string */
	va_start(pArgs, pFormat);
	vsnprintf(aBuffer, sizeof(aBuffer), pFormat, pArgs);
	va_end(pArgs);

	/* Output it */
	addConsoleMessage(aBuffer, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
}
