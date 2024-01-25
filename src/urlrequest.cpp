/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2024  Warzone 2100 Project

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

#define NOMINMAX
#include "urlrequest.h"
#include "urlrequest_private.h"
#include "lib/framework/wzapp.h"

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
