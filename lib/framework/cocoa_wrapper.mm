/*
 This file is part of Warzone 2100.
 Copyright (C) 1999-2004  Eidos Interactive
 Copyright (C) 2005-2010  Warzone 2100 Project

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

#import "cocoa_wrapper.h"

#ifdef WZ_OS_MAC
#import <AppKit/AppKit.h>

void cocoaInit()
{
	NSApplicationLoad();
}

void cocoaShowAlert(const char *message, const char *information, unsigned style)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSAlert *alert = [[NSAlert alloc] init];
	[alert addButtonWithTitle:@"OK"];
	[alert setMessageText:[NSString stringWithUTF8String:message]];
	[alert setInformativeText:[NSString stringWithUTF8String:information]];
	[alert setAlertStyle:style];
	[alert runModal];
	[pool release];
}

#endif // WZ_OS_MAC
