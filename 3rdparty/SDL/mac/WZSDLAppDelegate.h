/*   WZSDLAppDelegate.mm -
	 Provides the NSApplicationDelegate for our Cocoa-ized SDL app
	 Rewritten and simplified for Warzone 2100 3.2.x (which uses Qt5 and SDL 2).
 */

#ifndef _WZSDLAppDelegate_h_
#define _WZSDLAppDelegate_h_

#import <Cocoa/Cocoa.h>

@interface WZSDLAppDelegate : NSObject <NSApplicationDelegate>
- (void)setMainEventLoop:(void (*)())func;
@end

#endif /* _WZSDLAppDelegate_h_ */
