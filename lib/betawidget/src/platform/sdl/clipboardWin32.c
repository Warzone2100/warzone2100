/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2012  Warzone 2100 Project

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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../../../framework/utf.h"
// Defines most macros and types from <stdbool.h> and <stdint.h>
#include "../../../framework/types.h"

char *widgetGetClipboardText()
{
	uint16_t *clipboardText;
	char *ourText = NULL;

	// If there is any text on the clipboard, open it
	if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL))
	{
		// Get any text on the clipboard
		HANDLE hClipboardData = GetClipboardData(CF_UNICODETEXT);

		// If the handle is valid, fetch the text
		if (hClipboardData)
		{
			// Get the text
			clipboardText = GlobalLock(hClipboardData);

			// So long as we got something
			if (clipboardText)
			{
				int i, j;

				// Convert it to UTF-8 (from UTF-16)
				ourText = UTF16toUTF8(clipboardText, NULL);

				// Unlock the text
				GlobalUnlock(hClipboardData);

				// Strip any '\r' from the text
				for (i = j = 0; ourText[i]; i++)
				{
					if (ourText[i] != '\r')
					{
						ourText[j++] = ourText[i];
					}
				}

				// NUL terminate
				ourText[j] = '\0';
			}
		}

		// Close the clipboard
		CloseClipboard();
	}

	return ourText;
}

bool widgetSetClipboardText(const char *text)
{
	bool ret = false;

	// Copy of text with \n => \r\n
	char *newText;

	// UTF-16 version of newText
	uint16_t *utf16NewText;

	// Number of bytes utf16NewText is in size
	size_t nbytes;

	int count, i, j;

	// Get the number of '\n' characters in the text
	for (i = count = 0; text[i]; i++)
	{
		if (text[i] == '\n')
		{
			count++;
		}
	}

	// Allocate enough space for the \r\n string
	newText = malloc(strlen(text) + count + 1);

	// Copy the string, converting \n to \r\n
	for (i = j = 0; text[i]; i++, j++)
	{
		// If the character is a newline prepend a \r
		if (text[i] == '\n')
		{
			newText[j++] = '\r';
		}

		// Copy the character (\n or otherwise)
		newText[j] = text[i];
	}

	// NUL terminate
	newText[j] = '\0';

	// Convert to UTF-16
	utf16NewText = UTF8toUTF16(newText, &nbytes);

	// Open the clipboard
	if (OpenClipboard(NULL))
	{
		HGLOBAL hGlobal;
		uint16_t *clipboardText;

		// Empty it (which also transfers ownership of it to ourself)
		EmptyClipboard();

		// Allocate global space for the text
		hGlobal = GlobalAlloc(GMEM_MOVEABLE, nbytes);

		// Lock the newly allocated memory
		clipboardText = GlobalLock(hGlobal);

		// Copy the text
		memcpy(clipboardText, utf16NewText, nbytes);

		// Unlock the memory (must come before CloseClipboard())
		GlobalUnlock(hGlobal);

		// Place the handle on the clipboard
		if (SetClipboardData(CF_UNICODETEXT, hGlobal))
		{
			// We were successful
			ret = true;
		}

		// Close the clipboard
		CloseClipboard();
	}

	// Release the malloc-ed strings
	free(newText);
	free(utf16NewText);

	return ret;
}
