// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file resource_loading_frame_policy.h
 * Shared step/status and per-frame mainLoop policy for cooperative loading.
 */

#pragma once

class ResourceLoadingController;

/// <summary>
/// Outcome of one `ResourceLoadingController::stepOneQuantum()` call, or of a finished session.
/// </summary>
enum class LoadStepStatus
{
	InProgress,
	Completed,
	Failed,
};

/// <summary>
/// Whether an execution-stack frame is waiting for the next quantum or inside `resume()`.
/// </summary>
enum class ExecutionFrameState
{
	Paused,
	Running,
};

/// <summary>
/// Per-frame mainLoop behavior for an active load step. Copied onto each execution-stack
/// entry (root and nested children). Passed to `ResourceLoadingController::request()`.
/// </summary>
struct ResourceLoadingFramePolicy
{
	/// <summary>
	/// How `mainLoop` should finish the current iteration after `step()` for the active load.
	/// `ConsumeFrame` ends the tick after the load step and optional loading UI.
	/// `ContinueMainLoop` allows the same frame to proceed into normal game/title logic.
	/// </summary>
	enum class FrameProcessingMode
	{
		ConsumeFrame,
		ContinueMainLoop,
	};

	FrameProcessingMode frameMode = FrameProcessingMode::ConsumeFrame;
	bool showLoadingScreen = true;
};
