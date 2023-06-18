#ifndef __INCLUDED_SRC_PROFILING_H__
#define __INCLUDED_SRC_PROFILING_H__

#include <ctime>
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
	// Some additional opaque data for implementation.
	void* m_domain = nullptr;
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

	double elapsed() const;
private:
	timespec m_prevTimeStamp;
	const Domain* m_domain = nullptr;
	uint64_t m_backendRangeId = 0;
};

extern Domain wzRootDomain;
}

#define WZ_PROFILE_SCOPE(name) profiling::Scope mark_##name(&profiling::wzRootDomain, #name);
#define WZ_PROFILE_SCOPE2(object, name) profiling::Scope mark_##name(&profiling::wzRootDomain, #object, #name);

#endif // __INCLUDED_SRC_PROFILING_H__
