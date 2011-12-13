#include <QtCore/QCoreApplication>
#include "lib/framework/wzapp.h"
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_timer.h>

extern void mainLoop();

unsigned int screenWidth = 0;
unsigned int screenHeight = 0;

/**************************/
/***     Misc support   ***/
/**************************/

void wzSetCursor(CURSOR index)
{
	// TBD
}

void wzShowMouse(bool visible)
{
	SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

int wzGetTicks()
{
	return SDL_GetTicks();
}

void wzFatalDialog(char const*)
{
	// no-op
}

void wzScreenFlip()
{
	SDL_GL_SwapBuffers();
}

void wzQuit()
{
	// no-op
}

void wzGrabMouse()
{
	SDL_WM_GrabInput(SDL_GRAB_ON);
}

void wzReleaseMouse()
{
	SDL_WM_GrabInput(SDL_GRAB_OFF);
}

/**************************/
/***    Thread support  ***/
/**************************/

WZ_THREAD *wzThreadCreate(int (*threadFunc)(void *), void *data)
{
	return SDL_CreateThread(threadFunc, data);
}

int wzThreadJoin(WZ_THREAD *thread)
{
	int result;
	SDL_WaitThread(thread, &result);
	return result;
}

void wzThreadStart(WZ_THREAD *thread)
{
	(void)thread; // no-op
}

void wzYieldCurrentThread()
{
	SDL_Delay(40);
}

WZ_MUTEX *wzMutexCreate()
{
	return SDL_CreateMutex();
}

void wzMutexDestroy(WZ_MUTEX *mutex)
{
	SDL_DestroyMutex(mutex);
}

void wzMutexLock(WZ_MUTEX *mutex)
{
	SDL_LockMutex(mutex);
}

void wzMutexUnlock(WZ_MUTEX *mutex)
{
	SDL_UnlockMutex(mutex);
}

WZ_SEMAPHORE *wzSemaphoreCreate(int startValue)
{
	return SDL_CreateSemaphore(startValue);
}

void wzSemaphoreDestroy(WZ_SEMAPHORE *semaphore)
{
	SDL_DestroySemaphore(semaphore);
}

void wzSemaphoreWait(WZ_SEMAPHORE *semaphore)
{
	SDL_SemWait(semaphore);
}

void wzSemaphorePost(WZ_SEMAPHORE *semaphore)
{
	SDL_SemPost(semaphore);
}

/**************************/
/***     Main support   ***/
/**************************/

void wzMain(int &argc, char **argv)
{
	QCoreApplication app(argc, argv);  // For Qt-script.
}

bool wzMain2()
{
	return true;
}

void wzMain3()
{
	mainLoop();
}
