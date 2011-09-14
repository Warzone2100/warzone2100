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

#ifdef WZ_WS_MAC
#include <ApplicationServices/ApplicationServices.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
bool cocoaAppendAvailableScreenResolutions(QList<QSize> &resolutions, int minWidth, int minHeight)
{
	uint32_t displayCount;
	CGError error;
	if ((error = CGGetActiveDisplayList(UINT_MAX, NULL, &displayCount)) != kCGErrorSuccess)
	{
		qWarning("CGGetActiveDisplayList failed: %i", error);
		return false;
	}

	CGDirectDisplayID displays[displayCount];
	if ((error = CGGetActiveDisplayList(displayCount, displays, &displayCount)) != kCGErrorSuccess)
	{
		qWarning("CGGetActiveDisplayList failed: %i", error);
		return false;
	}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
	bool modern = (CGDisplayCopyAllDisplayModes != NULL); // where 'modern' means >= 10.6
#endif

	for (uint32_t i = 0; i < displayCount; i++)
	{
		CFArrayRef displayModes;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
		if (modern)
		{
			displayModes = CGDisplayCopyAllDisplayModes(displays[i], NULL);
		}
		else
#endif
		{
			displayModes = CGDisplayAvailableModes(displays[i]);
		}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
		CFStringRef currentPixelEncoding = NULL;
#endif
		double currentRefreshRate;
		int curBPP, curBPS, curSPP;

#define dictget(DICT, TYPE, KEY, VAR) CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(DICT, KEY), kCFNumber##TYPE##Type, VAR)

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
		if (modern)
		{
			CGDisplayModeRef currentDisplayMode = CGDisplayCopyDisplayMode(displays[i]);
			currentRefreshRate = CGDisplayModeGetRefreshRate(currentDisplayMode);
			currentPixelEncoding = CGDisplayModeCopyPixelEncoding(currentDisplayMode);
			CFRelease(currentDisplayMode);
		}
		else
#endif
		{
			CFDictionaryRef currentDisplayMode = CGDisplayCurrentMode(displays[i]);
			dictget(currentDisplayMode, Double, kCGDisplayRefreshRate, &currentRefreshRate);
			dictget(currentDisplayMode, Int, kCGDisplayBitsPerPixel, &curBPP);
			dictget(currentDisplayMode, Int, kCGDisplayBitsPerSample, &curBPS);
			dictget(currentDisplayMode, Int, kCGDisplaySamplesPerPixel, &curSPP);
		}

		for (CFIndex j = 0, c = CFArrayGetCount(displayModes); j < c; j++)
		{
			int width, height;
			double refreshRate;
			bool pixelEncodingsEqual;

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
			if (modern)
			{
				CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(displayModes, j);
				width = (int)CGDisplayModeGetWidth(displayMode);
				height = (int)CGDisplayModeGetHeight(displayMode);
				refreshRate = CGDisplayModeGetRefreshRate(displayMode);
				CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(displayMode);
				pixelEncodingsEqual = (CFStringCompare(pixelEncoding, currentPixelEncoding, 0) == kCFCompareEqualTo);
				CFRelease(pixelEncoding);
			}
			else
#endif
			{
				CFDictionaryRef displayMode = (CFDictionaryRef)CFArrayGetValueAtIndex(displayModes, j);
				dictget(displayMode, Int, kCGDisplayWidth, &width);
				dictget(displayMode, Int, kCGDisplayHeight, &height);
				dictget(displayMode, Double, kCGDisplayRefreshRate, &refreshRate);
				int bpp, bps, spp;
				dictget(displayMode, Int, kCGDisplayBitsPerPixel, &bpp);
				dictget(displayMode, Int, kCGDisplayBitsPerSample, &bps);
				dictget(displayMode, Int, kCGDisplaySamplesPerPixel, &spp);
				pixelEncodingsEqual = (bpp == curBPP && bps == curBPS && spp == curSPP);
			}

			QSize res(width, height);
			if (!resolutions.contains(res)
			    && width >= minWidth && height >= minHeight
			    && refreshRate == currentRefreshRate && pixelEncodingsEqual)
			{
				resolutions += res;
			}
		}

#undef dictget

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
		if (modern)
		{
			CFRelease(currentPixelEncoding);
			CFRelease(displayModes);
		}
#endif
	}

	return true;
}
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

#endif // WZ_WS_MAC
