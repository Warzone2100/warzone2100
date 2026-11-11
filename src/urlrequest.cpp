// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#define NOMINMAX
#include "urlrequest.h"
#include "urlrequest_private.h"
#include "lib/framework/wzapp.h"

const char* to_string(URLRequestFailureType val)
{
	switch (val)
	{
		case URLRequestFailureType::INITIALIZE_REQUEST_ERROR: return "INITIALIZE_REQUEST_ERROR";
		case URLRequestFailureType::OPERATION_TIMEOUT: return "OPERATION_TIMEOUT";
		case URLRequestFailureType::COULDNT_CONNECT: return "COULDNT_CONNECT";
		case URLRequestFailureType::TRANSFER_FAILED: return "TRANSFER_FAILED";
		case URLRequestFailureType::CANCELLED: return "CANCELLED";
		case URLRequestFailureType::CANCELLED_BY_SHUTDOWN: return "CANCELLED_BY_SHUTDOWN";
	}
	return nullptr;
}

const char* to_string(InternetProtocol protocol)
{
	switch (protocol)
	{
		case InternetProtocol::IP_ANY: return "IP_ANY";
		case InternetProtocol::IPv4: return "IPv4";
		case InternetProtocol::IPv6: return "IPv6";
	}
	return "";
}

const char* to_string(URLRequestMethod method)
{
	switch (method)
	{
		case URLRequestMethod::Get: return "GET";
		case URLRequestMethod::Post: return "POST";
		case URLRequestMethod::Patch: return "PATCH";
		case URLRequestMethod::Delete: return "DELETE";
	}
	return nullptr;
}

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

bool HTTPResponseHeadersContainer::hasHeader(const std::string& name) const
{
	return responseHeaders.count(name) > 0;
}

bool HTTPResponseHeadersContainer::getHeader(const std::string& name, std::string& output_value) const
{
	const auto it = responseHeaders.find(name);
	if (it == responseHeaders.end()) { return false; }
	output_value = it->second;
	return true;
}

void URLDataRequest::setPost(std::string&& jsonData)
{
	m_method = URLRequestMethod::Post;
	m_requestBody = std::move(jsonData);
	setRequestHeader("Content-Type", "application/json");
}

void URLDataRequest::setPatch(std::string&& jsonData)
{
	m_method = URLRequestMethod::Patch;
	m_requestBody = std::move(jsonData);
	setRequestHeader("Content-Type", "application/json");
}

void URLDataRequest::setDelete()
{
	m_method = URLRequestMethod::Delete;
}

URLRequestMethod URLDataRequest::method() const
{
	return m_method;
}

const std::string& URLDataRequest::requestBody() const
{
	return m_requestBody;
}
