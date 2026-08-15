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

#include "wzbindetails.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"

#include <atomic>
#include <utility>
#include <vector>

namespace
{

enum class ComputeState
{
	NotStarted,
	Running,
	Done
};

// NOTE: All state below (except stopRequested) is accessed from the main thread only
ComputeState state = ComputeState::NotStarted;
ExeDetails computedDetails;
std::vector<std::function<void(const ExeDetails&)>> pendingCallbacks;
WZ_THREAD* hashThread = nullptr;
std::atomic<bool> stopRequested(false);

ExeDetails computeExeDetails()
{
	ExeDetails details;
	optional<HashableFile> exeFile = HashableFile::ownExecutable();
	if (!exeFile.has_value())
	{
		details.errorDetails = "Unable to determine own executable path on this platform";
		return details;
	}
	details.path = exeFile.value().displayPath();
	details.pathConfidence = exeFile.value().pathConfidence();
	optional<HashableFile::WholeFileHash> hashResult = exeFile.value().hashWholeFile(&stopRequested);
	if (!hashResult.has_value())
	{
		details.errorDetails = stopRequested.load() ? "Cancelled by shutdown" : "Failed to read own executable";
		return details;
	}
	details.fileSize = hashResult.value().fileSize;
	details.rawHash = hashResult.value().hash;
	return details;
}

// NOTE: Main thread only.
void deliverResult(ExeDetails details)
{
	if (state == ComputeState::Done)
	{
		return;
	}
	computedDetails = std::move(details);
	state = ComputeState::Done;
	if (computedDetails.rawHash.has_value())
	{
		debug(LOG_WZ, "Self-executable: %s (%" PRIu64 " bytes), sha256: %s",
			  computedDetails.path.c_str(),
			  computedDetails.fileSize.value_or(0),
			  computedDetails.rawHash.value().toString().c_str());
	}
	else
	{
		debug(LOG_WZ, "Self-executable details unavailable: %s", computedDetails.errorDetails.c_str());
	}
	std::vector<std::function<void(const ExeDetails&)>> callbacks = std::move(pendingCallbacks);
	pendingCallbacks.clear();
	for (const auto& callback : callbacks)
	{
		callback(computedDetails);
	}
}

int selfExeHashThreadFunc(void*)
{
	ExeDetails details = computeExeDetails();
	wzAsyncExecOnMainThread([details]() {
		deliverResult(details);
	});
	return 0;
}

} // anonymous namespace

void selfExeDetailsInit()
{
	if (state != ComputeState::NotStarted)
	{
		return;
	}
	state = ComputeState::Running;
	hashThread = wzThreadCreate(selfExeHashThreadFunc, nullptr, "wzSelfExeHash");
	if (hashThread == nullptr)
	{
		ASSERT(false, "Failed to create self-exe hashing thread");
		ExeDetails details;
		details.errorDetails = "Unable to start thread";
		deliverResult(std::move(details));
		return;
	}
	wzThreadStart(hashThread);
}

void selfExeDetailsShutdown()
{
	stopRequested = true;
	if (hashThread != nullptr)
	{
		wzThreadJoin(hashThread);
		hashThread = nullptr;
	}
	pendingCallbacks.clear();
}

void getSelfExecutableDetails(std::function<void(const ExeDetails&)> resultFunc)
{
	ASSERT_OR_RETURN(, resultFunc != nullptr, "Null resultFunc");
	if (state == ComputeState::Done)
	{
		ExeDetails detailsCopy = computedDetails;
		wzAsyncExecOnMainThread([resultFunc, detailsCopy]() {
			resultFunc(detailsCopy);
		});
		return;
	}
	pendingCallbacks.push_back(std::move(resultFunc));
	if (state == ComputeState::NotStarted)
	{
		selfExeDetailsInit();
	}
}
