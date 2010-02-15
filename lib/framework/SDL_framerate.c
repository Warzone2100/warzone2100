
/*

 SDL_framerate: framerate manager

 LGPL (c) A. Schiffler
 
 */

#include "frame.h"
#include "wzapp_c.h"
#include "SDL_framerate.h"

/* 
   Initialize the framerate manager
*/

void SDL_initFramerate(FPSmanager * manager)
{
    /*
     * Store some sane values 
     */
    manager->framecount = 0;
    manager->rate = FPS_DEFAULT;
    manager->rateticks = (1000.0f / (float) FPS_DEFAULT);
    manager->lastticks = 0;
}

/* 
   Set the framerate in Hz 
*/

int SDL_setFramerate(FPSmanager * manager, int rate)
{
    if ((rate >= FPS_LOWER_LIMIT) && (rate <= FPS_UPPER_LIMIT)) {
	manager->framecount = 0;
	manager->rate = rate;
	manager->rateticks = (1000.0f / (float) rate);
	return (0);
    } else {
	return (-1);
    }
}

/* 
  Return the current target framerate in Hz 
*/

int SDL_getFramerate(FPSmanager * manager)
{
    if (manager == NULL) {
	return (-1);
    } else {
	return (manager->rate);
    }
}

/* 
  Delay execution to maintain a constant framerate. Calculate fps.
*/

void SDL_framerateDelay(FPSmanager * manager)
{
    uint32_t current_ticks;
    uint32_t target_ticks;
    uint32_t the_delay;

    /*
     * Next frame 
     */
    manager->framecount++;

    /*
     * Get/calc ticks 
     */
    current_ticks = wzGetTicks();
    target_ticks = manager->lastticks + (uint32_t) ((float) manager->framecount * manager->rateticks);

	if (current_ticks <= target_ticks)
	{
	the_delay = target_ticks - current_ticks;
#if defined(WZ_OS_WIN32) || defined(WZ_OS_WIN64)
	Sleep(the_delay / 1000);
#else
	usleep(the_delay);
#endif
	} else {
		manager->framecount = 0;
		manager->lastticks = wzGetTicks();
	}
}
