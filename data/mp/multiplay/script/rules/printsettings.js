function printGameSettings()
{
//add human readable method
var human = {
	scavengers : function () {
		switch (scavengers) {
			case ULTIMATE_SCAVENGERS: return _("Ultimate Scavengers");
			case SCAVENGERS: return _("Scavengers");
			case NO_SCAVENGERS: return _("No Scavengers");
			}
		},

	alliancesType : function () {
		switch (alliancesType) {
			case NO_ALLIANCES: return _("No Alliances");
			case ALLIANCES: return _("Allow Alliances");
			case ALLIANCES_TEAMS: return _("Locked Teams");
			case ALLIANCES_UNSHARED: return _("Locked Teams, No Shared Research");
			}
		},

	powerType : function () {
		switch (powerType) {
			case 0: return _("Low Power Levels");
			case 1: return _("Medium Power Levels");
			case 2: return _("High Power Levels");
			}
		},

	baseType : function () {
		switch (baseType) {
			case CAMP_CLEAN: return _("Start with No Bases");
			case CAMP_BASE: return _("Start with Bases");
			case CAMP_WALLS: return _("Start with Advanced Bases");
			}
		},
	};

//	debug( [mapName, human.scavengers(), human.alliancesType(), human.powerType(), human.baseType(), "T" + getMultiTechLevel(), version ].join("\n"));
	console( [mapName, human.scavengers(), human.alliancesType(), human.powerType(), human.baseType() ].join("\n"));
}
