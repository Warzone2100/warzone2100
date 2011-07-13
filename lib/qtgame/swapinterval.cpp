#include "swapinterval.h"
#include "lib/framework/wzglobal.h"
#include "lib/framework/opengl.h"

#if defined(WZ_WS_X11)

#include <GL/glxew.h>
// X11 polution
#ifdef Status
#undef Status
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef Bool
#undef Bool
#endif

#include <QtGui/QX11Info>
#include <QtOpenGL/QGLWidget>


void setSwapInterval(QGLWidget const &glWidget, int * interval)
{
	QGLContext const &context = *glWidget.context();
	QX11Info const &xinfo = glWidget.x11Info();
	if (GLXEW_EXT_swap_control)
	{
		unsigned clampedInterval;
		if (*interval < 0)
			*interval = 0;
		glXSwapIntervalEXT(xinfo.display(), glWidget.winId(), *interval);
		glXQueryDrawable(xinfo.display(), glWidget.winId(), GLX_SWAP_INTERVAL_EXT, &clampedInterval);
		*interval = clampedInterval;
	}
	else if (xinfo.display() && strstr(glXQueryExtensionsString(xinfo.display(), xinfo.screen()), "GLX_MESA_swap_control"))
	{
		typedef int (* PFNGLXSWAPINTERVALMESAPROC)(unsigned);
		typedef int (* PFNGLXGETSWAPINTERVALMESAPROC)(void);
		PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESA = (PFNGLXSWAPINTERVALMESAPROC) context.getProcAddress("glXSwapIntervalMESA");
		PFNGLXGETSWAPINTERVALMESAPROC glXGetSwapIntervalMESA = (PFNGLXGETSWAPINTERVALMESAPROC) context.getProcAddress("glXGetSwapIntervalMESA");

		if (glXSwapIntervalMESA && glXGetSwapIntervalMESA)
		{
			if (*interval < 0)
				*interval = 0;
			glXSwapIntervalMESA(*interval);
			*interval = glXGetSwapIntervalMESA();
		}
	}
	else if (GLXEW_SGI_swap_control)
	{
		if (*interval < 1)
			*interval = 1;
		if (glXSwapIntervalSGI(*interval))
		{
			// Error, revert to default
			*interval = 1;
			glXSwapIntervalSGI(1);
		}
	}
	else
	{
		*interval = -1;
	}
}

#elif defined(WZ_WS_WIN)
#include <GL/wglew.h>
#include <QtOpenGL/QGLWidget>

void setSwapInterval(QGLWidget const &, int * interval)
{
	if (WGLEW_EXT_swap_control)
	{
		if (*interval < 0)
			*interval = 0;
		wglSwapIntervalEXT(*interval);
		*interval = wglGetSwapIntervalEXT();
	}
	else
	{
		*interval = -1;
	}
}
#endif
