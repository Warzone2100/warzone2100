#ifndef _screen_h
#define _screen_h

#ifdef _WIN32
	#include <windows.h>	// required by gl.h
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include "pie_types.h"

class CScreen {
public:
	Uint16	m_width;
	Uint16	m_height;
	Sint32	m_bpp;
	Uint32	m_flags;

	int		initialize(void);
	bool	setVideoMode(void);

};

extern CScreen g_Screen;

#endif
