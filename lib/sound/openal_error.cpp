/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2012  Warzone 2100 Project

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
#include "lib/framework/frame.h"

#include "openal_error.h"

#include "lib/framework/stdio_ext.h"

#if !defined(WZ_NOSOUND)
#ifdef WZ_OS_MAC
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

ALenum __sound_GetError(const char *location_description)
{
	const char *errorString;
	ALenum error = alGetError();

	if (error == AL_NO_ERROR)
	{
		return error;
	}

	switch (error)
	{
	case AL_INVALID_NAME:
		errorString = "AL_INVALID_NAME: Invalid name parameter passed";
		break;

	case AL_INVALID_ENUM:
		errorString = "AL_INVALID_ENUM: Invalid enum value";
		break;

	case AL_INVALID_VALUE:
		errorString = "AL_INVALID_VALUE: Invalid parameter value";
		break;

	case AL_INVALID_OPERATION:
		errorString = "AL_INVALID_OPERATION: Illegal call";
		break;

	case AL_OUT_OF_MEMORY:
		errorString = "AL_OUT_OF_MEMORY: OpenAL ran out of memory";
		break;

	default:
		sasprintf((char **)&errorString, "unknown error code (%d); please report this number (along with the "
		        "fact that it is an \"unknown OpenAL error code\"): 0x%x", (int)error, (unsigned int)error);
		break;
	}

	debug(LOG_SOUND, "OpenAL raised an error: \"%s\"; at %s", errorString, location_description);

	return error;
}

ALenum __sound_GetContextError(ALCdevice *device, const char *location_description)
{
	const char *errorString;
	ALCenum error = alcGetError(device);

	if (error == ALC_NO_ERROR)
	{
		return error;
	}

	switch (error)
	{
	case ALC_INVALID_DEVICE:
		errorString = "ALC_INVALID_DEVICE: Invalid or no device selected";
		break;

	case ALC_INVALID_CONTEXT:
		errorString = "ALC_INVALID_CONTEXT: Invalid or no context selected";
		break;

	case ALC_INVALID_ENUM:
		errorString = "ALC_INVALID_ENUM: Invalid enum value";
		break;

	case ALC_INVALID_VALUE:
		errorString = "ALC_INVALID_VALUE: Invalid parameter value";
		break;

	case ALC_OUT_OF_MEMORY:
		errorString = "ALC_OUT_OF_MEMORY: OpenAL ran out of memory";
		break;

	default:
		sasprintf((char **)&errorString, "unknown error code (%d); please report this number (along with the "
		        "fact that it is an \"unknown OpenAL error code\"): 0x%x", (int)error, (unsigned int)error);
		break;
	}

	debug(LOG_SOUND, "OpenAL raised a context error: \"%s\"; at %s", errorString, location_description);

	return error;
}
#endif // !defined(WZ_NOSOUND)
