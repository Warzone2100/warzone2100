/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008  Warzone Resurrection Project

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

#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

char *widgetGetClipboardText()
{
	char *clipboardText;
	char *ourText = NULL;
	
	// If there is any text on the clipboard, open it
	if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(NULL))
	{
		// Get any text on the clipboard
		HANDLE hClipboardData = GetClipboardData(CF_TEXT);
		
		// If the handle is valid, fetch the text
		if (hClipboardData)
		{
			// Get the text
			clipboardText = GlobalLock(hClipboardData);
			
			// So long as we got something, copy it
			if (clipboardText)
			{
				int i;
				
				// Copy it into our address space
				ourText = malloc(strlen(clipboardText) + 1);
				
				// Strip \r from the text when copying
				for (i = 0; *clipboardText; clipboardText++)
				{
					if (*clipboardText != '\r')
					{
						ourText[i++] = *clipboardText;
					}
				}
				
				// NULL-terminate
				ourText[i] = '\0';
				
				// Unlock the text
				GlobalUnlock(hClipboardData);
			}
		}
		
		// Close the clipboard
		CloseClipboard();
	}
	
	return ourText;
}
