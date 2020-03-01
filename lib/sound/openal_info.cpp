/*
   This file is derived from openal-info.c
   (https://github.com/kcat/openal-soft/blob/master/utils/openal-info.c)

   Original, and changes / additions, are licensed under the MIT License:

   * Copyright (C) 2020 Warzone 2100 Project
   * Copyright (c) 2010 by Chris Robinson <chris.kcat@gmail.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   * THE SOFTWARE.
*/

#include "lib/framework/frame.h"
#include "openal_info.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <sstream>
#include <iomanip>

#ifndef ALC_ENUMERATE_ALL_EXT
#define ALC_DEFAULT_ALL_DEVICES_SPECIFIER        0x1012
#define ALC_ALL_DEVICES_SPECIFIER                0x1013
#endif

#ifndef ALC_EXT_EFX
#define ALC_EFX_MAJOR_VERSION                    0x20001
#define ALC_EFX_MINOR_VERSION                    0x20002
#define ALC_MAX_AUXILIARY_SENDS                  0x20003
#endif

#define MAX_WIDTH  80

static std::string printList(const char *list, char separator)
{
	std::stringstream buf;

	size_t col = MAX_WIDTH, len;
	const char *indent = "    ";
	const char *next;

	if(!list || *list == '\0')
	{
		buf << "\n" << indent << "!!! none !!!";
		return buf.str();
	}

	do {
		next = strchr(list, separator);
		if(next)
		{
			len = (size_t)(next-list);
			do {
				next++;
			} while(*next == separator);
		}
		else
			len = strlen(list);

		if(len + col + 2 >= MAX_WIDTH)
		{
			buf << "\n" << indent;
			col = strlen(indent);
		}
		else
		{
			buf << " ";
			col++;
		}

		buf << std::string(list, len);
		col += len;

		if(!next || *next == '\0')
			break;
		buf << " ";
		col++;

		list = next;
	} while(1);

	return buf.str();
}

static std::string printDeviceList(const char *list)
{
	std::stringstream buf;

	if(!list || *list == '\0')
	{
		buf << "    !!! none !!!\n";
	}
	else do {
		buf << "    " << list << "\n";
		list += strlen(list) + 1;
	} while(*list != '\0');

	return buf.str();
}

static ALenum checkALErrors(std::stringstream& buf, int linenum)
{
	ALenum err = alGetError();
	if(err != AL_NO_ERROR)
	{
		const char *pErrStr = alGetString(err);
		std::ios init(nullptr);
		init.copyfmt(buf);
		buf << "OpenAL Error: " << ((pErrStr) ? pErrStr : "<n/a>") << " (0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << err;
		buf.copyfmt(init);
		buf << "), @ " << linenum << "\n";
	}
	return err;
}
#define checkALErrors(buf) checkALErrors(buf, __LINE__)

static ALCenum checkALCErrors(std::stringstream& buf, ALCdevice *device, int linenum)
{
	ALCenum err = alcGetError(device);
	if(err != ALC_NO_ERROR)
	{
		const char *pErrStr = alcGetString(device, err);
		std::ios init(nullptr);
		init.copyfmt(buf);
		buf << "ALC Error: " << ((pErrStr) ? pErrStr : "<n/a>") << " (0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << err;
		buf.copyfmt(init);
		buf << "), @ " << linenum << "\n";
	}
	return err;
}
#define checkALCErrors(buf, x) checkALCErrors(buf, (x),__LINE__)

void OpenALInfo::Output_ALInfo(const outputHandlerFuncType& outputHandler)
{
	std::stringstream buf;

	const char * pOpenAL_Str = nullptr;

	pOpenAL_Str = alGetString(AL_VENDOR);
	buf << "OpenAL vendor string: " << ((pOpenAL_Str) ? pOpenAL_Str : "") << "\n";
	pOpenAL_Str = alGetString(AL_RENDERER);
	buf << "OpenAL renderer string: " << ((pOpenAL_Str) ? pOpenAL_Str : "") << "\n";
	pOpenAL_Str = alGetString(AL_VERSION);
	buf << "OpenAL version string: " << ((pOpenAL_Str) ? pOpenAL_Str : "") << "\n";

	std::string openALExtensionsOutput = printList(alGetString(AL_EXTENSIONS), ' ');
	buf << "OpenAL extensions:" << openALExtensionsOutput << "\n";

	checkALErrors(buf);

	if (outputHandler)
	{
		outputHandler(buf.str());
	}
}

void OpenALInfo::Output_ALCInfo(ALCdevice *device, const outputHandlerFuncType& outputHandler)
{
	std::stringstream buf;

	ALCint major, minor;

	if(device)
	{
		const ALCchar *devname = nullptr;
		buf << "\n";
		if(alcIsExtensionPresent(device, "ALC_ENUMERATE_ALL_EXT") != AL_FALSE)
		{
			devname = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
		}
		if(checkALCErrors(buf, device) != ALC_NO_ERROR || !devname)
		{
			devname = alcGetString(device, ALC_DEVICE_SPECIFIER);
		}
		buf << "** Info for device \"" << ((devname) ? devname : "<Name not found>") << "\" **\n";
	}
	alcGetIntegerv(device, ALC_MAJOR_VERSION, 1, &major);
	alcGetIntegerv(device, ALC_MINOR_VERSION, 1, &minor);
	if(checkALCErrors(buf, device) == ALC_NO_ERROR)
	{
		buf << "ALC version: " << major << "." << minor << "\n";
	}
	if(device)
	{
		buf << "ALC extensions:" << printList(alcGetString(device, ALC_EXTENSIONS), ' ') << "\n";
		checkALCErrors(buf, device);

		ALCint srate;
		alcGetIntegerv(device, ALC_FREQUENCY, 1, &srate);
		buf << "Output frequency: " << srate << " Hz \n";

		alcGetIntegerv(device, ALC_REFRESH, 1, &srate);
		buf << "Update rate: " << srate << " Hz \n";
	}

	if (outputHandler)
	{
		outputHandler(buf.str());
	}
}

void OpenALInfo::Output_PlaybackDevices(const outputHandlerFuncType& outputHandler)
{
	std::stringstream buf;

	buf << "Available playback devices:\n";
	ALCenum enumerateDevicesSpecifier = ALC_DEVICE_SPECIFIER;
	if(alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT") != AL_FALSE)
	{
		enumerateDevicesSpecifier = ALC_ALL_DEVICES_SPECIFIER;
	}
	buf << printDeviceList(alcGetString(nullptr, enumerateDevicesSpecifier));

	ALCenum defaultDeviceParam = ALC_DEFAULT_DEVICE_SPECIFIER;
	if(alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT") != AL_FALSE)
	{
		defaultDeviceParam = ALC_DEFAULT_ALL_DEVICES_SPECIFIER;
	}
	const ALCchar * pDefaultDeviceStr = alcGetString(nullptr, defaultDeviceParam);
	buf << "Default playback device: " << ((pDefaultDeviceStr) ? pDefaultDeviceStr : "<Not found>") << "\n";

	Output_ALCInfo(nullptr, [&buf](const std::string& output) { buf << output; });

	if (outputHandler)
	{
		outputHandler(buf.str());
	}
}
