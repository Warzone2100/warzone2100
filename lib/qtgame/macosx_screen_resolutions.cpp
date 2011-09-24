#include "macosx_screen_resolutions.h"

#ifdef WZ_WS_MAC
#include <ApplicationServices/ApplicationServices.h>


#define dictget(DICT, TYPE, KEY, VAR) CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(DICT, KEY), kCFNumber##TYPE##Type, VAR)

static CGDirectDisplayID displayAtPoint(QPoint point)
{
	CGDirectDisplayID display;
	uint32_t matchingDisplayCount;
	CGGetDisplaysWithPoint(CGPointMake(point.x(), point.y()), 1, &display, &matchingDisplayCount);
	if (matchingDisplayCount == 0)
	{
		display = kCGNullDirectDisplay;
		qWarning("No display found at %ix%i", point.x(), point.y());
	}
	return display;
}

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
bool macosxAppendAvailableScreenResolutions(QList<QSize> &resolutions, QSize minSize, QPoint screenPoint)
{
	CGDirectDisplayID display = displayAtPoint(screenPoint);
	if (display == kCGNullDirectDisplay)
	{
		return false;
	}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
	bool modern = (CGDisplayCopyAllDisplayModes != NULL); // where 'modern' means >= 10.6
#endif

	CFArrayRef displayModes;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
	if (modern)
	{
		displayModes = CGDisplayCopyAllDisplayModes(display, NULL);
	}
	else
#endif
	{
		displayModes = CGDisplayAvailableModes(display);
	}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
	CFStringRef currentPixelEncoding = NULL;
#endif
	double currentRefreshRate;
	int curBPP, curBPS, curSPP;

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
	if (modern)
	{
		CGDisplayModeRef currentDisplayMode = CGDisplayCopyDisplayMode(display);
		currentRefreshRate = CGDisplayModeGetRefreshRate(currentDisplayMode);
		currentPixelEncoding = CGDisplayModeCopyPixelEncoding(currentDisplayMode);
		CFRelease(currentDisplayMode);
	}
	else
#endif
	{
		CFDictionaryRef currentDisplayMode = CGDisplayCurrentMode(display);
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
		    && width >= minSize.width() && height >= minSize.height()
		    && refreshRate == currentRefreshRate && pixelEncodingsEqual)
		{
			resolutions += res;
		}
	}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
	if (modern)
	{
		CFRelease(currentPixelEncoding);
		CFRelease(displayModes);
	}
#endif

	return true;
}

bool macosxSetScreenResolution(QSize resolution, QPoint screenPoint)
{
	CGDirectDisplayID display = displayAtPoint(screenPoint);
	if (display == kCGNullDirectDisplay)
	{
		return false;
	}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
	if (CGDisplayCopyAllDisplayModes != NULL)
	{
		bool ok = false;
		CGDisplayModeRef currentMainDisplayMode = CGDisplayCopyDisplayMode(display);
		CFStringRef currentPixelEncoding = CGDisplayModeCopyPixelEncoding(currentMainDisplayMode);
		CFArrayRef displayModes = CGDisplayCopyAllDisplayModes(display, NULL);
        for (CFIndex i = 0, c = CFArrayGetCount(displayModes); i < c; i++)
		{
			bool isEqual = false;
			CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(displayModes, i);
            CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(mode);
			if (CFStringCompare(pixelEncoding, currentPixelEncoding, 0) == kCFCompareEqualTo
			    && CGDisplayModeGetWidth(mode) == (size_t)resolution.width()
			    && CGDisplayModeGetHeight(mode) == (size_t)resolution.height())
			{
				isEqual = true;
			}
			CFRelease(pixelEncoding);

			if (isEqual)
			{
				CGDisplaySetDisplayMode(display, mode, NULL);
				ok = true;
				break;
			}
		}
		CFRelease(currentPixelEncoding);
		CFRelease(displayModes);
		return ok;
	}
	else
#endif
	{
		CFDictionaryRef currentMainDisplayMode = CGDisplayCurrentMode(display);
		int bpp;
		dictget(currentMainDisplayMode, Int, kCGDisplayBitsPerPixel, &bpp);
		boolean_t exactMatch = false;
		CFDictionaryRef bestMode = CGDisplayBestModeForParameters(display, bpp, resolution.width(), resolution.height(), &exactMatch);
		if (bestMode != NULL)
		{
			if (!exactMatch)
			{
				qWarning("No optimal display mode for requested parameters.");
			}
			CGDisplaySwitchToMode(display, bestMode);
			return true;
		}
		else
		{
			qWarning("Bad resolution change: Invalid display.");
			return false;
		}
	}
}
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

#endif // WZ_WS_MAC
