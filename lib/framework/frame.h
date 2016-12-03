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
/*! \file frame.h
 * \brief The framework library initialisation and shutdown routines.
 */
#ifndef _frame_h
#define _frame_h

#include "wzglobal.h"
#include <stdlib.h>

// Workaround X11 headers #defining Status
#ifdef Status
# undef Status
#endif

#ifndef WZ_CXX11
# define nullptr NULL
#endif

#include "types.h"
/**
* NOTE: the next two #include lines are needed by MSVC to override the default,
* non C99 compliant routines, and redefinition; different linkage errors
*/
#include "stdio_ext.h"
#include "string_ext.h"

#include "macros.h"
#include "debug.h"

#include "i18n.h"
#include "trig.h"
#include "cursors.h"

#define REALCONCAT(x, y) x ## y
#define CONCAT(x, y) REALCONCAT(x, y)

extern uint32_t selectedPlayer;      ///< The player number corresponding to this client.
extern uint32_t realSelectedPlayer;  ///< The player number corresponding to this client (same as selectedPlayer, unless changing players in the debug menu).
#define MAX_PLAYERS         11                 ///< Maximum number of players in the game.
#define MAX_PLAYERS_IN_GUI  (MAX_PLAYERS - 1)  ///< One player reserved for scavengers.
#define PLAYER_FEATURE      (MAX_PLAYERS + 1)
#define MAX_PLAYER_SLOTS    (MAX_PLAYERS + 2)  ///< Max players plus 1 baba and 1 reserved for features. Actually, if baba is a regular player, then it's plus 1 unused?

#if MAX_PLAYERS <= 8
typedef uint8_t PlayerMask;
#elif MAX_PLAYERS <= 16
typedef uint16_t PlayerMask;
#else
#error Warzone 2100 is not a MMO.
#endif

#if defined(WZ_OS_WIN)
# define WZ_WRITEDIR "Warzone 2100 3.2"
#elif defined(WZ_OS_MAC)
# define WZ_WRITEDIR "Warzone 2100 3.2"
#else
# define WZ_WRITEDIR ".warzone2100-3.2"
#endif

enum QUEUE_MODE
{
	ModeQueue,      ///< Sends a message on the game queue, which will get synchronised, by sending a GAME_ message.
	ModeImmediate   ///< Performs the action immediately. Must already have been synchronised, for example by sending a GAME_ message.
};


/** Initialise the framework library
 *  @param pWindowName the text to appear in the window title bar
 *  @param width the display widget
 *  @param height the display height
 *  @param bitDepth the display bit depth
 *  @param fullScreen whether to start full screen or windowed
 *  @param vsync if to sync to the vertical blanking interval or not
 *
 *  @return true when the framework library is successfully initialised, false
 *          when a part of the initialisation failed.
 */
bool frameInitialise();

/** Shut down the framework library.
 */
void frameShutDown();

/*!
 * Set the framerate limit
 *
 * \param fpsLimit Desired framerate
 */
void setFramerateLimit(int fpsLimit);

/*!
 * Get the framerate limit
 *
 * \return Desired framerate
 */
int getFramerateLimit();

/** Call this each cycle to allow the framework to deal with
 * windows messages, and do general house keeping.
 */
void frameUpdate();

/** Returns the current frame we're on - used to establish whats on screen. */
UDWORD frameGetFrameNumber();

/** Return framerate of the last second. */
int frameRate();

static inline WZ_DECL_CONST const char *bool2string(bool var)
{
	return (var ? "true" : "false");
}

#endif
