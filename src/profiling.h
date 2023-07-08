#ifndef __INCLUDED_SRC_PROFILING_H__
#define __INCLUDED_SRC_PROFILING_H__

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

#endif // __INCLUDED_SRC_PROFILING_H__
