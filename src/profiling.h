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

#pragma once

#include "lib/framework/wzglobal.h" // required for config.h

#if defined(WZ_PROFILING_INSTRUMENTATION)

#include <cstdint>

namespace profiling {

/// Application-level profiling domain.
/// It is often created only once per application or per large component.
class Domain
{
public:
	struct Internal;

	explicit Domain(const char* name);
	~Domain();

	const Internal* getInternal() const {
		return m_internal;
	}

private:
	Internal* m_internal;
};

/// Profiling scope.
/// Instrumentation backend will create starting mark when the scope is entered
/// and finishing mark when scope is left.
class Scope
{
public:
	Scope(const Domain* domain, const char* name);
	Scope(const Domain* domain, const char* object, const char* name);
	~Scope();

	/// Get a domain.
	const Domain* domain() const {
		return m_domain;
	}
	/// Get time from scope start, in seconds.
	double elapsed() const;

private:
	const Domain* m_domain = nullptr;
};

extern Domain wzRootDomain;

void mark(const Domain *domain, const char *mark);
void mark(const Domain *domain, const char *object, const char *mark);

}

#define WZ_PROFILE_SCOPE(name) profiling::Scope mark_##name(&profiling::wzRootDomain, #name);
#define WZ_PROFILE_SCOPE2(object, name) profiling::Scope mark_##name(&profiling::wzRootDomain, #object, #name);

#else // !defined(WZ_PROFILING_INSTRUMENTATION)

#define WZ_PROFILE_SCOPE(name)
#define WZ_PROFILE_SCOPE2(object, name)

#endif // defined(WZ_PROFILING_INSTRUMENTATION)
