//
//  cocoa_sdl_helpers.m
//  Warzone
//
//	Copyright © 2017 pastdue (https://github.com/past-due/)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.
//

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <Availability.h>
#import "cocoa_sdl_helpers.h"
#include "SDL_syswm.h"
#include <mach-o/dyld.h>

bool cocoaIsNSWindowFullscreened(NSWindow __unsafe_unretained *window)
{
// note use of 101200 instead of __MAC_10_12 (to support earlier SDKs)
#if __MAC_OS_X_VERSION_MIN_REQUIRED < 101200
	return [window styleMask] & NSFullScreenWindowMask;
#else
	return [window styleMask] & NSWindowStyleMaskFullScreen;
#endif
}

bool cocoaIsSDLWindowFullscreened(SDL_Window *window)
{
	if (window == nil) return false;
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version); /* initialize info structure with SDL version info */
	if(SDL_GetWindowWMInfo(window, &info) == SDL_FALSE) {
		NSLog(@"Unable to get SDL_SysWMinfo, with error: %s", SDL_GetError());
		return false;
	}
	if (info.subsystem != SDL_SYSWM_COCOA) {
		NSLog(@"Unexpected SDL subsystem: %d", info.subsystem);
		return false;
	}
	assert(info.info.cocoa.window != nil);
	return cocoaIsNSWindowFullscreened(info.info.cocoa.window);
}

std::string cocoaGetCurrentExecutablePath()
{
	std::string result;
	char path[MAXPATHLEN] = {};
	uint32_t size = MAXPATHLEN;
	int ret = _NSGetExecutablePath(path, &size);
	if (ret == 0)
	{
		result = path;
	}
	else
	{
		// allocate larger buffer
		char* larger_path = (char*)malloc(size);
		ret =_NSGetExecutablePath(larger_path, &size);
		if (ret == 0)
		{
			result = larger_path;
		}
		free(larger_path);
	}

	return result;
}

std::string cocoaGetFrameworksPath(const char* resourceName)
{
	std::string path = cocoaGetCurrentExecutablePath();
	if (path.empty())
	{
		return "";
	}
	// Executable path will end with "/MacOS/<executable name>"
	// Remove last two path components
	for (size_t i = 0; i < 2; i++)
	{
		size_t lastSlashPos = path.rfind("/");
		if (lastSlashPos == std::string::npos)
		{
			break;
		}
		path = path.substr(0, lastSlashPos);
	}
	if (path.empty())
	{
		return "";
	}
	// Append "Frameworks" folder name
	path += "/Frameworks/";
	// Append resourceName (if provided)
	if (resourceName != nullptr)
	{
		path += resourceName;
	}
	return path;
}
