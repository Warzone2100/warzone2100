#import "lib/framework/wzglobal.h"
#import "lib/framework/wzapp.h"

#ifdef WZ_OS_MAC

#import <AppKit/NSOpenGL.h>

void wzSetSwapInterval(int interval)
{
	NSOpenGLContext *context = [NSOpenGLContext currentContext];
	if (interval >= 0)
	{
		[context setValues:&interval forParameter:NSOpenGLCPSwapInterval];
	}
}

int wzGetSwapInterval()
{
	NSOpenGLContext *context = [NSOpenGLContext currentContext];
	int interval;
	[context getValues:&interval forParameter:NSOpenGLCPSwapInterval];
	return interval;
}

#endif // WZ_OS_MAC
