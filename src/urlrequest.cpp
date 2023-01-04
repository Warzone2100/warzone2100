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

	------

	Portions of this file are derived from cURL examples, which are licensed under the curl license.
	(The terms are also available at https://curl.haxx.se/docs/copyright.html.)

	curl license:
	COPYRIGHT AND PERMISSION NOTICE

	Copyright (c) 1996 - 2020, Daniel Stenberg, daniel@haxx.se, and many
	contributors, see the THANKS file.

	All rights reserved.

	Permission to use, copy, modify, and distribute this software for any purpose
	with or without fee is hereby granted, provided that the above copyright notice
	and this permission notice appear in all copies.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
	DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	Except as contained in this notice, the name of a copyright holder shall not be
	used in advertising or otherwise to promote the sale, use or other dealings in
	this Software without prior written authorization of the copyright holder.
*/

#define NOMINMAX
#include "urlrequest.h"
#include "lib/framework/wzapp.h"
#include <atomic>
#include <list>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <regex>
#include <stdlib.h>
#include <chrono>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#if defined(WZ_OS_UNIX)
# include <fcntl.h>
# ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 1
# endif
# ifndef _XOPEN_SOURCE
#  define _XOPEN_SOURCE
# endif
# ifndef _POSIX_SOURCE
#  define _POSIX_SOURCE
# endif
# include <stdio.h>
#endif

#undef max
#undef min

#define MAX_WAIT_ON_SHUTDOWN_SECONDS 60

static volatile bool urlRequestQuit = false;

static WZ_THREAD        *urlRequestThread = nullptr;
static WZ_MUTEX         *urlRequestMutex = nullptr;
static WZ_SEMAPHORE     *urlRequestSemaphore = nullptr;

MemoryStruct::MemoryStruct()
{
	memory = (char *) malloc(1);
	size = 0;
}

MemoryStruct::~MemoryStruct()
{
	if (memory != nullptr)
	{
		free(memory);
		memory = nullptr;
	}
}

HTTPResponseHeaders::~HTTPResponseHeaders() { }
HTTPResponseDetails::~HTTPResponseDetails() { }
AsyncRequest::~AsyncRequest() { }

bool URLRequestBase::setRequestHeader(const std::string& name, const std::string& value)
{
	ASSERT_OR_RETURN(false, !name.empty(), "Header name must not be empty");
	ASSERT_OR_RETURN(false, name.find_first_of(":") == std::string::npos, "Header name must not contain ':'");
	requestHeaders[name] = value;
	return true;
}

struct AsyncRequestImpl : public AsyncRequest
{
public:
	// **MUST** be accessed while holding the urlRequestMutex
	bool cancelFlag = false;
};

// MARK: - Handle thread-safety for cURL

#if defined(USE_OPENSSL_LOCKS_INIT)
# include <openssl/crypto.h>

// lock callbacks are only needed for OpenSSL < 1.1.0
# if OPENSSL_VERSION_NUMBER < 0x10100000L

/* we have this global to let the callback get easy access to it */
static std::vector<WZ_MUTEX *> lockarray;

static void lock_callback(int mode, int type, const char * file, int line)
{
	(void)file;
	(void)line;
	if(mode & CRYPTO_LOCK) {
		wzMutexLock(lockarray[type]);
	}
	else {
		wzMutexUnlock(lockarray[type]);
	}
}

#  if OPENSSL_VERSION_NUMBER >= 0x10000000
// for CRYPTO_THREADID_set_callback API
static void thread_id(CRYPTO_THREADID *id)
{
	CRYPTO_THREADID_set_numeric(id, (unsigned long)wzThreadID(nullptr));
}
#  else
// for old CRYPTO_set_id_callback API
static unsigned long thread_id(void)
{
  unsigned long ret;

  ret = (unsigned long)wzThreadID(nullptr);
  return ret;
}
#  endif

static void init_locks(void)
{
	lockarray.resize(CRYPTO_num_locks());

	for(size_t i = 0; i < lockarray.size(); i++) {
		lockarray[i] = wzMutexCreate();
	}
#  if OPENSSL_VERSION_NUMBER >= 0x10000000
	CRYPTO_THREADID_set_callback(thread_id);
#  else
	CRYPTO_set_id_callback(thread_id);
#  endif
	CRYPTO_set_locking_callback(lock_callback);
}

static void kill_locks(void)
{
	CRYPTO_set_locking_callback(NULL);
	for(size_t i = 0; i < lockarray.size(); i++) {
		wzMutexDestroy(lockarray[i]);
	}

	lockarray.clear();
}
# else // no OpenSSL lock callbacks needed
# define init_locks()
# define kill_locks()
# endif
#elif defined(USE_OLD_GNUTLS_LOCKS_INIT)
# include <gcrypt.h>
# include <errno.h>

GCRY_THREAD_OPTION_PTHREAD_IMPL;

void init_locks(void)
{
  gcry_control(GCRYCTL_SET_THREAD_CBS);
}

# define kill_locks()
#else
# define init_locks()
# define kill_locks()
#endif

struct SemVer
{
	unsigned long maj;
	unsigned long min;
	unsigned long rev;

	SemVer()
	: maj(0)
	, min(0)
	, rev(0)
	{ }

	SemVer(unsigned long maj, unsigned long min, unsigned long rev)
	: maj(maj)
	, min(min)
	, rev(rev)
	{ }

	bool operator <(const SemVer& b) {
		return (maj < b.maj) ||
				((maj == b.maj) && (
					(min < b.min) ||
					((min == b.min) && (rev < b.rev))
				));
	}
	bool operator <=(const SemVer& b) {
		return (maj < b.maj) ||
				((maj == b.maj) && (
					(min < b.min) ||
					((min == b.min) && (rev <= b.rev))
				));
	}
	bool operator >(const SemVer& b) {
		return !(*this <= b);
	}
	bool operator >=(const SemVer& b) {
		return !(*this < b);
	}
};

bool verify_curl_ssl_thread_safe_setup()
{
	// verify SSL backend version (if possible explicit thread-safety locks setup is required)
	const curl_version_info_data * info = curl_version_info(CURLVERSION_NOW);
	std::cmatch cm;

	const char* ssl_version_str = info->ssl_version;

	// GnuTLS
	std::regex gnutls_regex("^GnuTLS\\/([\\d]+)\\.([\\d]+)\\.([\\d]+).*");
	std::regex_match(ssl_version_str, cm, gnutls_regex);
	if(!cm.empty())
	{
		SemVer version;
		try {
			version.maj = std::stoul(cm[1]);
			version.min = std::stoul(cm[2]);
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert string to unsigned long because of error: %s", e.what());
		}

		// explicit gcry_control() is required when GnuTLS < 2.11.0
		SemVer min_safe_version = {2, 11, 0};
		if (version >= min_safe_version)
		{
			return true;
		}

		// explicit gcry_control() is required when GnuTLS < 2.11.0
#if defined(USE_OPENSSL_LOCKS_INIT) || !defined(USE_OLD_GNUTLS_LOCKS_INIT)
		// but we didn't initialize it
		return false;
#else
		// and we *did* initialize it
		return true;
#endif
	}

	// OpenSSL, libressl, BoringSSL
	std::regex e("^(OpenSSL|libressl|BoringSSL)\\/([\\d]+)\\.([\\d]+)\\.([\\d]+).*");
	std::regex_match(ssl_version_str, cm, e);
	if(!cm.empty())
	{
		ASSERT(cm.size() == 5, "Unexpected # of match results: %zu", cm.size());
		std::string variant = cm[1];
		SemVer version;
		try {
			version.maj = std::stoul(cm[2]);
			version.min = std::stoul(cm[3]);
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert string to unsigned long because of error: %s", e.what());
		}
		if (variant == "OpenSSL")
		{
			// for OpenSSL < 1.1.0, callbacks must be set
			SemVer min_safe_version = {1, 1, 0};
			if (version >= min_safe_version)
			{
				return true;
			}
			// for OpenSSL < 1.1.0, callbacks must be set
#if !defined(USE_OPENSSL_LOCKS_INIT)
			// but we didn't set them
			return false;
#else
			// and we *did* set them
			return true;
#endif
		}
		else
		{
			// TODO: Handle libressl, BoringSSL
		}
	}

	// otherwise, ssl backend should be thread-safe automatically
	return true;
}

// MARK: - Handle progress callbacks

#if LIBCURL_VERSION_NUM >= 0x073d00
/* In libcurl 7.61.0, support was added for extracting the time in plain
   microseconds. Older libcurl versions are stuck in using 'double' for this
   information so we complicate this example a bit by supporting either
   approach. */
#define TIME_IN_US 1
#define TIMETYPE curl_off_t
#define TIMEOPT CURLINFO_TOTAL_TIME_T
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3000000
#else
#define TIMETYPE double
#define TIMEOPT CURLINFO_TOTAL_TIME
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3
#endif

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000
#define MAXIMUM_DOWNLOAD_SIZE                       2147483647L
#define MAXIMUM_IN_MEMORY_DOWNLOAD_SIZE				1 << 29		// 512 MB, default max download limit

class URLTransferRequest;

struct myprogress {
  TIMETYPE lastruntime = (TIMETYPE)0; /* type depends on version, see above */
  URLTransferRequest *request;
};

// MARK: -

static size_t WriteMemoryCallback_URLTransferRequest(void *contents, size_t size, size_t nmemb, void *userp);
static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
#if LIBCURL_VERSION_NUM < 0x072000
/* for libcurl older than 7.32.0 (CURLOPT_PROGRESSFUNCTION) */
static int older_progress(void *p, double dltotal, double dlnow, double ultotal, double ulnow);
#endif
static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata);

class CaseInsensitiveHash
{
public:
	size_t operator() (std::string key) const
	{
		std::transform(key.begin(), key.end(), key.begin(), [](char c) { return std::tolower(c); });
		return std::hash<std::string>{}(key);
	}
};
class CaseInsensitiveEqualFunc
{
public:
	bool operator() (std::string lhs, std::string rhs) const
	{
		std::transform(lhs.begin(), lhs.end(), lhs.begin(), [](char c) { return std::tolower(c); });
		std::transform(rhs.begin(), rhs.end(), rhs.begin(), [](char c) { return std::tolower(c); });
		return lhs == rhs;
	}
};

typedef std::unordered_map<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqualFunc> ResponseHeaderContainer;

class HTTPResponseHeadersContainer: public HTTPResponseHeaders {
public:
	virtual bool hasHeader(const std::string& name) const override
	{
		return responseHeaders.count(name) > 0;
	}
	virtual bool getHeader(const std::string& name, std::string& output_value) const override
	{
		const auto it = responseHeaders.find(name);
		if (it == responseHeaders.end()) { return false; }
		output_value = it->second;
		return true;
	}
public:
	ResponseHeaderContainer responseHeaders;
};

#if LIBCURL_VERSION_NUM >= 0x071000	// cURL 7.16.0+
static int sockopt_callback(void *clientp, curl_socket_t curlfd,
							curlsocktype purpose)
{
#if defined(WZ_OS_UNIX)
	// Set FD_CLOEXEC flag
	int sockopts = fcntl(curlfd, F_SETFD);
	if (sockopts != -1)
	{
		sockopts |= FD_CLOEXEC;
		if (fcntl(curlfd, F_SETFD, sockopts) == -1)
		{
			// Failed to set FD_CLOEXEC
			// Ignore and continue
		}
	}
#elif defined(WZ_OS_WIN)
	if (::SetHandleInformation((HANDLE)curlfd, HANDLE_FLAG_INHERIT, 0) == 0)
	{
		// Failed to set HANDLE_FLAG_INHERIT to 0
		// Ignore and continue
	}
#endif
	return CURL_SOCKOPT_OK;
}
#endif // LIBCURL_VERSION_NUM >= 0x071000	// cURL 7.16.0+

class URLTransferRequest
{
public:
	CURL *handle = nullptr;
	struct myprogress progress;
	std::shared_ptr<AsyncRequestImpl> requestHandle;

public:
	URLTransferRequest(const std::shared_ptr<AsyncRequestImpl>& requestHandle)
	: requestHandle(requestHandle)
	{ }

	virtual ~URLTransferRequest() {
		if (request_header_list != nullptr)
		{
			curl_slist_free_all(request_header_list);
		}
	}

	virtual const std::string& url() const = 0;
	virtual InternetProtocol protocol() const = 0;
	virtual bool noProxy() const = 0;
	virtual const std::unordered_map<std::string, std::string>& requestHeaders() const = 0;
	virtual curl_off_t maxDownloadSize() const { return MAXIMUM_DOWNLOAD_SIZE; }

	virtual CURL* createCURLHandle()
	{
		// Create cURL easy handle
		handle = curl_easy_init();
		if (!handle)
		{
			// Something went wrong with curl_easy_init
			return nullptr;
		}
		curl_easy_setopt(handle, CURLOPT_URL, url().c_str());

#if LIBCURL_VERSION_NUM >= 0x071000	// cURL 7.16.0+
		curl_easy_setopt(handle, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
#endif

#if LIBCURL_VERSION_NUM >= 0x070A08 // CURLOPT_IPRESOLVE is available since cURL 7.10.8
		switch (protocol())
		{
			case InternetProtocol::IP_ANY:
				// default - do nothing
				break;
			case InternetProtocol::IPv4:
				curl_easy_setopt(handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
				break;
			case InternetProtocol::IPv6:
				const curl_version_info_data * info = curl_version_info(CURLVERSION_NOW);
				if (info && ((info->features & CURL_VERSION_IPV6) != CURL_VERSION_IPV6))
				{
					// cURL was not compiled with IPv6 support - fail out
					curl_easy_cleanup(handle);
					handle = nullptr;
					return nullptr;
				}
				curl_easy_setopt(handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
				break;
		}
#endif
		if (noProxy())
		{
#if LIBCURL_VERSION_NUM >= 0x071304	// cURL 7.19.4+
			CURLcode proxyResult = curl_easy_setopt(handle, CURLOPT_NOPROXY, "*");
			if (proxyResult != CURLE_OK)
			{
				// Failed to disable proxy
				wzAsyncExecOnMainThread([proxyResult]{
					debug(LOG_NET, "cURL: Failed to set CURLOPT_NOPROXY: %d", static_cast<int>(proxyResult));
				});
			}
#else
			wzAsyncExecOnMainThread([]{
				debug(LOG_NET, "cURL: CURLOPT_NOPROXY is not supported");
			});
#endif
		}

		const auto& _requestHeaders = requestHeaders();
		for (auto it : _requestHeaders)
		{
			const auto& header_key = it.first;
			const auto& header_value = it.second;
			std::string header_line = header_key + ": " + header_value;
			request_header_list = curl_slist_append(request_header_list, header_line.c_str());
		}
		if (request_header_list != nullptr)
		{
			curl_easy_setopt(handle, CURLOPT_HTTPHEADER, request_header_list);
		}

		if (hasWriteMemoryCallback())
		{
			curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback_URLTransferRequest);
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)this);
		}
		curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, header_callback);
		curl_easy_setopt(handle, CURLOPT_HEADERDATA, (void *)this);
	#if LIBCURL_VERSION_NUM >= 0x075500		// cURL 7.85.0+
		/* only allow HTTP and HTTPS */
		curl_easy_setopt(handle, CURLOPT_PROTOCOLS_STR, "http,https");
	#elif LIBCURL_VERSION_NUM >= 0x071304	// cURL 7.19.4+
		/* only allow HTTP and HTTPS */
		curl_easy_setopt(handle, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
	#endif
		/* tell libcurl to follow redirection */
		curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
		/* set max redirects */
		curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 10L);

		progress.request = this;
	#if LIBCURL_VERSION_NUM >= 0x072000
		/* xferinfo was introduced in 7.32.0, no earlier libcurl versions will
		   compile as they won't have the symbols around.

		   If built with a newer libcurl, but running with an older libcurl:
		   curl_easy_setopt() will fail in run-time trying to set the new
		   callback, making the older callback get used.

		   New libcurls will prefer the new callback and instead use that one even
		   if both callbacks are set. */

		curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, xferinfo);
		/* pass the struct pointer into the xferinfo function, note that this is
		   an alias to CURLOPT_PROGRESSDATA */
		curl_easy_setopt(handle, CURLOPT_XFERINFODATA, &progress);
	#else
		curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, older_progress);
		/* pass the struct pointer into the progress function */
		curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &progress);
	#endif

		curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);

	#if LIBCURL_VERSION_NUM >= 0x070B00	// cURL 7.11.0+
		/* refuse to download if larger than limit */
		curl_off_t downloadSizeLimit = maxDownloadSize();
		if (downloadSizeLimit > 0)
		{
			curl_easy_setopt(handle, CURLOPT_MAXFILESIZE_LARGE, downloadSizeLimit);
		}
	#endif

	#if LIBCURL_VERSION_NUM >= 0x070A00	// cURL 7.10.0+
		curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
	#endif

		/* abort if slower than 30 bytes/sec during 60 seconds */
		curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, 60L);
		curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, 30L);

		return handle;
	}

	virtual bool hasWriteMemoryCallback() { return false; }
	virtual size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb) { return 0; }

	virtual bool onProgressUpdate(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow) { return false; }

	virtual bool waitOnShutdown() const { return false; }

	virtual void handleRequestDone(CURLcode result) { }
	virtual void requestFailedToFinish(URLRequestFailureType type) { }

protected:
	friend size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata);
	std::shared_ptr<HTTPResponseHeadersContainer> responseHeaders = std::make_shared<HTTPResponseHeadersContainer>();
private:
	struct curl_slist *request_header_list = nullptr;
};

static size_t
WriteMemoryCallback_URLTransferRequest(void *contents, size_t size, size_t nmemb, void *userp)
{
	// expects that userp will be the URLTransferRequest
	if (!userp)
	{
		return 0;
	}

	URLTransferRequest* pRequest = static_cast<URLTransferRequest*>(userp);
	return pRequest->writeMemoryCallback(contents, size, nmemb);
}

/* this is how the CURLOPT_XFERINFOFUNCTION callback works */
static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
	int retValue = 0;
	struct myprogress *myp = (struct myprogress *)p;
	CURL *curl = myp->request->handle;
	TIMETYPE curtime = 0;

	curl_easy_getinfo(curl, TIMEOPT, &curtime);

	/* under certain circumstances it may be desirable for certain functionality
	 to only run every N seconds, in order to do this the transaction time can
	 be used */
	if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
		myp->lastruntime = curtime;
//		#ifdef TIME_IN_US
//		fprintf(stderr, "TOTAL TIME: %" CURL_FORMAT_CURL_OFF_T ".%06ld\r\n",
//				(curtime / 1000000), (long)(curtime % 1000000));
//		#else
//		fprintf(stderr, "TOTAL TIME: %f \r\n", curtime);
//		#endif

		retValue = myp->request->onProgressUpdate(dltotal, dlnow, ultotal, ulnow);
	}

	// prevent infinite download denial-of-service
	if(dlnow > myp->request->maxDownloadSize())
		return 1;
	return retValue;
}

#if LIBCURL_VERSION_NUM < 0x072000
/* for libcurl older than 7.32.0 (CURLOPT_PROGRESSFUNCTION) */
static int older_progress(void *p,
                          double dltotal, double dlnow,
                          double ultotal, double ulnow)
{
  return xferinfo(p,
                  (curl_off_t)dltotal,
                  (curl_off_t)dlnow,
                  (curl_off_t)ultotal,
                  (curl_off_t)ulnow);
}
#endif

static void trim_str(std::string& str)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int c) {
		return !std::isspace(c);
	}));
	str.erase(std::find_if(str.rbegin(), str.rend(), [](int c) {
		return !std::isspace(c);
	}).base(), str.end());
}

static size_t header_callback(char *buffer, size_t size,
							  size_t nitems, void *userdata)
{
	/* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
	/* 'userdata' is set with CURLOPT_HEADERDATA */
	if (userdata != nullptr && buffer != nullptr)
	{
		URLTransferRequest* pRequest = static_cast<URLTransferRequest*>(userdata);
		std::string header_line = std::string(buffer, size * nitems);
		const size_t header_separator = header_line.find_first_of(":");
		if (header_separator != std::string::npos)
		{
			std::string header_key = header_line.substr(0, header_separator);
			std::string header_value = header_line.substr(header_separator + 1);
			trim_str(header_key);
			trim_str(header_value);
			pRequest->responseHeaders->responseHeaders[header_key] = header_value;
		}
	}
	return nitems * size;
}

class RunningURLTransferRequestBase : public URLTransferRequest
{
public:
	virtual const URLRequestBase& getBaseRequest() const = 0;
public:
	RunningURLTransferRequestBase(const std::shared_ptr<AsyncRequestImpl>& requestHandle)
	: URLTransferRequest(requestHandle)
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

	virtual bool onProgressUpdate(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow) override
	{
		auto request = getBaseRequest();
		if (request.progressCallback)
		{
			request.progressCallback(request.url, dltotal, dlnow);
		}
		return false;
	}

	virtual void handleRequestDone(CURLcode result) override
	{
		long code;
		if (curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code) != CURLE_OK)
		{
			code = 0;
		}

		if (result == CURLE_OK)
		{
			onSuccess(HTTPResponseDetails(result, code, responseHeaders));
		}
		else
		{
			onFailure(URLRequestFailureType::TRANSFER_FAILED, HTTPResponseDetails(result, code, responseHeaders));
		}
	}

	virtual void requestFailedToFinish(URLRequestFailureType type) override
	{
		ASSERT_OR_RETURN(, type != URLRequestFailureType::TRANSFER_FAILED, "TRANSFER_FAILED should be handled by handleRequestDone");
		onFailure(type, nullopt);
	}

private:
	virtual void onSuccess(const HTTPResponseDetails& responseDetails) = 0;

	void onFailure(URLRequestFailureType type, optional<HTTPResponseDetails> transferDetails)
	{
		auto request = getBaseRequest();

		const std::string& url = request.url;
		switch (type)
		{
			case URLRequestFailureType::INITIALIZE_REQUEST_ERROR:
				wzAsyncExecOnMainThread([url]{
					debug(LOG_NET, "cURL: Failed to initialize request for (%s)", url.c_str());
				});
				break;
			case URLRequestFailureType::TRANSFER_FAILED:
				if (!transferDetails.has_value())
				{
					wzAsyncExecOnMainThread([url]{
						debug(LOG_ERROR, "cURL: Request for (%s) failed - but no transfer failure details provided!", url.c_str());
					});
				}
				else
				{
					CURLcode result = transferDetails->curlResult();
					long httpStatusCode = transferDetails->httpStatusCode();
					wzAsyncExecOnMainThread([url, result, httpStatusCode]{
						debug(LOG_NET, "cURL: Request for (%s) failed with error %d, and HTTP response code: %ld", url.c_str(), result, httpStatusCode);
					});
				}
				break;
			case URLRequestFailureType::CANCELLED:
				wzAsyncExecOnMainThread([url]{
					debug(LOG_NET, "cURL: Request for (%s) was cancelled", url.c_str());
				});
				break;
			case URLRequestFailureType::CANCELLED_BY_SHUTDOWN:
				wzAsyncExecOnMainThread([url]{
					debug(LOG_NET, "cURL: Request for (%s) was cancelled by application shutdown", url.c_str());
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
	curl_off_t _maxDownloadSize = MAXIMUM_IN_MEMORY_DOWNLOAD_SIZE; // 512 MB, default max download limit

public:
	RunningURLDataRequest(const URLDataRequest& request, const std::shared_ptr<AsyncRequestImpl>& requestHandle)
	: RunningURLTransferRequestBase(requestHandle)
	, request(request)
	{
		chunk = std::make_shared<MemoryStruct>();
		if (request.maxDownloadSizeLimit > 0)
		{
			_maxDownloadSize = std::min(request.maxDownloadSizeLimit, static_cast<curl_off_t>(MAXIMUM_IN_MEMORY_DOWNLOAD_SIZE));
		}
	}

	virtual const URLRequestBase& getBaseRequest() const override
	{
		return request;
	}

	virtual curl_off_t maxDownloadSize() const override
	{
		// For downloading to memory, set a lower default max download limit
		return _maxDownloadSize;
	}

	virtual bool hasWriteMemoryCallback() override { return true; }
	virtual size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb) override
	{
		size_t realsize = size * nmemb;
		MemoryStruct *mem = chunk.get();

		char *ptr = (char*) realloc(mem->memory, mem->size + realsize + 1);
		if(ptr == NULL) {
			/* out of memory! */
			return 0;
		}

		mem->memory = ptr;
		memcpy(&(mem->memory[mem->size]), contents, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;

		return realsize;
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

class RunningURLFileDownloadRequest : public RunningURLTransferRequestBase
{
public:
	URLFileDownloadRequest request;
	FILE *outFile;

	RunningURLFileDownloadRequest(const URLFileDownloadRequest& request, const std::shared_ptr<AsyncRequestImpl>& requestHandle)
	: RunningURLTransferRequestBase(requestHandle)
	, request(request)
	{
#if defined(WZ_OS_WIN)
		// On Windows, path strings passed to fopen() are interpreted using the ANSI or OEM codepage
		// (and not as UTF-8). To support Unicode paths, the string must be converted to a wide-char
		// string and passed to _wfopen.
		int wstr_len = MultiByteToWideChar(CP_UTF8, 0, request.outFilePath.c_str(), -1, NULL, 0);
		if (wstr_len <= 0)
		{
			DWORD dwError = GetLastError();
			std::ostringstream errMsg;
			errMsg << "Could not not convert string from UTF-8; MultiByteToWideChar failed with error " << dwError << ": " << request.outFilePath;
			throw std::runtime_error(errMsg.str());
			return;
		}
		std::vector<wchar_t> wstr_filename(wstr_len, 0);
		if (MultiByteToWideChar(CP_UTF8, 0, request.outFilePath.c_str(), -1, &wstr_filename[0], wstr_len) == 0)
		{
			DWORD dwError = GetLastError();
			std::ostringstream errMsg;
			errMsg << "Could not not convert string from UTF-8; MultiByteToWideChar[2] failed with error " << dwError << ": " << request.outFilePath;
			throw std::runtime_error(errMsg.str());
			return;
		}
		outFile = _wfopen(&wstr_filename[0], L"wb");
#else
		outFile = fopen(request.outFilePath.c_str(), "wb");
#endif
		if (!outFile)
		{
			std::ostringstream errMsg;
			errMsg << "Could not not open file for output: " << request.outFilePath;
			throw std::runtime_error(errMsg.str());
		}

#if defined(WZ_OS_UNIX)
		int fd = fileno(outFile);
		if (fd != -1)
		{
			fcntl(fd, F_SETFD, FD_CLOEXEC);
		}
#endif
	}

	virtual const URLRequestBase& getBaseRequest() const override
	{
		return request;
	}

	virtual bool hasWriteMemoryCallback() override { return true; }
	virtual size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb) override
	{
		if (!outFile) return 0;
		size_t written = fwrite(contents, size, nmemb, outFile);
		return written;
	}

	virtual void handleRequestDone(CURLcode result) override
	{
		if (outFile)
		{
			fclose(outFile);
		}

		RunningURLTransferRequestBase::handleRequestDone(result);
	}

private:
	void onSuccess(const HTTPResponseDetails& responseDetails) override
	{
		if (request.onSuccess)
		{
			request.onSuccess(request.url, responseDetails, request.outFilePath);
		}
	}
};

static std::list<URLTransferRequest*> newUrlRequests;

/** This runs in a separate thread */
static int urlRequestThreadFunc(void *)
{
	// initialize cURL
	CURLM *multi_handle = curl_multi_init();

	std::list<URLTransferRequest*> runningTransfers;
	int transfers_running = 0;
	CURLMcode multiCode = CURLM_OK;

	auto performTransfers = [&]() -> bool {
		multiCode = curl_multi_perform ( multi_handle, &transfers_running );
		if (multiCode == CURLM_OK )
		{
			/* wait for activity, timeout or "nothing" */
		#if LIBCURL_VERSION_NUM >= 0x071C00	// cURL 7.28.0+ required for curl_multi_wait
			multiCode = curl_multi_wait ( multi_handle, NULL, 0, 1000, NULL);
		#else
			#error "Needs a fallback for lack of curl_multi_wait (or, even better, update libcurl!)"
		#endif
		}
		if (multiCode != CURLM_OK)
		{
			fprintf(stderr, "curl_multi failed, code %d.\n", multiCode);
			return false;
		}

		// Handle finished transfers
		CURLMsg *msg; /* for picking up messages with the transfer status */
		int msgs_left; /* how many messages are left */
		while((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
			if(msg->msg == CURLMSG_DONE) {
				CURL *e = msg->easy_handle;
				CURLcode result = msg->data.result;

				// printf("HTTP transfer completed with status %d\n", msg->data.result);

				/* Find out which handle this message is about */
				auto it = std::find_if(runningTransfers.begin(), runningTransfers.end(), [e](const URLTransferRequest* req) {
					return req->handle == e;
				});

				if (it != runningTransfers.end())
				{
					URLTransferRequest* urlTransfer = *it;
					urlTransfer->handleRequestDone(result);
					delete urlTransfer;
					runningTransfers.erase(it);
				}
				else
				{
					// log failure to find request in running transfers list
					wzAsyncExecOnMainThread([]{
						debug(LOG_ERROR, "Failed to find request in running transfers list");
					});
				}

				curl_multi_remove_handle(multi_handle, e);
				curl_easy_cleanup(e);
			}
		}

		return true;
	};

	wzMutexLock(urlRequestMutex);
	while (!urlRequestQuit)
	{
		if (newUrlRequests.empty() && transfers_running == 0)
		{
			wzMutexUnlock(urlRequestMutex);
			wzSemaphoreWait(urlRequestSemaphore);  // Go to sleep until needed.
			wzMutexLock(urlRequestMutex);
			continue;
		}

		// Start handling new requests
		while(!newUrlRequests.empty())
		{
			URLTransferRequest* newRequest = newUrlRequests.front();
			newUrlRequests.pop_front();

			// Create cURL easy handle
			if (!newRequest->createCURLHandle())
			{
				// something went wrong creating the handle
				newRequest->requestFailedToFinish(URLRequestFailureType::INITIALIZE_REQUEST_ERROR);
				delete newRequest;
				continue; // skip to next request
			}

			// Add to multi-handle
			multiCode = curl_multi_add_handle(multi_handle, newRequest->handle);
			if (multiCode != CURLM_OK)
			{
				// curl_multi_add_handle failed
				wzAsyncExecOnMainThread([multiCode]{
					debug(LOG_ERROR, "curl_multi_add_handle failed: %d", multiCode);
				});
				newRequest->requestFailedToFinish(URLRequestFailureType::INITIALIZE_REQUEST_ERROR);
				curl_easy_cleanup(newRequest->handle);
				delete newRequest;
				continue; // skip to next request
			}
			runningTransfers.push_back(newRequest);
		}
		newUrlRequests.clear();

		// cancel any requests that have been signaled to cancel
		auto it = runningTransfers.begin();
		while (it != runningTransfers.end())
		{
			URLTransferRequest* runningTransfer = *it;
			if (runningTransfer->requestHandle->cancelFlag) // must be checked while holding the urlRequestMutex
			{
				curl_multi_remove_handle(multi_handle, runningTransfer->handle);
				runningTransfer->requestFailedToFinish(URLRequestFailureType::CANCELLED);
				curl_easy_cleanup(runningTransfer->handle);
				delete runningTransfer;
				it = runningTransfers.erase(it);
			}
			else
			{
				++it;
			}
		}

		wzMutexUnlock(urlRequestMutex); // when performing / waiting on curl requests, unlock the mutex

		performTransfers();

		wzMutexLock(urlRequestMutex);
	}
	wzMutexUnlock(urlRequestMutex);

	auto it = runningTransfers.begin();
	while (it != runningTransfers.end())
	{
		URLTransferRequest* runningTransfer = *it;
		if (!runningTransfer->waitOnShutdown())
		{
			curl_multi_remove_handle(multi_handle, runningTransfer->handle);
			runningTransfer->requestFailedToFinish(URLRequestFailureType::CANCELLED_BY_SHUTDOWN);
			curl_easy_cleanup(runningTransfer->handle);
			delete runningTransfer;
			it = runningTransfers.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Wait for all remaining ("waitOnShutdown") transfers to complete
	auto waitForTransfersStart = std::chrono::high_resolution_clock::now();
	while (!runningTransfers.empty())
	{
		performTransfers();
		auto currentWaitDuration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - waitForTransfersStart);
		if (currentWaitDuration.count() > MAX_WAIT_ON_SHUTDOWN_SECONDS)
		{
			printf("Failed to finish all \"waitOnShutdown\" transfers before timeout: %d\n", MAX_WAIT_ON_SHUTDOWN_SECONDS);
			break;
		}
	}

	for (auto runningTransfer : runningTransfers)
	{
		curl_multi_remove_handle(multi_handle, runningTransfer->handle);
		runningTransfer->requestFailedToFinish(URLRequestFailureType::CANCELLED_BY_SHUTDOWN);
		curl_easy_cleanup(runningTransfer->handle);
		delete runningTransfer;
	}
	runningTransfers.clear();

	curl_multi_cleanup(multi_handle);
	return 0;
}

AsyncURLRequestHandle urlRequestData(const URLDataRequest& request)
{
	ASSERT_OR_RETURN(nullptr, !request.url.empty(), "A valid request must specify a URL");
	ASSERT_OR_RETURN(nullptr, request.maxDownloadSizeLimit <= (curl_off_t)MAXIMUM_IN_MEMORY_DOWNLOAD_SIZE, "Requested maxDownloadSizeLimit exceeds maximum in-memory download size limit %zu", (size_t)MAXIMUM_IN_MEMORY_DOWNLOAD_SIZE);
	std::shared_ptr<AsyncRequestImpl> requestHandle = std::make_shared<AsyncRequestImpl>();
	RunningURLDataRequest *pNewRequest = new RunningURLDataRequest(request, requestHandle);
	wzMutexLock(urlRequestMutex);
	bool isFirstJob = newUrlRequests.empty();
	newUrlRequests.push_back(pNewRequest);
	wzMutexUnlock(urlRequestMutex);

	if (isFirstJob)
	{
		wzSemaphorePost(urlRequestSemaphore);  // Wake up processing thread.
	}
	return requestHandle;
}

AsyncURLRequestHandle urlDownloadFile(const URLFileDownloadRequest& request)
{
	ASSERT_OR_RETURN(nullptr, !request.url.empty(), "A valid request must specify a URL");
	ASSERT_OR_RETURN(nullptr, !request.outFilePath.empty(), "A valid request must specify an output filepath");
	RunningURLFileDownloadRequest *pNewRequest = nullptr;
	std::shared_ptr<AsyncRequestImpl> requestHandle = std::make_shared<AsyncRequestImpl>();
	try {
		pNewRequest = new RunningURLFileDownloadRequest(request, requestHandle);
	}
	catch (const std::exception &e)
	{
		debug(LOG_ERROR, "Failed to create new file download request with error: %s", e.what());
		if (request.onFailure)
		{
			request.onFailure(request.url, URLRequestFailureType::INITIALIZE_REQUEST_ERROR, nullopt);
		}
		return nullptr;
	}
	wzMutexLock(urlRequestMutex);
	bool isFirstJob = newUrlRequests.empty();
	newUrlRequests.push_back(pNewRequest);
	wzMutexUnlock(urlRequestMutex);

	if (isFirstJob)
	{
		wzSemaphorePost(urlRequestSemaphore);  // Wake up processing thread.
	}
	return requestHandle;
}

void urlRequestSetCancelFlag(AsyncURLRequestHandle requestHandle)
{
	std::shared_ptr<AsyncRequestImpl> pRequestImpl = std::dynamic_pointer_cast<AsyncRequestImpl>(requestHandle);
	if (pRequestImpl)
	{
		wzMutexLock(urlRequestMutex);
		pRequestImpl->cancelFlag = true;
		wzMutexUnlock(urlRequestMutex);
	}
}

#if LIBCURL_VERSION_NUM >= 0x073800 // cURL 7.56.0+
std::vector<std::pair<std::string, curl_sslbackend>> listSSLBackends()
{
	// List available cURL SSL backends
	std::vector<std::pair<std::string, curl_sslbackend>> output;

	const curl_ssl_backend **list;
	int i;
	CURLsslset result = curl_global_sslset((curl_sslbackend)-1, NULL, &list);
	if(result != CURLSSLSET_UNKNOWN_BACKEND)
	{
		return output;
	}
	for(i = 0; list[i]; i++)
	{
		output.push_back({list[i]->name, list[i]->id});
	}
	return output;
}
#endif

void urlRequestOutputDebugInfo()
{
	const curl_version_info_data * info = curl_version_info(CURLVERSION_NOW);
	if (info->age >= 0)
	{
		debug(LOG_WZ, "cURL: %s", info->version);
		debug(LOG_WZ, "- SSL: %s", info->ssl_version);

		#if LIBCURL_VERSION_NUM >= 0x073800 // cURL 7.56.0+
		if (!!(info->features & CURL_VERSION_MULTI_SSL))
		{
			auto availableSSLBackends = listSSLBackends();
			std::string sslBackendStr;
			for (const auto &backend : availableSSLBackends)
			{
				if (!sslBackendStr.empty())
				{
					sslBackendStr += ",";
				}
				sslBackendStr += backend.first;
			}
			debug(LOG_WZ, "- Available SSL backends: %s", sslBackendStr.c_str());
		}
		#endif
		#if LIBCURL_VERSION_NUM >= 0x070A07 // cURL 7.10.7+
		debug(LOG_WZ, "- AsynchDNS: %d", !!(info->features & CURL_VERSION_ASYNCHDNS));
		#endif
	}

	if (info->age >= 1)
	{
		if (info->ares)
		{
			debug(LOG_WZ, "- ares: %s", info->ares);
		}
	}
	if (info->age >= 2)
	{
		if (info->libidn)
		{
			debug(LOG_WZ, "- libidn: %s", info->libidn);
		}
	}
}

void urlSelectSSLBackend()
{
#if LIBCURL_VERSION_NUM >= 0x073800 // cURL 7.56.0+
	auto availableSSLBackends = listSSLBackends();
	if (availableSSLBackends.empty())
	{
		// Could not retrieve ssl backend list - libcurl is probably a slightly older version built with only a single ssl backend
		return;
	}
	// Note: Use CURLSSLBACKEND_DARWINSSL instead of CURLSSLBACKEND_SECURETRANSPORT to support older cURL versions
	const std::vector<curl_sslbackend> backendPreferencesOrder = {CURLSSLBACKEND_SCHANNEL, CURLSSLBACKEND_DARWINSSL, CURLSSLBACKEND_GNUTLS, CURLSSLBACKEND_NSS};
	std::vector<curl_sslbackend> ignoredBackends;
#if !defined(USE_OPENSSL_LOCKS_INIT) && !defined(CURL_OPENSSL_DOES_NOT_REQUIRE_LOCKS_INIT)
	// Did not compile with support for thread-safety / locks for OpenSSL, so ignore it
	ignoredBackends.push_back(CURLSSLBACKEND_OPENSSL);
#endif
#if !defined(USE_OLD_GNUTLS_LOCKS_INIT) && !defined(CURL_GNUTLS_DOES_NOT_REQUIRE_LOCKS_INIT)
	// Did not compile with support for thread-safety / locks for GnuTLS, so ignore it
	ignoredBackends.push_back(CURLSSLBACKEND_GNUTLS);
#endif
	if (!ignoredBackends.empty())
	{
		availableSSLBackends.erase(std::remove_if(availableSSLBackends.begin(),
					   availableSSLBackends.end(),
					   [ignoredBackends](const std::pair<std::string, curl_sslbackend>& backend)
		{
			return std::find(ignoredBackends.begin(), ignoredBackends.end(), backend.second) != ignoredBackends.end();
		}),
		availableSSLBackends.end());
	}
	if (!availableSSLBackends.empty())
	{
		curl_sslbackend sslBackendChoice = availableSSLBackends.front().second;
		for (const auto& preferredBackend : backendPreferencesOrder)
		{
			auto it = std::find_if(availableSSLBackends.begin(), availableSSLBackends.end(), [preferredBackend](const std::pair<std::string, curl_sslbackend>& backend) {
				return backend.second == preferredBackend;
			});
			if (it != availableSSLBackends.end())
			{
				sslBackendChoice = it->second;
				break;
			}
		}
		if (sslBackendChoice != CURLSSLBACKEND_NONE)
		{
			CURLsslset result = curl_global_sslset(sslBackendChoice, nullptr, nullptr);
			if (result == CURLSSLSET_OK)
			{
				debug(LOG_WZ, "cURL: selected SSL backend: %d", sslBackendChoice);
			}
			else
			{
				debug(LOG_WZ, "cURL: failed to select SSL backend (%d) with error: %d", sslBackendChoice, result);
			}
		}
	}
	else
	{
		debug(LOG_INFO, "cURL has no available thread-safe SSL backends to configure");
		for (const auto& ignoredBackend : ignoredBackends)
		{
			debug(LOG_INFO, "(Ignored backend: %d (build did not permit thread-safe configuration)", ignoredBackend);
		}
	}
#else
	/* do nothing */
	return;
#endif
}

void urlRequestInit()
{
	/* Must be called *before* curl_global_init */
	urlSelectSSLBackend();

	/* Must initialize libcurl before any threads are started */
	CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
	if (result != CURLE_OK)
	{
		debug(LOG_ERROR, "curl_global_init failed: %d", result);
	}

	init_locks();

	if (!verify_curl_ssl_thread_safe_setup())
	{
		debug(LOG_ERROR, "curl initialized, but ssl backend could not be configured to be thread-safe");
	}

	// start background thread for processing HTTP requests
	urlRequestQuit = false;

	if (!urlRequestThread)
	{
		urlRequestMutex = wzMutexCreate();
		urlRequestSemaphore = wzSemaphoreCreate(0);
		urlRequestThread = wzThreadCreate(urlRequestThreadFunc, nullptr);
		wzThreadStart(urlRequestThread);
	}
}

void urlRequestShutdown()
{
	if (urlRequestThread)
	{
		// Signal the path finding thread to quit
		wzMutexLock(urlRequestMutex);
		urlRequestQuit = true;
		wzMutexUnlock(urlRequestMutex);
		wzSemaphorePost(urlRequestSemaphore);  // Wake up thread.

		wzThreadJoin(urlRequestThread);
		urlRequestThread = nullptr;
		wzMutexDestroy(urlRequestMutex);
		urlRequestMutex = nullptr;
		wzSemaphoreDestroy(urlRequestSemaphore);
		urlRequestSemaphore = nullptr;
	}

	kill_locks();

	curl_global_cleanup();
}
