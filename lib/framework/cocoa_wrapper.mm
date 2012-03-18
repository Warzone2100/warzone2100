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
#import <stdarg.h>

void cocoaInit()
{
	NSApplicationLoad();
}

static inline NSString *nsstringify(const char *str)
{
	return [NSString stringWithUTF8String:str];
}

int cocoaShowAlert(const char *message, const char *information, unsigned style,
                   const char *buttonTitle, ...)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSAlert *alert = [[NSAlert alloc] init];
	[alert setMessageText:nsstringify(message)];
	[alert setInformativeText:nsstringify(information)];
	[alert setAlertStyle:style];

	va_list args;
	va_start(args, buttonTitle);
	const char *currentButtonTitle = buttonTitle;
	do {
		[alert addButtonWithTitle:nsstringify(currentButtonTitle)];
	} while ((currentButtonTitle = va_arg(args, const char *)));
	va_end(args);

	NSInteger buttonID = [alert runModal];
	[pool release];
	return buttonID - NSAlertFirstButtonReturn;
}

void cocoaSelectFileInFinder(const char *filename)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[[NSWorkspace sharedWorkspace] selectFile:nsstringify(filename) inFileViewerRootedAtPath:nil];
	[pool release];
}

void cocoaOpenURL(const char *url)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:nsstringify(url)]];
	[pool release];
}

void cocoaOpenUserCrashReportFolder()
{
	SInt32 maj, min;
	if (Gestalt(gestaltSystemVersionMajor, &maj) == noErr
		&& Gestalt(gestaltSystemVersionMinor, &min) == noErr)
	{
		if (maj == 10 && min <= 5)
		{
			cocoaOpenURL("file://~/Library/Logs/CrashReporter");
		}
		else
		{
			cocoaOpenURL("file://~/Library/Logs/DiagnosticReports");
		}
	}
}

#endif // WZ_OS_MAC
