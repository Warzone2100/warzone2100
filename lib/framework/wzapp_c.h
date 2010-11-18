/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

// TODO Replace this file during Qt merge:
// git checkout origin/qt-trunk lib/framework/wzapp_c.h
#define WZ_THREAD SDL_Thread
#define WZ_MUTEX SDL_mutex
#define WZ_SEMAPHORE SDL_sem
#define wzMutexLock SDL_LockMutex
#define wzMutexUnlock SDL_UnlockMutex
#define wzSemaphoreCreate SDL_CreateSemaphore
#define wzSemaphoreDestroy SDL_DestroySemaphore
#define wzSemaphoreWait SDL_SemWait
#define wzSemaphorePost SDL_SemPost
#define wzThreadJoin(x) SDL_WaitThread(x, NULL)
#define wzMutexDestroy SDL_DestroyMutex
#define wzMutexCreate SDL_CreateMutex
#define wzYieldCurrentThread() SDL_Delay(10)
#define wzThreadCreate SDL_CreateThread
#define wzThreadStart(x)
#define wzGetTicks() SDL_GetTicks()
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>

#endif
