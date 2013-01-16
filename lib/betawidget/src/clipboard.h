/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2013  Warzone 2100 Project

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

#ifndef CLIPBOARD_H_
#define CLIPBOARD_H_

#include <stdbool.h>

/**
 * Returns a copy of the text in the systems clipboard. Should the clipboard be
 * empty, or populated with non-textual data NULL is returned. The character set
 * of the returned is guaranteed to be UTF-8.
 *
 * It remains the responsibility of the caller to free() the string when
 * finished with it.
 *
 * @return The textual contents of the clipboard (if any), otherwise NULL.
 */
char *widgetGetClipboardText(void);

/**
 * Attempts to set the contents of the systems clipboard to text. The character
 * set of text must be UTF-8.
 *
 * @param text  The UTF-8 text to set the clipboard to.
 * @return True if the contents were successfully set, false otherwise.
 */
bool widgetSetClipboardText(const char *text);

#endif /*CLIPBOARD_H_*/
