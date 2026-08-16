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

#include "wzhashrequest.h"
#include "wzhashablefile.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"

#include <sodium.h>

#include <algorithm>
#include <chrono>
#include <deque>
#include <mutex>
#include <utility>

namespace
{

constexpr int SUPPORTED_HASH_REQUEST_VER = 1;

struct ParsedTarget
{
	std::string kind;
	std::vector<wzkeyedhash::Range> ranges;
};

struct HashRequestTask
{
	std::string requestId;
	std::vector<uint8_t> nonce;
	std::vector<ParsedTarget> targets;
	std::shared_ptr<std::atomic<bool>> cancelFlag;
	std::function<void(nlohmann::ordered_json)> resultFunc;
};

std::mutex taskMutex;
std::deque<HashRequestTask> taskQueue;	// guarded by taskMutex
WZ_THREAD* hashRequestThread = nullptr;	// main thread only
WZ_SEMAPHORE* taskSemaphore = nullptr;
std::atomic<bool> stopRequested(false);

// Token bucket limiting answered requests (main thread only)
uint32_t rateTokens = WZ_HASH_REQUEST_RATE_BUCKET_CAPACITY;
std::chrono::steady_clock::time_point rateLastRefill = std::chrono::steady_clock::now();

// NOTE: Main thread only. Returns true (and consumes a token) if a request may be answered now.
bool rateLimitConsumeToken()
{
	const auto now = std::chrono::steady_clock::now();
	const auto refillInterval = std::chrono::seconds(WZ_HASH_REQUEST_RATE_REFILL_SECONDS);
	while (rateTokens < WZ_HASH_REQUEST_RATE_BUCKET_CAPACITY && now - rateLastRefill >= refillInterval)
	{
		++rateTokens;
		rateLastRefill += refillInterval;
	}
	if (rateTokens >= WZ_HASH_REQUEST_RATE_BUCKET_CAPACITY)
	{
		rateLastRefill = now;
	}
	if (rateTokens == 0)
	{
		return false;
	}
	--rateTokens;
	return true;
}

nlohmann::ordered_json declinedResponse(const std::string& requestId)
{
	nlohmann::ordered_json response;
	if (!requestId.empty())
	{
		response["reqId"] = requestId;
	}
	response["status"] = "declined";
	return response;
}

std::string base64Encode(const uint8_t* data, size_t len)
{
	std::string encoded;
	encoded.resize(sodium_base64_ENCODED_LEN(len, sodium_base64_VARIANT_ORIGINAL));
	sodium_bin2base64(&encoded[0], encoded.size(), data, len, sodium_base64_VARIANT_ORIGINAL);
	encoded.resize(strlen(encoded.c_str()));
	return encoded;
}

// Parses and validates against the hard limits. On failure returns false,
// with echoId set when a well-formed reqId was at least present.
bool parseAndValidate(const nlohmann::json& hashRequest, HashRequestTask& output, std::string& echoId)
{
	if (!hashRequest.is_object())
	{
		return false;
	}
	try {
		auto idIt = hashRequest.find("reqId");
		if (idIt == hashRequest.end() || !idIt->is_string())
		{
			return false;
		}
		std::string requestId = idIt->get<std::string>();
		if (requestId.empty() || requestId.size() > WZ_HASH_REQUEST_MAX_ID_LENGTH || requestId.find('\0') != std::string::npos)
		{
			return false;
		}
		echoId = requestId;

		if (hashRequest.value("ver", 0) != SUPPORTED_HASH_REQUEST_VER)
		{
			return false;
		}

		std::string nonceB64 = hashRequest.value("nonce", std::string());
		std::vector<uint8_t> nonce(wzkeyedhash::NONCE_BYTES + 1);
		size_t nonceLen = 0;
		if (sodium_base642bin(nonce.data(), nonce.size(), nonceB64.c_str(), nonceB64.size(),
							  nullptr, &nonceLen, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0
			|| nonceLen != wzkeyedhash::NONCE_BYTES)
		{
			return false;
		}
		nonce.resize(wzkeyedhash::NONCE_BYTES);

		auto targetsIt = hashRequest.find("targets");
		if (targetsIt == hashRequest.end() || !targetsIt->is_array()
			|| targetsIt->empty() || targetsIt->size() > WZ_HASH_REQUEST_MAX_TARGETS)
		{
			return false;
		}
		uint64_t totalRequestedBytes = 0;
		std::vector<ParsedTarget> targets;
		for (const auto& targetJson : *targetsIt)
		{
			ParsedTarget target;
			target.kind = targetJson.value("kind", std::string());
			if (target.kind != "exe")
			{
				return false;
			}
			auto rangesIt = targetJson.find("ranges");
			if (rangesIt == targetJson.end() || !rangesIt->is_array()
				|| rangesIt->empty() || rangesIt->size() > WZ_HASH_REQUEST_MAX_RANGES_PER_TARGET)
			{
				return false;
			}
			for (const auto& rangeJson : *rangesIt)
			{
				if (!rangeJson.is_array() || rangeJson.size() != 2
					|| !rangeJson[0].is_number_unsigned() || !rangeJson[1].is_number_unsigned())
				{
					return false;
				}
				wzkeyedhash::Range range { rangeJson[0].get<uint64_t>(), rangeJson[1].get<uint64_t>() };
				if (range.len < WZ_HASH_REQUEST_MIN_RANGE_BYTES || range.len > wzkeyedhash::MAX_TOTAL_REQUESTED_BYTES
					|| totalRequestedBytes + range.len > wzkeyedhash::MAX_TOTAL_REQUESTED_BYTES
					|| range.offset > UINT64_MAX - range.len)
				{
					return false;
				}
				totalRequestedBytes += range.len;
				target.ranges.push_back(range);
			}
			targets.push_back(std::move(target));
		}

		// Ranges must not overlap (all targets address the same file)
		std::vector<wzkeyedhash::Range> allRanges;
		for (const auto& target : targets)
		{
			allRanges.insert(allRanges.end(), target.ranges.begin(), target.ranges.end());
		}
		std::sort(allRanges.begin(), allRanges.end(), [](const wzkeyedhash::Range& a, const wzkeyedhash::Range& b) {
			return a.offset < b.offset;
		});
		for (size_t i = 1; i < allRanges.size(); ++i)
		{
			if (allRanges[i - 1].offset + allRanges[i - 1].len > allRanges[i].offset)
			{
				return false;
			}
		}

		output.requestId = std::move(requestId);
		output.nonce = std::move(nonce);
		output.targets = std::move(targets);
		return true;
	}
	catch (const std::exception&) {
		return false;
	}
}

// Hash request worker thread
nlohmann::ordered_json computeResponse(const HashRequestTask& task)
{
	optional<HashableFile> exe = HashableFile::ownExecutable();
	if (!exe.has_value())
	{
		return declinedResponse(task.requestId);
	}
	const std::atomic<bool>* stopFlag = (task.cancelFlag) ? task.cancelFlag.get() : &stopRequested;
	std::vector<std::string> answers;
	for (size_t i = 0; i < task.targets.size(); ++i)
	{
		if (stopRequested.load(std::memory_order_relaxed))
		{
			return declinedResponse(task.requestId);
		}
		const auto& target = task.targets[i];
		auto answer = exe.value().keyedHash(task.nonce, task.requestId, static_cast<uint32_t>(i),
											  target.kind, target.ranges, stopFlag);
		if (!answer.has_value())
		{
			return declinedResponse(task.requestId);
		}
		answers.push_back(base64Encode(answer.value().data(), answer.value().size()));
	}
	nlohmann::ordered_json response;
	response["reqId"] = task.requestId;
	response["status"] = "ok";
	response["answers"] = answers;
	return response;
}

int hashRequestThreadFunc(void*)
{
	while (true)
	{
		wzSemaphoreWait(taskSemaphore);
		if (stopRequested.load(std::memory_order_relaxed))
		{
			return 0;
		}
		HashRequestTask task;
		{
			std::lock_guard<std::mutex> lock(taskMutex);
			if (taskQueue.empty())
			{
				continue;
			}
			task = std::move(taskQueue.front());
			taskQueue.pop_front();
		}
		nlohmann::ordered_json response = computeResponse(task);
		auto resultFunc = task.resultFunc;
		wzAsyncExecOnMainThread([resultFunc, response]() {
			resultFunc(response);
		});
	}
}

} // anonymous namespace

void answerHashRequest(const nlohmann::json& hashRequest,
						std::shared_ptr<std::atomic<bool>> cancelFlag,
						std::function<void(nlohmann::ordered_json hashResponse)> resultFunc)
{
	ASSERT_OR_RETURN(, resultFunc != nullptr, "Null resultFunc");
	HashRequestTask task;
	std::string echoId;
	bool accepted = !stopRequested.load(std::memory_order_relaxed)
		&& parseAndValidate(hashRequest, task, echoId)
		&& rateLimitConsumeToken();
	if (!accepted)
	{
		auto response = declinedResponse(echoId);
		wzAsyncExecOnMainThread([resultFunc, response]() {
			resultFunc(response);
		});
		return;
	}
	task.cancelFlag = std::move(cancelFlag);
	task.resultFunc = std::move(resultFunc);
	if (hashRequestThread == nullptr)
	{
		taskSemaphore = wzSemaphoreCreate(0);
		hashRequestThread = wzThreadCreate(hashRequestThreadFunc, nullptr, "wzHashRequest");
		if (hashRequestThread == nullptr)
		{
			ASSERT(false, "Failed to create hash request thread");
			wzSemaphoreDestroy(taskSemaphore);
			taskSemaphore = nullptr;
			auto response = declinedResponse(task.requestId);
			auto deliverFunc = task.resultFunc;
			wzAsyncExecOnMainThread([deliverFunc, response]() {
				deliverFunc(response);
			});
			return;
		}
		wzThreadStart(hashRequestThread);
	}
	{
		std::lock_guard<std::mutex> lock(taskMutex);
		taskQueue.push_back(std::move(task));
	}
	wzSemaphorePost(taskSemaphore);
}

void hashRequestRateLimitStartEvent()
{
	rateTokens = WZ_HASH_REQUEST_RATE_BUCKET_CAPACITY;
	rateLastRefill = std::chrono::steady_clock::now();
}

void hashRequestShutdown()
{
	stopRequested = true;
	if (hashRequestThread != nullptr)
	{
		wzSemaphorePost(taskSemaphore);
		wzThreadJoin(hashRequestThread);
		hashRequestThread = nullptr;
	}
	if (taskSemaphore != nullptr)
	{
		wzSemaphoreDestroy(taskSemaphore);
		taskSemaphore = nullptr;
	}
	{
		std::lock_guard<std::mutex> lock(taskMutex);
		taskQueue.clear();
	}
}
