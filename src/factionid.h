#ifndef __INCLUDED_FACTIONID_H__
#define __INCLUDED_FACTIONID_H__

#include <optional-lite/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

enum FactionID : uint8_t {
	FACTION_NORMAL = 0,
	FACTION_NEXUS = 1,
	FACTION_COLLECTIVE = 2
};
#define MAX_FACTION_ID FACTION_COLLECTIVE

inline optional<FactionID> uintToFactionID(uint8_t value)
{
	if (value > static_cast<uint8_t>(MAX_FACTION_ID))
	{
		return nullopt;
	}
	return static_cast<FactionID>(value);
}

namespace std {
	template <> struct hash<FactionID>
	{
		size_t operator()(const FactionID& faction) const
		{
			return hash<uint8_t>()(static_cast<uint8_t>(faction));
		}
	};
}

#endif
