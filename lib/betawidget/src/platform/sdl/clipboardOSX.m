/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2009  Warzone Resurrection Project

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

#import <AppKit/AppKit.h>

#include "../../clipboard.h"

char *widgetGetClipboardText()
{
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSArray *pasteboardTypes = [pasteboard types];
	char *clipboardText = NULL;
	
	// Check if the pasteboard contains any text
	if ([pasteboardTypes containsObject:NSStringPboardType])
	{
		// Fetch the text
		NSString *text = [pasteboard stringForType:NSStringPboardType];
		
		// So long as there is some text; duplicate it
		if ([text length])
		{
			clipboardText = strdup([text UTF8String]);
		}
		
		// Release the text
		[text release];
	}
	
	// Clean-up
	[pasteboard release];
	[pasteboardTypes release];
	
	return clipboardText;
}

bool widgetSetClipboardText(const char *text)
{	
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	
	// We are willing to provide an NSString representation only
	NSArray *types = [NSArray arrayWithObject:NSStringPboardType];
	
	// Convert text to an NSString instance
	NSString *copyText = [[NSString alloc] initWithUTF8String:text];
	
	// Return status
	bool ret;
	
	// Register the data types we are willing to provide
	[pasteboard declareTypes:types owner:nil];
	
	// Set the pasteboard text
	ret = [pasteboard setString:copyText forType:NSStringPboardType];
	
	// Clean-up
	[copyText release];
	[pasteboard release];
	[types release];
	
	return ret;
}
