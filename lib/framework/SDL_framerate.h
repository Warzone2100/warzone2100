
/*

 SDL_framerate: framerate manager
 
 LGPL (c) A. Schiffler
 
 */

#ifndef _SDL_framerate_h
#define _SDL_framerate_h

#include <SDL.h>

/* --------- Definitions */

/* Some rates in Hz */

#define FPS_UPPER_LIMIT		200
#define FPS_LOWER_LIMIT		1
#define FPS_DEFAULT		30

/* --------- Structure variables */

    typedef struct {
	Uint32 framecount;
	float rateticks;
	Uint32 lastticks;
	Uint32 rate;
    } FPSmanager;

/* Functions return 0 or value for sucess and -1 for error */

    void SDL_initFramerate(FPSmanager * manager);
    int SDL_setFramerate(FPSmanager * manager, int rate);
    int SDL_getFramerate(FPSmanager * manager);
    void SDL_framerateDelay(FPSmanager * manager);

/* --- */

#endif				/* _SDL_framerate_h */
