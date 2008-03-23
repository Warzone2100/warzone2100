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

/** \file
	Functions for the in-game console.
*/

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/gamelib/gtime.h"
#include "basedef.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/rendmode.h"
#include "intimage.h"
#include "console.h"
#include "scriptextern.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "radar.h"

/* Alex McLean, Pumpkin Studios, EIDOS Interactive */

/** Is the console history on or off? */
static BOOL	bConsoleDropped = FALSE;

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
static BOOL		bTextBoxActive;

/** Is the console being displayed? */
static BOOL		bConsoleDisplayEnabled;

/** How many lines are displayed? */
static UDWORD	consoleVisibleLines;

/** Whether new messages are allowed to be added */
static int allowNewMessages;

/** What's the default justification? */
static CONSOLE_TEXT_JUSTIFICATION	defJustification;

static UDWORD	messageId;	// unique ID

/// Global string for new console messages.
char ConsoleString[MAX_CONSOLE_TMP_STRING_LENGTH];


/* MODULE CONSOLE PROTOTYPES */
void	consolePrintf				( char *layout, ... );
void	setConsoleSizePos			( UDWORD x, UDWORD y, UDWORD width );
BOOL	addConsoleMessage			( const char *messageText, CONSOLE_TEXT_JUSTIFICATION jusType, CONSOLE_TEXT_TYPE textType );
void	updateConsoleMessages		( void );
void	displayConsoleMessages		( void );
void	initConsoleMessages			( void );
void	setConsoleMessageDuration	( UDWORD time );
void	removeTopConsoleMessage		( void );
void	flushConsoleMessages		( void );
void	setConsoleBackdropStatus	( BOOL state );
void	enableConsoleDisplay		( BOOL state );
BOOL	getConsoleDisplayStatus		( void );
void	setDefaultConsoleJust		( CONSOLE_TEXT_JUSTIFICATION defJ );
void	setConsolePermanence		( BOOL state, BOOL bClearOld );
BOOL	mouseOverConsoleBox			( void );
void	setConsoleLineInfo			( UDWORD vis );
UDWORD	getConsoleLineInfo			( void );
void	permitNewConsoleMessages	( BOOL allow);
int		displayOldMessages			( void );
void	setConsoleTextColor			( CONSOLE_TEXT_TYPE type );
CONSOLE_TEXT_TYPE pickConsolePlayerTextType(UDWORD player1, UDWORD player2);

/** Sets the system up */
void	initConsoleMessages( void )
{
	messageIndex = 0;

	/* Console can extend to half screen height */
	maxDrop = ((pie_GetVideoBufferHeight()/iV_GetTextLineSize())/2);

	if(maxDrop>32) maxDrop = 32;

	consoleDrop = maxDrop;//MAX_DROP;

	dropState = DROP_CLOSED;

	/* No active messages to begin with */
	numActiveMessages = 0;

	lastDropChange = 0;

	bConsoleDropped = FALSE;

	/* Linked list is empty */
	consoleMessages = NULL;

	/* Setup how long messages are displayed for... */
	setConsoleMessageDuration(DEFAULT_MESSAGE_DURATION);

	/* No box under the text */
	setConsoleBackdropStatus(TRUE);

	/* Turn on the console display */
	enableConsoleDisplay(TRUE);

	/* Set left justification as default */
	setDefaultConsoleJust(LEFT_JUSTIFY);

	/*	Set up the console size and postion
		x,y,width */
	setConsoleSizePos(16, 16, pie_GetVideoBufferWidth()-32);

	setConsoleLineInfo(MAX_CONSOLE_MESSAGES/4 + 4);

	/* We're not initially having permanent messages */
	setConsolePermanence(FALSE,TRUE);

	/* Allow new messages */
	permitNewConsoleMessages(TRUE);
}

/** Open the console when it's closed and close it when it's open. */
void	toggleConsoleDrop( void )
{
	/* If it's closed ... */
	if(bConsoleDropped == FALSE)
	{
		dropState = DROP_DROPPING;
		consoleDrop = 0;
		bConsoleDropped = TRUE;

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
static BOOL _addConsoleMessage(const char *messageText, CONSOLE_TEXT_JUSTIFICATION jusType,
							   CONSOLE_TEXT_TYPE textType)
{
	int textLength;
	CONSOLE_MESSAGE	*psMessage;

	/* Just don't add it if there's too many already */
	if(numActiveMessages>=MAX_CONSOLE_MESSAGES-1)
	{
		return FALSE;
	}

	/* Don't allow it to be added if we've disabled adding of new messages */
	if(!allowNewMessages)
	{
		return FALSE ;
	}

	/* Is the string too long? */
	textLength = strlen(messageText);

	ASSERT( textLength<MAX_CONSOLE_STRING_LENGTH,
		"Attempt to add a message to the console that exceeds MAX_CONSOLE_STRING_LENGTH" );

	/* Are we using a defualt justification? */
	if(jusType == DEFAULT_JUSTIFY)
	{
		/* Then set it */
		jusType = defJustification;
	}

	consoleStorage[messageIndex].textType = textType;

	/* Precalculate and store (quicker!) the indent for justified text */
	switch(jusType)
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
		debug( LOG_ERROR, "Weirdy type of text justification for console print" );
		abort();
		break;
	}

	/* Copy over the text of the message */
	strlcpy(consoleStorage[messageIndex].text, messageText, sizeof(consoleStorage[messageIndex].text));

	/* Set the time when it was added - this might not be needed */
	consoleStorage[messageIndex].timeAdded = gameTime2;

	/* This is the present newest message */
	consoleStorage[messageIndex].psNext = NULL;

	consoleStorage[messageIndex].id = 0;

	/* Are there no messages? */
	if(consoleMessages == NULL)
	{
		consoleMessages = &consoleStorage[messageIndex];
	}
	else
	{
		/* Get to the last element in our message list */
		for(psMessage = consoleMessages; psMessage->psNext; psMessage = psMessage->psNext)
		{
			/* NOP */
			;
		}
		/* Add it to the end */
		psMessage->psNext = &consoleStorage[messageIndex];
	}

	/* Move on in our array */
	if(messageIndex++ >= MAX_CONSOLE_MESSAGES-1)
	{
		/* Reset */
		messageIndex = 0;
	}

	/* There's one more active console message */
	numActiveMessages++;
	return TRUE;
}

/// Wrapper for _addConsoleMessage
BOOL addConsoleMessage(const char *messageText, CONSOLE_TEXT_JUSTIFICATION jusType, 
					   CONSOLE_TEXT_TYPE textType)
{
	return _addConsoleMessage(messageText, jusType, textType);
}

/// \return The number of console messages currently active
UDWORD	getNumberConsoleMessages( void )
{
	return(numActiveMessages);
}

/** Update the console messages.
	This function will remove messages that are overdue.
*/
void	updateConsoleMessages( void )
{
	if(dropState == DROP_DROPPING)
	{
	 	if(gameTime - lastDropChange > DROP_STEP_INTERVAL)
		{
			lastDropChange = gameTime;
			if(++consoleDrop > maxDrop)
			{
				consoleDrop = maxDrop;
				dropState = DROP_STATIC;
			}
		}
	}
	else if (dropState == DROP_CLOSING)
	{
		if(gameTime - lastDropChange > DROP_STEP_INTERVAL)
		{
			lastDropChange = gameTime;
			if(consoleDrop)
			{
				consoleDrop--;
			}
			else
			{
				dropState = DROP_CLOSED;
				bConsoleDropped = FALSE;
			}
		}
	}

	/* Don't do anything for DROP_STATIC */

	/* If there are no messages or we're on permanent then exit */
 	if(consoleMessages == NULL || mainConsole.permanent)
	{
		return;
	}

	/* Time to kill the top one ?*/
	if(gameTime2 - consoleMessages->timeAdded > messageDuration)
	{
		consoleMessages->id = messageId++;
		/* Is this the only message? */
		if(consoleMessages->psNext == NULL)
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
	Specify how long messages will stay on screen.
*/
void	setConsoleMessageDuration(UDWORD time)
{
	messageDuration = time;
}

/**
	Remove the top message on screen.
	This and setConsoleMessageDuration should be sufficient to allow
	us to put up messages that stay there until we remove them
	ourselves - be sure and reset message duration afterwards
*/
void	removeTopConsoleMessage( void )
{
	/* No point unless there is at least one */
	if(consoleMessages!=NULL)
	{
		/* Is this the only message? */
		if(consoleMessages->psNext == NULL)
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
void	flushConsoleMessages( void )
{
	consoleMessages = NULL;
	numActiveMessages = 0;
	messageId = 0;
}

/** Choose an appropriate message type, which will result in
  appropriate console text color, depending on current radar
  type and whether source and destination players are in alliance. */
CONSOLE_TEXT_TYPE pickConsolePlayerTextType(UDWORD player1, UDWORD player2)
{
	if(bEnemyAllyRadarColor)
	{
		if(aiCheckAlliances(player1,player2))
		{
			return CONSOLE_USER_ALLY;
		}
		else
		{
			return CONSOLE_USER_ENEMY;
		}
	}

	return CONSOLE_USER;	// pick a default color if friend-foe radar colors are off
}

/** Sets console text color depending on message type */
void setConsoleTextColor(CONSOLE_TEXT_TYPE type)
{
	switch(type) // run relevant title screen code.
	{
		// System message: 'research complete' etc
		case CONSOLE_SYSTEM:
			iV_SetTextColour(WZCOL_CONS_TEXT_SYSTEM);
			break;
		// Human or AI Chat messages
		case CONSOLE_USER:
			iV_SetTextColour(WZCOL_CONS_TEXT_USER);
			break;
		case CONSOLE_USER_ALLY:
			iV_SetTextColour(WZCOL_CONS_TEXT_USER_ALLY);
			break;
		case CONSOLE_USER_ENEMY:
			iV_SetTextColour(WZCOL_CONS_TEXT_USER_ENEMY);
			break;
		// Currently debug output from scripts
		case CONSOLE_DEBUG:
			iV_SetTextColour(WZCOL_CONS_TEXT_USER);
			break;
		default:
			debug( LOG_ERROR, "unknown console message type" );
			abort();
	}
}


/** Displays all the console messages */
void	displayConsoleMessages( void )
{
	CONSOLE_MESSAGE *psMessage;
	int numProcessed;
	int linePitch;
	int boxDepth;
	int drop;
	int MesY;
	int clipDepth;
	int exceed;

	/* Are there any to display? */
	if(consoleMessages == NULL && !bConsoleDropped)
	{
		/* No point - so get out */
 		return;
	}

	/* Return if it's disabled */
	if(!bConsoleDisplayEnabled)
	{
		return;
	}

	/* Haven't done any yet */
	numProcessed = 0;

	/* Get the travel to the next line */
	linePitch = iV_GetTextLineSize();

	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(FALSE);

	drop = 0;
	if(bConsoleDropped)
	{
		drop = displayOldMessages();
	}
	if(consoleMessages==NULL)
	{
		return;
	}

	/* Do we want a box under it? */
	if(bTextBoxActive)
	{
		for(psMessage = consoleMessages,exceed = 0;
			psMessage && (numProcessed<consoleVisibleLines) && (exceed < 4); // ho ho ho!!!
			psMessage = psMessage->psNext)
		{
			if((UDWORD)iV_GetTextWidth(psMessage->text) > mainConsole.width)
			{
				exceed++;
			}
		}

		/* How big a box is necessary? */
		boxDepth = (numActiveMessages> consoleVisibleLines ? consoleVisibleLines-1 : numActiveMessages-1);

		/* Add on the extra - hope it doesn't exceed two lines! */
		boxDepth+=exceed;

		/* GET RID OF THE MAGIC NUMBERS BELOW */
		clipDepth = (mainConsole.topY+(boxDepth*linePitch)+CON_BORDER_HEIGHT+drop);
		if(clipDepth > (pie_GetVideoBufferHeight() - linePitch))
		{
			clipDepth = (pie_GetVideoBufferHeight() - linePitch);
		}

		iV_TransBoxFill(mainConsole.topX - CON_BORDER_WIDTH,mainConsole.topY-mainConsole.textDepth-CON_BORDER_HEIGHT+drop+1,
			mainConsole.topX+mainConsole.width ,clipDepth);
	}

	/* Stop when we've drawn enough or we're at the end */
	MesY = mainConsole.topY + drop;

	for(psMessage = consoleMessages,numProcessed = 0;
		psMessage && numProcessed<consoleVisibleLines && MesY < (pie_GetVideoBufferHeight()-linePitch);
		psMessage = psMessage->psNext)
	{

		/* Set text color depending on message type */
		setConsoleTextColor(psMessage->textType);

 		/* Draw the text string */
		MesY = iV_DrawFormattedText(psMessage->text, mainConsole.topX, MesY,
									mainConsole.width, psMessage->JustifyType);

		/* Move on */
		numProcessed++;
	}
}

/** Display up to the last 8 messages.
	\return The number of messages actually shown */
int displayOldMessages()
{
	int thisIndex;
	int i;
	int count;
	BOOL bGotIt;
	BOOL bQuit;
	int marker = 0;
	int linePitch;
	int MesY;

	/* Check there actually are any messages */
	thisIndex = messageId;
	count = 0;

	if(thisIndex)
	{
		bQuit = FALSE;
		while(!bQuit)
		{
			for(i=0,bGotIt = FALSE; i<MAX_CONSOLE_MESSAGES && !bGotIt; i++)
			{
				if(consoleStorage[i].id == thisIndex-1)
				{
					bGotIt = TRUE;
					marker = i;
				}
			}
			/* We found an older one */
			if(bGotIt)
			{
				history[count++] = marker;
			}
			else
			{
				bQuit = TRUE;	// count holds how many we got
			}
			if(thisIndex)
			{
			 	/* Look for an older one */
				thisIndex--;
			}
			else
			{
				bQuit = TRUE;	// We've reached the big bang - there is nothing older...
			}
			/* History can only hold so many */
			if(count>=consoleDrop)
			{
				bQuit = TRUE;
			}
		}
	}

	if(!count)
	{
		/* there are messages - just no old ones yet */
		return(0);
	}

	if(count)
	{
		/* Get the line pitch */
		linePitch = iV_GetTextLineSize();

		/* How big a box is necessary? */
		/* GET RID OF THE MAGIC NUMBERS BELOW */
		iV_TransBoxFill(mainConsole.topX - CON_BORDER_WIDTH,mainConsole.topY-mainConsole.textDepth-CON_BORDER_HEIGHT,
			mainConsole.topX+mainConsole.width ,mainConsole.topY+((count)*linePitch)+CON_BORDER_HEIGHT-linePitch);
	}
	/*
	if(count)
	{
		sprintf(buildData,"%s,%s",__TIME__,__DATE__);

		buildWidth = iV_GetTextWidth(buildData);

		iV_DrawText(buildData,((mainConsole.topX+mainConsole.width) - buildWidth - 16),
			mainConsole.topY);
	}
	*/
	MesY = mainConsole.topY;
	/* Render what we found */
	for(i=count-1; i>0; i--)
	{
		/* Set text color depending on message type */
		setConsoleTextColor(consoleStorage[history[i]].textType);

		/* Draw the text string */
		MesY = iV_DrawFormattedText(consoleStorage[history[i]].text,
                                    mainConsole.topX,
                                    MesY,
                                    mainConsole.width,
                                    consoleStorage[history[i]].JustifyType);
	}

	/* Set text color depending on message type */
	setConsoleTextColor(consoleStorage[history[0]].textType);

	/* Draw the top one */
	iV_DrawFormattedText(consoleStorage[history[0]].text,
	                     mainConsole.topX,
	                     MesY,
	                     mainConsole.width,
	                     consoleStorage[history[0]].JustifyType);

	/* Return how much to drop the existing console by... Fix this for lines>screenWIDTH */
	if(count)
	{
		return((count)*linePitch);
	}
	else
	{
		return(0);
	}
}


/** Allows toggling of the box under the console text */
void	setConsoleBackdropStatus(BOOL state)
{
	bTextBoxActive = state;
}

/**
	Turns on and off display of console. It's worth
	noting that this is just the display so if you want
	to make sure that when it's turned back on again, there
	are no messages, the call flushConsoleMessages first.
*/
void	enableConsoleDisplay(BOOL state)
{
	bConsoleDisplayEnabled = state;
}

/** Sets the default justification for text */
void	setDefaultConsoleJust(CONSOLE_TEXT_JUSTIFICATION defJ)
{
	switch(defJ)
	{
	case LEFT_JUSTIFY:
	case RIGHT_JUSTIFY:
	case CENTRE_JUSTIFY:
		defJustification = defJ;
		break;
	default:
		debug( LOG_ERROR, "Weird default text justification for console" );
		abort();
		break;
	}
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
void	setConsolePermanence(BOOL state, BOOL bClearOld)
{
 	if(mainConsole.permanent == TRUE && state == FALSE)
	{
		if(bClearOld)
		{
			flushConsoleMessages();
		}
		mainConsole.permanent = FALSE;
	}
	else
	{
		if(bClearOld)
		{
			flushConsoleMessages();
		}
		mainConsole.permanent = state;
	}
}

/** TRUE or FALSE as to whether the mouse is presently over the console window */
BOOL	mouseOverConsoleBox( void )
{
	if	(
		((UDWORD)mouseX() > mainConsole.topX)	// condition 1
		&& ((UDWORD)mouseY() > mainConsole.topY)	// condition 2
		&& ((UDWORD)mouseX() < mainConsole.topX + mainConsole.width)	//condition 3
		&& ((UDWORD)mouseY() < (mainConsole.topY + iV_GetTextLineSize()*numActiveMessages))	//condition 4
	)
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

/** Sets up how many lines are allowed and how many are visible */
void	setConsoleLineInfo(UDWORD vis)
{
	ASSERT( vis<=MAX_CONSOLE_MESSAGES,"Request for more visible lines in the console than exist" );
	consoleVisibleLines = vis;
}

/** get how many lines are allowed and how many are visible */
UDWORD getConsoleLineInfo(void)
{
	return consoleVisibleLines;
}

/// Function with printf arguments to print to the console
void	consolePrintf(char *layout, ...)
{
char	consoleString[MAX_CONSOLE_STRING_LENGTH];
va_list	arguments;		// Formatting info

	/* Boot off the argument List */
	va_start(arguments,layout);

	/* 'print' it out into our buffer */
	vsnprintf(consoleString, sizeof(consoleString), layout, arguments);
	// Guarantee to nul-terminate
	consoleString[sizeof(consoleString) - 1] = '\0';

	/* Add the message through the normal channels! */
	addConsoleMessage(consoleString,DEFAULT_JUSTIFY,CONSOLE_SYSTEM);

	/* Close arguments */
	va_end(arguments);
}

/// Set if new messages may be added to the console
void	permitNewConsoleMessages(BOOL allow)
{
	allowNewMessages = allow;
}

/// \return the visibility of the console
BOOL	getConsoleDisplayStatus( void )
{
	return(bConsoleDisplayEnabled);
}

/** output warnings directly to the in-game console */
void printf_console(const char *pFormat, ...)
{
#ifdef DEBUG
	char		aBuffer[500];   // Output string buffer
    va_list		pArgs;					  // Format arguments

	/* Initialise the argument list */
	va_start(pArgs, pFormat);

	/* Print out the string */
	vsnprintf(aBuffer, sizeof(aBuffer), pFormat, pArgs);
	// Guarantee to nul-terminate
	aBuffer[sizeof(aBuffer) - 1] = '\0';

	/* Output it */

	addConsoleMessage(aBuffer,RIGHT_JUSTIFY,CONSOLE_SYSTEM);		//debug messages are displayed right-aligned
#endif
}

/** like printf_console, but for release */
void console(const char *pFormat, ...)
{
	char		aBuffer[500];   // Output string buffer
    va_list		pArgs;					  // Format arguments

	/* Initialise the argument list */
	va_start(pArgs, pFormat);

	/* Print out the string */
	vsnprintf(aBuffer, sizeof(aBuffer), pFormat, pArgs);
	// Guarantee to nul-terminate
	aBuffer[sizeof(aBuffer) - 1] = '\0';

	/* Output it */
	addConsoleMessage(aBuffer,DEFAULT_JUSTIFY,CONSOLE_SYSTEM);

}
