#import "swapinterval.h"
#import "lib/framework/wzglobal.h"

#ifdef WZ_OS_MAC

#import <AppKit/NSOpenGL.h>

void setSwapInterval(QGLWidget const &, int * interval)
{
	NSOpenGLContext *context = [NSOpenGLContext currentContext];
	if (*interval >= 0)
	{
		[context setValues:interval forParameter:NSOpenGLCPSwapInterval];
	}
	[context getValues:interval forParameter:NSOpenGLCPSwapInterval];
}

#endif // WZ_OS_MAC
