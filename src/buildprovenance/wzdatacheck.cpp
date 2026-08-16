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

#include "wzdatacheck.h"
#include "wzhashablefile.h"
#include "wzmanifest.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/physfs_ext.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <utility>

namespace
{

const char* const CORE_ARCHIVE_NAMES[] = {"base.wz", "mp.wz"};

enum class CheckState
{
	NotStarted,
	Running,
	Done
};

struct ArchiveCheckTask
{
	std::string name;
	HashableFile file;
	std::string expectedSha256;
	uint64_t expectedSize = 0;
};

// NOTE: All state below (except stopRequested and workerTasks while Running) is main thread only
CheckState state = CheckState::NotStarted;
CoreDataCheckResult checkResult;
std::vector<std::function<void(const CoreDataCheckResult&)>> pendingCallbacks;
WZ_THREAD* checkThread = nullptr;
std::atomic<bool> stopRequested(false);
std::vector<ArchiveCheckTask> workerTasks;	// set before thread start, immutable while Running

// NOTE: Main thread only.
void deliverResult(CoreDataCheckResult result)
{
	if (state == CheckState::Done)
	{
		return;
	}
	checkResult = std::move(result);
	state = CheckState::Done;
	if (checkResult.checkPerformed)
	{
		if (checkResult.modifiedArchives.empty())
		{
			debug(LOG_WZ, "Core data archives match the build manifest");
		}
		else
		{
			for (const auto& name : checkResult.modifiedArchives)
			{
				debug(LOG_INFO, "Core data archive is modified (does not match the build manifest): %s", name.c_str());
			}
		}
	}
	else
	{
		debug(LOG_WZ, "Core data check not performed (no validated manifest or no mounted core archives)");
	}
	auto callbacks = std::move(pendingCallbacks);
	pendingCallbacks.clear();
	for (const auto& callback : callbacks)
	{
		callback(checkResult);
	}
}

int dataCheckThreadFunc(void*)
{
	CoreDataCheckResult result;
	result.checkPerformed = true;
	for (const auto& task : workerTasks)
	{
		if (stopRequested.load(std::memory_order_relaxed))
		{
			result.checkPerformed = false;
			break;
		}
		auto measured = task.file.hashWholeFile(&stopRequested);
		if (!measured.has_value())
		{
			continue;
		}
		if (measured.value().hash.toString() != task.expectedSha256
			|| (task.expectedSize != 0 && measured.value().fileSize != task.expectedSize))
		{
			result.modifiedArchives.push_back(task.name);
		}
	}
	wzAsyncExecOnMainThread([result]() {
		deliverResult(result);
	});
	return 0;
}

// NOTE: Main thread only.
optional<std::string> findMountedArchivePath(const char* archiveName)
{
	optional<std::string> result;
	size_t nameLen = strlen(archiveName);
	char** searchPath = PHYSFS_getSearchPath();
	for (char** i = searchPath; *i != nullptr; ++i)
	{
		std::string entry = *i;
		if (entry.size() < nameLen)
		{
			continue;
		}
		if (entry.compare(entry.size() - nameLen, nameLen, archiveName) != 0)
		{
			continue;
		}
		if (entry.size() > nameLen)
		{
			char sep = entry[entry.size() - nameLen - 1];
			if (sep != '/' && sep != '\\')
			{
				continue;
			}
		}
		result = entry;
		break;
	}
	PHYSFS_freeList(searchPath);
	return result;
}

// NOTE: Main thread only.
void startCheck(const optional<nlohmann::json>& manifest)
{
	if (state != CheckState::NotStarted)
	{
		return;
	}
	if (!manifest.has_value() || !manifest.value().contains("artifacts"))
	{
		deliverResult(CoreDataCheckResult());
		return;
	}
	const auto& artifacts = manifest.value().at("artifacts");
	workerTasks.clear();
	for (const char* archiveName : CORE_ARCHIVE_NAMES)
	{
		auto artifactIt = artifacts.find(archiveName);
		if (artifactIt == artifacts.end() || !artifactIt->contains("sha256"))
		{
			continue;
		}
		optional<std::string> mountedPath = findMountedArchivePath(archiveName);
		if (!mountedPath.has_value())
		{
			continue;
		}
		optional<HashableFile> file = HashableFile::mountedDataArchive(mountedPath.value());
		if (!file.has_value())
		{
			continue;
		}
		ArchiveCheckTask task {archiveName, std::move(file.value()),
							   artifactIt->at("sha256").get<std::string>(),
							   artifactIt->value("size", uint64_t(0))};
		workerTasks.push_back(std::move(task));
	}
	if (workerTasks.empty())
	{
		deliverResult(CoreDataCheckResult());
		return;
	}
	state = CheckState::Running;
	checkThread = wzThreadCreate(dataCheckThreadFunc, nullptr, "wzCoreDataCheck");
	if (checkThread == nullptr)
	{
		ASSERT(false, "Failed to create core data check thread");
		deliverResult(CoreDataCheckResult());
		return;
	}
	wzThreadStart(checkThread);
}

} // anonymous namespace

void getCoreDataModifiedCheck(std::function<void(const CoreDataCheckResult&)> resultFunc)
{
	ASSERT_OR_RETURN(, resultFunc != nullptr, "Null resultFunc");
	if (state == CheckState::Done)
	{
		CoreDataCheckResult resultCopy = checkResult;
		wzAsyncExecOnMainThread([resultFunc, resultCopy]() {
			resultFunc(resultCopy);
		});
		return;
	}
	pendingCallbacks.push_back(std::move(resultFunc));
	if (state == CheckState::NotStarted)
	{
		getValidatedManifest([](const optional<nlohmann::json>& manifest) {
			startCheck(manifest);
		});
	}
}

void coreDataCheckShutdown()
{
	stopRequested = true;
	if (checkThread != nullptr)
	{
		wzThreadJoin(checkThread);
		checkThread = nullptr;
	}
	pendingCallbacks.clear();
}
