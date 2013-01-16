/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#include <QtCore/QSize>

struct WZ_THREAD;
struct WZ_MUTEX;
struct WZ_SEMAPHORE;


void wzMain(int &argc, char **argv);
bool wzMain2();
void wzMain3();
void wzQuit(void);              ///< Quit game
void wzShutdown();
void wzToggleFullscreen();
void wzSetCursor(CURSOR index);
void wzScreenFlip(void);	///< Swap the graphics buffers
void wzShowMouse(bool visible); ///< Show the Mouse?
void wzGrabMouse(void);		///< Trap mouse cursor in application window
void wzReleaseMouse(void);	///< Undo the wzGrabMouse operation
bool wzActiveWindow(void);	///< Whether application currently has the mouse pointer over it
int wzGetTicks(void);		///< Milliseconds since start of game
void wzFatalDialog(const char *text);	///< Throw up a modal warning dialog
QList<QSize> wzAvailableResolutions();  ///< Get list of available resolutions.
void wzSetSwapInterval(int swap);
int wzGetSwapInterval();
QString wzGetSelection();

// Thread related
WZ_THREAD *wzThreadCreate(int (*threadFunc)(void *), void *data);
int wzThreadJoin(WZ_THREAD *thread);
void wzThreadStart(WZ_THREAD *thread);
void wzYieldCurrentThread(void);
WZ_MUTEX *wzMutexCreate(void);
void wzMutexDestroy(WZ_MUTEX *mutex);
void wzMutexLock(WZ_MUTEX *mutex);
void wzMutexUnlock(WZ_MUTEX *mutex);
WZ_SEMAPHORE *wzSemaphoreCreate(int startValue);
void wzSemaphoreDestroy(WZ_SEMAPHORE *semaphore);
void wzSemaphoreWait(WZ_SEMAPHORE *semaphore);
void wzSemaphorePost(WZ_SEMAPHORE *semaphore);

#endif
