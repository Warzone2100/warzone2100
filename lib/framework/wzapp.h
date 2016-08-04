/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef __INCLUDED_WZAPP_C_H__
#define __INCLUDED_WZAPP_C_H__

#include "frame.h"
#include <vector>

struct WZ_THREAD;
struct WZ_MUTEX;
struct WZ_SEMAPHORE;

struct screeninfo
{
	int width;
	int height;
	int refresh_rate;
	int screen;
};

void wzMain(int &argc, char **argv);
bool wzMainScreenSetup(int antialiasing = 0, bool fullscreen = false, bool vsync = true);
void wzMainEventLoop();
void wzQuit();              ///< Quit game
void wzShutdown();
void wzToggleFullscreen();
bool wzIsFullscreen();
void wzSetCursor(CURSOR index);
void wzScreenFlip();	///< Swap the graphics buffers
void wzShowMouse(bool visible); ///< Show the Mouse?
void wzGrabMouse();		///< Trap mouse cursor in application window
void wzReleaseMouse();	///< Undo the wzGrabMouse operation
bool wzActiveWindow();	///< Whether application currently has the mouse pointer over it
int wzGetTicks();		///< Milliseconds since start of game
WZ_DECL_NONNULL(1) void wzFatalDialog(const char *text);	///< Throw up a modal warning dialog

std::vector<screeninfo> wzAvailableResolutions();
void wzSetSwapInterval(int swap);
int wzGetSwapInterval();
QString wzGetSelection();
QString wzGetCurrentText();
unsigned int wzGetCurrentKey();
void wzDelay(unsigned int delay);	//delay in ms
// unicode text support
void StartTextInput();
void StopTextInput();
// Thread related
WZ_THREAD *wzThreadCreate(int (*threadFunc)(void *), void *data);
WZ_DECL_NONNULL(1) int wzThreadJoin(WZ_THREAD *thread);
WZ_DECL_NONNULL(1) void wzThreadStart(WZ_THREAD *thread);
void wzYieldCurrentThread();
WZ_MUTEX *wzMutexCreate();
WZ_DECL_NONNULL(1) void wzMutexDestroy(WZ_MUTEX *mutex);
WZ_DECL_NONNULL(1) void wzMutexLock(WZ_MUTEX *mutex);
WZ_DECL_NONNULL(1) void wzMutexUnlock(WZ_MUTEX *mutex);
WZ_SEMAPHORE *wzSemaphoreCreate(int startValue);
WZ_DECL_NONNULL(1) void wzSemaphoreDestroy(WZ_SEMAPHORE *semaphore);
WZ_DECL_NONNULL(1) void wzSemaphoreWait(WZ_SEMAPHORE *semaphore);
WZ_DECL_NONNULL(1) void wzSemaphorePost(WZ_SEMAPHORE *semaphore);

#endif
