/*
	This file is part of Warzone 2100.
	Copyright (C) 2022-2025  Warzone 2100 Project

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
/** @file
 *  Private header for sdl-backend only
 */

#pragma once

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include <vector>
#include <memory>
#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

void wzGetWindowToRendererScaleFactor(float *horizScaleFactor, float *vertScaleFactor);
float wzGetDisplayContentScale();

// Input state setters shared by the SDL event handlers and other backend input sources
// Positions are in logical screen coordinates - i.e. those returned from mouseX()/mouseY()
void inputSetKey(KEY_CODE code, bool pressed);
void inputSetMouseButton(MOUSE_KEY_CODE mouseKeyCode, bool pressed, Vector2i logicalPos);
void inputSetMousePos(int logicalX, int logicalY);

// Adds an editing key press (enter, escape, arrows, etc) to the text input buffer
void inputAddEditingKey(KEY_CODE code);

// Initializes an SDL subsystem the first time it is requested
bool wzSDLOneTimeInitSubsystem(uint32_t subsystem_flag);

// Moves the system mouse pointer to the given logical screen coordinates
void wzWarpMouseToLogicalPos(int logicalX, int logicalY);

class VideoInitProgress
{
public:
	virtual ~VideoInitProgress();
	virtual void RecordAttemptingBackend(optional<video_backend> backend) = 0;
	virtual void RecordFailedBackend(optional<video_backend> backend, const char* errorMessage) = 0;
	virtual void RecordInitFinished(bool success) = 0;
};

std::unique_ptr<VideoInitProgress> wzResumeFailedVideoInit(optional<video_backend>& backend, std::vector<video_backend>& availableBackends, std::vector<std::string>& backendInitErrors);
