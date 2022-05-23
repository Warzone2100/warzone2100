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

#include "nethelpers.h"
#include "urlrequest.h"
#include <mutex>
#include <memory>
#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;
#include <nlohmann/json.hpp>
using json = nlohmann::json;

static std::string publicIPv4LookupService = WZ_DEFAULT_PUBLIC_IPv4_LOOKUP_SERVICE_URL;
static std::string publicIPv4LookupServiceJSONKey = WZ_DEFAULT_PUBLIC_IPv4_LOOKUP_SERVICE_JSONKEY;
static std::string publicIPv6LookupService = WZ_DEFAULT_PUBLIC_IPv6_LOOKUP_SERVICE_URL;
static std::string publicIPv6LookupServiceJSONKey = WZ_DEFAULT_PUBLIC_IPv6_LOOKUP_SERVICE_JSONKEY;

void setPublicIPv4LookupService(const std::string& publicIPv4LookupServiceUrl, const std::string& jsonKey)
{
	if (publicIPv4LookupServiceUrl.empty() || jsonKey.empty())
	{
		return;
	}
	publicIPv4LookupService = publicIPv4LookupServiceUrl;
	publicIPv4LookupServiceJSONKey = jsonKey;
}
const std::string& getPublicIPv4LookupServiceUrl()
{
	return publicIPv4LookupService;
}
const std::string& getPublicIPv4LookupServiceJSONKey()
{
	return publicIPv4LookupServiceJSONKey;
}

void setPublicIPv6LookupService(const std::string& publicIPv6LookupServiceUrl, const std::string& jsonKey)
{
	if (publicIPv6LookupServiceUrl.empty() || jsonKey.empty())
	{
		return;
	}
	publicIPv6LookupService = publicIPv6LookupServiceUrl;
	publicIPv6LookupServiceJSONKey = jsonKey;
}
const std::string& getPublicIPv6LookupServiceUrl()
{
	return publicIPv6LookupService;
}
const std::string& getPublicIPv6LookupServiceJSONKey()
{
	return publicIPv6LookupServiceJSONKey;
}

static void requestPublicIPAddress(const std::string& lookupServiceUrl, InternetProtocol protocol, const std::string& jsonResponseKey, const std::function<void (optional<std::string> ipAddressString, optional<std::string> errorString)>& callback)
{
	URLDataRequest request;
	request.url = lookupServiceUrl;
	request.protocol = protocol;
	request.noProxy = true;
	request.onSuccess = [jsonResponseKey, callback](const std::string& url, const HTTPResponseDetails& responseDetails, const std::shared_ptr<MemoryStruct>& data) {

		long httpStatusCode = responseDetails.httpStatusCode();
		if (httpStatusCode != 200)
		{
			std::string errorString = "Request returned HTTP status code: ";
			errorString += std::to_string(httpStatusCode);
			callback(nullopt, errorString);
			return;
		}

		// Parse the remaining json (minus the digital signature)
		json updateData;
		try {
			updateData = json::parse(data->memory, data->memory + data->size);
		}
		catch (const std::exception &e) {
			std::string errorString = "Failed to parse JSON response: ";
			errorString += e.what();
			callback(nullopt, errorString);
			return;
		}

		// Get the IP address from the JSON response
		if (!updateData.is_object())
		{
			callback(nullopt, "JSON response is not an object");
			return;
		}

		auto it = updateData.find(jsonResponseKey);
		if (it == updateData.end() || !it.value().is_string())
		{
			// Missing expected property
			std::string errorString = "JSON object missing expected property: ";
			errorString += jsonResponseKey;
			callback(nullopt, errorString);
			return;
		}

		std::string ipAddressString = it.value().get<std::string>();
		callback(ipAddressString, nullopt);
	};
	request.onFailure = [callback](const std::string& url, URLRequestFailureType type, optional<HTTPResponseDetails> transferDetails) {
		std::string errorString = "Request failed; failure type: ";
		errorString += std::to_string(type);
		if (transferDetails.has_value())
		{
			errorString += "; status code: " + std::to_string(transferDetails.value().httpStatusCode());
		}
		callback(nullopt, errorString);
	};
	request.maxDownloadSizeLimit = 1024 * 1024; // response should never be > 1 MB
	AsyncURLRequestHandle handle = urlRequestData(request);
}

class GetPublicIPAddress {
private:
	IPAddressResultCallbackFunc resultCallback;
	optional<std::string> ipv4;
	optional<std::string> ipv4_error;
	optional<std::string> ipv6;
	optional<std::string> ipv6_error;
	std::mutex ip_mutex;

	bool allRequestsFinished() const {
		return (ipv4.has_value() || ipv4_error.has_value()) && (ipv6.has_value() || ipv6_error.has_value());
	}
	bool dispatchCallbackIfDone() const {
		if (!allRequestsFinished())
		{
			return false;
		}
		std::vector<std::string> errors;
		if (ipv4_error.has_value())
		{
			errors.push_back(std::string("IPv4: ") + ipv4_error.value());
		}
		if (ipv6_error.has_value())
		{
			errors.push_back(std::string("IPv6: ") + ipv6_error.value());
		}
		resultCallback((ipv4.has_value()) ? ipv4.value() : "", (ipv6.has_value()) ? ipv6.value() : "", errors);
		return true;
	}
public:
	GetPublicIPAddress(const IPAddressResultCallbackFunc& resultCallback)
	: resultCallback(resultCallback)
	{ }

public:
	static void getPublicIPAddress(const IPAddressResultCallbackFunc& resultCallback, bool ipv4 = true, bool ipv6 = true)
	{
		std::shared_ptr<GetPublicIPAddress> pPublicIpAddresses = std::make_shared<GetPublicIPAddress>(resultCallback);
		if (!ipv4)
		{
			pPublicIpAddresses->ipv4 = std::string();
		}
		if (!ipv6)
		{
			pPublicIpAddresses->ipv6 = std::string();
		}

		if (ipv4)
		{
			requestPublicIPAddress(publicIPv4LookupService, InternetProtocol::IPv4, publicIPv4LookupServiceJSONKey, [pPublicIpAddresses](optional<std::string> ipAddressString, optional<std::string> errorString){
				std::lock_guard<std::mutex> guard(pPublicIpAddresses->ip_mutex);
				if (!errorString.has_value())
				{
					if (ipAddressString.has_value() && (pPublicIpAddresses->ipv6 != ipAddressString))
					{
						pPublicIpAddresses->ipv4 = ipAddressString;
					}
					else
					{
						pPublicIpAddresses->ipv4 = std::string();
					}
				}
				else
				{
					pPublicIpAddresses->ipv4_error = errorString.value();
				}
				pPublicIpAddresses->dispatchCallbackIfDone();
			});
		}
		if (ipv6)
		{
			requestPublicIPAddress(publicIPv6LookupService, InternetProtocol::IPv6, publicIPv6LookupServiceJSONKey, [pPublicIpAddresses](optional<std::string> ipAddressString, optional<std::string> errorString){
				std::lock_guard<std::mutex> guard(pPublicIpAddresses->ip_mutex);
				if (!errorString.has_value())
				{
					if (ipAddressString.has_value() && (pPublicIpAddresses->ipv4 != ipAddressString))
					{
						pPublicIpAddresses->ipv6 = ipAddressString;
					}
					else
					{
						pPublicIpAddresses->ipv6 = std::string();
					}
				}
				else
				{
					pPublicIpAddresses->ipv6_error = errorString.value();
				}
				pPublicIpAddresses->dispatchCallbackIfDone();
			});
		}
	}
};

void getPublicIPAddress(const IPAddressResultCallbackFunc& resultCallback, bool ipv4 /*= true*/, bool ipv6 /*= true*/)
{
	GetPublicIPAddress::getPublicIPAddress(resultCallback, ipv4, ipv6);
}
