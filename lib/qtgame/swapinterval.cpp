#include "swapinterval.h"
#include "lib/framework/wzglobal.h"
#include "lib/framework/opengl.h"

#if defined(WZ_WS_X11)

#include <GL/glx.h> // GLXDrawable
// X11 polution
#ifdef Status
#undef Status
#endif // Status
#ifdef CursorShape
#undef CursorShape
#endif // CursorShape
#ifdef Bool
#undef Bool
#endif // Bool

#ifndef GLX_SWAP_INTERVAL_EXT
#define GLX_SWAP_INTERVAL_EXT 0x20F1
#endif // GLX_SWAP_INTERVAL_EXT

#include <QtX11Extras/QX11Info>
#include <QtOpenGL/QGLWidget>


void setSwapInterval(QGLWidget const &glWidget, int *interval)
{
	typedef void (* PFNGLXQUERYDRAWABLEPROC)(Display *, GLXDrawable, int, unsigned int *);
	typedef void (* PFNGLXSWAPINTERVALEXTPROC)(Display *, GLXDrawable, int);
	typedef int (* PFNGLXGETSWAPINTERVALMESAPROC)(void);
	typedef int (* PFNGLXSWAPINTERVALMESAPROC)(unsigned);
	typedef int (* PFNGLXSWAPINTERVALSGIPROC)(int);
	PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT;
	PFNGLXQUERYDRAWABLEPROC glXQueryDrawable;
	PFNGLXGETSWAPINTERVALMESAPROC glXGetSwapIntervalMESA;
	PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESA;
	PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI;
	QGLContext const &context = *glWidget.context();

	glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC) context.getProcAddress("glXSwapIntervalEXT");
	glXQueryDrawable = (PFNGLXQUERYDRAWABLEPROC) context.getProcAddress("glXQueryDrawable");
	if (glXSwapIntervalEXT && glXQueryDrawable)
	{
		unsigned clampedInterval;
		if (*interval < 0)
		{
			*interval = 0;
		}
		glXSwapIntervalEXT(QX11Info::display(), glWidget.winId(), *interval);
		glXQueryDrawable(QX11Info::display(), glWidget.winId(), GLX_SWAP_INTERVAL_EXT, &clampedInterval);
		*interval = clampedInterval;
		return;
	}

	glXSwapIntervalMESA = (PFNGLXSWAPINTERVALMESAPROC) context.getProcAddress("glXSwapIntervalMESA");
	glXGetSwapIntervalMESA = (PFNGLXGETSWAPINTERVALMESAPROC) context.getProcAddress("glXGetSwapIntervalMESA");
	if (glXSwapIntervalMESA && glXGetSwapIntervalMESA)
	{
		if (*interval < 0)
		{
			*interval = 0;
		}
		glXSwapIntervalMESA(*interval);
		*interval = glXGetSwapIntervalMESA();
		return;
	}

	glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC) context.getProcAddress("glXSwapIntervalSGI");
	if (glXSwapIntervalSGI)
	{
		if (*interval < 1)
		{
			*interval = 1;
		}
		if (glXSwapIntervalSGI(*interval))
		{
			// Error, revert to default
			*interval = 1;
			glXSwapIntervalSGI(1);
		}
		return;
	}

	*interval = -1;
}

#elif defined(WZ_WS_WIN) // WZ_WS_X11
#include <QtOpenGL/QGLWidget>

void setSwapInterval(QGLWidget const &glWidget, int *interval)
{
	typedef int (WINAPI * PFNWGLGETSWAPINTERVALEXTPROC)(void);
	typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int);
	PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
	QGLContext const &context = *glWidget.context();

	wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC) context.getProcAddress("wglGetSwapIntervalEXT");
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) context.getProcAddress("wglSwapIntervalEXT");

	if (wglGetSwapIntervalEXT && wglSwapIntervalEXT)
	{
		if (*interval < 0)
		{
			*interval = 0;
		}
		wglSwapIntervalEXT(*interval);
		*interval = wglGetSwapIntervalEXT();
	}
	else
	{
		*interval = -1;
	}
}
#endif // WZ_WS_X11
