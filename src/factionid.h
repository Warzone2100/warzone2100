#ifndef __INCLUDED_FACTIONID_H__
#define __INCLUDED_FACTIONID_H__

enum FactionID : uint8_t {
	FACTION_NORMAL = 0,
	FACTION_NEXUS = 1,
	FACTION_COLLECTIVE = 2
};

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
