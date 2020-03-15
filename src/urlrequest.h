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
#ifndef _URL_REQUEST_H_
#define _URL_REQUEST_H_

#include <functional>
#include <curl/curl.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <optional-lite/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

struct MemoryStruct {
	char *memory = nullptr;
	size_t size = 0;

	MemoryStruct();
	~MemoryStruct();
};

enum URLRequestFailureType {
	INITIALIZE_REQUEST_ERROR,
	TRANSFER_FAILED,
	CANCELLED_BY_SHUTDOWN
};

class HTTPResponseHeaders {
public:
	virtual ~HTTPResponseHeaders();

	// Permits retrieving HTTP response headers in a case-insensitive manner
	virtual bool hasHeader(const std::string& name) const = 0;
	virtual bool getHeader(const std::string& name, std::string& output_value) const = 0;
};

class HTTPResponseDetails {
public:
	HTTPResponseDetails(CURLcode curlResult, long httpStatusCode, std::shared_ptr<HTTPResponseHeaders> responseHeaders)
	: _curlResult(curlResult)
	, _httpStatusCode(httpStatusCode)
	, _responseHeaders(responseHeaders)
	{ }
	virtual ~HTTPResponseDetails();

	CURLcode curlResult() const { return _curlResult; }
	long httpStatusCode() const { return _httpStatusCode; }

	// Permits retrieving HTTP response headers in a case-insensitive manner
	bool hasHeader(const std::string& name) const { return _responseHeaders->hasHeader(name); }
	bool getHeader(const std::string& name, std::string& output_value) const { return _responseHeaders->getHeader(name, output_value); }
private:
	CURLcode _curlResult;
	long _httpStatusCode;
	std::shared_ptr<HTTPResponseHeaders> _responseHeaders;
};

typedef std::function<void (const std::string& url, URLRequestFailureType type, optional<HTTPResponseDetails> transferDetails)> UrlRequestFailure;
typedef std::function<void (const std::string& url, int64_t dltotal, int64_t dlnow)> UrlProgressCallback;

struct URLRequestBase
{
public:
	std::string url;

	// MARK: callbacks
	// IMPORTANT:
	// - callbacks will be called on a background thread
	// - if you need to do something on the main thread, please wrap that logic
	//   (inside your callback) in wzAsyncExecOnMainThread
	UrlProgressCallback progressCallback;
	// IMPORTANT:
	// - `transferDetails` may be null
	// - `transferDetails` is only valid until this onFailure handler returns
	// - Do *not* pass `transferDetails` to any asynchronous dispatch, like `wzAsyncExecOnMainThread`. Instead, copy the necessary information.
	UrlRequestFailure onFailure;

public:
	// Adding request headers
	bool setRequestHeader(const std::string& name, const std::string& value);

	// Get request headers
	const std::unordered_map<std::string, std::string>& getRequestHeaders() const { return requestHeaders; }
private:
	std::unordered_map<std::string, std::string> requestHeaders;
};

typedef std::function<void (const std::string& url, const HTTPResponseDetails& response, const std::shared_ptr<MemoryStruct>& data)> UrlRequestSuccess;

struct URLDataRequest : public URLRequestBase
{
	curl_off_t maxDownloadSizeLimit = 0;

	// MARK: callbacks
	// IMPORTANT:
	// - callbacks will be called on a background thread
	// - if you need to do something on the main thread, please wrap that logic
	//   (inside your callback) in wzAsyncExecOnMainThread
	UrlRequestSuccess onSuccess;
};

typedef std::function<void (const std::string& url, const HTTPResponseDetails& response, const std::string& outFilePath)> UrlDownloadFileSuccess;

struct URLFileDownloadRequest : public URLRequestBase
{
	std::string outFilePath;

	// MARK: callbacks
	// IMPORTANT:
	// - callbacks will be called on a background thread
	// - if you need to do something on the main thread, please wrap that logic
	//   (inside your callback) in wzAsyncExecOnMainThread
	UrlDownloadFileSuccess onSuccess;
};

// Request data from a URL (stores the response in memory)
// Generally, you should define both onSuccess and onFailure callbacks
// If you want to actually process the response, you *must* define an onSuccess callback
//
// IMPORTANT: Callbacks will be called on a background thread
void urlRequestData(const URLDataRequest& request);

// Download a file (stores the response in the outFilePath)
// Generally, you should define both onSuccess and onFailure callbacks
//
// IMPORTANT: Callbacks will be called on a background thread
void urlDownloadFile(const URLFileDownloadRequest& request);

void urlRequestInit();
void urlRequestOutputDebugInfo();
void urlRequestShutdown();

#endif //_URL_REQUEST_H_
