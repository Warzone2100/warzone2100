#include "faction.h"
#include "lib/netplay/netplay.h"

const struct FACTION factions[NUM_FACTIONS] = {
	{ "Normal", {} },
	{
		"NEXUS",
		{
			{"blwallc1.pie", "blwallc3.pie"},
			{"blwallh.pie", "blwall3.pie"},
		}
	},
	{
		"Collective",
		{
			{"blwallc1.pie", "blwallc2.pie"},
			{"blwallh.pie", "blwall2.pie"},
		}
	}
};

iIMDShape* getFactionIMD(uint8_t player, iIMDShape* imd)
{
	uint8_t faction = NetPlay.players[player].faction;

	const std::map<WzString, WzString> *replaceIMD = &factions[faction].replaceIMD;

	WzString name = WzString::fromUtf8(modelName(imd));
	auto pos = replaceIMD->find(name);
	if (pos == replaceIMD->end())
	{
		return imd;
	} else {
		debug(LOG_INFO, "render struct of player_%i in faction %s (%i)", player,
			factions[faction].name.toUtf8().c_str(), faction
		);
		debug(LOG_INFO, "replace imd %s", name.toUtf8().c_str());
		return modelGet(pos->second);
	}
}
