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
#if !defined(__EMSCRIPTEN__)
#include <curl/curl.h>
#else
typedef long curl_off_t;
#endif
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

struct MemoryStruct {
	char *memory = nullptr;
	size_t size = 0;

	MemoryStruct();
	~MemoryStruct();
};

enum class URLRequestFailureType {
	INITIALIZE_REQUEST_ERROR,
	OPERATION_TIMEOUT,
	COULDNT_CONNECT,
	TRANSFER_FAILED,
	CANCELLED,
	CANCELLED_BY_SHUTDOWN
};

const char* to_string(URLRequestFailureType val);

class HTTPResponseHeaders {
public:
	virtual ~HTTPResponseHeaders();

	// Permits retrieving HTTP response headers in a case-insensitive manner
	virtual bool hasHeader(const std::string& name) const = 0;
	virtual bool getHeader(const std::string& name, std::string& output_value) const = 0;
};

class HTTPResponseDetails {
public:
	HTTPResponseDetails(long httpStatusCode, std::shared_ptr<HTTPResponseHeaders> responseHeaders)
	: _httpStatusCode(httpStatusCode)
	, _responseHeaders(responseHeaders)
	{ }
	virtual ~HTTPResponseDetails();

	virtual std::string getInternalResultDescription() const = 0;
	long httpStatusCode() const { return _httpStatusCode; }

	// Permits retrieving HTTP response headers in a case-insensitive manner
	bool hasHeader(const std::string& name) const { return _responseHeaders->hasHeader(name); }
	bool getHeader(const std::string& name, std::string& output_value) const { return _responseHeaders->getHeader(name, output_value); }
private:
	long _httpStatusCode;
	std::shared_ptr<HTTPResponseHeaders> _responseHeaders;
};

typedef std::function<void (const std::string& url, URLRequestFailureType type, std::shared_ptr<HTTPResponseDetails> transferDetails)> UrlRequestFailure;
typedef std::function<void (const std::string& url, int64_t dltotal, int64_t dlnow)> UrlProgressCallback;

enum class InternetProtocol {
	IP_ANY,
	IPv4,
	IPv6
};
const char* to_string(InternetProtocol protocol);

enum class URLRequestMethod {
	GET,
	POST,
	PATCH,
	DELETE
};
const char* to_string(URLRequestMethod method);

struct URLRequestBase
{
public:
	std::string url;
	InternetProtocol protocol = InternetProtocol::IP_ANY;

	// Whether to disable the use of proxy.
	// NOTE: Only supported with cURL backend.
	bool noProxy = false;

	// Whether to disable following redirects.
	// NOTE: Only supported with cURL backend.
	bool noFollowRedirects = false;

	// The desired maximum time in milliseconds for the connection phase to take
	// (DNS resolution, establishing connection). 0 for default.
	// NOTE: Only supported with cURL backend.
	uint32_t connectTimeoutMs = 0;

	// MARK: callbacks
	// IMPORTANT:
	// - callbacks *may* be called on a background thread
	// - if you need to do something on the main thread, please wrap that logic
	//   (inside your callback) in wzAsyncExecOnMainThread
	// IMPORTANT:
	// - `dltotal` may be <= 0 if the server did not report the Content-Length field
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

constexpr long HttpRequestConnectionFailure = 0; // 0 is returned as the http response code if there is an internal curl error or error connecting altogether
constexpr long HttpStatusTooManyRequests = 429; // RFC 6585, 4

struct URLRequestAutoRetryStrategy
{
public:
	URLRequestAutoRetryStrategy()
	{ }
public:
	static URLRequestAutoRetryStrategy None()
	{
		return URLRequestAutoRetryStrategy();
	}
	static URLRequestAutoRetryStrategy RetryAfter(std::chrono::milliseconds minDelay, size_t maxRetries = 1, const std::unordered_set<long> retryResponseCodes = {HttpRequestConnectionFailure, HttpStatusTooManyRequests}, std::chrono::milliseconds maxDelay = std::chrono::milliseconds(10000))
	{
		auto result = URLRequestAutoRetryStrategy();
		result.m_minDelay = minDelay;
		result.m_maxRetries = maxRetries;
		result.m_maxDelay = maxDelay;
		result.m_retryResponseCodes = retryResponseCodes;
		return result;
	}
public:
	bool shouldRetryOnHttpResponseCode(long httpResponseCode) const { return m_maxRetries > 0 && m_retryResponseCodes.count(httpResponseCode) != 0; }
	size_t maxRetries() const { return m_maxRetries; }
	std::chrono::milliseconds minDelay() const { return m_minDelay; }
	std::chrono::milliseconds maxDelay() const { return m_maxDelay; }
private:
	std::chrono::milliseconds m_minDelay = std::chrono::milliseconds(0);
	std::chrono::milliseconds m_maxDelay = std::chrono::milliseconds(0);
	size_t m_maxRetries = 0;
	std::unordered_set<long> m_retryResponseCodes;
};

struct URLRequestHandlingBehavior
{
public:
	enum class Strategy
	{
		Done,
		RetryAfter
	};
protected:
	URLRequestHandlingBehavior(Strategy strategy)
	: m_strategy(strategy)
	{ }
public:
	static URLRequestHandlingBehavior Done()
	{
		return URLRequestHandlingBehavior(Strategy::Done);
	}
	static URLRequestHandlingBehavior RetryAfter(std::chrono::milliseconds delay)
	{
		auto result = URLRequestHandlingBehavior(Strategy::RetryAfter);
		result.m_delay = delay;
		return result;
	}
public:
	Strategy strategy() const { return m_strategy; }
	std::chrono::milliseconds delay() const { return m_delay; }
private:
	Strategy m_strategy;
	std::chrono::milliseconds m_delay;
};

typedef std::function<URLRequestHandlingBehavior (const std::string& url, const HTTPResponseDetails& response, const std::shared_ptr<MemoryStruct>& data)> UrlRequestResponseHandler;

struct URLDataRequest : public URLRequestBase
{
public:
	curl_off_t maxDownloadSizeLimit = 0;
	URLRequestAutoRetryStrategy autoRetryStrategy = URLRequestAutoRetryStrategy::None();

	// MARK: callbacks
	// IMPORTANT:
	// - callbacks will be called on a background thread
	// - if you need to do something on the main thread, please wrap that logic
	//   (inside your callback) in wzAsyncExecOnMainThread
	UrlRequestResponseHandler onResponse;

public:
	void setPost(std::string&& jsonData);
	void setPatch(std::string&& jsonData);
	void setDelete();

	URLRequestMethod method() const;
	const std::string& requestBody() const;

private:
	URLRequestMethod m_method = URLRequestMethod::GET;

	// Data to be provided in a POST/PATCH body
	std::string m_requestBody;
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
	UrlDownloadFileSuccess onResponse;
};

struct AsyncRequest
{
public:
	virtual ~AsyncRequest();
};

typedef std::shared_ptr<AsyncRequest> AsyncURLRequestHandle;

// Request data from a URL (stores the response in memory)
// Generally, you should define both onSuccess and onFailure callbacks
// If you want to actually process the response, you *must* define an onSuccess callback
//
// IMPORTANT: Callbacks will be called on a background thread
AsyncURLRequestHandle urlRequestData(const URLDataRequest& request);

// Download a file (stores the response in the outFilePath)
// Generally, you should define both onSuccess and onFailure callbacks
//
// IMPORTANT: Callbacks will be called on a background thread
AsyncURLRequestHandle urlDownloadFile(const URLFileDownloadRequest& request);

// Sets a flag that will cancel an asynchronous url request
// NOTE: It is possible that the request will finish successfully before it is cancelled.
void urlRequestSetCancelFlag(AsyncURLRequestHandle requestHandle);

void urlRequestInit();
void urlRequestOutputDebugInfo();
void urlRequestShutdown();

#endif //_URL_REQUEST_H_
