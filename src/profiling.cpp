/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

#include "profiling.h"

#if defined(WZ_PROFILING_INSTRUMENTATION)

#include <cstdio>
#include <string>

#ifdef WZ_PROFILING_NVTX
#if defined( _MSC_VER )
#  pragma warning( push )
#  pragma warning( disable : 4191 )
#endif
#include <nvtx3/nvToolsExt.h>
#if defined( _MSC_VER )
#  pragma warning( pop )
#endif
#endif

#ifdef WZ_PROFILING_VTUNE
#include <ittnotify.h>
#endif

namespace profiling
{

struct Domain::Internal
{
#ifdef WZ_PROFILING_NVTX
	nvtxDomainHandle_t nvtxDomain = nullptr;
#endif

#ifdef WZ_PROFILING_VTUNE
	__itt_domain* ittDomain = nullptr;
#endif
	std::string name;

	~Internal()
	{
#ifdef WZ_PROFILING_NVTX
		if (nvtxDomain)
		{
			nvtxDomainDestroy(nvtxDomain);
			nvtxDomain = nullptr;
		}
#endif
	}
};

Domain::Domain(const char* name)
{
	m_internal = new Internal();
	m_internal->name = name ? name : "Unnamed";
#ifdef WZ_PROFILING_NVTX
	m_internal->nvtxDomain = nvtxDomainCreateA(name);
#endif

#ifdef WZ_PROFILING_VTUNE
	m_internal->ittDomain = __itt_domain_create(name);
#endif
}

Domain::~Domain()
{
	if (m_internal)
	{
#ifdef WZ_PROFILING_NVTX
		nvtxDomainDestroy(m_internal->nvtxDomain);
		m_internal->nvtxDomain = nullptr;
#endif
		delete m_internal;
		m_internal = nullptr;
	}
}

// Global domain for warzone.
Domain wzRootDomain{"warzone2100"};

Scope::Scope(const Domain *domain, const char *name)
	:m_domain(domain)
{
	if (m_domain && name)
	{
		#ifdef WZ_PROFILING_NVTX
		{
			nvtxRangePushA(name);
		}
		#endif
		#ifdef WZ_PROFILING_VTUNE
		{
		__itt_string_handle* task = __itt_string_handle_create(name);
		auto ittDomain = m_domain ? m_domain->getInternal()->ittDomain : nullptr;
		__itt_task_begin(ittDomain, __itt_null, __itt_null, task);
		}
		#endif
	}
}

Scope::Scope(const Domain *domain, const char *object, const char *name)
	:m_domain(domain)
{
	if (m_domain && object && name)
	{
		static char tmpBuffer[255];
		std::snprintf(tmpBuffer, sizeof(tmpBuffer), "%s::%s", object, name);
		#ifdef WZ_PROFILING_NVTX
		{
			nvtxRangePushA(tmpBuffer);
		}
		#endif
		#ifdef WZ_PROFILING_VTUNE
		{
			__itt_string_handle* task = __itt_string_handle_create(tmpBuffer);
			auto ittDomain = m_domain ? m_domain->getInternal()->ittDomain : nullptr;
			__itt_task_begin(ittDomain, __itt_null, __itt_null, task);
		}
		#endif
	}
}

Scope::~Scope()
{
	if (m_domain) {
#ifdef WZ_PROFILING_NVTX
		nvtxRangePop();
#endif
#ifdef WZ_PROFILING_VTUNE
		auto ittDomain = m_domain->getInternal()->ittDomain;
		__itt_task_end(ittDomain);
#endif
	}
}

void mark(const Domain *domain, const char *mark)
{
	if (!domain || !mark)
		return;
	#ifdef WZ_PROFILING_NVTX
	{
		nvtxEventAttributes_t eventAttrib = {};
		eventAttrib.version = NVTX_VERSION;
		eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
		eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
		eventAttrib.message.ascii = mark;
		auto nvtxDomain = domain ? domain->getInternal()->nvtxDomain : nullptr;
		nvtxDomainMarkEx(nvtxDomain, &eventAttrib);
	}
	#endif
	#ifdef WZ_PROFILING_VTUNE
		auto string = __itt_string_handle_create(mark);
		auto ittDomain = domain ? domain->getInternal()->ittDomain : nullptr;
		__itt_marker(ittDomain, __itt_null, string, __itt_scope::__itt_scope_task);
	#endif
}

void mark(const Domain *domain, const char *object, const char *mark)
{
	if (!domain || !object || !mark)
		return;
	static char tmpBuffer[255];
	std::snprintf(tmpBuffer, sizeof(tmpBuffer), "%s::%s", object, mark);

	#ifdef WZ_PROFILING_NVTX
	{
		nvtxEventAttributes_t eventAttrib = {};
		eventAttrib.version = NVTX_VERSION;
		eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
		eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
		eventAttrib.message.ascii = tmpBuffer;
		auto nvtxDomain = domain ? domain->getInternal()->nvtxDomain : nullptr;
		nvtxDomainMarkEx(nvtxDomain, &eventAttrib);
	}
	#endif
	#ifdef WZ_PROFILING_VTUNE
	{
		auto string = __itt_string_handle_create(msg.c_str());
		auto ittDomain = domain ? domain->getInternal()->ittDomain : nullptr;
		__itt_marker(ittDomain, __itt_null, string, __itt_scope::__itt_scope_task);
	}
	#endif
}

}

#endif // defined(WZ_PROFILING_INSTRUMENTATION)
