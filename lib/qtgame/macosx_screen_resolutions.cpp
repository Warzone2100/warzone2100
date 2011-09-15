#include "macosx_screen_resolutions.h"

#ifdef WZ_WS_MAC
#include <ApplicationServices/ApplicationServices.h>


#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
bool macosxAppendAvailableScreenResolutions(QList<QSize> &resolutions, int minWidth, int minHeight)
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
