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

#pragma once

#ifndef __INCLUDED_LIB_SOUND_OPENAL_INFO_H__
#define __INCLUDED_LIB_SOUND_OPENAL_INFO_H__

#include "lib/framework/frame.h"

#ifdef WZ_OS_MAC
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

#include <functional>
#include <string>

namespace OpenALInfo
{
	typedef std::function<void (const std::string& output)> outputHandlerFuncType;

	void Output_ALInfo(const outputHandlerFuncType& outputHandler);
	void Output_ALCInfo(ALCdevice *device, const outputHandlerFuncType& outputHandler);
	void Output_PlaybackDevices(const outputHandlerFuncType& outputHandler);
}

#endif // __INCLUDED_LIB_SOUND_OPENAL_INFO_H__
