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
#include <memory>
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

ExeDetails computeExeDetails(const HashableFile& exeFile)
{
	ExeDetails details;
	details.path = exeFile.displayPath();
	details.pathConfidence = exeFile.pathConfidence();
	optional<HashableFile::WholeFileHash> hashResult = exeFile.hashWholeFile(&stopRequested);
	if (!hashResult.has_value())
	{
		details.errorDetails = stopRequested.load() ? "Cancelled by shutdown" : "Failed to read own executable";
		return details;
	}
	details.fileSize = hashResult.value().fileSize;
	details.rawHash = hashResult.value().hash;
#if defined(WZ_OS_MAC)
	auto canonicalSlices = exeFile.machoCanonicalHashes(&stopRequested);
	if (canonicalSlices.has_value())
	{
		details.machoCanonicalSlices = std::move(canonicalSlices.value());
	}
#endif
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

// Delivers on the main thread, asynchronously.
// Safe to call from any thread.
void postResult(ExeDetails details)
{
	wzAsyncExecOnMainThread([details]() {
		deliverResult(details);
	});
}

ExeDetails unavailableDetails(const char* errorDetails)
{
	ExeDetails details;
	details.errorDetails = errorDetails;
	return details;
}

// `data` is a heap-allocated HashableFile, which this thread takes ownership of
int selfExeHashThreadFunc(void* data)
{
	std::unique_ptr<HashableFile> exeFile(static_cast<HashableFile*>(data));
	postResult(computeExeDetails(*exeFile));
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

	optional<HashableFile> exeFile = HashableFile::ownExecutable();
	if (!exeFile.has_value())
	{
		postResult(unavailableDetails("Unable to determine own executable path on this platform"));
		return;
	}

	auto threadExeFile = std::make_unique<HashableFile>(std::move(exeFile.value()));
	hashThread = wzThreadCreate(selfExeHashThreadFunc, threadExeFile.get(), "wzSelfExeHash");
	if (hashThread == nullptr)
	{
		ASSERT(false, "Failed to create self-exe hashing thread");
		postResult(unavailableDetails("Unable to start thread"));
		return;
	}
	(void)threadExeFile.release(); // now owned by the thread
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
