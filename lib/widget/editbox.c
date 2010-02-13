/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
 *  Functions for the edit box widget.
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/utf.h"
#include "widget.h"
#include "widgint.h"
#include "editbox.h"
#include "form.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/textdraw.h"
#include "scrap.h"


/* Pixel gap between edge of edit box and text */
#define WEDB_XGAP	4
#define WEDB_YGAP	2

/* Size of the overwrite cursor */
#define WEDB_CURSORSIZE		8

/* Whether the cursor blinks or not */
#define CURSOR_BLINK		1

/* The time the cursor blinks for */
#define WEDB_BLINKRATE		800

/* Number of characters to jump the edit box text when moving the cursor */
#define WEDB_CHARJUMP		6

/* Calculate how much of the start of a string can fit into the edit box */
static void fitStringStart(utf_32_char *pBuffer, UDWORD boxWidth, UWORD *pCount, UWORD *pCharWidth);

/* Create an edit box widget data structure */
W_EDITBOX* editBoxCreate(const W_EDBINIT* psInit)
{
	W_EDITBOX* psWidget;
	const char *text;

	if (psInit->style & ~(WEDB_PLAIN | WIDG_HIDDEN | WEDB_DISABLED))
	{
		ASSERT( false, "Unknown edit box style" );
		return NULL;
	}

	/* Allocate the required memory */
	psWidget = (W_EDITBOX *)malloc(sizeof(W_EDITBOX));
	if (psWidget == NULL)
	{
		debug(LOG_FATAL, "editBoxCreate: Out of memory");
		abort();
		return NULL;
	}

	/* Initialise the structure */
	psWidget->type = WIDG_EDITBOX;
	psWidget->id = psInit->id;
	psWidget->formID = psInit->formID;
	psWidget->style = psInit->style;
	psWidget->x = psInit->x;
	psWidget->y = psInit->y;
	psWidget->width = psInit->width;
	psWidget->height = psInit->height;
	psWidget->FontID = psInit->FontID;
	if (psInit->pDisplay)
	{
		psWidget->display = psInit->pDisplay;
	}
	else
	{
		psWidget->display = editBoxDisplay;
	}
	psWidget->callback = psInit->pCallback;
	psWidget->pUserData = psInit->pUserData;
	psWidget->UserData = psInit->UserData;
	psWidget->pBoxDisplay = psInit->pBoxDisplay;
	psWidget->pFontDisplay = psInit->pFontDisplay;

	psWidget->AudioCallback = WidgGetAudioCallback();
	psWidget->HilightAudioID = WidgGetHilightAudioID();
	psWidget->ClickedAudioID = WidgGetClickedAudioID();

	text = psInit->pText;
	if (!text)
	{
		text = "";
	}
	psWidget->aText = UTF8toUTF32(text, &psWidget->aTextAllocated);

	editBoxInitialise(psWidget);

	psWidget->blinkOffset = SDL_GetTicks();

	init_scrap();

	return psWidget;
}


/* Free the memory used by an edit box */
void editBoxFree(W_EDITBOX *psWidget)
{
	free(psWidget->aText);
	free(psWidget);
}


/* Initialise an edit box widget */
void editBoxInitialise(W_EDITBOX *psWidget)
{
	ASSERT( psWidget != NULL,
		"editBoxInitialise: Invalid edit box pointer" );

	psWidget->state = WEDBS_FIXED;
	psWidget->printStart = 0;
	iV_SetFont(psWidget->FontID);
	fitStringStart(psWidget->aText, psWidget->width,
		&psWidget->printChars, &psWidget->printWidth);
}


/* Insert a character into a text buffer */
static void insertChar(utf_32_char **pBuffer, size_t *pBufferAllocated, UDWORD *pPos, utf_32_char ch, W_EDITBOX *psWidget)
{
	utf_32_char	*pSrc, *pDest;
	UDWORD	len, count;

	if (ch == '\0') return;

	ASSERT( *pPos <= utf32len(*pBuffer), "Invalid insertion point" );
	ASSERT_OR_RETURN ( , psWidget, "No widget to modify?");

	len = utf32len(*pBuffer);

	if (len == *pBufferAllocated/sizeof(utf_32_char) - 1)
	{
		/* Buffer is full */
		utf_32_char *newBuffer = realloc(*pBuffer, *pBufferAllocated*2);
		if (!newBuffer)
		{
			debug(LOG_ERROR, "Couldn't realloc() string?");
			return;
		}
		// update old pointer from the widget
		psWidget->aText = *pBuffer = newBuffer;
		*pBufferAllocated *= 2;
	}

	/* Move the end of the string up by one (including terminating \0) */
	count = len - *pPos + 1;
	pSrc = *pBuffer + *pPos;
	pDest = pSrc + 1;
	memmove(pDest, pSrc, count*sizeof(utf_32_char));

	/* Insert the character */
	*pSrc = ch;

	/* Update the insertion point */
	*pPos += 1;
}


/* Put a character into a text buffer overwriting any text under the cursor */
static void overwriteChar(utf_32_char **pBuffer, size_t *pBufferAllocated, UDWORD *pPos, utf_32_char ch, W_EDITBOX *psWidget)
{
	UDWORD	len;

	if (ch == '\0') return;

	ASSERT( *pPos <= utf32len(*pBuffer),
		"insertChar: Invalid insertion point" );

	len = utf32len(*pBuffer);

	if (*pPos == len)
	{
		// At end of string.
		insertChar(pBuffer, pBufferAllocated, pPos, ch, psWidget);
		return;
	}

	/* Store the character */
	(*pBuffer)[*pPos] = ch;

	/* Update the insertion point */
	*pPos += 1;
}

/* Put a character into a text buffer overwriting any text under the cursor */
static void putSelection(utf_32_char **pBuffer, size_t *pBufferAllocated, UDWORD *pPos)
{
	static char* scrap = NULL;
	int scraplen;

	get_scrap(T('T','E','X','T'), &scraplen, &scrap);
	if (scraplen > 0 && scraplen < WIDG_MAXSTR-2)
	{
		free(*pBuffer);
		*pBuffer = UTF8toUTF32(scrap, pBufferAllocated);
		*pPos = *pBufferAllocated/sizeof(utf_32_char) - 1;
	}
}


/* Delete a character to the right of the position */
static void delCharRight(utf_32_char *pBuffer, UDWORD *pPos)
{
	utf_32_char	*pSrc, *pDest;
	UDWORD	len, count;

	ASSERT(*pPos <= utf32len(pBuffer), "Invalid insertion point" );

	len = utf32len(pBuffer);

	/* Can't delete if we are at the end of the string */
	if (*pPos == len)
	{
		return;
	}

	/* Move the end of the string down by one */
	count = len - *pPos;
	pDest = pBuffer + *pPos;
	pSrc = pDest + 1;
	memmove(pDest, pSrc, count*sizeof(utf_32_char));  // Moves nul too.
}


/* Delete a character to the left of the position */
static void delCharLeft(utf_32_char *pBuffer, UDWORD *pPos)
{
	/* Can't delete if we are at the start of the string */
	if (*pPos == 0)
	{
		return;
	}

	--*pPos;
	delCharRight(pBuffer, pPos);
}


/* Calculate how much of the start of a string can fit into the edit box */
static void fitStringStart(utf_32_char *pBuffer, UDWORD boxWidth, UWORD *pCount, UWORD *pCharWidth)
{
	UDWORD		len;
	UWORD		printWidth, printChars, width;
	utf_32_char		*pCurr;

	len = utf32len(pBuffer);
	printWidth = 0;
	printChars = 0;
	pCurr = pBuffer;

	/* Find the number of characters that will fit in boxWidth */
	while (printChars < len)
	{
		width = (UWORD)(printWidth + iV_GetCharWidth(*pCurr));
		if (width > boxWidth - WEDB_XGAP*2)
		{
			/* We've got as many characters as will fit in the box */
			break;
		}
		printWidth = width;
		printChars += 1;
		pCurr += 1;
	}

	/* Return the number of characters and their width */
	*pCount = printChars;
	*pCharWidth = printWidth;
}


/* Calculate how much of the end of a string can fit into the edit box */
static void fitStringEnd(utf_32_char *pBuffer, UDWORD boxWidth, UWORD *pStart, UWORD *pCount, UWORD *pCharWidth)
{
	UDWORD		len;
	UWORD		printWidth, printChars, width;
	utf_32_char		*pCurr;

	len = utf32len(pBuffer);

	pCurr = pBuffer + len - 1;
	printChars = 0;
	printWidth = 0;

	/* Find the number of characters that will fit in boxWidth */
	while (printChars < len)
	{
		width = (UWORD)(printWidth + iV_GetCharWidth(*pCurr));
		if (width > boxWidth - (WEDB_XGAP*2 + WEDB_CURSORSIZE))
		{
			/* Got as many characters as will fit into the box */
			break;
		}
		printWidth = width;
		printChars += 1;
		pCurr -= 1;
	}

	/* Return the number of characters and their width */
	*pStart = (UWORD)(len - printChars);
	*pCount = printChars;
	*pCharWidth = printWidth;
}


/* Run an edit box widget */
void editBoxRun(W_EDITBOX *psWidget, W_CONTEXT *psContext)
{

	UDWORD	key, len = 0, editState;
	UDWORD	pos;
	utf_32_char	*pBuffer, unicode;
	size_t	pBufferAllocated;
	BOOL	done;
	UWORD	printStart, printWidth, printChars;
	SDWORD	mx,my;

	/* Note the edit state */
	editState = psWidget->state & WEDBS_MASK;

	/* Only have anything to do if the widget is being edited */
	if ((editState & WEDBS_MASK) == WEDBS_FIXED)
	{
		return;
	}

	/* If there is a mouse click outside of the edit box - stop editing */
	mx = psContext->mx;
	my = psContext->my;
	if (mousePressed(MOUSE_LMB) &&
		(mx < psWidget->x ||
		 (mx > psWidget->x + psWidget->width) ||
		 my < psWidget->y ||
		 (my > psWidget->y + psWidget->height)))
	{
		screenClearFocus(psContext->psScreen);
		return;
	}

	/* note the widget state */
	pos = psWidget->insPos;
	pBuffer = psWidget->aText;
	pBufferAllocated = psWidget->aTextAllocated;
	printStart = psWidget->printStart;
	printWidth = psWidget->printWidth;
	printChars = psWidget->printChars;
	iV_SetFont(psWidget->FontID);

	/* Loop through the characters in the input buffer */
	done = false;
	for (key = inputGetKey(&unicode); key != 0 && !done; key = inputGetKey(&unicode))
	{
		// Don't blink while typing.
		psWidget->blinkOffset = SDL_GetTicks();

		/* Deal with all the control keys, assume anything else is a printable character */
		switch (key)
		{
		case INPBUF_LEFT :
			/* Move the cursor left */
			if (pos > 0)
			{
				pos -= 1;
			}

			/* If the cursor has gone off the left of the edit box,
			 * need to update the printable text.
			 */
			if (pos < printStart)
			{
				if (printStart <= WEDB_CHARJUMP)
				{
					/* Got to the start of the string */
					printStart = 0;
					fitStringStart(pBuffer, psWidget->width, &printChars, &printWidth);
				}
				else
				{
					printStart -= WEDB_CHARJUMP;
					fitStringStart(pBuffer + printStart, psWidget->width,
						&printChars, &printWidth);
				}
			}
			debug(LOG_INPUT, "EditBox cursor left");
			break;
		case INPBUF_RIGHT :
			/* Move the cursor right */
			len = utf32len(pBuffer);
			if (pos < len)
			{
				pos += 1;
			}

			/* If the cursor has gone off the right of the edit box,
			 * need to update the printable text.
			 */
			if (pos > (UDWORD)(printStart + printChars))
			{
				printStart += WEDB_CHARJUMP;
				if (printStart >= len)
				{
					printStart = (UWORD)(len - 1);
				}
				fitStringStart(pBuffer + printStart, psWidget->width,
					&printChars, &printWidth);
			}
			debug(LOG_INPUT, "EditBox cursor right");
			break;
		case INPBUF_UP :
			debug(LOG_INPUT, "EditBox cursor up");
			break;
		case INPBUF_DOWN :
			debug(LOG_INPUT, "EditBox cursor down");
			break;
		case INPBUF_HOME :
			/* Move the cursor to the start of the buffer */
			pos = 0;
			printStart = 0;
			fitStringStart(pBuffer, psWidget->width, &printChars, &printWidth);
			debug(LOG_INPUT, "EditBox cursor home");
			break;
		case INPBUF_END :
			/* Move the cursor to the end of the buffer */
			pos = utf32len(pBuffer);
			if (pos != (UWORD)(printStart + printChars))
			{
				fitStringEnd(pBuffer, psWidget->width, &printStart, &printChars, &printWidth);
			}
			debug(LOG_INPUT, "EditBox cursor end");
			break;
		case INPBUF_INS :
			if (editState == WEDBS_INSERT)
			{
				editState = WEDBS_OVER;
			}
			else
			{
				editState = WEDBS_INSERT;
			}
			debug(LOG_INPUT, "EditBox cursor insert");
			break;
		case INPBUF_DEL :
			delCharRight(pBuffer, &pos);

			/* Update the printable text */
			fitStringStart(pBuffer + printStart, psWidget->width,
				&printChars, &printWidth);
			debug(LOG_INPUT, "EditBox cursor delete");
			break;
		case INPBUF_PGUP :
			debug(LOG_INPUT, "EditBox cursor page up");
			break;
		case INPBUF_PGDN :
			debug(LOG_INPUT, "EditBox cursor page down");
			break;
		case INPBUF_BKSPACE :
			/* Delete the character to the left of the cursor */
			delCharLeft(pBuffer, &pos);

			/* Update the printable text */
			if (pos <= printStart)
			{
				if (printStart <= WEDB_CHARJUMP)
				{
					/* Got to the start of the string */
					printStart = 0;
					fitStringStart(pBuffer, psWidget->width, &printChars, &printWidth);
				}
				else
				{
					printStart -= WEDB_CHARJUMP;
					fitStringStart(pBuffer + printStart, psWidget->width,
						&printChars, &printWidth);
				}
			}
			else
			{
				fitStringStart(pBuffer + printStart, psWidget->width,
					&printChars, &printWidth);
			}
			debug(LOG_INPUT, "EditBox cursor backspace");
			break;
		case INPBUF_TAB :
			putSelection(&pBuffer, &pBufferAllocated, &pos);

			/* Update the printable text */
			fitStringEnd(pBuffer, psWidget->width, &printStart, &printChars, &printWidth);
			debug(LOG_INPUT, "EditBox cursor tab");
			break;
		case INPBUF_CR :
		case SDLK_KP_ENTER:		// either normal return key || keypad enter
			/* Finish editing */
			editBoxFocusLost(psContext->psScreen, psWidget);
			screenClearFocus(psContext->psScreen);
			debug(LOG_INPUT, "EditBox cursor return");
			return;
			break;
		case INPBUF_ESC :
			debug(LOG_INPUT, "EditBox cursor escape");
			break;

		default:
			/* Dealt with everything else this must be a printable character */
			if (editState == WEDBS_INSERT)
			{
				insertChar(&pBuffer, &pBufferAllocated, &pos, unicode, psWidget);
			}
			else
			{
				overwriteChar(&pBuffer, &pBufferAllocated, &pos, unicode, psWidget);
			}

			/* Update the printable chars */
			if (pos == utf32len(pBuffer))
			{
				fitStringEnd(pBuffer, psWidget->width, &printStart, &printChars, &printWidth);
			}
			else
			{
				fitStringStart(pBuffer + printStart, psWidget->width, &printChars, &printWidth);
				if (pos > (UDWORD)(printStart + printChars))
				{
					printStart += WEDB_CHARJUMP;
					if (printStart >= len)
					{
						printStart = (UWORD)(len - 1);
						fitStringStart(pBuffer + printStart, psWidget->width,
							&printChars, &printWidth);
					}
				}
			}
			break;
		}
	}

	/* Store the current widget state */
	psWidget->aText = pBuffer;
	psWidget->aTextAllocated = pBufferAllocated;
	psWidget->insPos = (UWORD)pos;
	psWidget->state = (psWidget->state & ~WEDBS_MASK) | editState;
	psWidget->printStart = printStart;
	psWidget->printWidth = printWidth;
	psWidget->printChars = printChars;

}


/* Set the current string for the edit box */
void editBoxSetString(W_EDITBOX *psWidget, const char *pText)
{
	ASSERT( psWidget != NULL,
		"editBoxSetString: Invalid edit box pointer" );

	free(psWidget->aText);
	psWidget->aText = UTF8toUTF32(pText, &psWidget->aTextAllocated);

	psWidget->state = WEDBS_FIXED;
	psWidget->printStart = 0;
	iV_SetFont(psWidget->FontID);
	fitStringStart(psWidget->aText, psWidget->width,
	               &psWidget->printChars, &psWidget->printWidth);
}


/* Respond to a mouse click */
void editBoxClicked(W_EDITBOX *psWidget, W_CONTEXT *psContext)
{
	UDWORD		len;

	if(!psWidget || (psWidget->state & WEDBS_DISABLE))	// disabled button.
	{
		return;
	}

	if ((psWidget->state & WEDBS_MASK) == WEDBS_FIXED)
	{
		if(!(psWidget->style & WEDB_DISABLED)) {
			if(psWidget->AudioCallback) {
				psWidget->AudioCallback(psWidget->ClickedAudioID);
			}

			/* Set up the widget state */
			psWidget->state = (psWidget->state & ~WEDBS_MASK) | WEDBS_INSERT;
			len = utf32len(psWidget->aText);
			psWidget->insPos = (UWORD)len;

			/* Calculate how much of the string can appear in the box */
			iV_SetFont(psWidget->FontID);
			fitStringEnd(psWidget->aText, psWidget->width,
				&psWidget->printStart, &psWidget->printChars, &psWidget->printWidth);


			/* Clear the input buffer */
			inputClearBuffer();

			/* Tell the form that the edit box has focus */
			screenSetFocus(psContext->psScreen, (WIDGET *)psWidget);

			// Cursor should be visible instantly.
			psWidget->blinkOffset = SDL_GetTicks();
		}
	}
}


/* Respond to loss of focus */
void editBoxFocusLost(W_SCREEN* psScreen, W_EDITBOX *psWidget)
{
	ASSERT( !(psWidget->state & WEDBS_DISABLE),
		"editBoxFocusLost: disabled edit box" );

	/* Stop editing the widget */
	psWidget->state = WEDBS_FIXED;
	psWidget->printStart = 0;
	fitStringStart(psWidget->aText,psWidget->width,
				   &psWidget->printChars, &psWidget->printWidth);

	widgSetReturn(psScreen, (WIDGET *)psWidget);

}


/* Respond to a mouse button up */
void editBoxReleased(W_EDITBOX *psWidget)
{
	(void)psWidget;
}


/* Respond to a mouse moving over an edit box */
void editBoxHiLite(W_EDITBOX *psWidget)
{
	if(psWidget->state & WEDBS_DISABLE)
	{
		return;
	}

	if(psWidget->AudioCallback) {
		psWidget->AudioCallback(psWidget->HilightAudioID);
	}

	psWidget->state |= WEDBS_HILITE;
}


/* Respond to the mouse moving off an edit box */
void editBoxHiLiteLost(W_EDITBOX *psWidget)
{
	if(psWidget->state & WEDBS_DISABLE)
	{
		return;
	}

	psWidget->state = psWidget->state & WEDBS_MASK;
}


/* The edit box display function */
void editBoxDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	W_EDITBOX	*psEdBox;
	SDWORD		x0,y0,x1,y1, fx,fy, cx,cy;
	enum iV_fonts CurrFontID;
	utf_32_char	ch, *pInsPoint, *pPrint;
#if CURSOR_BLINK
	BOOL		blink;
#endif
	char *utf;

	psEdBox = (W_EDITBOX *)psWidget;
	CurrFontID = psEdBox->FontID;

	x0=psEdBox->x + xOffset;
	y0=psEdBox->y + yOffset;
	x1=x0 + psEdBox->width;
	y1=y0 + psEdBox->height;

	if(psEdBox->pBoxDisplay) {
		psEdBox->pBoxDisplay((WIDGET *)psEdBox, xOffset, yOffset, pColours);
	} else {
		pie_BoxFill(x0, y0, x1, y1, pColours[WCOL_BKGRND]);

		iV_Line(x0,y0, x1,y0, pColours[WCOL_DARK]);
		iV_Line(x0,y0, x0,y1, pColours[WCOL_DARK]);
		iV_Line(x0,y1, x1,y1, pColours[WCOL_LIGHT]);
		iV_Line(x1,y1, x1,y0, pColours[WCOL_LIGHT]);
	}

	fx = x0 + WEDB_XGAP;// + (psEdBox->width - fw) / 2;

	iV_SetFont(CurrFontID);
	iV_SetTextColour(pColours[WCOL_TEXT]);

  	fy = y0 + (psEdBox->height - iV_GetTextLineSize())/2 - iV_GetTextAboveBase();


	/* If there is more text than will fit into the box,
	   display the bit with the cursor in it */
	pPrint = psEdBox->aText + psEdBox->printStart;
	pInsPoint = pPrint + psEdBox->printChars;
	ch = *pInsPoint;

	*pInsPoint = '\0';
	utf = UTF32toUTF8(pPrint, NULL);
//	if(psEdBox->pFontDisplay) {
//		psEdBox->pFontDisplay(fx,fy, pPrint);
//	} else {
		iV_DrawText(utf, fx, fy);
//	}
	free(utf);
	*pInsPoint = ch;

	/* Display the cursor if editing */
#if CURSOR_BLINK
	blink = !(((SDL_GetTicks() - psEdBox->blinkOffset)/WEDB_BLINKRATE) % 2);
	if ((psEdBox->state & WEDBS_MASK) == WEDBS_INSERT && blink)
#else
	if ((psEdBox->state & WEDBS_MASK) == WEDBS_INSERT)
#endif
	{
		pInsPoint = psEdBox->aText + psEdBox->insPos;
		ch = *pInsPoint;
		*pInsPoint = '\0';
		utf = UTF32toUTF8(psEdBox->aText + psEdBox->printStart, NULL);
		cx = x0 + WEDB_XGAP + iV_GetTextWidth(utf);
		cx += iV_GetTextWidth("-");
		free(utf);
		*pInsPoint = ch;
		cy = fy;
		iV_Line(cx, cy + iV_GetTextAboveBase(), cx, cy - iV_GetTextBelowBase(), pColours[WCOL_CURSOR]);
	}
#if CURSOR_BLINK
	else if ((psEdBox->state & WEDBS_MASK) == WEDBS_OVER && blink)
#else
	else if ((psEdBox->state & WEDBS_MASK) == WEDBS_OVER)
#endif
	{
		pInsPoint = psEdBox->aText + psEdBox->insPos;
		ch = *pInsPoint;
		*pInsPoint = '\0';
		utf = UTF32toUTF8(psEdBox->aText + psEdBox->printStart, NULL);
		cx = x0 + WEDB_XGAP + iV_GetTextWidth(utf);
		free(utf);
		*pInsPoint = ch;
	  	cy = fy;
		iV_Line(cx, cy, cx + WEDB_CURSORSIZE, cy, pColours[WCOL_CURSOR]);
	}


	if(psEdBox->pBoxDisplay == NULL) {
		if (psEdBox->state & WEDBS_HILITE)
		{
			/* Display the button hilite */
			iV_Line(x0-2,y0-2, x1+2,y0-2, pColours[WCOL_HILITE]);
			iV_Line(x0-2,y0-2, x0-2,y1+2, pColours[WCOL_HILITE]);
			iV_Line(x0-2,y1+2, x1+2,y1+2, pColours[WCOL_HILITE]);
			iV_Line(x1+2,y1+2, x1+2,y0-2, pColours[WCOL_HILITE]);
		}
	}
}



/* Set an edit box'sstate */
void editBoxSetState(W_EDITBOX *psEditBox, UDWORD state)
{
	if (state & WEDBS_DISABLE)
	{
		psEditBox->state |= WEDBS_DISABLE;
	}
	else
	{
		psEditBox->state &= ~WEDBS_DISABLE;
	}

}
