/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 *  Definitions for the edit box functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_EDITBOX_H__
#define __INCLUDED_LIB_WIDGET_EDITBOX_H__

#include "widget.h"
#include "widgbase.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/framework/utf.h"

/* Edit Box states */
#define WEDBS_FIXED		0x0001		// No editing is going on
#define WEDBS_INSERT	0x0002		// Insertion editing
#define WEDBS_OVER		0x0003		// Overwrite editing
#define WEDBS_MASK		0x000f		// 
#define WEDBS_HILITE	0x0010		//
#define WEDBS_DISABLE   0x0020		// disable button from selection


struct W_EDITBOX : public WIDGET
{
	W_EDITBOX(W_EDBINIT const *init);

	void initialise();  // Moves the cursor to the end.
	void setString(char const *utf8);

	void clicked(W_CONTEXT *psContext, WIDGET_KEY key);
	void highlight(W_CONTEXT *psContext);
	void highlightLost(W_CONTEXT *psContext);
	void focusLost(W_SCREEN *psScreen);
	void run(W_CONTEXT *psContext);

	UDWORD		state;						// The current edit box state
	QString         aText;                  // The text in the edit box
	iV_fonts        FontID;
	int 		blinkOffset;		// Cursor should be visible at time blinkOffset.
	int             insPos;                 // The insertion point in the buffer
	int             printStart;					// Where in the string appears at the far left of the box
	int             printChars;					// The number of characters appearing in the box
	int             printWidth;					// The pixel width of the characters in the box
	WIDGET_DISPLAY	pBoxDisplay;			// Optional callback to display the edit box background.
	FONT_DISPLAY pFontDisplay;				// Optional callback to display a string.
	SWORD HilightAudioID;					// Audio ID for form clicked sound
	SWORD ClickedAudioID;					// Audio ID for form hilighted sound
	WIDGET_AUDIOCALLBACK AudioCallback;		// Pointer to audio callback function

private:
	void delCharRight();
	void delCharLeft();
	void insertChar(QChar ch);
	void overwriteChar(QChar ch);
	void fitStringStart();  // Calculate how much of the start of a string can fit into the edit box
	void fitStringEnd();
	void setCursorPosPixels(int xPos);
};

/* Create an edit box widget data structure */
extern W_EDITBOX* editBoxCreate(const W_EDBINIT* psInit);

/* The edit box display function */
extern void editBoxDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

/* set state of edit box */
extern void editBoxSetState(W_EDITBOX *psEditBox, UDWORD state);

#endif // __INCLUDED_LIB_WIDGET_EDITBOX_H__
