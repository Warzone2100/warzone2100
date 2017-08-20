/*   WZSDLAppDelegate.mm -
	 Provides the NSApplicationDelegate for our Cocoa-ized SDL app
	 Rewritten and simplified for Warzone 2100 3.2.x (which uses Qt5 and SDL 2).
*/

#include <SDL.h>
#include "WZSDLAppDelegate.h"
// Fix MIN, MAX redefinition error when including frame.h below
#undef MIN
#undef MAX
#include "lib/framework/frame.h"

static void (*mainEventLoop)() = nullptr;

/* The main class of the application, the application's delegate */
@implementation WZSDLAppDelegate
- (void)setMainEventLoop:(void (*)())func
{
    mainEventLoop = func;
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
	#pragma unused (note)
	
    ASSERT(mainEventLoop != nullptr, "The NSApplication's (SDLAppDelegate) mainEventLoop is NULL in applicationDidFinishLaunching.");
    
    [self
     performSelectorOnMainThread:@selector(applicationRunMainEventLoop)
     withObject:nil
     waitUntilDone:NO];
}

- (void) applicationRunMainEventLoop
{
    /* Hand off to main application code */
    if (mainEventLoop != NULL) {
        mainEventLoop();
    }
    
    /* We're done, thank you for playing */
    [NSApp stop:nil];
}

/* Called when the Quit menu item is clicked */
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    #pragma unused (sender)
    
    /* Post a SDL_QUIT event */
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);

    // Cancel this immediate termination -
    // The game itself will handle saving and shutting down gracefully
    // (once it processes the SDL_QUIT event).
    return NSTerminateCancel;
}

@end


