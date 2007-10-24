#include "screen.h"

CScreen g_Screen;

int		CScreen::initialize(void) {
	return (SDL_Init(SDL_INIT_VIDEO));
}

bool	CScreen::setVideoMode(void) {
	return (SDL_SetVideoMode(m_width, m_height, m_bpp, m_flags));
}
