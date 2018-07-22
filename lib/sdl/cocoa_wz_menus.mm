//
//  cocoa_wz_menus.mm
//  Warzone
//
//	Copyright © 2018 pastdue (https://github.com/past-due/)
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
#import "cocoa_wz_menus.h"

#if ! __has_feature(objc_arc)
#error "Objective-C ARC (Automatic Reference Counting) is off"
#endif

// MARK: - Custom Menu Actions

@interface WZMenuActionsHandler : NSObject

+ (id)sharedInstance;

- (void) openQuickstartGuide: (id)sender;
- (void) openJSScriptingAPIDoc: (id)sender;

@end

@implementation WZMenuActionsHandler

+ (id)sharedInstance {
	static WZMenuActionsHandler *sharedMenuHandler = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedMenuHandler = [[self alloc] init];
	});
	return sharedMenuHandler;
}

- (id)init {
	if (self = [super init]) {
		// Initialize any properties
	}
	return self;
}

- (void)dealloc {
	// Should never be called
}

typedef NS_ENUM(NSUInteger, WZOpenDocResult) {
	WZOpenDocResultSuccess,
	WZOpenDocResultErrorNotFound,
	WZOpenDocResultErrorOpenFailed
};

- (WZOpenDocResult) openDoc: (nullable NSString *)name withExtension:(nullable NSString *)ext
{
	NSURL *docURL = [[NSBundle mainBundle] URLForResource:name withExtension:ext subdirectory:@"docs"];
	if (docURL == nil)
	{
		return WZOpenDocResultErrorNotFound;
	}

	if (![[NSWorkspace sharedWorkspace] openURL:docURL])
	{
		return WZOpenDocResultErrorOpenFailed;
	}

	return WZOpenDocResultSuccess;
}

- (void) openDocAndDisplayError: (nullable NSString *)name withExtension:(nullable NSString *)ext withDocDescription:(nullable NSString *)description
{
	switch ([self openDoc:name withExtension:ext])
	{
		case WZOpenDocResultSuccess:
			break;
		case WZOpenDocResultErrorNotFound:
		{
			NSAlert *alert = [[NSAlert alloc] init];
			[alert setMessageText:[NSString stringWithFormat:@"Failed to locate the %@ doc", description]];
			[alert setInformativeText:[NSString stringWithFormat:@"Unable to find: %@.%@\n\nYour copy of the Warzone 2100 app may be corrupt or incomplete.", name, ext]];
			[alert runModal];
			break;
		}
		case WZOpenDocResultErrorOpenFailed:
		{
			NSAlert *alert = [[NSAlert alloc] init];
			[alert setMessageText:[NSString stringWithFormat:@"Failed to open the %@ doc", description]];
			[alert setInformativeText:[NSString stringWithFormat:@"openURL failed to open: %@.%@\n\nPlease report this bug.", name, ext]];
			[alert runModal];
			break;
		}
	}
}

- (void) openQuickstartGuide: (id)sender
{
	@autoreleasepool {
		[self openDocAndDisplayError:@"quickstartguide" withExtension:@"html" withDocDescription:@"Quickstart Guide"];
	}
}

- (void) openJSScriptingAPIDoc: (id)sender
{
	@autoreleasepool {
		[self openDocAndDisplayError:@"Scripting" withExtension:@"md" withDocDescription:@"JS Scripting API"];
	}
}

@end

bool cocoaSetupWZMenus()
{
	@autoreleasepool {

		// Add "Help" menu
		NSMenuItem* mainMenuHelpItem = [[NSMenuItem alloc] initWithTitle:@"Help" action:nil keyEquivalent:@""];
		NSMenu* helpMenu = [[NSMenu alloc] initWithTitle:@"Help"];
		NSMenuItem* menuItem = [helpMenu addItemWithTitle:@"Warzone Quickstart Guide" action:@selector(openQuickstartGuide:) keyEquivalent:@""];
		[menuItem setTarget: WZMenuActionsHandler.sharedInstance];
		menuItem = [helpMenu addItemWithTitle:@"JS Scripting API" action:@selector(openJSScriptingAPIDoc:) keyEquivalent:@""];
		[menuItem setTarget: WZMenuActionsHandler.sharedInstance];
		[mainMenuHelpItem setSubmenu: helpMenu];

		NSMenu* rootMenu = [NSApp mainMenu];
		if (rootMenu == nil)
		{
			return false; // Should not happen
		}
		[rootMenu addItem:mainMenuHelpItem];

		// Add "About Warzone" menu item (which opens the standard Cocoa About panel)
		// to the first menu item's submenu (which should be the "Warzone" menu)
		NSMenu* firstSubmenu = [[rootMenu itemAtIndex:0] submenu];
		[firstSubmenu insertItemWithTitle:@"About Warzone" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@"" atIndex:0];
	}

	return true;
}
