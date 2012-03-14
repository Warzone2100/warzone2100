
	/********************************/
	/* Generic branch specific code */
	/********************************/

function constructPersonality() {
		
	this.MIN_BUILDERS = 2;		// the usual number of trucks that construct base structures
	this.MAX_BUILDERS = 3;		// the maximum number of trucks that construct base structures
	this.MIN_OILERS = 2;		// the usual number of trucks used for oil hunting
	this.MAX_OILERS = 3+random(2);		// the maximum number of trucks used for oil hunting
	this.MIN_DEFENDERS = 4;		// the minimum number of tanks before producing more trucks than necessary
	this.DEFENSIVENESS = 2+random(4);		// regulates chance of spending money on defensive structures
	this.PEACE_TIME = 10;		// the amount of minutes for free scouting (regrouping disabled, no defenses built)

	this.THIS_AI_MAKES_TANKS = true;
	this.THIS_AI_MAKES_CYBORGS = true;
	this.THIS_AI_MAKES_VTOLS = true;

	const MR = 0; // mg/rockets
	const MC = 1; // mg/cannons
	const FR = 2; // flamers/rockets
	const FC = 3; // flamers/cannons
	const MF = 4; // mg/flamers (uses flamers as AT weapon)
	var subpersonality = random(5);
	
	this.tankBodies = standardTankBodies;
	
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

	this.abWeapons = [
	[
		"Missile-MdArt", // seraph
		"Mortar-Incenediary", // incendiary mortar
		"Mortar2Mk1", // bombard
		"Mortar3ROTARYMk1", // pepperpot
		"Rocket-BB", // bunker buster
		"Mortar1Mk1", // mortar
	],
	[
		"Missile-MdArt", // seraph
		"Mortar2Mk1", // bombard
		"Mortar3ROTARYMk1", // pepperpot
		"Mortar-Incenediary", // incendiary mortar
		"Rocket-BB", // bunker buster
		"Mortar1Mk1", // mortar
	],
	[
		"Missile-MdArt", // seraph
		"Mortar3ROTARYMk1", // pepperpot
		"Mortar-Incenediary", // incendiary mortar
		"Mortar2Mk1", // bombard
		"Rocket-BB", // bunker buster
		"Mortar1Mk1", // mortar
	],
	[
		"Missile-MdArt", // seraph
		"Rocket-BB", // bunker buster
		"Rocket-MRL", // mra
	],
	];
	
	this.aaWeapons = [
	[
		"AAGunLaser", // stormbringer
		"Missile-HvySAM", // vindicator
		"Missile-LtSAM", // avenger
		"QuadRotAAGun", // whirlwind
		"AAGun2Mk1", // flak cannon
		"Rocket-Sunburst", // sunburst
		"QuadMg1AAGun", // hurricane
	],
	];

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
		this.buildOrder = standardBuildOrderFRCFR;
		
	} else {
		
		switch(subpersonality) {
		case MR: case FR: case MF:
			this.researchPathPrimary = [
				"R-Wpn-MG2Mk1",
				"R-Vehicle-Prop-Halftracks",
			];
			break;
		case MC: case FC:
			this.researchPathPrimary = [
				"R-Wpn-MG-Damage02",
				"R-Wpn-Cannon-Damage01",
			];
			break;
		}
		
		this.researchPathFundamental = standardResearchPathFundamental;
		

		switch(subpersonality) {
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

		switch(subpersonality) {
		case MR: case MC: case MF: 
			this.researchPathAP = standardResearchPathMachineguns;
			break;
		case FR: case FC: 
			this.researchPathAP = standardResearchPathFlamers;
			break;
		}
		this.researchPathAP = this.researchPathAP.concat(standardResearchPathLasers);

		switch(subpersonality) {
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

		switch(subpersonality) {
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
		
		this.apWeapons = [
			standardTankLaser,
		];
		switch(subpersonality) {
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

		switch(subpersonality) {
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
		switch(subpersonality) {
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
		
		switch(subpersonality) {
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
		
		if ( subpersonality == MC || subpersonality == FC || ((subpersonality == MF) && (random(2) == 1)) )
			this.buildOrderIdx = 0;
		else
			this.buildOrderIdx = 1;
		
		/*if (typeof(chat)!="undefined") switch(subpersonality) {
			case MC: chat(ALLIES, "Machineguns Cannons"); break;
			case MR: chat(ALLIES, "Machineguns Rockets"); break;
			case FC: chat(ALLIES, "Flamers Cannons"); break;
			case FR: chat(ALLIES, "Flamers Rockets"); break;
			case MF: chat(ALLIES, "Machineguns Flamers"); break;
		}*/
	}
	
}

function buildOrder() {
	if (personality.buildOrderIdx==0)
		return standardBuildOrderFRCFR();
	else
		return standardBuildOrderRFFRC();

}