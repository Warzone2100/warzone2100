/*
	This file is part of Warzone 2100.
	Copyright (C) 2022  Warzone 2100 Project

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

#if defined(__EMSCRIPTEN__)

#include "urlrequest.h"
#include "urlrequest_private.h"
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include <list>
#include <memory>

#include <emscripten/fetch.h>

#define MAXIMUM_DOWNLOAD_SIZE                       2147483647L
#define MAXIMUM_IN_MEMORY_DOWNLOAD_SIZE				1 << 29		// 512 MB, default max download limit

class URLTransferRequest;

static WZ_MUTEX         *urlRequestMutex = nullptr;
static std::list<std::shared_ptr<URLTransferRequest>> activeURLRequests;

struct AsyncRequestImpl : public AsyncRequest
{
public:
	AsyncRequestImpl(const std::shared_ptr<URLTransferRequest>& request)
	: weakRequest(request)
	{ }
public:
	std::weak_ptr<URLTransferRequest> weakRequest;
};

class EmFetchHTTPResponseDetails : public HTTPResponseDetails {
public:
	EmFetchHTTPResponseDetails(bool fetchResult, long httpStatusCode, std::shared_ptr<HTTPResponseHeaders> responseHeaders)
	: HTTPResponseDetails(httpStatusCode, responseHeaders)
	, _fetchResult(fetchResult)
	{ }
	virtual ~EmFetchHTTPResponseDetails()
	{ }

	std::string getInternalResultDescription() const override
	{
		return (_fetchResult) ? "Success" : "Returned Error";
	}

private:
	bool _fetchResult;
};

void wz_fetch_success(emscripten_fetch_t *fetch);
void wz_fetch_failure(emscripten_fetch_t *fetch);
void wz_fetch_onProgress(emscripten_fetch_t *fetch);
void wz_fetch_onReadyStateChange(emscripten_fetch_t *fetch);

class URLTransferRequest : public std::enable_shared_from_this<URLTransferRequest>
{
public:
	emscripten_fetch_t *handle = nullptr;

	struct FetchRequestHeaders
	{
	private:
		std::vector<char*> fetchHeadersLayout;
	public:
		FetchRequestHeaders(const std::unordered_map<std::string, std::string>& requestHeaders)
		{
			for (auto it : requestHeaders)
			{
				const auto& header_key = it.first;
				const auto& header_value = it.second;
				fetchHeadersLayout.push_back(strdup(it.first.c_str()));
				fetchHeadersLayout.push_back(strdup(it.second.c_str()));
			}
			fetchHeadersLayout.push_back(0);
		}
		~FetchRequestHeaders()
		{
			for (auto& value : fetchHeadersLayout)
			{
				if (value)
				{
					free(value);
				}
			}
			fetchHeadersLayout.clear();
		}

		// FetchRequestHeaders is non-copyable
		FetchRequestHeaders(const FetchRequestHeaders&) = delete;
		FetchRequestHeaders& operator=(const FetchRequestHeaders&) = delete;

		// FetchRequestHeaders is movable
		FetchRequestHeaders(FetchRequestHeaders&& other) = default;
		FetchRequestHeaders& operator=(FetchRequestHeaders&& other) = default;
	public:
		const char* const * getPointer()
		{
			return fetchHeadersLayout.data();
		}
	};

public:
	URLTransferRequest()
	{ }

	virtual ~URLTransferRequest()
	{ }

	void cancel()
	{
		deliberatelyCancelled = true;
		emscripten_fetch_close(handle);
		handle = nullptr;
	}

	virtual const std::string& url() const = 0;
	virtual const char* requestMethod() const { return "GET"; }
	virtual InternetProtocol protocol() const = 0;
	virtual bool noProxy() const = 0;
	virtual const std::unordered_map<std::string, std::string>& requestHeaders() const = 0;
	virtual uint64_t maxDownloadSize() const { return MAXIMUM_DOWNLOAD_SIZE; }

	virtual emscripten_fetch_t* initiateFetch()
	{
		// Create emscripten fetch attr struct
		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);

		// Set request method
		strcpy(attr.requestMethod, requestMethod());

		// Always load to memory
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

		if (noProxy())
		{
			wzAsyncExecOnMainThread([]{
				debug(LOG_NET, "Fetch: NOPROXY is not supported");
			});
		}

		formattedRequestHeaders = std::make_shared<FetchRequestHeaders>(requestHeaders());
		attr.requestHeaders = formattedRequestHeaders->getPointer();

		// Set callbacks
		attr.onsuccess = wz_fetch_success;
		attr.onerror = wz_fetch_failure;
		attr.onprogress = wz_fetch_onProgress;
		attr.onreadystatechange = wz_fetch_onReadyStateChange;

		// Set userdata to point to this
		attr.userData = (void*)this;

		// Create handle
		handle = emscripten_fetch(&attr, url().c_str());

		return handle;
	}

	bool wasCancelled() const  { return deliberatelyCancelled; }

	virtual bool hasWriteMemoryCallback() { return false; }
	virtual bool writeMemoryCallback(const void *contents, uint64_t numBytes, uint64_t dataOffset) { return false; }

	virtual bool onProgressUpdate(uint64_t dltotal, uint64_t dlnow) { return false; }

	virtual bool waitOnShutdown() const { return false; }

	virtual void handleRequestSuccess(unsigned short status) { }
	virtual void handleRequestError(unsigned short status) { }
	virtual void requestFailedToFinish(URLRequestFailureType type) { }

protected:
	friend void wz_fetch_onReadyStateChange(emscripten_fetch_t *fetch);
	std::shared_ptr<HTTPResponseHeadersContainer> responseHeaders = std::make_shared<HTTPResponseHeadersContainer>();
private:
	bool deliberatelyCancelled = false;
	std::shared_ptr<FetchRequestHeaders> formattedRequestHeaders;
};

void wz_fetch_success(emscripten_fetch_t *fetch)
{
	std::shared_ptr<URLTransferRequest> pSharedRequest;
	if (fetch->userData)
	{
		URLTransferRequest* pRequest = static_cast<URLTransferRequest*>(fetch->userData);

		if (pRequest->hasWriteMemoryCallback())
		{
			// Need to pass the downloaded block to the callback
			pRequest->writeMemoryCallback(fetch->data, fetch->numBytes, fetch->dataOffset);
		}

		pRequest->handleRequestSuccess(fetch->status);

		// now remove from the list of activeURLRequests
		pSharedRequest = pRequest->shared_from_this();
		wzMutexLock(urlRequestMutex);
		activeURLRequests.remove(pSharedRequest);
		wzMutexUnlock(urlRequestMutex);
	}

	emscripten_fetch_close(fetch);
}

void wz_fetch_failure(emscripten_fetch_t *fetch)
{
	if (!fetch) return;
	bool wasCancelled = false;
	std::shared_ptr<URLTransferRequest> pSharedRequest;
	if (fetch->userData)
	{
		URLTransferRequest* pRequest = static_cast<URLTransferRequest*>(fetch->userData);
		wasCancelled = pRequest->wasCancelled();
		if (!wasCancelled)
		{
			// handle the error
			pRequest->handleRequestError(fetch->status);
		}

		// now remove from the list of activeURLRequests
		pSharedRequest = pRequest->shared_from_this();
		wzMutexLock(urlRequestMutex);
		activeURLRequests.remove(pSharedRequest);
		wzMutexUnlock(urlRequestMutex);
	}

	if (!wasCancelled) // until emscripten bug is fixed, can't call emscripten_fetch_close from within error handler if already triggered by cancellation
	{
		emscripten_fetch_close(fetch);
	}
}

void wz_fetch_onProgress(emscripten_fetch_t *fetch)
{
	if (!fetch) return;
	if (fetch->status != 200) return;
	if (fetch->userData == nullptr) return;

	URLTransferRequest* pRequest = static_cast<URLTransferRequest*>(fetch->userData);
	pRequest->onProgressUpdate(fetch->totalBytes /* NOTE: May be 0 */, fetch->dataOffset + fetch->numBytes);
}

void wz_fetch_onReadyStateChange(emscripten_fetch_t *fetch)
{
	if (fetch->readyState != 2) return; // 2 = HEADERS_RECEIVED

	if (fetch->userData == nullptr) return;
	URLTransferRequest* pRequest = static_cast<URLTransferRequest*>(fetch->userData);

	size_t headersLengthBytes = emscripten_fetch_get_response_headers_length(fetch) + 1;
	char *headerString = new char[headersLengthBytes];
	emscripten_fetch_get_response_headers(fetch, headerString, headersLengthBytes);
	char **responseHeaders = emscripten_fetch_unpack_response_headers(headerString);
	delete[] headerString;

	int numHeaders = 0;
	for(; responseHeaders[numHeaders * 2]; ++numHeaders)
	{
		if (responseHeaders[(numHeaders * 2) + 1] == nullptr)
		{
			break;
		}

		std::string header_key = (responseHeaders[numHeaders * 2]) ? responseHeaders[numHeaders * 2] : "";
		std::string header_value = (responseHeaders[(numHeaders * 2) + 1]) ? responseHeaders[(numHeaders * 2) + 1] : "";
		trim_str(header_key);
		trim_str(header_value);
		pRequest->responseHeaders->responseHeaders[header_key] = header_value;
	}

	emscripten_fetch_free_unpacked_response_headers(responseHeaders);
}

class RunningURLTransferRequestBase : public URLTransferRequest
{
public:
	virtual const URLRequestBase& getBaseRequest() const = 0;
public:
	RunningURLTransferRequestBase()
	: URLTransferRequest()
	{ }

	virtual const std::string& url() const override
	{
		return getBaseRequest().url;
	}

	virtual InternetProtocol protocol() const override
	{
		return getBaseRequest().protocol;
	}

	virtual bool noProxy() const override
	{
		return getBaseRequest().noProxy;
	}

	virtual const std::unordered_map<std::string, std::string>& requestHeaders() const override
	{
		return getBaseRequest().getRequestHeaders();
	}

	virtual bool onProgressUpdate(uint64_t dltotal, uint64_t dlnow) override
	{
		auto request = getBaseRequest();
		if (request.progressCallback)
		{
			request.progressCallback(request.url, static_cast<int64_t>(dltotal), static_cast<int64_t>(dlnow));
		}
		return false;
	}

	virtual void handleRequestSuccess(unsigned short status) override
	{
		onSuccess(EmFetchHTTPResponseDetails(true, status, responseHeaders));
	}

	virtual void handleRequestError(unsigned short status) override
	{
		onFailure(URLRequestFailureType::TRANSFER_FAILED, std::make_shared<EmFetchHTTPResponseDetails>(false, status, responseHeaders));
	}

	virtual void requestFailedToFinish(URLRequestFailureType type) override
	{
		ASSERT_OR_RETURN(, type != URLRequestFailureType::TRANSFER_FAILED, "TRANSFER_FAILED should be handled by handleRequestDone");
		onFailure(type, nullptr);
	}

private:
	virtual void onSuccess(const HTTPResponseDetails& responseDetails) = 0;

	void onFailure(URLRequestFailureType type, const std::shared_ptr<EmFetchHTTPResponseDetails>& transferDetails)
	{
		auto request = getBaseRequest();

		const std::string& url = request.url;
		switch (type)
		{
			case URLRequestFailureType::INITIALIZE_REQUEST_ERROR:
				wzAsyncExecOnMainThread([url]{
					debug(LOG_NET, "Fetch: Failed to initialize request for (%s)", url.c_str());
				});
				break;
			case URLRequestFailureType::TRANSFER_FAILED:
				if (!transferDetails)
				{
					wzAsyncExecOnMainThread([url]{
						debug(LOG_ERROR, "Fetch: Request for (%s) failed - but no transfer failure details provided!", url.c_str());
					});
				}
				else
				{
					long httpStatusCode = transferDetails->httpStatusCode();
					wzAsyncExecOnMainThread([url, httpStatusCode]{
						debug(LOG_NET, "Fetch: Request for (%s) failed with HTTP response code: %ld", url.c_str(), httpStatusCode);
					});
				}
				break;
			case URLRequestFailureType::CANCELLED:
				wzAsyncExecOnMainThread([url]{
					debug(LOG_NET, "Fetch: Request for (%s) was cancelled", url.c_str());
				});
				break;
			case URLRequestFailureType::CANCELLED_BY_SHUTDOWN:
				wzAsyncExecOnMainThread([url]{
					debug(LOG_NET, "Fetch: Request for (%s) was cancelled by application shutdown", url.c_str());
				});
				break;
		}

		if (request.onFailure)
		{
			request.onFailure(request.url, type, transferDetails);
		}
	}
};

class RunningURLDataRequest : public RunningURLTransferRequestBase
{
public:
	URLDataRequest request;
	std::shared_ptr<MemoryStruct> chunk;
private:
	uint64_t _maxDownloadSize = MAXIMUM_IN_MEMORY_DOWNLOAD_SIZE; // 512 MB, default max download limit

public:
	RunningURLDataRequest(const URLDataRequest& request)
	: RunningURLTransferRequestBase()
	, request(request)
	{
		chunk = std::make_shared<MemoryStruct>();
		if (request.maxDownloadSizeLimit > 0)
		{
			_maxDownloadSize = std::min<uint64_t>(request.maxDownloadSizeLimit, static_cast<uint64_t>(MAXIMUM_IN_MEMORY_DOWNLOAD_SIZE));
		}
	}

	virtual const URLRequestBase& getBaseRequest() const override
	{
		return request;
	}

	virtual uint64_t maxDownloadSize() const override
	{
		// For downloading to memory, set a lower default max download limit
		return _maxDownloadSize;
	}

	virtual bool hasWriteMemoryCallback() override { return true; }
	virtual bool writeMemoryCallback(const void *contents, uint64_t numBytes, uint64_t dataOffset) override
	{
		size_t realsize = numBytes;
		MemoryStruct *mem = chunk.get();

#if UINT64_MAX > SIZE_MAX
		if (numBytes > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
		{
			return false;
		}
		if (dataOffset > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
		{
			return false;
		}
		if ((dataOffset + numBytes) > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
		{
			return false;
		}
#endif

		if (static_cast<size_t>(dataOffset + numBytes) > mem->size)
		{
			char *ptr = (char*) realloc(mem->memory, static_cast<size_t>(dataOffset + numBytes) + 1);
			if(ptr == NULL) {
				/* out of memory! */
				return false;
			}
			mem->memory = ptr;
			mem->size = static_cast<size_t>(dataOffset + numBytes);
		}

		memcpy(&(mem->memory[static_cast<size_t>(dataOffset)]), contents, static_cast<size_t>(numBytes));
		mem->memory[mem->size] = 0;

		return true;
	}

private:
	void onSuccess(const HTTPResponseDetails& responseDetails) override
	{
		if (request.onSuccess)
		{
			request.onSuccess(request.url, responseDetails, chunk);
		}
	}
};


// Request data from a URL (stores the response in memory)
// Generally, you should define both onSuccess and onFailure callbacks
// If you want to actually process the response, you *must* define an onSuccess callback
//
// IMPORTANT: Callbacks may be called on a background thread
AsyncURLRequestHandle urlRequestData(const URLDataRequest& request)
{
	ASSERT_OR_RETURN(nullptr, !request.url.empty(), "A valid request must specify a URL");
	ASSERT_OR_RETURN(nullptr, request.maxDownloadSizeLimit <= (curl_off_t)MAXIMUM_IN_MEMORY_DOWNLOAD_SIZE, "Requested maxDownloadSizeLimit exceeds maximum in-memory download size limit %zu", (size_t)MAXIMUM_IN_MEMORY_DOWNLOAD_SIZE);

	std::shared_ptr<RunningURLDataRequest> pNewRequest = std::make_shared<RunningURLDataRequest>(request);
	std::shared_ptr<AsyncRequestImpl> requestHandle = std::make_shared<AsyncRequestImpl>(pNewRequest);

	wzMutexLock(urlRequestMutex);
	activeURLRequests.push_back(pNewRequest);
	wzMutexUnlock(urlRequestMutex);

	if (!pNewRequest->initiateFetch())
	{
		pNewRequest->requestFailedToFinish(URLRequestFailureType::INITIALIZE_REQUEST_ERROR);
		wzMutexLock(urlRequestMutex);
		activeURLRequests.remove(pNewRequest);
		wzMutexUnlock(urlRequestMutex);
		return nullptr;
	}

	return requestHandle;
}

// Download a file (stores the response in the outFilePath)
// Generally, you should define both onSuccess and onFailure callbacks
//
// IMPORTANT: Callbacks may be called on a background thread
AsyncURLRequestHandle urlDownloadFile(const URLFileDownloadRequest& request)
{
	// TODO: Implement
	return nullptr;
}

// Sets a flag that will cancel an asynchronous url request
// NOTE: It is possible that the request will finish successfully before it is cancelled.
void urlRequestSetCancelFlag(AsyncURLRequestHandle requestHandle)
{
	std::shared_ptr<AsyncRequestImpl> pRequestImpl = std::dynamic_pointer_cast<AsyncRequestImpl>(requestHandle);
	if (pRequestImpl)
	{
		if (auto request = pRequestImpl->weakRequest.lock())
		{
			request->cancel();
		}
	}
}

void urlRequestInit()
{
	// Currently, nothing to do
}
void urlRequestOutputDebugInfo()
{
	// Currently a no-op for Emscripten Fetch backend
}
void urlRequestShutdown()
{
	// For now, just cancel all outstanding non-"waitOnShutdown" requests
	// FUTURE TODO: Examine how to best handle "waitOnShutdown" requests?

	// build a copy of the active list - the actual active list is managed by the callbacks for url requests
	wzMutexLock(urlRequestMutex);
	auto activeURLRequestsCopy = activeURLRequests;
	wzMutexUnlock(urlRequestMutex);

	debug(LOG_NET, "urlRequestShutdown: Remaining requests: %zu", activeURLRequestsCopy.size());

	auto it = activeURLRequestsCopy.begin();
	while (it != activeURLRequestsCopy.end())
	{
		auto runningTransfer = *it;
		if (!runningTransfer->waitOnShutdown())
		{
			// just cancel it
			runningTransfer->cancel(); // this will ultimately handle removing from the main global list
			runningTransfer->requestFailedToFinish(URLRequestFailureType::CANCELLED_BY_SHUTDOWN);
			it = activeURLRequestsCopy.erase(it);
		}
		else
		{
			++it;
		}
	}
}

#endif
