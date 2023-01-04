/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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

#include <nlohmann/json.hpp> // Must come before WZ includes
using json = nlohmann::json;

#include "version.h"
#include "updatemanager.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "lib/framework/wzglobal.h" // required for config.h
#include "lib/framework/frame.h"
#include "lib/framework/file.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/wzstring.h"
#include "urlhelpers.h"
#include "urlrequest.h"
#include "notifications.h"
#include <EmbeddedJSONSignature.h>
#include "3rdparty/propertymatcher.h"
#include "wzpropertyproviders.h"

#include <sodium.h>
#include <re2/re2.h>

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && ((4 < __GNUC__) || ((4 == __GNUC__) && (7 <= __GNUC_MINOR__)))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmaybe-uninitialized" // Ignore on GCC 4.7+
#endif
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (12 <= __GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wstringop-overflow" // Ignore on GCC 12+`
#endif
#define ONLY_C_LOCALE 1
#include <date/date.h>
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (12 <= __GNUC__)
# pragma GCC diagnostic pop
#endif
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && ((4 < __GNUC__) || ((4 == __GNUC__) && (7 <= __GNUC_MINOR__)))
# pragma GCC diagnostic pop
#endif

enum class ProcessResult {
	INVALID_JSON,
	NO_MATCHING_CHANNEL,
	MATCHED_CHANNEL_NO_UPDATE,
	UPDATE_FOUND
};

typedef std::function<ProcessResult (const json& updateData, bool validSignature, bool validExpiry)> ProcessJSONDataFileFunc;

struct CachePaths {
	const char* cache_data_path;
	const char* cache_info_path;
};

static std::string configureLinkURL(const std::string& url, BuildPropertyProvider& propProvider);
static bool isValidExpiry(const json& updateData);
static void initProcessData(const std::vector<std::string> &updateDataUrls, ProcessJSONDataFileFunc processDataFunc, CachePaths outputPaths);
static void fetchLatestData(const std::vector<std::string> &updateDataUrls, ProcessJSONDataFileFunc processDataFunc, CachePaths outputPaths);

class WzUpdateManager {
public:
	static void initUpdateCheck();
private:
	static ProcessResult processUpdateJSONFile(const json& updateData, bool validSignature, bool validExpiry);
};

class WzCompatCheckManager {
public:
	static void initCompatCheck();
private:
	static ProcessResult processCompatCheckJSONFile(const json& updateData, bool validSignature, bool validExpiry);
};

const std::string WZ_UPDATES_VERIFY_KEY = "5d9P+Z1SirsWSsYICZAr7QFlPB01s6tzXkhPZ+X/FQ4=";
const std::string WZ_DEFAULT_UPDATE_LINK = "https://warzone2100.github.io/update-data/redirect/updatelink.html";
const std::string WZ_DEFAULT_COMPATINFO_LINK = "https://warzone2100.github.io/update-data/redirect/compatinfolink.html";
#define WZ_UPDATES_CACHE_DIR "cache"
#define WZ_CACHE_INFO_JSON_WZVERSION_KEY "wz_version"
#define WZ_UPDATES_JSON_MAX_SIZE (1 << 25)

static const char updatesCacheDataPath[] = WZ_UPDATES_CACHE_DIR "/wz2100_updates.json";
static const char cacheInfoPath[] = WZ_UPDATES_CACHE_DIR "/cache_info.json";
static const char compatDataPath[] = WZ_UPDATES_CACHE_DIR "/wz2100_compat.json";
static CachePaths updatesCachePaths = CachePaths{updatesCacheDataPath, cacheInfoPath};
static CachePaths compatCachePaths = CachePaths{compatDataPath, nullptr};

template<class Duration>
date::sys_time<Duration> parse_ISO_8601(const std::string& timeStr)
{
	std::istringstream inputStream(timeStr);
	date::sys_time<Duration> timepoint;
	inputStream >> date::parse("%FT%TZ", timepoint);
	if (inputStream.fail())
	{
		inputStream.clear();
		inputStream.str(timeStr);
		inputStream >> date::parse("%FT%T%Ez", timepoint);
		if (inputStream.fail())
		{
			throw std::runtime_error("Failed to parse time string");
		}
	}
	return timepoint;
}

// Replaces specific build property keys with their values in a URL string
// Build property keys are surrounded by "{{}}" - i.e. "{{PLATFORM}}" is replaced with the value of the PLATFORM build property
// May be called from a background thread
static std::string configureLinkURL(const std::string& url, BuildPropertyProvider& propProvider)
{
	const std::unordered_set<std::string> permittedBuildPropertySubstitutions = { "PLATFORM", "VERSION_STRING", "GIT_BRANCH" };

	std::vector<std::string> tokens;
	re2::StringPiece input(url);
	std::string tmpToken;

	RE2 re("({{[\\S]+}})");
	while (RE2::FindAndConsume(&input, re, &tmpToken))
	{
		tokens.push_back(tmpToken);
	}

	std::string resultUrl = url;
	for (const auto& token : tokens)
	{
		std::string::size_type pos = url.find(token, 0);
		if (pos != std::string::npos)
		{
			std::string propValue;
			std::string buildProperty = token.substr(2, token.length() - 4);
			if (permittedBuildPropertySubstitutions.count(buildProperty) > 0)
			{
				propProvider.getPropertyValue(buildProperty, propValue);
			}
			else
			{
				propValue = "prop_not_supported";
			}
			if (!propValue.empty())
			{
				propValue = urlEncode(propValue.c_str());
			}
			resultUrl.replace(pos, token.length(), propValue);
		}
	}

	return resultUrl;
}

// May be called from a background thread
static bool isValidExpiry(const json& updateData)
{
	if (!updateData.is_object())
	{
		wzAsyncExecOnMainThread([]{ debug(LOG_WARNING, "Update data is not an object"); });
		return false;
	}
	if (!updateData.contains("validThru")) { return false; }
	const auto& validThru = updateData["validThru"];
	if (!validThru.is_string()) { return false; }
	std::string validThruStr = validThru.get<std::string>();
	try {
		const auto validThruTimePoint = parse_ISO_8601<std::chrono::system_clock::duration>(validThruStr);
		if (validThruTimePoint >= std::chrono::system_clock::now())
		{
			return true;
		}
		return false;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

static void applyBaseNotificationInfo(WZ_Notification& notification, const json& notificationInfo, size_t maxMinShown = 10)
{
	if (!notificationInfo.is_object())
	{
		return;
	}

	try
	{
		json notificationBase;
		json notificationId;
		json minTimesShown;
		if (notificationInfo.contains("base"))
		{
			notificationBase = notificationInfo["base"];
		}
		if (notificationInfo.contains("id"))
		{
			notificationId = notificationInfo["id"];
		}
		if (notificationInfo.contains("minShown"))
		{
			minTimesShown = notificationInfo["minShown"];
		}

		if (notificationBase.is_string() && notificationId.is_string())
		{
			const std::string notificationIdentifierPrefix = notificationBase.get<std::string>() + "::";
			const std::string notificationIdentifier = notificationIdentifierPrefix + notificationId.get<std::string>();
			removeNotificationPreferencesIf([&notificationIdentifierPrefix, &notificationIdentifier](const std::string &uniqueNotificationIdentifier) -> bool {
				bool hasPrefix = (strncmp(uniqueNotificationIdentifier.c_str(), notificationIdentifierPrefix.c_str(), notificationIdentifierPrefix.size()) == 0);
				return hasPrefix && (notificationIdentifier != uniqueNotificationIdentifier);
			});
			uint8_t minTimesShownValue = 3;
			if (minTimesShown.is_number_integer())
			{
				auto intValue = minTimesShown.get<json::number_integer_t>();
				if (intValue >= 0)
				{
					minTimesShownValue = static_cast<uint8_t>(std::min<json::number_integer_t>(intValue, maxMinShown));
				}
			}
			notification.displayOptions = WZ_Notification_Display_Options::makeIgnorable(notificationIdentifier, minTimesShownValue);
		}
	}
	catch (const std::exception&)
	{
		// Parsing notificationInfo failed
		// no-op - just ignore
	}

	try
	{
		if (notificationInfo.contains("modal"))
		{
			notification.isModal = notificationInfo["modal"].get<bool>();
		}
	}
	catch (const std::exception&)
	{
		// Parsing notificationInfo "modal" failed
		// no-op - just ignore
	}
}

// May be called from a background thread
ProcessResult WzUpdateManager::processUpdateJSONFile(const json& updateData, bool validSignature, bool validExpiry)
{
	if (!updateData.is_object())
	{
		wzAsyncExecOnMainThread([]{ debug(LOG_WARNING, "Update data is not an object"); });
		return ProcessResult::INVALID_JSON;
	}
	const auto& channels = updateData["channels"];
	if (!channels.is_array())
	{
		wzAsyncExecOnMainThread([]{ debug(LOG_WARNING, "Channels should be an array"); });
		return ProcessResult::INVALID_JSON;
	}
	auto buildPropProvider = std::make_shared<BuildPropertyProvider>();
	CombinedPropertyProvider propProvider({buildPropProvider, std::make_shared<EnvironmentPropertyProvider>()});
	for (const auto& channel : channels)
	{
		try
		{
			if (!channel.is_object()) continue;
			const auto& channelName = channel.at("channel");
			if (!channelName.is_string()) continue;
			std::string channelNameStr = channelName.get<std::string>();
			const auto& channelConditional = channel.at("channelConditional");
			if (!channelConditional.is_string()) continue;
			if (!PropertyMatcher::evaluateConditionString(channelConditional.get<std::string>(), propProvider))
			{
				// non-matching channel conditional
				continue;
			}
			const auto& releases = channel.at("releases");
			if (!releases.is_array()) continue;
			for (const auto& release : releases)
			{
				try
				{
					const auto& buildPropertyMatch = release.at("buildPropertyMatch");
					if (!buildPropertyMatch.is_string()) continue;
					if (!PropertyMatcher::evaluateConditionString(buildPropertyMatch.get<std::string>(), propProvider))
					{
						// non-matching release buildPropertyMatch
						continue;
					}
					// it matches!
					// verify the release has the required properties
					const auto& releaseVersion = release.at("version");
					if (!releaseVersion.is_string())
					{
						// release version is not a string
						continue;
					}
					std::string releaseVersionStr = releaseVersion.get<std::string>();
					json notificationInfo;
					bool importantUpdate = false;
					if (release.contains("notification"))
					{
						notificationInfo = release["notification"];
					}
					if (notificationInfo.is_object())
					{
						if (notificationInfo.contains("important"))
						{
							const auto& importantVal = notificationInfo["important"];
							if (importantVal.is_boolean())
							{
								importantUpdate = notificationInfo["important"].get<bool>();
							}
						}
					}
					else
					{
						// TODO: Handle lack of notification info?
					}
					std::string updateLink;
					if (release.contains("updateLink"))
					{
						const auto& updateLinkJson = release["updateLink"];
						if (updateLinkJson.is_string())
						{
							updateLink = updateLinkJson.get<std::string>();
						}
					}
					bool hasValidURLPrefix = urlHasAcceptableProtocol(updateLink.c_str());
					if (!validSignature || !validExpiry || updateLink.empty() || !hasValidURLPrefix)
					{
						// use default update link
						updateLink = WZ_DEFAULT_UPDATE_LINK;
					}
					updateLink = configureLinkURL(updateLink, (*buildPropProvider.get()));
					// submit notification (on main thread)
					wzAsyncExecOnMainThread([validSignature, channelNameStr, releaseVersionStr, notificationInfo, updateLink, importantUpdate]{
						debug(LOG_INFO, "Found an available update (%s) in channel (%s)", releaseVersionStr.c_str(), channelNameStr.c_str());
						WZ_Notification notification;
						notification.duration = 0;
						notification.contentTitle = _("Update Available");
						if (validSignature)
						{
							notification.contentText = astringf(_("A new build of Warzone 2100 (%s) is available!"), releaseVersionStr.c_str());
							if (importantUpdate)
							{
								notification.contentText += "\n\n";
								notification.contentText += _("This new version includes important bug fixes and updates, and it is recommended that you update now.");
							}
						}
						else
						{
							notification.contentText = _("A new build of Warzone 2100 is available!");
						}
						notification.action = WZ_Notification_Action(_("Get Update Now"), [updateLink](const WZ_Notification&){
							// Open the updateLink url
							wzAsyncExecOnMainThread([updateLink]{
								if (!openURLInBrowser(updateLink.c_str()))
								{
									debug(LOG_ERROR, "Failed to open url in browser: \"%s\"", updateLink.c_str());
								}
							});
						});
						notification.largeIcon = WZ_Notification_Image("images/warzone2100.png");
						if (notificationInfo.is_object())
						{
							applyBaseNotificationInfo(notification, notificationInfo, 10);
						}
						addNotification(notification, WZ_Notification_Trigger::Immediate());
					});
					return ProcessResult::UPDATE_FOUND;
				}
				catch (const std::exception&)
				{
					// Parsing release failed - skip to next
					continue;
				}
			}
			return ProcessResult::MATCHED_CHANNEL_NO_UPDATE;
		}
		catch (const std::exception&)
		{
			// Parsing channel failed - skip to next
			continue;
		}
	}
	return ProcessResult::NO_MATCHING_CHANNEL;
}

// May be called from a background thread
void WzUpdateManager::initUpdateCheck()
{
	std::vector<std::string> updateDataUrls = {"https://data.wz2100.net/wz2100.json", "https://warzone2100.github.io/update-data/wz2100.json"};
	initProcessData(updateDataUrls, WzUpdateManager::processUpdateJSONFile, updatesCachePaths);
}

// May be called from a background thread
ProcessResult WzCompatCheckManager::processCompatCheckJSONFile(const json& updateData, bool validSignature, bool validExpiry)
{
	if (!updateData.is_object())
	{
		wzAsyncExecOnMainThread([]{ debug(LOG_WARNING, "Update data is not an object"); });
		return ProcessResult::INVALID_JSON;
	}
	const auto& channels = updateData["channels"];
	if (!channels.is_array())
	{
		wzAsyncExecOnMainThread([]{ debug(LOG_WARNING, "Channels should be an array"); });
		return ProcessResult::INVALID_JSON;
	}
	auto buildPropProvider = std::make_shared<BuildPropertyProvider>();
	CombinedPropertyProvider propProvider({buildPropProvider, std::make_shared<EnvironmentPropertyProvider>()});
	for (const auto& channel : channels)
	{
		try
		{
			if (!channel.is_object()) continue;
			const auto& channelName = channel.at("channel");
			if (!channelName.is_string()) continue;
			std::string channelNameStr = channelName.get<std::string>();
			const auto& channelConditional = channel.at("channelConditional");
			if (!channelConditional.is_string()) continue;
			if (!PropertyMatcher::evaluateConditionString(channelConditional.get<std::string>(), propProvider))
			{
				// non-matching channel conditional
				continue;
			}
			const auto& compatNotices = channel.at("compatNotices");
			if (!compatNotices.is_array()) continue;
			for (const auto& compatNotice : compatNotices)
			{
				try
				{
					const auto& propertyMatch = compatNotice.at("propertyMatch");
					if (!propertyMatch.is_string()) continue;
					if (!PropertyMatcher::evaluateConditionString(propertyMatch.get<std::string>(), propProvider))
					{
						// non-matching compatNotice propertyMatch
						continue;
					}
					// it matches - current system matches a compatibility notice
					// verify the compatNotice has the required properties
					const auto& compatNoticeId = compatNotice.at("id");
					if (!compatNoticeId.is_string())
					{
						// compatNoticeId is not a string
						continue;
					}
					std::string compatNoticeIdStr = compatNoticeId.get<std::string>();
					json notificationInfo;
					if (compatNotice.contains("notification"))
					{
						notificationInfo = compatNotice["notification"];
					}
					if (!notificationInfo.is_object())
					{
						// TODO: Handle lack of notification info?
					}
					std::string infoLink;
					if (compatNotice.contains("infoLink"))
					{
						const auto& updateLinkJson = compatNotice["infoLink"];
						if (updateLinkJson.is_string())
						{
							infoLink = updateLinkJson.get<std::string>();
						}
					}
					bool hasValidURLPrefix = urlHasAcceptableProtocol(infoLink.c_str());
					if (!validSignature || !validExpiry || infoLink.empty() || !hasValidURLPrefix)
					{
						// use default compat info link
						infoLink = WZ_DEFAULT_COMPATINFO_LINK;
					}
					infoLink = configureLinkURL(infoLink, (*buildPropProvider.get()));
					// submit notification (on main thread)
					wzAsyncExecOnMainThread([validSignature, channelNameStr, compatNoticeIdStr, notificationInfo, infoLink]{
						debug(LOG_WZ, "Found a matching compatibility notice (%s) in channel (%s)", compatNoticeIdStr.c_str(), channelNameStr.c_str());
						WZ_Notification notification;
						notification.duration = 0;
						notification.contentTitle = _("Compatibility Warning");
						notification.contentText = _("An issue has been detected that may affect Warzone 2100's operation / performance.");
						notification.contentText += "\n\n";
						notification.contentText += _("Please click the button below for more information on how to fix it.");
						if (validSignature)
						{
							notification.contentText += "\n\n";
							notification.contentText += astringf(_("(Notice ID: %s)"), compatNoticeIdStr.c_str());
						}
						notification.action = WZ_Notification_Action(_("More Information"), [infoLink](const WZ_Notification&){
							// Open the infoLink url
							wzAsyncExecOnMainThread([infoLink]{
								if (!openURLInBrowser(infoLink.c_str()))
								{
									debug(LOG_ERROR, "Failed to open url in browser: \"%s\"", infoLink.c_str());
								}
							});
						});
						notification.largeIcon = WZ_Notification_Image("images/notifications/exclamation_triangle.png");
						if (notificationInfo.is_object())
						{
							applyBaseNotificationInfo(notification, notificationInfo, 10);
						}
						addNotification(notification, WZ_Notification_Trigger::Immediate());
					});
					return ProcessResult::UPDATE_FOUND;
				}
				catch (const std::exception&)
				{
					// Parsing compatNotice failed - skip to next
					continue;
				}
			}
			return ProcessResult::MATCHED_CHANNEL_NO_UPDATE;
		}
		catch (const std::exception&)
		{
			// Parsing channel failed - skip to next
			continue;
		}
	}
	return ProcessResult::NO_MATCHING_CHANNEL;
}

// May be called from a background thread
void WzCompatCheckManager::initCompatCheck()
{
	std::vector<std::string> updateDataUrls = {"https://data.wz2100.net/wz2100_compat.json", "https://warzone2100.github.io/update-data/wz2100_compat.json"};
	initProcessData(updateDataUrls, WzCompatCheckManager::processCompatCheckJSONFile, compatCachePaths);
}

template<typename T>
static json loadDataJsonObject(T&& updateJsonStr)
{
	json updateData;
	try {
		updateData = json::parse(std::forward<T>(updateJsonStr));
	}
	catch (const std::exception &e) {
		std::string errorStr = e.what();
		std::ostringstream errMsg;
		errMsg << "JSON document parsing failed: " << errorStr.c_str();
		throw std::runtime_error(errMsg.str());
	}
	catch (...) {
		throw std::runtime_error("Unexpected exception parsing JSON");
	}
	if (updateData.is_null())
	{
		throw std::runtime_error("JSON document is null");
	}
	if (!updateData.is_object())
	{
		throw std::runtime_error("JSON document is not an object");
	}
	return updateData;
}

static bool cacheInfoIsUsable(CachePaths& paths)
{
	if (!paths.cache_info_path)
	{
		return true;
	}

	// Check if cache was written by the same version of WZ - if not, ignore it
	if (PHYSFS_exists(paths.cache_info_path))
	{
		try {
			// Open the file + read the data
			PHYSFS_file *fileHandle = PHYSFS_openRead(paths.cache_info_path);
			PHYSFS_sint64 filesize = PHYSFS_fileLength(fileHandle);
			if (filesize <= 0)
			{
				// Invalid file size
				throw std::runtime_error("Invalid filesize");
			}
			std::vector<char> fileData(static_cast<size_t>(filesize + 1), '\0');
			if (WZ_PHYSFS_readBytes(fileHandle, fileData.data(), static_cast<PHYSFS_uint32>(filesize)) != filesize)
			{
				// Read failed
				throw std::runtime_error("Read failed");
			}
			PHYSFS_close(fileHandle);

			// Parse the json
			json updateData = loadDataJsonObject(fileData.data());

			// Retrieve the version of WZ used to write the cache
			const auto& wz_version = updateData.at(WZ_CACHE_INFO_JSON_WZVERSION_KEY);
			if (!wz_version.is_string())
			{
				return false;
			}
			if (wz_version.get<std::string>() != version_getBuildIdentifierReleaseString())
			{
				return false;
			}

			// passed all checks
			return true;
		}
		catch (const std::exception &e) {
			std::string errorStr = e.what();
			wzAsyncExecOnMainThread([errorStr]{
				debug(LOG_WZ, "Cache info file: %s", errorStr.c_str());
			});
		}
	}
	return false;
}

// May be called from a background thread
static void initProcessData(const std::vector<std::string> &updateDataUrls, ProcessJSONDataFileFunc processDataFunc, CachePaths outputPaths)
{
	if (PHYSFS_exists(outputPaths.cache_data_path) && cacheInfoIsUsable(outputPaths))
	{
		try {
			// Open the file + read the data
			PHYSFS_file *fileHandle = PHYSFS_openRead(outputPaths.cache_data_path);
			PHYSFS_sint64 filesize = PHYSFS_fileLength(fileHandle);
			if (filesize < 0 || filesize > WZ_UPDATES_JSON_MAX_SIZE)
			{
				// Invalid file size
				throw std::runtime_error("Invalid filesize");
			}
			std::vector<char> fileData(static_cast<size_t>(filesize + 1), '\0');
			if (WZ_PHYSFS_readBytes(fileHandle, fileData.data(), static_cast<PHYSFS_uint32>(filesize)) != filesize)
			{
				// Read failed
				throw std::runtime_error("Read failed");
			}
			PHYSFS_close(fileHandle);

			// Extract the digital signature, and verify it
			std::string updateJsonStr;
			bool validSignature = EmbeddedJSONSignature::verifySignedJson(fileData.data(), fileData.size() - 1, WZ_UPDATES_VERIFY_KEY, updateJsonStr);

			if (!validSignature)
			{
				throw std::runtime_error("Invalid file");
			}

			// Parse the remaining json (minus the digital signature)
			json updateData = loadDataJsonObject(updateJsonStr);

			// Determine if the JSON is still valid
			bool validExpiry = isValidExpiry(updateData);
			if (!validExpiry)
			{
				// updateData is outdated
				throw std::runtime_error("Outdated");
			}

			// Process the updates JSON, notify if new version is available
			if (!processDataFunc)
			{
				throw std::runtime_error("Missing processDataFunc");
			}
			const auto processResult = processDataFunc(updateData, validSignature, validExpiry);
			if (processResult == ProcessResult::INVALID_JSON)
			{
				throw std::runtime_error("Invalid JSON");
			}

			// handled with cached data
			return;
		}
		catch (const std::exception &e) {
			std::string errorStr = e.what();
			wzAsyncExecOnMainThread([errorStr]{
				debug(LOG_WZ, "Cached updates file: %s", errorStr.c_str());
			});
			// continue on to fetch a fresh copy
		}
	}

	// Fall-back to URL request for the latest data
	fetchLatestData(updateDataUrls, processDataFunc, outputPaths);
}

// May be called from a background thread
static void fetchLatestData(const std::vector<std::string> &updateDataUrls, ProcessJSONDataFileFunc processDataFunc, CachePaths outputPaths)
{
	if (updateDataUrls.empty())
	{
		// No urls to check
		wzAsyncExecOnMainThread([]{
			debug(LOG_WARNING, "No more URLs to fetch - failed update check");
		});
		return;
	}

	URLDataRequest* pRequest = new URLDataRequest();
	pRequest->url = updateDataUrls.front();
	std::vector<std::string> additionalUrls(updateDataUrls.begin() + 1, updateDataUrls.end());
	pRequest->onSuccess = [additionalUrls, processDataFunc, outputPaths](const std::string& url, const HTTPResponseDetails& responseDetails, const std::shared_ptr<MemoryStruct>& data) {

		std::string urlCopy = url;
		long httpStatusCode = responseDetails.httpStatusCode();
		if (httpStatusCode != 200)
		{
			wzAsyncExecOnMainThread([httpStatusCode]{
				debug(LOG_WARNING, "Update check returned HTTP status code: %ld", httpStatusCode);
			});
			fetchLatestData(additionalUrls, processDataFunc, outputPaths);
			return;
		}

		// Extract the digital signature, and verify it
		std::string updateJsonStr;
		bool validSignature = false;
		try {
			validSignature = EmbeddedJSONSignature::verifySignedJson(data->memory, data->size, WZ_UPDATES_VERIFY_KEY, updateJsonStr);
		}
		catch (const std::exception& e) {
			std::string errorStr = e.what();
			wzAsyncExecOnMainThread([urlCopy, errorStr]{
				debug(LOG_INFO, "Failed to verify signature: %s; %s", errorStr.c_str(), urlCopy.c_str());
			});
			fetchLatestData(additionalUrls, processDataFunc, outputPaths);
			return;
		}

		// Parse the remaining json (minus the digital signature)
		json updateData;
		try {
			updateData = loadDataJsonObject(updateJsonStr);
		}
		catch (const std::exception &e) {
			std::string errorStr = e.what();
			wzAsyncExecOnMainThread([urlCopy, errorStr]{
				debug(LOG_INFO, "Failed to parse JSON: %s; %s", errorStr.c_str(), urlCopy.c_str());
			});
			fetchLatestData(additionalUrls, processDataFunc, outputPaths);
			return;
		}

		// Determine if the JSON is still valid (note: requires accurate system clock)
		bool validExpiry = isValidExpiry(updateData);

		if ((!validSignature || !validExpiry) && !additionalUrls.empty())
		{
			// signature is invalid, or data is expired, and there are further urls to try to fetch
			// instead of proceeding, try the next url
			fetchLatestData(additionalUrls, processDataFunc, outputPaths);
			return;
		}

		// Otherwise,
		// Do not immediately fail if the validThru date doesn't check out - pass this status to processUpdateJSONFile
		// so it can decide how to trust the parts of the data

		// Process the updates JSON, notify if new version is available
		if (!processDataFunc)
		{
			wzAsyncExecOnMainThread([]{
				debug(LOG_ERROR, "Missing processDataFunc");
			});
			return;
		}
		const auto processResult = processDataFunc(updateData, validSignature, validExpiry);

		if (validSignature && (processResult != ProcessResult::INVALID_JSON) && isValidExpiry(updateData))
		{
			// Cache the data JSON on disk
			if (!WZ_PHYSFS_isDirectory(WZ_UPDATES_CACHE_DIR))
			{
				// Cache dir should have already been created?
				return;
			}
			if (outputPaths.cache_data_path)
			{
				PHYSFS_uint32 size = data->size;
				PHYSFS_file *fileHandle = PHYSFS_openWrite(outputPaths.cache_data_path);
				if (fileHandle)
				{
					if (WZ_PHYSFS_writeBytes(fileHandle, data->memory, size) != size)
					{
						// Failed to write data to file
						std::string pathStr = outputPaths.cache_data_path;
						wzAsyncExecOnMainThread([pathStr]{
							debug(LOG_ERROR, "Failed to write cache file: %s", pathStr.c_str());
						});
					}
					PHYSFS_close(fileHandle);
				}
			}
			if (outputPaths.cache_info_path)
			{
				// Also write out the cache info file (which contains the version of WZ used to write the cache)
				json cacheInfoObj = json::object();
				cacheInfoObj[WZ_CACHE_INFO_JSON_WZVERSION_KEY] = version_getBuildIdentifierReleaseString();
				std::string cacheInfoData = cacheInfoObj.dump(2, ' ', true, json::error_handler_t::replace);
				PHYSFS_uint32 size = static_cast<PHYSFS_uint32>(cacheInfoData.size());
				PHYSFS_file *fileHandle = PHYSFS_openWrite(outputPaths.cache_info_path);
				if (fileHandle)
				{
					if (WZ_PHYSFS_writeBytes(fileHandle, cacheInfoData.data(), size) != size)
					{
						// Failed to write data to file
						std::string pathStr = outputPaths.cache_info_path;
						wzAsyncExecOnMainThread([pathStr]{
							debug(LOG_ERROR, "Failed to write cache info file: %s", pathStr.c_str());
						});
					}
					PHYSFS_close(fileHandle);
				}
			}
		}
	};
	pRequest->onFailure = [additionalUrls, processDataFunc, outputPaths](const std::string& url, URLRequestFailureType type, optional<HTTPResponseDetails> transferDetails) {
		bool tryNextUrl = false;
		switch (type)
		{
			case URLRequestFailureType::INITIALIZE_REQUEST_ERROR:
				wzAsyncExecOnMainThread([]{
					debug(LOG_WARNING, "Failed to initialize request for update check");
				});
				tryNextUrl = true;
				break;
			case URLRequestFailureType::TRANSFER_FAILED:
				if (!transferDetails.has_value())
				{
					wzAsyncExecOnMainThread([]{
						debug(LOG_WARNING, "Update check request failed - but no transfer failure details provided!");
					});
				}
				else
				{
					CURLcode result = transferDetails->curlResult();
					long httpStatusCode = transferDetails->httpStatusCode();
					wzAsyncExecOnMainThread([result, httpStatusCode]{
						debug(LOG_WARNING, "Update check request failed with error %d, and HTTP response code: %ld", result, httpStatusCode);
					});
				}
				tryNextUrl = true;
				break;
			case URLRequestFailureType::CANCELLED:
				wzAsyncExecOnMainThread([]{
					debug(LOG_INFO, "Update check was cancelled");
				});
				break;
			case URLRequestFailureType::CANCELLED_BY_SHUTDOWN:
				wzAsyncExecOnMainThread([]{
					debug(LOG_WARNING, "Update check was cancelled by application shutdown");
				});
				break;
		}
		if (tryNextUrl)
		{
			fetchLatestData(additionalUrls, processDataFunc, outputPaths);
		}
	};
	pRequest->maxDownloadSizeLimit = WZ_UPDATES_JSON_MAX_SIZE; // 32 MB (the response should never be this big)
	wzAsyncExecOnMainThread([pRequest]{
		urlRequestData(*pRequest);
		delete pRequest;
	});
}

/** This runs in a separate thread */
static int updateManagerThreadFunc(void *)
{
	WzUpdateManager::initUpdateCheck();
	return 0;
}

/** This runs in a separate thread */
static int compatManagerThreadFunc(void *)
{
	WzCompatCheckManager::initCompatCheck();
	return 0;
}

void WzInfoManager::initialize()
{
	// Create the cache dir if it doesn't exist
	if (!WZ_PHYSFS_isDirectory(WZ_UPDATES_CACHE_DIR))
	{
		if (PHYSFS_mkdir(WZ_UPDATES_CACHE_DIR) == 0)
		{
			// PHYSFS_mkdir failed?
			debug(LOG_ERROR, "Failed to create cache folder");
		}
	}

	WZ_THREAD* updateManagerThread = wzThreadCreate(updateManagerThreadFunc, nullptr);
	wzThreadStart(updateManagerThread);
	wzThreadDetach(updateManagerThread);

	WZ_THREAD* compatManagerThread = wzThreadCreate(compatManagerThreadFunc, nullptr);
	wzThreadStart(compatManagerThread);
	wzThreadDetach(compatManagerThread);
}

void WzInfoManager::shutdown()
{
	/* currently, no-op */
}
