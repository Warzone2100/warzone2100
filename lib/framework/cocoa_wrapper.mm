/*
 This file is part of Warzone 2100.
 Copyright (C) 1999-2004  Eidos Interactive
 Copyright (C) 2005-2020  Warzone 2100 Project

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

#if ! __has_feature(objc_arc)
#error "Objective-C ARC (Automatic Reference Counting) is off"
#endif

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>

static inline NSString * _Nonnull nsstringify(const char *str)
{
    NSString * nsString = [NSString stringWithUTF8String:str];
    if (nsString == nil)
    {
        return @"stringWithUTF8String failed";
    }
	return nsString;
}

int cocoaShowAlert(const char *message, const char *information, unsigned style,
                   const char *buttonTitle, ...)
{
    NSInteger buttonID = -1;
    @autoreleasepool {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:nsstringify(message)];
        [alert setInformativeText:nsstringify(information)];
        [alert setAlertStyle:(NSAlertStyle)style];

        va_list args;
        va_start(args, buttonTitle);
        const char *currentButtonTitle = buttonTitle;
        do {
            [alert addButtonWithTitle:nsstringify(currentButtonTitle)];
        } while ((currentButtonTitle = va_arg(args, const char *)));
        va_end(args);

        buttonID = [alert runModal];
    }
    return static_cast<int>(buttonID - NSAlertFirstButtonReturn);
}

bool cocoaSelectFileInFinder(const char *filename)
{
    if (filename == nullptr) return false;
    BOOL success = NO;
	@autoreleasepool {
        success = [[NSWorkspace sharedWorkspace] selectFile:nsstringify(filename) inFileViewerRootedAtPath:@""];
    }
    return success;
}

bool cocoaSelectFolderInFinder(const char* path)
{
	if (path == nullptr) return false;
	BOOL success = NO;
	@autoreleasepool {
		NSURL *pathURL = [NSURL fileURLWithPath:nsstringify(path) isDirectory:YES];
		if (pathURL == nil) return false;
		success = [[NSWorkspace sharedWorkspace] openURL:pathURL];
	}
	return success;
}

bool cocoaOpenURL(const char *url)
{
    assert(url != nullptr);
    BOOL success = NO;
	@autoreleasepool {
        success = [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:nsstringify(url)]];
    }
    return success;
}

bool cocoaOpenUserCrashReportFolder()
{
    BOOL success = NO;
    @autoreleasepool {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
        NSString *libraryPath = ([paths count] > 0) ? [paths objectAtIndex:0] : nil;
        if (libraryPath == nil) return false;
        NSURL *libraryURL = [NSURL fileURLWithPath:libraryPath isDirectory:YES];
        NSURL *crashReportsURL = [NSURL URLWithString:@"Logs/DiagnosticReports" relativeToURL:libraryURL];
        success = [[NSWorkspace sharedWorkspace] openURL:crashReportsURL];
    }
    return success;
}

bool cocoaGetApplicationSupportDir(char *const tmpstr, size_t const size)
{
	@autoreleasepool {
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, TRUE);
		NSString *path = ([paths count] > 0) ? [paths objectAtIndex:0] : nil;
		if (path == nil) return false;
		BOOL success = [path getCString:tmpstr maxLength:size encoding:NSUTF8StringEncoding];
		return success;
	}
}

bool cocoaSetFileQuarantineAttribute(const char *path)
{
	@autoreleasepool {
		//	kLSQuarantineTypeKey -> kLSQuarantineTypeOtherDownload
		NSDictionary * quarantineProperties = @{
			(NSString *)kLSQuarantineTypeKey : (NSString *)kLSQuarantineTypeOtherDownload
		};

//		if (@available(macOS 10.10, *)) {	// "@available" is only available on Xcode 9+
		if (floor(NSAppKitVersionNumber) >= NSAppKitVersionNumber10_10) { // alternative to @available
			NSURL *fileURL = [NSURL fileURLWithFileSystemRepresentation:path isDirectory:NO relativeToURL:NULL];
			if (fileURL == nil) return false;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability" // only required until we can use @available
			NSError *error = nil;
			BOOL success = [fileURL setResourceValue:quarantineProperties forKey:NSURLQuarantinePropertiesKey error:&error];
#pragma clang diagnostic pop
			if (!success)
			{
				// Failed to set resource value
				NSLog(@"Failed to set resource file: %@", error);
				return false;
			}
		} else {
			// macOS 10.9 and earlier require now-deprecated APIs
			return false;
		}
		return true;
	}
}

bool TransformProcessState(ProcessApplicationTransformState newState)
{
	@autoreleasepool {
		ProcessSerialNumber psn = {0, kCurrentProcess};
		OSStatus result = TransformProcessType(&psn, newState);

		if (result != 0)
		{
			NSError *error = [NSError errorWithDomain:NSOSStatusErrorDomain code:result userInfo:nil];
			NSLog(@"TransformProcessType failed with error - %@", error);
		}
		return (result == 0);
	}
}

bool cocoaTransformToBackgroundApplication()
{
    return TransformProcessState(kProcessTransformToBackgroundApplication);
}

#endif // WZ_OS_MAC
