#include "faction.h"
#include "lib/netplay/netplay.h"

const struct FACTION factions[NUM_FACTIONS] = {
	{ "Normal",
		{
		}
	},
	{
		"NEXUS",
		{
			{"blwallc1.pie", "blwallc3.pie"}, //rename
			{"blwallh.pie", "blwall3.pie"}, //rename
			{"blhq.pie", "blhq_nex.pie"}, // rename
			{"blguard1.pie", "blguard3.pie"}, // rename
			{"blguardr.pie", "blgrdnex.pie"}, // rename
			{"blresch0.pie", "blresch0_nex.pie"},
			{"blresch4.pie", "blresch4_nex.pie"},
			{"research_module4.pie", "research_module4_nex.pie"},
			{"blbresch.pie", "blbresch_nex.pie"},
			{"blbpower.pie", "blbpower3.pie"},  // rename
			{"blbhq.pie",  "blbhq_nex.pie"},
			{"blbdrdcm.pie", "blbdrdcm_3.pie"},  //rename
			{"blpower0.pie", "blpower0_3.pie"},  // rename
			{"blpower4.pie", "blpower4_3.pie"}, // rename
			{"blpilbox.pie", "blpilbox_nex.pie"},
			{"blfact0.pie", "blfact0_nex.pie"},
			{"factory_module1.pie", "factory_module1_nex.pie"},
			{"blfact1.pie", "blfact1_nex.pie"},
			{"factory_module2.pie", "factory_module2_nex.pie"},
			{"blfact2.pie", "blfact2_nex.pie"},
			{"blbfact.pie", "blbfact_nex.pie"},
		}
	},
	{
		"Collective",
		{
			{"blwallc1.pie", "blwallc2.pie"},
			{"blwallh.pie", "blwall2.pie"}, // rename
			{"blhq.pie", "blhq4.pie"},
			{"blguard1.pie", "blguard2.pie"}, // rename
			{"blguardr.pie", "blguardn.pie"},
			{"blresch0.pie", "blresch0_col.pie"},
			{"blresch4.pie", "blresch4_col.pie"},
			{"research_module4.pie", "research_module4_col.pie"},
			{"blbresch.pie", "blbresch_col.pie"},
		}
	}
};

iIMDShape* getFactionIMD(uint8_t player, iIMDShape* imd)
{
	uint8_t faction = NetPlay.players[player].faction;

	const std::map<WzString, WzString> *replaceIMD = &factions[faction].replaceIMD;

	WzString name = WzString::fromUtf8(modelName(imd));
	auto pos = replaceIMD->find(name);
//	debug(LOG_INFO, "render struct of player_%i in faction %s (%i): %s", player,
//		factions[faction].name.toUtf8().c_str(), faction, name.toUtf8().c_str()
//	);
	if (pos == replaceIMD->end())
	{
		return imd;
	} else {
		return modelGet(pos->second);
	}
}
