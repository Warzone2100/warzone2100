/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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

#include "sdl_backend_private.h"

#include "lib/framework/file.h"
#include "lib/framework/physfs_ext.h"

#include <unordered_map>
#include <string>
#include "src/version.h"
#include "src/wzjsonhelpers.h"

#include <SDL_messagebox.h>

// On some older systems with broken graphics drivers, attempting to create a window or initialize a graphics backend
// may crash the WZ process. Unfortunately, these issues are sometimes not readily resolvable by WZ, but we can at least
// record enough information to resume trying any remaining graphics backends once WZ is restarted.

// Record this information in a .json file with the following general format:
//	{
//		"wz_version": "<WZ_VERSION>",
//		"attempting_backend": "opengl",
//		"failed_backends": {
//			"vulkan": "error message",
//		},
//	}

void to_json(nlohmann::json& j, const video_backend& v)
{
	j = to_string(v);
}

void to_json(nlohmann::ordered_json& j, const video_backend& v)
{
	j = to_string(v);
}

void from_json(const nlohmann::json& j, video_backend& v)
{
	auto str = j.get<std::string>();
	if (video_backend_from_str(str.c_str(), v))
	{
		// success
		return;
	}
	throw nlohmann::json::type_error::create(302, "video_backend value is unknown: \"" + str + "\"", &j);
}

VideoInitProgress::~VideoInitProgress()
{ }

class VideoInitProgressImpl : public VideoInitProgress
{
public:
	VideoInitProgressImpl(const std::string& filePath);
	virtual ~VideoInitProgressImpl() { }
	virtual void RecordAttemptingBackend(optional<video_backend> backend) override;
	virtual void RecordFailedBackend(optional<video_backend> backend, const char* errorMessage) override;
	virtual void RecordInitFinished(bool success) override;
private:
	bool tryLoadFromFile();
	void persistChanges();
	std::string currentWZVersionString();
public:
	std::string wz_version;
	optional<video_backend> attempting_backend = nullopt;
	typedef std::unordered_map<video_backend, std::string> FailedBackendsMap;
	FailedBackendsMap failed_backends;
	std::string filePath;
};

nlohmann::ordered_json failedbackends_to_json(const VideoInitProgressImpl::FailedBackendsMap& v)
{
	auto j = nlohmann::ordered_json::object();
	for (auto it : v)
	{
		j[to_string(it.first)] = it.second;
	}
	return j;
}

void from_json(const nlohmann::json& j, VideoInitProgressImpl::FailedBackendsMap& v)
{
	v.clear();
	for (auto it : j.items())
	{
		video_backend backendKey;
		if (video_backend_from_str(it.key().c_str(), backendKey))
		{
			v.insert(VideoInitProgressImpl::FailedBackendsMap::value_type(backendKey, it.value().get<std::string>()));
		}
		else
		{
			throw nlohmann::json::type_error::create(302, "video_backend value is unknown: \"" + it.key() + "\"", &j);
		}
	}
}

VideoInitProgressImpl::VideoInitProgressImpl(const std::string& filePath)
: filePath(filePath)
{
	if (!tryLoadFromFile())
	{
		wz_version = currentWZVersionString();
	}
}

void VideoInitProgressImpl::RecordAttemptingBackend(optional<video_backend> backend)
{
	attempting_backend = backend;
	persistChanges();
}

void VideoInitProgressImpl::RecordFailedBackend(optional<video_backend> backend, const char* errorMessage)
{
	attempting_backend.reset();
	if (backend.has_value())
	{
		failed_backends.insert(FailedBackendsMap::value_type(backend.value(), (errorMessage != nullptr) ? errorMessage : ""));
	}
	persistChanges();
}

void VideoInitProgressImpl::RecordInitFinished(bool success)
{
	attempting_backend.reset();
	PHYSFS_delete(filePath.c_str());
}

std::string VideoInitProgressImpl::currentWZVersionString()
{
	auto pVersionStr = version_getVersionString();
	if (pVersionStr == nullptr)
	{
		return std::string();
	}
	return std::string(pVersionStr);
}

bool VideoInitProgressImpl::tryLoadFromFile()
{
	auto optJson = wzLoadJsonObjectFromFile(filePath, true);
	if (!optJson.has_value())
	{
		return false;
	}

	auto& objRoot = optJson.value();
	auto it = objRoot.find("wz_version");
	if (it == objRoot.end())
	{
		return false;
	}

	// check recorded wz_version matches this WZ's version string
	std::string recorded_wz_version;
	try {
		recorded_wz_version = it.value().get<std::string>();
	}
	catch (const std::exception &e) {
		debug(LOG_INFO, "Failed to load wz_version value: %s", e.what());
	}
	if (recorded_wz_version != currentWZVersionString())
	{
		debug(LOG_INFO, "Skipping resume of video init because of different version (%s)", recorded_wz_version.c_str());
		return false;
	}

	it = objRoot.find("attempting_backend");
	if (it != objRoot.end())
	{
		try {
			attempting_backend = it.value().get<video_backend>();
		}
		catch (const std::exception &e) {
			debug(LOG_INFO, "Invalid attempting_backend value: %s", e.what());
		}
	}

	it = objRoot.find("failed_backends");
	if (it != objRoot.end())
	{
		try {
			failed_backends = it.value().get<FailedBackendsMap>();
		}
		catch (const std::exception &e) {
			debug(LOG_INFO, "Failed to load failed_backends value: %s", e.what());
		}
	}

	return false;
}

void VideoInitProgressImpl::persistChanges()
{
	auto objRoot = nlohmann::ordered_json::object();
	objRoot["wz_version"] = currentWZVersionString();
	if (attempting_backend.has_value())
	{
		objRoot["attempting_backend"] = to_string(attempting_backend.value());
	}
	objRoot["failed_backends"] = failedbackends_to_json(failed_backends);

	auto outputStr = objRoot.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
	PHYSFS_uint32 size = static_cast<PHYSFS_uint32>(outputStr.size());
	PHYSFS_file *fileHandle = PHYSFS_openWrite(filePath.c_str());
	if (fileHandle)
	{
		if (WZ_PHYSFS_writeBytes(fileHandle, outputStr.data(), size) != size)
		{
			// Failed to write data to file
			debug(LOG_INFO, "Failed to write video init progress file: %s", filePath.c_str());
		}
		PHYSFS_close(fileHandle);
	}
}

void resumeAfterCrashInitializingGraphicsBackendMessage_internal(video_backend backend)
{
	const SDL_MessageBoxButtonData buttons[] = {
	   { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, _("OK") }
	};
	std::string titleString = std::string(_("Warzone 2100: Prior launch failed"));
	std::string messageString = std::string(_("The prior attempt to initialize the following graphics backend caused a crash:")) + "\n";
	messageString += astringf(_("Graphics Backend: %s"), to_string(backend).c_str()) + "\n\n";
	messageString += _("Warzone 2100 will now attempt to launch using a different graphics backend.");
	const SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_ERROR, /* .flags */
		NULL, /* .window */
		titleString.c_str(), /* .title */
		messageString.c_str(), /* .message */
		SDL_arraysize(buttons), /* .numbuttons */
		buttons, /* .buttons */
		nullptr /* .colorScheme */
	};
	int buttonid;
	if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0) {
		// error displaying message box
		debug(LOG_FATAL, "Failed to display message box");
	}
}

std::unique_ptr<VideoInitProgress> wzResumeFailedVideoInit(optional<video_backend>& backend, std::vector<video_backend>& availableBackends, std::vector<std::string>& backendInitErrors)
{
	auto progress = std::make_unique<VideoInitProgressImpl>("videoInitProgress.json");

	// If attempting_backend is set, the WZ process presumably crashed while trying to initialize it - add it to failed_backends
	optional<video_backend> lastCrashedBackend = nullopt;
	if (progress->attempting_backend.has_value())
	{
		lastCrashedBackend = progress->attempting_backend.value();
		debug(LOG_INFO, "Last attempt to initialize %s backend failed, crashed the process", to_string(lastCrashedBackend.value()).c_str());
		progress->RecordFailedBackend(lastCrashedBackend, "Crashed the process");
	}

	// Erase any failed backends from the availableBackends list
	for (auto it : progress->failed_backends)
	{
		debug(LOG_INFO, "Failed backend: %s, error: %s", to_string(it.first).c_str(), it.second.c_str());
		availableBackends.erase(std::remove_if(availableBackends.begin(), availableBackends.end(), [&it](video_backend a) { return a == it.first; }), availableBackends.end());

		// Also restore the backendInitError (if present)
		if (!it.second.empty())
		{
			backendInitErrors.push_back(astringf("[%s]: %s", to_display_string(it.first).c_str(), it.second.c_str()));
		}
	}

	if (backend.has_value())
	{
		// If backend matches a failed backend, try the next available (if possible)
		if (progress->failed_backends.count(backend.value()) > 0)
		{
			if (availableBackends.empty())
			{
				// No more backends to try :(
				debug(LOG_INFO, "Resuming failed video init, no remaining backends to try");
				backend = nullopt;
			}
			else
			{
				// Try with a new backend (first in the current list)
				backend = availableBackends.front();
				debug(LOG_INFO, "Resuming failed video init, with backend: %s", to_string(backend.value()).c_str());
			}
		}
	}

	// NOTE: Would ideally do this on all OSes, but SDL_ShowMessageBox may crash on Linux / X11 in some rare circumstances (supposedly fixed in SDL3?)
	// For now, limit to specific OS builds, as old Windows graphics drivers are the more common cause of this condition anyway
#if defined(_WIN32) || defined(__APPLE__)
	if (lastCrashedBackend.has_value() && backend.has_value())
	{
		// Message that prior attempt crashed, but WZ will resume trying additional backends
		resumeAfterCrashInitializingGraphicsBackendMessage_internal(lastCrashedBackend.value());
	}
#endif

	return progress;
}
