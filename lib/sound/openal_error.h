/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2017  Warzone 2100 Project

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

#ifndef __INCLUDED_LIB_SOUND_OPENAL_ERROR_H__
#define __INCLUDED_LIB_SOUND_OPENAL_ERROR_H__

#include "lib/framework/frame.h"

#ifdef WZ_OS_MAC
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

extern ALenum __sound_GetError(const char *location_description);
extern ALenum __sound_GetContextError(ALCdevice *device, const char *location_description);

/** Check whether an error occurred in OpenAL's current sound context. If one
 *  did occur, print an error message and return the error code.
 *  \return the OpenAL error code
 */
#define sound_GetError() \
	__sound_GetError(AT_MACRO)

/** Check whether an error occurred for the specified OpenAL device. If one
 *  did occur, print an error message and return the error code.
 *  \param device the OpenAL device (ALCdevice* pointer) to check for errors on
 *  \return the OpenAL error code
 */
#define sound_GetContextError(device) \
	__sound_GetContextError(device, AT_MACRO)

#endif // __INCLUDED_LIB_SOUND_OPENAL_ERROR_H__
