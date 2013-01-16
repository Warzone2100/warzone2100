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
#ifndef TEXTENTRY_H_
#define TEXTENTRY_H_

#include "widget.h"

/*
 * Forward declarations 
 */
typedef struct _textEntry textEntry;
typedef struct _textEntryVtbl textEntryVtbl;

struct _textEntryVtbl
{
	struct _widgetVtbl widgetVtbl;
};

struct _textEntry
{
	/*
	 * Parent
	 */
	struct _widget widget;
	
	/*
	 * Our vtable
	 */
	textEntryVtbl *vtbl;
	
	/*
	 * The character buffer (contents), UTF-8 encoded
	 */
	char *text;
	
	/*
	 * The size of text (how much space was allocated), not including the '\0'
	 * byte
	 */
	size_t textSize;
	
	/*
	 * The actual length of the string (how much space is used); since text is
	 * encoded in UTF-8 this is *not* the character count
	 */
	size_t textHead;
	
	/*
	 * The current insert position (where in the byte-stream characters should
	 * be inserted to); again this has no relation to the character count
	 */
	int insertPosition;
	
	/*
	 * The offet of the leftmost pixel that is to be rendered:
	 *               |*-------------------|  
	 * "The rain in S|pain falls mainly on|the..."
	 *               |--------------------|
	 * 
	 * The * shows the location represented by this value
	 */
	int renderLeftOffset;
};

/*
 * Type information
 */
extern const classInfo textEntryClassInfo;

/*
 * Helper macros
 */
#define TEXT_ENTRY(self) (assert(widgetIsA(WIDGET(self), &textEntryClassInfo)), \
                          (textEntry *) (self))

/*
 * Protected methods
 */
void textEntryInit(textEntry *self, const char *id);
void textEntryDestroyImpl(widget *self);
void textEntryDoDrawImpl(widget *self);
size textEntryGetMinSizeImpl(widget *self);
size textEntryGetMaxSizeImpl(widget *self);


/*
 * Public methods
 */

/**
 * Constructs a new textEntry object and returns it.
 * 
 * @param id    The id of the widget.
 * @return A pointer to an textEntry on success; otherwise NULL.
 */
textEntry *textEntryCreate(const char *id);

/**
 * Fetches the contents of the text entry field, returning a copy of them. It is
 * important to note that the returned string remains the responsibility of the
 * user and must be free'ed when no longer required.
 * 
 * @param self  The text entry field to get the contents of.
 * @return A pointer to a freshly allocated copy of the contents on success;
 *         otherwise NULL.
 */
const char *textEntryGetContents(textEntry *self);

/**
 * Sets the contents of the entry field to a copy of the contents of contents.
 * 
 * @param self  The text entry to set the contents of.
 * @param contents  The string to set the contents to. This should be UTF-8
 *                  encoded and NULL terminated.
 * @return true if the contents were successfully set; otherwise false.
 */
bool textEntrySetContents(textEntry *self, const char *contents);

/*
 * Protected methods
 */

/**
 * Called by widgetDoDraw to draw the frame of the text entry field. The frame
 * consists of the background and border of the entry field. This method will be
 * called before the text and caret are rendered.
 * 
 * @param self  The text entry to draw the frame of.
 */
void textEntryDoDrawFrame(textEntry *self);

void textEntryDoDrawText(textEntry *self);

#endif /*TEXTENTRY_H_*/
