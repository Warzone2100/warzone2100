
/*

 SDL_framerate: framerate manager
 
 LGPL (c) A. Schiffler
 
 */

#ifndef _SDL_framerate_h
#define _SDL_framerate_h

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* --------- Definitions */

/* Some rates in Hz */

#define FPS_UPPER_LIMIT		200
#define FPS_LOWER_LIMIT		1
#define FPS_DEFAULT		30

/* --------- Structure variables */

    typedef struct {
	uint32_t framecount;
	float rateticks;
	uint32_t lastticks;
	uint32_t rate;
    } FPSmanager;

/* Functions return 0 or value for sucess and -1 for error */

    void SDL_initFramerate(FPSmanager * manager);
    int SDL_setFramerate(FPSmanager * manager, int rate);
    int SDL_getFramerate(FPSmanager * manager);
    void SDL_framerateDelay(FPSmanager * manager);

/* --- */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif				/* _SDL_framerate_h */
