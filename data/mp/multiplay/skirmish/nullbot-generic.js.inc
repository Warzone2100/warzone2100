
	/********************************/
	/* Generic branch specific code */
	/********************************/

const MR = 0; // mg/rockets
const MC = 1; // mg/cannons
const FR = 2; // flamers/rockets
const FC = 3; // flamers/cannons
const MF = 4; // mg/flamers (uses flamers as AT weapon)

function introduceYourselfToAlly() {
	var str=nbVersionString();
	var substr=_("No subpersonalities available on EASY difficulty");
	if (difficulty==EASY) {
		return str+": "+substr;
	}
	switch(personality.subpersonality) {
		case MC: substr=_("Machineguns/Cannons"); break;
		case MR: substr=_("Machineguns/Rockets"); break;
		case FC: substr=_("Flamers/Cannons"); break;
		case FR: substr=_("Flamers/Rockets"); break;
		case MF: substr=_("Machineguns/Flamers"); break;
		default: substr=_("Unknown personality");
	}
	return str+": "+substr;
}

function introduceYourselfToEnemy() {
	return nbVersionString();
}

function switchSubpersonality(msg) {
	if (difficulty==EASY)
		return _("Subpersonalities are not supported on Easy difficulty.");
	switch(msg) {
		case "mr": personality=new constructPersonality(MR); break;
		case "mc": personality=new constructPersonality(MC); break;
		case "fr": personality=new constructPersonality(FR); break;
		case "fc": personality=new constructPersonality(FC); break;
		case "mf": personality=new constructPersonality(MF); break;
		default:
			return _("Try one of these: mr mc fr fc mf");
	}
	return introduceYourselfToAlly();
}

function switchResearchPath(msg) {
	return standardSwitchResearchPath(msg);
}

function constructPersonality(subpersonality) {
		
	this.MIN_BUILDERS = 2;		// the usual number of trucks that construct base structures
	this.MAX_BUILDERS = 3;		// the maximum number of trucks that construct base structures
	this.MIN_OILERS = 2;		// the usual number of trucks used for oil hunting
	this.MAX_OILERS = 3+random(2);		// the maximum number of trucks used for oil hunting
	this.MIN_DEFENDERS = 4;		// the minimum number of tanks before producing more trucks than necessary
	this.DEFENSIVENESS = 2;		// regulates chance of spending money on defensive structures
	this.PEACE_TIME = 10;		// the amount of minutes for free scouting (regrouping disabled, no defenses built)

	this.THIS_AI_MAKES_TANKS = true;
	this.THIS_AI_MAKES_CYBORGS = true;
	this.THIS_AI_MAKES_VTOLS = true;

	if (typeof(subpersonality)=="undefined")
		this.subpersonality = (teamIdentifier()+getTime())%5; // pseudo-random choice, equal for all team members
	else
		this.subpersonality = subpersonality;

	this.tankBodies = [ standardTankBodiesLight, ];
	
	this.vtolBodies = [
		"Body7ABT", // retribution
		"Body6SUPP", // panther
		"Body8MBT", // scorpion
		"Body5REC", // cobra
		"Body3MBT", // retaliation
		"Body2SUP", // leopard
		"Body4ABT", // bug
		"Body1REC", // viper
	];

	this.abWeapons = standardTankAB;
	
	this.aaWeapons = standardTankAA;
	
	this.vtolWeapons = [
		"Bomb5-VTOL-Plasmite", // plasmite bombs
		"RailGun2-VTOL", // rail gun
		"Bomb4-VTOL-HvyINC", // thermite bombs
		"Bomb2-VTOL-HvHE", // heap bombs
		"RailGun1-VTOL", // needle
		"Missile-VTOL-AT", // scourge
		"Laser2PULSE-VTOL", // pulse laser
		"Cannon5Vulcan-VTOL", // ac
		"Rocket-VTOL-HvyA-T", // tk
		"Bomb3-VTOL-LtINC", // phosphor bombs
		"MG4ROTARY-VTOL", // ag
		"Cannon4AUTO-VTOL", // hpv
		"Rocket-VTOL-LtA-T", // lancer
		"Bomb1-VTOL-LtHE", // cluster bombs
		"MG3-VTOL", // hmg
		"Rocket-VTOL-Pod", // minipod
	];

	this.defenses = [
	[
		"GuardTower-ATMiss", // scourge tower
		"Emplacement-Rail3", // gauss emplacement
		"Emplacement-Rail2", // railgun emplacement
		"GuardTower-Rail1", // needle gun tower
		"PillBox-Cannon6", // tac bunker
		"Emplacement-HvyATrocket", // tk emplacement
		"GuardTower5", // lancer tower
		"Emplacement-HPVcannon", // hpv emplacement
		"PillBox4", // lc bunker
		"GuardTower6", // minipod tower
	],
	[
		"Emplacement-MdART-pit", // seraph
		"GuardTower-BeamLas", // pulse laser tower
		"Emplacement-PrisLas", // flashlight emplacement
		"Plasmite-flamer-bunker", // plasmite bunker
		"Pillbox-RotMG", // ag bunker
		"Tower-Projector", // inferno bunker
		"Emplacement-MRL-pit", // mra emplacement
		"PillBox5", // flamer bunker
		"PillBox1", // mg bunker
		"GuardTower1", // mg tower
	],
	[
		"X-Super-Missile", // super missile fort
		"X-Super-Rocket", // super rocket fort
	],
	[
		"X-Super-MassDriver", // super mass driver
		"X-Super-Cannon", // super cannon
	],
	[
		"Emplacement-MortarEMP", // emp mortar
		"Sys-SpyTower", // nexus link tower
	],
	];

	this.artillery = [
	[
		"Emplacement-Howitzer150", // ground shaker
		"Emplacement-RotHow", // hellstorm
		"Emplacement-Howitzer-Incenediary", // incendiary howitzer
		"Emplacement-Howitzer105", // howitzer
		"Emplacement-MortarPit02", // bombard
		"Emplacement-RotMor", // pepperpot
		"Emplacement-MortarPit-Incenediary", // incendiary mortar
		"Emplacement-MortarPit01", // mortar
	],
	[
		"Emplacement-RotHow", // hellstorm
		"Emplacement-Howitzer-Incenediary", // incendiary howitzer
		"Emplacement-Howitzer150", // ground shaker
		"Emplacement-Howitzer105", // howitzer
		"Emplacement-RotMor", // pepperpot
		"Emplacement-MortarPit-Incenediary", // incendiary mortar
		"Emplacement-MortarPit02", // bombard
		"Emplacement-MortarPit01", // mortar
	],
	[
		"Emplacement-Howitzer-Incenediary", // incendiary howitzer
		"Emplacement-Howitzer150", // ground shaker
		"Emplacement-RotHow", // hellstorm
		"Emplacement-Howitzer105", // howitzer
		"Emplacement-MortarPit-Incenediary", // incendiary mortar
		"Emplacement-MortarPit02", // bombard
		"Emplacement-RotMor", // pepperpot
		"Emplacement-MortarPit01", // mortar
	],
	[
		"Emplacement-HvART-pit", // archangel
		"Emplacement-Rocket06-IDF", // ripples
	],
	];

	this.antiair = [
		"P0-AASite-Laser", // stormbringer
		"P0-AASite-SAM2", // vindicator
		"P0-AASite-SAM1", // avenger
		"AASite-QuadRotMg", // whirlwind
		"P0-AASite-Sunburst", // sunburst
		"AASite-QuadBof", // flak cannon
		"AASite-QuadMg1", // hurricane
	];

	this.hardpoints = [
	[
		"WallTower-PulseLas", // pulse laser
		"WallTower-TwinAssaultGun", // tag
		"Wall-RotMg", // ag
		"WallTower01", // hmg
	],
	[
		"WallTower-Rail3", // gauss
		"WallTower-Rail2", // railgun
		"WallTower04", // hc
		"Wall-VulcanCan", // ac
		"WallTower-HPVcannon", // hpv
		"WallTower03", // mc
		"WallTower02", // lc
	],
	[
		"WallTower-Atmiss", // scourge
		"WallTower-HvATrocket", // tk
		"WallTower06", // lancer
	],
	[
		"WallTower-EMP", // emp
	],
	];
	
	if (difficulty == EASY) {
		this.researchPathPrimary = standardResearchPathNoob;
		this.researchPathFundamental = [];
		this.researchPathAT = [];
		this.researchPathAP = [];
		this.researchPathAA = [];
		this.researchPathAB = [];
		this.tankPropulsions = standardTankPropulsionsNoob;
		this.apWeapons = [ standardTankAPNoobOne, standardTankAPNoobTwo, ];
		this.atWeapons = [ standardTankATNoobOne, standardTankATNoobTwo, ];
		this.apCyborgStats = [ standardCyborgAPNoobOne, standardCyborgAPNoobTwo, ];
		this.atCyborgStats = [ standardCyborgATNoobOne, standardCyborgATNoobTwo, standardCyborgATNoobThree];
	} else {
		
		switch(this.subpersonality) {
		case MR:
			this.buildOrderIdx = 2;
			this.tankBodiesAT = [ standardTankBodiesHeavy, ];
			this.researchPathPrimary = [ // rffrc
				"R-Vehicle-Prop-Halftracks",
				"R-Wpn-MG2Mk1",
				"R-Struc-PowerModuleMk1",
				"R-Wpn-Rocket-Damage02",
				"R-Wpn-MG3Mk1",
			];
			break;
		case MC:
			this.buildOrderIdx = random(2); 
			this.tankBodiesAP = [ standardTankBodiesHeavy, ];
			if (this.buildOrderIdx == 0) {
				this.researchPathPrimary = [ // frcfr
					"R-Wpn-MG-Damage02",
					"R-Wpn-Cannon-Damage03",
					"R-Vehicle-Prop-Halftracks",
					"R-Struc-PowerModuleMk1",
					"R-Wpn-MG3Mk1",
				];
			} else {
				this.researchPathPrimary = [ // rffcr
					"R-Wpn-Cannon-Damage02",
					"R-Vehicle-Prop-Halftracks",
					"R-Struc-PowerModuleMk1",
					"R-Struc-Factory-Cyborg",
					"R-Wpn-MG3Mk1",
				];
			}
			break;
		case FR:
			this.buildOrderIdx = 2;
			this.tankBodiesAP = [ standardTankBodiesHeavy, ];
			this.tankBodiesAT = [ standardTankBodiesHeavy, ];
			this.researchPathPrimary = [ // rffrc
				"R-Wpn-MG2Mk1",
				"R-Wpn-Rocket-Damage02",
				"R-Struc-PowerModuleMk1",
				"R-Vehicle-Prop-Halftracks",
				"R-Wpn-Flamer01Mk1",
				"R-Struc-Factory-Cyborg",
			];
			break;
		case FC:
			this.buildOrderIdx = 2 * random(2);
			this.tankBodiesAP = [ standardTankBodiesHeavy, ];
			if (this.buildOrderIdx == 0) {
				this.researchPathPrimary = [ // frcfr
					"R-Wpn-MG2Mk1",
					"R-Vehicle-Prop-Halftracks",
					"R-Wpn-Flamer01Mk1",
					"R-Struc-Factory-Cyborg",
				];
			} else {
				this.researchPathPrimary = [ // rffrc
					"R-Wpn-MG-Damage01",
					"R-Struc-Factory-Cyborg",
					"R-Wpn-Flamer01Mk1",
					"R-Struc-PowerModuleMk1",
				];
			}
			break;
		case MF:
			this.buildOrderIdx = random(3);
 			this.tankBodies = standardTankBodies;
			this.researchPathPrimary = [ // any
				"R-Wpn-MG3Mk1",
				"R-Vehicle-Prop-Halftracks",
				"R-Struc-PowerModuleMk1",
			];
			break;
		}
		
		this.researchPathPrimary = this.researchPathPrimary.concat(["R-Vehicle-Body05","R-Struc-RprFac-Upgrade01",]);

		
		this.researchPathFundamental = standardResearchPathFundamental;
		

		switch(this.subpersonality) {
		case MR: case FR: 
			this.researchPathAT = standardResearchPathRockets;
			break;
		case MC: case FC:
			this.researchPathAT = standardResearchPathCannons;
			break;
		case MF: 
			this.researchPathAT = standardResearchPathFlamers;
			break;
		}

		switch(this.subpersonality) {
		case MR: case MC: case MF: 
			this.researchPathAP = standardResearchPathMachineguns;
			break;
		case FR: case FC: 
			this.researchPathAP = standardResearchPathFlamers;
			break;
		}
		this.researchPathAP = this.researchPathAP.concat(standardResearchPathLasers);

		switch(this.subpersonality) {
		case MF: 
			this.researchPathAA = standardResearchPathHurricane;
			this.researchPathAA = this.researchPathAA.concat(standardResearchPathWhirlwind);
			break;
		case MC: 
			this.researchPathAA = standardResearchPathHurricane;
			this.researchPathAA = this.researchPathAA.concat(standardResearchPathFlakCannon);
			this.researchPathAA = this.researchPathAA.concat(standardResearchPathWhirlwind);
			break;
		case MR: 
			this.researchPathAA = standardResearchPathHurricane;
			this.researchPathAA = this.researchPathAA.concat(standardResearchPathVindicator);
			this.researchPathAA = this.researchPathAA.concat(standardResearchPathWhirlwind);
			break;
		case FC:
			this.researchPathAA = standardResearchPathFlakCannon;
			break;
		case FR:
			this.researchPathAA = standardResearchPathVindicator;
			break;
		}
		this.researchPathAA = this.researchPathAA.concat(standardResearchPathStormbringer);

		switch(this.subpersonality) {
		case MF: 
			this.researchPathAB = standardResearchPathIncendiary;
			break;
		case MC: 
			this.researchPathAB = standardResearchPathHowitzers;
			break;
		case MR: 
			this.researchPathAB = standardResearchPathBunkerBuster;
			this.researchPathAB = this.researchPathAB.concat(standardResearchPathRipples);
			break;
		case FC:
			this.researchPathAB = standardResearchPathIncendiary;
			this.researchPathAB = this.researchPathAB.concat(standardResearchPathHowitzers);
			break;
		case FR:
			this.researchPathAB = standardResearchPathBunkerBuster;
			this.researchPathAB = this.researchPathAB.concat(standardResearchPathIncendiary);
			this.researchPathAB = this.researchPathAB.concat(standardResearchPathRipples);
			break;
		}
		
		this.tankPropulsions = standardTankPropulsions;
		this.tankPropulsionsAA = [
		[
			"hover01", // hover
			"HalfTrack", // half-track
			"wheeled01", // wheels
		],
		];
		this.tankPropulsionsAB = [
		[
			"tracked01", // tracks
			"HalfTrack", // half-track
			"wheeled01", // wheels
		],
		];
		
		switch(this.subpersonality) {
		case MF: 
			this.tankPropulsionsAT = this.tankPropulsionsAA;
			break;
		case FR: case FC:
			this.tankPropulsionsAP = this.tankPropulsionsAA;
			break;
		case MC:
			this.tankPropulsionsAT = this.tankPropulsionsAB;
			this.tankPropulsionsAP = this.tankPropulsionsAB;
			break;
		}
		
		this.apWeapons = [
			standardTankLaser,
		];
		switch(this.subpersonality) {
		case MR:
			this.apWeapons[1] = standardTankAPMachinegunRocket;
		case MC:
		case MF:
			this.apWeapons[0] = this.apWeapons[0].concat(standardTankMachinegun);
			break;
		case FR:
			this.apWeapons[1] = standardTankAPRocket;
			this.apWeapons[1] = this.apWeapons[1].concat(standardTankFlamer); // to make sure apWeapons[1] is used even when no MRAs yet
		case FC:
			this.apWeapons[0] = this.apWeapons[0].concat(standardTankFlamer);
			break;
		}

		switch(this.subpersonality) {
		case MF:
			this.atWeapons = [ standardTankLaser, ];
			this.atWeapons[0] = this.atWeapons[0].concat(standardTankFlamer);
			break;
		case MC:
		case FC:
			this.atWeapons = [ standardTankCannonOne, standardTankCannonTwo, ];
			break;
		case MR:
		case FR:
			this.atWeapons = [ standardTankRocket, ];
			break;
		}
		
		this.apCyborgStats = [ standardCyborgLaser, ];
		switch(this.subpersonality) {
		case MF:
			this.apCyborgStats[1] = standardCyborgLaser;
			this.apCyborgStats[1] = this.apCyborgStats[1].concat(standardCyborgMachinegun);
		case FC:
		case FR:
			this.apCyborgStats[0] = this.apCyborgStats[0].concat(standardCyborgFlamer);
			break;
		case MC:
		case MR:
			this.apCyborgStats[0] = this.apCyborgStats[0].concat(standardCyborgMachinegun);
			break;
		}
		
		switch(this.subpersonality) {
		case MF:
			this.atCyborgStats = [ standardCyborgLaser, ];
			this.atCyborgStats[0] = this.atCyborgStats[0].concat(standardCyborgFlamer);
			break;
		case MC:
		case FC:
			this.atCyborgStats = [ standardCyborgCannonOne, standardCyborgCannonTwo, standardCyborgCannonThree ];
			break;
		case MR:
		case FR:
			this.atCyborgStats = [ standardCyborgRocket ];
			break;
		}
	}
}

function buildOrder() {
	if (difficulty == EASY) 
		return standardBuildOrderNoob();
	switch(personality.buildOrderIdx) {
		case 0: // rush
			return standardBuildOrderFRCFR();
		case 1: // average
			return standardBuildOrderRFFCR();
		case 2: // tech
			return standardBuildOrderRFFRC();
	}
}

function checkLowPower() {
	return standardCheckLowPower();
}
