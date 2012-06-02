
	/*************/
	/* Variables */
	/*************/
	
var	UNUSUAL_SITUATION = 0;
// UNUSUAL_SITUATION is true if we are participating in challenge of some sort;
// this is detected at eventStartLevel and does some tweaks to the AI to make it
// behave more campaign-like during challenges

var	HIGH_TECH_START = 0;
// Defined at start of the game, this value remembers if we have started at T2
// or at least at T1/advbases

const	NUM_GROUPS = 6; // the maximum number of combat groups

var	MAX_WARRIORS;		// the maximum number of droids in a group
var	NUM_WARRIORS;		// the number of battle droids to maintain in a single group
var	MIN_WARRIORS;		// the number of battle droids necessary to engage in combat
const	NO_WARRIORS_AT_ALL = 1;		// the number of droids that doesn't make a cluster yet

const	MIN_VTOL_GROUPS = 2;		// the maximum number of VTOL groups
const	MAX_VTOL_GROUPS = 6;		// the maximum number of VTOL groups
var	NUM_VTOL_GROUPS = MIN_VTOL_GROUPS;

const	MIN_VTOLS = 4;		// the minimum number of vtols to have in attack group before attack
const	NUM_VTOLS = 8;		// thenumber of vtols to maintain in an attack group
const	MAX_VTOLS = 12;		// the maximum size of a vtol attack group

var	ABS_MAX_TRUCKS = 10;			// try not to make too many trucks
const	ABS_MAX_HOVER_TRUCKS = 13;			// make sure we build at least some hover trucks

const	MAX_DISPERSE = 3; 		// the maximum dispersion of the attack group
const	RETREAT_AT = 70;		// send the unit for repair if it has this much hit point percent left

const	NUM_VTOL_PADS = 1;		// this many vtols per vtol pad are allowed
const	BASE_SIZE = 20;		// range of alertness

var	RATE_AP = 2;		// these values affect the amount of a certain weapon class in the mix
var	RATE_AT = 1;		// this is for anti-tank
var	RATE_AA = 0;		// how much more AA do we need
var	RATE_AB = 0;		// this is for anti-building

var	RATE_TANK = 1; // these values affect the amount of hover tanks in the mix
var	RATE_HOVER = 0;

const	LOW_POWER = 700;		//decrease ambitions when power is below this level
var	EXTREME_LOW_POWER = 250;

const	MIN_SENSOR_DISTANCE = 10;		// minimum distance between sensor towers
const	MAX_SENSOR_TANKS = 4;

const	lab = "A0ResearchFacility";
const	factory = "A0LightFactory";
const	command = "A0CommandCentre";
const	generator = "A0PowerGenerator";
const	derrick = "A0ResourceExtractor";
const	borgfac = "A0CyborgFactory";
const	vtolfac = "A0VTolFactory1";
const	vtolpad = "A0VtolPad";
const	satlink = "A0Sat-linkCentre";
const	lassat = "A0LasSatCommand";
const	oilres = "OilResource";
const	cbtower = "Sys-CB-Tower01";
const	repair = "A0RepairCentre3";
const	oildrum = "OilDrum";

const	pmod = "A0PowMod1";
const	fmod = "A0FacMod1";
const	rmod = "A0ResearchModule1";

// the same for all personalities ...
const	sensors = [
	"Sys-SensoTowerWS", // wide spectrum sensor
	"Sys-SensoTower02", // hardened sensor
	"Sys-SensoTower01", // sensor
];

// this one too ...
const sensorTurrets = [
	"Sensor-WideSpec", // wide spectrum sensor
	"SensorTurret1Mk1", // sensor
];

// this one too ...
const	vtolPropulsions = [
	"V-Tol", // guess what ...
];

// this one too ...
const	truckBodies = [
	"Body3MBT", // retaliation
	"Body2SUP", // leopard
	"Body4ABT", // bug
	"Body1REC", // viper
];

// this one too ...
const	truckPropulsions = [
	"hover01", // hover
	"wheeled01", // wheels
];


// attack this; bottom items first
const	attackTargets = [
	borgfac,
	factory,
	vtolfac,
	derrick,
	lassat,
];

// attack this when going all-in
const allInAttackTargets = [
	borgfac,
	factory,
	vtolfac,
	lassat,
];

// finally some variables

var	personality;

var	builderGroup, oilerGroup, battleGroup = [], vtolGroup = [], defendGroup;
var	groupInfo = [];
var	enemyInfo = [];
var	derrickStats = [];

const	STATUS_NORMAL=0, STATUS_PANIC=1, STATUS_EARLYGAME=2, STATUS_UNSTUCK=3;
const	TARGET_DROID=0, TARGET_STRUCTURE=1;
var	status=STATUS_NORMAL;
var	statusX, statusY, statusTime;
var	lastArtyAttackX=-1, lastArtyAttackY=-1;
var	lastAirAttackX=-1, lastAirAttackY=-1;

var	global=this;
var	basePosition;

	/**************/
	/* Procedures */
	/**************/
	
// creates a new instance of group info
function constructGroupInfo(enemy) {
	this.enemy=enemy;
	this.targetX=-1;
	this.targetY=-1;
	this.targetType=TARGET_STRUCTURE;
	this.targetAge=0;
	this.delayregroup=0;
	this.delaystaticregroup=0;
	this.lastX=-1;
	this.lastY=-1;
	this.idleTime=0;
}

// creates a new instance of enemy info
function constructEnemyInfo() {
	this.tank = 0;
	this.borg = 0;
	this.vtol = 0;
	this.defs = 0;
	this.aa = 0;
	this.lastUpdate = 0;
	this.reachable = true;
}

// this AI doesn't cheat yet
function cheat() {
	switch(difficulty) {
		case INSANE:
			break;
		case HARD:
			break;
		case MEDIUM:
			break;
		case EASY:
			break;
	}
}

// makes attack groups bigger
function becomeHarder() {
	var enemies=countEnemies()+1;
	MAX_WARRIORS = Math.floor(Math.min(150/enemies,13+gameTime/180000));
	NUM_WARRIORS = Math.floor(Math.min(150/enemies,9+gameTime/180000));
	MIN_WARRIORS = Math.floor(Math.min(150/enemies,5+gameTime/180000));
}

// checks if we can design tanks and research towers
function iHaveCC() {
	var list=enumStruct(me,command);
	if (list.length == 0)
		return false;
	if (list[0].status == BEING_BUILT)
		return false;
	return true;
}

// checks if we can produce hovertanks
function iHaveHovers() {
	const	COMP_PROPULSION=3; // HACK: define it here until these constants are exposed to scripts
	return componentAvailable(COMP_PROPULSION,"hover01");	
}

// checks if it's time to build sensors all over the place
function iHaveArty() {
	return isStructureAvailable(cbtower,me);
}

// checks if at least one generator is up and running when necessary
function dontRunOutOfPower() {
	// check for T2>= start and don't hang with no oil because of this procedure until a generator is built
	if (HIGH_TECH_START==1) {
		var list=enumStruct(me,generator)
		for (i=0; i<list.length; ++i)
			if (list[i].status==BUILT)
				return true;
		return false;
	}
	return true;
}

// checks if a structure is a dedicated anti-air defense
function isDedicatedAA(struct) {
	if (typeof(struct.canHitAir)!="undefined" && typeof(struct.canHitGround)!="undefined") {
		return struct.canHitAir && !struct.canHitGround;
	} else {
		// some workaround since we lack the functions used above in v3.1
		if (struct.name.indexOf("AA ")>-1) 
			return true;
		if (struct.name.indexOf("SAM ")>-1) 
			return true;
		if (struct.name.indexOf("Vindicator")>-1) 
			return true;
		if (struct.name.indexOf("Whirlwhind")>-1) 
			return true;
		if (struct.name.indexOf("Avenger")>-1) 
			return true;
		if (struct.name.indexOf("Stormbringer")>-1) 
			return true;
	}
	return false;
}

// count tank/borg balance of a particular enemy
// enemy=-1 will count all enemies as one
function enemyCountBalance(enemy) {
	if (difficulty == EASY || difficulty == MEDIUM)
		return {
			tank:8,
			borg:16,
			vtol:4,
			defs:2,
			aa:7
		};
	// the rest involves deity cheating
	if (typeof(enemyInfo[enemy])!="undefined") {
		if (gameTime - enemyInfo[enemy].lastUpdate < 60000)
			return enemyInfo[enemy];
	} else 
		enemyInfo[enemy]=new Object();
	enemyInfo[enemy].tank=0;
	enemyInfo[enemy].borg=0;
	enemyInfo[enemy].vtol=0;
	enemyInfo[enemy].defs=0;
	enemyInfo[enemy].aa=0;
	var list=cachedEnumEnemies(enemy);
	for (var i=0; i<list.length; ++i) {
		var obj=list[i];
		if (obj.player == enemy || (enemy == -1 && isAnEnemy(obj.player))) {
			if (obj.type == DROID) {
				if (obj.isVTOL) {
					++enemyInfo[enemy].vtol;
				} else if (obj.droidType == DROID_WEAPON) {
					++enemyInfo[enemy].tank;
				} else if (obj.droidType == DROID_CYBORG) {
					++enemyInfo[enemy].borg;
				}
			} else if (obj.type == STRUCTURE) {
				if (obj.stattype == DEFENSE) {
					if (isDedicatedAA(obj)) {
						++enemyInfo[enemy].aa;
					} else {
						++enemyInfo[enemy].defs;
					}
				}
			}
		}
	}
	enemyInfo[enemy].lastUpdate = gameTime;
	return enemyInfo[enemy];
}

// counts AP/AT balance of a battle group
function groupCountBalance(gr) {
	var list=enumGroup(battleGroup[gr]);
	var ap=0, at=0, ab=0, aa=0, se=0;
	for (var i=0; i<list.length; ++i) {
		if (list[i].name.indexOf("AP") > -1)
			++ap;
		else if (list[i].name.indexOf("AT") > -1)
			++at;
		else if (list[i].name.indexOf("AB") > -1)
			++ab;
		else if (list[i].name.indexOf("AA") > -1)
			++aa;
		else if (list[i].name.indexOf("Sensor") > -1)
			++se;
	}
	return {ap:ap,at:at,ab:ab,aa:aa,se:se};
}

// the heart of the AI adaptation algorithm
function adaptationMatrix(ap, at, aa, ab, tank, borg, vtol, defs) {
	if (difficulty == EASY || difficulty == MEDIUM)
		return {ap:8,at:4,aa:2,ab:1};
	// the rest involves deity cheating
	var newAP, newAT, newAA, newAB;
	var totalMe=ap+at+aa+ab; 
	var totalEnemy=tank+borg+vtol+defs;
	if (totalMe==0 || totalEnemy==0)
		return {ap:2,at:1,aa:0,ab:0};
	newAP = 100*(borg/totalEnemy - ap/totalMe);
	newAT = 100*(tank/totalEnemy - at/totalMe);
	newAA = 100*(vtol/totalEnemy - 2*aa/totalMe)+5;
	newAB = 100*(defs/totalEnemy - 2*ab/totalMe);
	newAP = Math.floor(newAP); if (newAP < 2) newAP=2;
	newAT = Math.floor(newAT); if (newAT < 1) newAT=1;
	newAA = Math.floor(newAA); if (newAA < 0) newAA=0;
	newAB = Math.floor(newAB); if (newAB < 0) newAB=0;
	return {ap:newAP,at:newAT,aa:newAA,ab:newAB};
}

// adapt to enemy choices and game situation
function adapt() {
	var seaEnemies = 0, landEnemies = 0;
	for (var i=0; i<maxPlayers; ++i) if (isAnEnemy(i)) {
		if (enemyInfo[i].reachable)
			++landEnemies;
		else
			++seaEnemies;
	}
	if (RATE_TANK > 0) {
		RATE_TANK = landEnemies;
		RATE_HOVER = seaEnemies;
	} else {
		RATE_TANK = 0; // don't use tanks once we switched to pure hovers at least once (kind of the safe way)
		RATE_HOVER = 1;
	}
	if (difficulty == EASY || difficulty == MEDIUM) {
		RATE_AP = 16;
		RATE_AT = 7;
		RATE_AA = 3;
		RATE_AB = 3;
	} else { // the rest involves deity cheating
		var enemyBorgCount = 0, enemyTankCount = 0, enemyVtolCount = 0, enemyBuildingCount = 0;
		var ret=enemyInfo[-1];
		if (typeof(ret)=="undefined") { // usually happens on game load
			callUpdateEnemyInfo();
			ret=enemyInfo[-1];
		}
		enemyBorgCount+=ret.borg;
		enemyTankCount+=ret.tank;
		enemyVtolCount+=ret.vtol;
		enemyBuildingCount+=ret.defs;
		for (var i=0; i<maxPlayers; ++i) if (isAnEnemy(i)) {
			enemyVtolCount+=2*enumStruct(i,vtolfac).length;
		}
		var myATCount = 0, myAPCount = 0, myAACount = 0, myABCount = 0;
		for (var i=0; i<NUM_GROUPS; ++i) {
			var ret=groupCountBalance(i);
			myAPCount+=ret.ap;
			myATCount+=ret.at;
			myABCount+=ret.ab;
			myAACount+=ret.aa;
		}
		// count stationary AA as well
		var list=enumStruct(me);
		for (var i=0; i<list.length; ++i)
			if (isDedicatedAA(list[i])) 
				++myAACount;
		var ret=adaptationMatrix(
			myAPCount, myATCount, myAACount, myABCount, 
			enemyTankCount, enemyBorgCount, enemyVtolCount, enemyBuildingCount
		);
		RATE_AA = ret.aa;
		RATE_AB = ret.ab;
		RATE_AP = ret.ap;
		RATE_AT = ret.at;
	}

}


// switch to pure hovers as soon as at least one enemy base or factory is beyond the sea
function tankReachibilityCheck(droid) {
	if (!iHaveHovers()) // woundn't make much sense to switch to hovers when we don't have hovers
		return false;
	if (RATE_TANK==0) // we have already switched, no need to check further
		return false;
	for (var i=0; i<maxPlayers; ++i) if (isAnEnemy(i)) {
		if (!droidCanReach(droid,startPositions[i].x,startPositions[i].y))
			return true;
	}
	for (var i=0; i<maxPlayers; ++i) if (isAnEnemy(i)) {
		for (var j=0; j<allInAttackTargets.length; ++j) {
			var list = enumStruct(i,allInAttackTargets[j]);
			for (var k=0; k<list.length; ++k)
				if (!droidCanReach(droid,list[k].x,list[k].y))
					return true;
		}
	}
	return false;
}

// returns a droid to base
function forceReturnToBase(droid) {
	var list=enumStruct(me,lab);
	if (list.length<=0)
		return;
	var i=random(list.length);
	orderDroidLoc(droid, DORDER_MOVE, list[i].x, list[i].y);
}

// returns a droid to base if not in base yet
function returnToBase(droid) {
	if (distBetweenTwoPoints(droid.x, droid.y, basePosition.x, basePosition.y) > BASE_SIZE)
		forceReturnToBase(droid);
}

// returns list of all enemies, that is not re-generated too often
function cachedEnumEnemies(col) {
	if (typeof(cachedEnumEnemies.cache)=="undefined") 
		cachedEnumEnemies.cache=[];
	if (typeof(cachedEnumEnemies.renewTime)=="undefined") 
		cachedEnumEnemies.renewTime=[];
	if (typeof(col)=="undefined")
		col=-1;
	if (typeof(cachedEnumEnemies.cache[col])=="undefined" || (gameTime - cachedEnumEnemies.renewTime[col] > 5000)) {
		cachedEnumEnemies.cache[col] = [];
		for (var enemy=0; enemy<maxPlayers; ++enemy) if (col==-1 || col==enemy) if (isAnEnemy(enemy)) {
			cachedEnumEnemies.cache[col] = cachedEnumEnemies.cache[col].concat(enumDroid(enemy));
		}
		// can't run isVTOL() on cached units, so need to cache its value as well
		for (var i=0; i<cachedEnumEnemies.cache[col].length; ++i)
			if (isVTOL(cachedEnumEnemies.cache[col][i]))
				cachedEnumEnemies.cache[col][i].isVTOL=1;
			else
				cachedEnumEnemies.cache[col][i].isVTOL=0;
		for (var enemy=0; enemy<maxPlayers; ++enemy) if (col==-1 || col==enemy) if (isAnEnemy(enemy)) {
			cachedEnumEnemies.cache[col] = cachedEnumEnemies.cache[col].concat(enumStruct(enemy));
		}
		if (scavengerPlayer!=-1 && col==-1) {
			cachedEnumEnemies.cache[col] = cachedEnumEnemies.cache[col].concat(enumDroid(scavengerPlayer));
			cachedEnumEnemies.cache[col] = cachedEnumEnemies.cache[col].concat(enumStruct(scavengerPlayer));
		}
		cachedEnumEnemies.renewTime[col] = gameTime;
	}
	return cachedEnumEnemies.cache[col];
}

// estimates enemy activity at particular point
function dangerLevel(x,y) {
	if (DEF_LIGHT==1) {
		return 0;
	}
	if (difficulty == EASY || difficulty == MEDIUM)
		return (safeDest(me,x,y)?0:1); // a non-cheating procedure here
	const DIST=BASE_SIZE/2; // deity-cheating procedure for higher difficulties follows
	var list=[];
	var badGuys = 0;
	if (typeof(enumRange)!="undefined") {
		list = enumRange(x,y,DIST,ENEMIES,false);
	} else {
		list = cachedEnumEnemies();
	}
	for (var i=0; i<list.length; ++i) 
		if (distBetweenTwoPoints(list[i].x,list[i].y,x,y) <= DIST)
			if (list[i].type == DROID || (list[i].type==STRUCTURE && list[i].stattype==DEFENSE))
				++badGuys;
	return badGuys;
}

// checks if a droid needs to be sent for repair
function needsRepair(droid) {
	if (droid.health < RETREAT_AT) 
		return true;
	if (droid.health >= 99)
		return false;
	var centers = enumStruct(me, repair);
	if (centers.length == 0)
		return false;
	for (var i=0; i<centers.length; ++i) {
		if (distBetweenTwoPoints(centers[i].x,centers[i].y,droid.x,droid.y) < 3)
			return true;
	}
	return false;
}

// sends a droid to a nearest repair center if necessary;
// returns true iff the droid was actually sent
function sendForRepair(droid) {
	if (status==STATUS_UNSTUCK)
		return false;
	if (!needsRepair(droid))
		return false;
	var centers = enumStruct(me, repair);
	for (var i=0; i<maxPlayers; ++i) if (allianceExistsBetween(me,i)) {
		centers = centers.concat(enumStruct(i, repair));
	}
	var gr=groupOfTank(droid);
	if (centers.length == 0)
		return false;
	var idx=-1, dist=Infinity;
	for (var i=0; i<centers.length; ++i) {
		if (centers[i].status != BUILT) 
			continue;
		var d=distBetweenTwoPoints(centers[i].x,centers[i].y,droid.x,droid.y);
		if (d<dist) {
			idx=i;
			dist=d;
		}
	}
	if (idx>-1)
		orderDroidLoc(droid,DORDER_MOVE,centers[idx].x,centers[idx].y);
	else
		returnToBase(droid);
	return true;
}

// checks if any droids around need to be sent for repair
function sendAllForRepair() {
	var list;
	for (var gr=0; gr<NUM_GROUPS; ++gr) {
		list=enumGroup(battleGroup[gr]);
		for (var i=0; i<list.length; ++i) 
			sendForRepair(list[i]);
	}
}


/* Status manipulations */


// changes the status and adjusts necessary things
function setStatus(newStatus) {
	status=newStatus;
	switch(status) {
		case STATUS_NORMAL:
			statusTime=0;
			break;
		case STATUS_PANIC:
			statusTime=3;
			break;
		case STATUS_EARLYGAME:
			statusTime=30;
			break;
		case STATUS_UNSTUCK:
			statusTime=3;
			break;
	}
}

// relaxes the status to normal and also panics sometimes
// called every 5 seconds
function updateStatus() {
	var goodGuys=0, badGuys=0;
	if (DEF_LIGHT==0) {
		for (var enemy=0; enemy<maxPlayers; ++enemy) {
			var list = enumDroid(enemy,DROID_WEAPON,me);
			list = list.concat(enumDroid(enemy,DROID_CYBORG,me));
			for (var i=0; i<list.length; ++i) 
				if (distBetweenTwoPoints(list[i].x,list[i].y,basePosition.x,basePosition.y) <= BASE_SIZE) {
					if (isAnEnemy(enemy))
						++badGuys;
					else
						++goodGuys;
				}
			
		}
		if (scavengerPlayer!=-1) {
			var list=enumDroid(scavengerPlayer,DROID_WEAPON,me);
			list = list.concat(enumDroid(scavengerPlayer,DROID_PERSON,me));
			for (var i=0; i<list.length; ++i) 
				if (distBetweenTwoPoints(list[i].x,list[i].y,basePosition.x,basePosition.y) <= BASE_SIZE)
					++badGuys;
		}
	}
	if (badGuys>goodGuys) {
		setStatus(STATUS_PANIC);
		return;
	}
	if (statusTime>0)
		--statusTime;
	else
		setStatus(STATUS_NORMAL);
}


/* Constructing the base */


// make sure builderGroup is non-empty
function balanceTrucks() {
	if (groupSize(builderGroup)==0 && groupSize(oilerGroup)>0) {
		var trucks=enumGroup(oilerGroup);
		for (var i=0; i<trucks.length; ++i) {
			groupAddDroid(builderGroup,trucks[i]);
			return; // add only one truck
		}
	}
}


// checks if a truck is busy doing something
function isTruckBusy(truck) {
	if (truck.order == DORDER_BUILD)
		return true;
	if (truck.order == DORDER_HELPBUILD)
		return true;
	if (truck.order == DORDER_LINEBUILD)
		return true;
	if (truck.order == DORDER_DEMOLISH)
		return true;
	return false;
	
}

// order the builderGroup to build some base structure somewhere
function buildBasicStructure(struct,forceLevel) {
	if (typeof(buildBasicStructure.lastCall)=="undefined")
		buildBasicStructure.lastCall=gameTime;
	if (gameTime - buildBasicStructure.lastCall < 2000)
		return false;
	if (typeof(forceLevel)=="undefined")
		forceLevel=1;
	if (typeof(struct)=="undefined")
		return false;
	var droidlist = enumGroup(builderGroup);
	if (droidlist.length == 0) 
		return false;
	var found=false;
	for (var i=0; i<droidlist.length; ++i)
		if (!isTruckBusy(droidlist[i])) {
			if (!found) {
				loc=pickStructLocation(
					droidlist[i], 
					struct, 
					droidlist[i].x, 
					droidlist[i].y, 
					0
				);
				if (typeof(loc)=="undefined")
					return false;
				if (dangerLevel(loc.x,loc.y)>0 && forceLevel == 0)
					return false;
				found=true;
				if (struct == command) // where is our base?
					basePosition=loc;
			}
			orderDroidBuild(droidlist[i], DORDER_BUILD, struct, loc.x, loc.y);
			buildBasicStructure.lastCall=gameTime;
		}
	return found;
}

// demolish some hopefully currently unnecessary structure
function recycleStructure(struct) {
	var droidlist = enumGroup(builderGroup);
	if (droidlist.length == 0)
		return false;
	for (var i=0; i<droidlist.length; ++i) if (!isTruckBusy(droidlist[i])) {
		orderDroidObj(droidlist[i], DORDER_DEMOLISH, struct);
		return true;
	}
	return false;
}

// build a specific module
function buildModule(struct, moduleType) {
	if (typeof(struct)=="undefined")
		return false;
	if (!isStructureAvailable(moduleType, me))
		return false;
	var droidlist = enumGroup(builderGroup);
	for (var i=0; i<droidlist.length; ++i)	
		if (!isTruckBusy(droidlist[i]))
			if (orderDroidBuild(droidlist[i], DORDER_BUILD, moduleType, struct.x, struct.y))
				return true;
	return false;
}

// build counter-battery towers
function buildCB() {
	var x=lastArtyAttackX, y = lastArtyAttackY;
	if (x == -1 || y == -1)
		return false;
	if (distanceToNearestStructure(x,y,[cbtower,])<MIN_SENSOR_DISTANCE) {
		lastArtyAttackX = lastArtyAttackY = -1;
		return false;
	}
	if (buildTower(x,y,[cbtower,],2,0)) {
		lastArtyAttackX = lastArtyAttackY = -1;
		return true;
	}
	return false;	
}

// build anti-air defenses at specific spots
function buildAA() {
	var x=lastAirAttackX, y = lastAirAttackY;
	if (x == -1 || y == -1)
		return false;
	if (buildTower(x,y,personality.antiair,2,0)) {
		lastAirAttackX = lastAirAttackY = -1;
		return true;
	}
	return false;	
}

// build defensive structures on gateways (v3.2+ only)
function buildGateways() {
	// what to do on v3.1, when the gateway list is not exposed to scripts yet
	if (typeof(enumGateways)=="undefined" || DEF_LIGHT == 1) {
		var i=random(personality.hardpoints.length);
		return buildTower(basePosition.x,basePosition.y,personality.hardpoints[i],0,0);
	}
	if (personality.hardpoints.length == 0) 
		return false;
	var gateways = enumGateways();
	if (gateways.length == 0)
		return false;
	var gate=gateways[random(gateways.length)];
	if (getNearestCCPlayer(gate.x2,gate.y2,1) != me) {
		queue("buildGateways",100);
		return;
	}
	if (gate.y2 < gate.y1) {
		var t=gate.y1;
		gate.y1=gate.y2;
		gate.y2=t;
	}
	if (gate.x2 < gate.x1) {
		var t=gate.x1;
		gate.x1=gate.x2;
		gate.x2=t;
	}
	var i=random(personality.hardpoints.length);
	return buildTower(gate.x1+random(gate.x2-gate.x1+1),gate.y1+random(gate.y2-gate.y1+1),personality.hardpoints[i],0,0);
}

// how many defenses do i have?
function countMyDefenses() {
	var list=enumStruct(me,DEFENSE);
	var count=0;
	for (var i=0; i<list.length; ++i)
		if (!isDedicatedAA(list[i]))
			++count;
	return count;
}

// build some defensive structure somewhere near (x,y)
function buildTower(x,y,list,maxDangerLevel,forceLevel) {
	if (maxDangerLevel == 0 && !safeDest(me,x,y))
		return false;
	var droidlist = enumGroup(oilerGroup);
	if (droidlist.length == 0 || list.length == 0) 
		return false;
	var tower;
	for (var i=0; i<list.length; ++i)
		if (isStructureAvailable(list[i],me)) {
			tower=list[i];
			break;
		}
	if (typeof(tower)=="undefined")
		return false;
	var loc = pickStructLocation(droidlist[0], tower, x, y, 0);
	if (typeof(loc)=="undefined")
		return false;
	if (list==sensors) { // not really the best way to check ...
		if (distanceToNearestStructure(loc.x,loc.y,sensors)<MIN_SENSOR_DISTANCE) {
			return false;
		}
	}
	if (maxDangerLevel == 0 && !safeDest(me,loc.x,loc.y))
		return false;
	for (var i=0; i<droidlist.length; ++i)
		if ((random(forceLevel+1)>0) || (!isTruckBusy(droidlist[i]) && droidlist[i].order != DORDER_MOVE)) {
			if (personality.DEFENSIVENESS>0) 
				if (forceLevel==0 && random(personality.DEFENSIVENESS)>0)
					return false;
			orderDroidBuild(droidlist[i], DORDER_BUILD, tower, loc.x, loc.y); 
			return true;
		}
	return false;
}

// demolish outdated towers
function sellOldDefenses() {
	var doublelist=personality.defenses;
	doublelist=doublelist.concat(personality.hardpoints);
	for (var i=0; i<doublelist.length; ++i)
		for (var j=3; j<doublelist[i].length; ++j)
			if (isStructureAvailable(doublelist[i][j-3],me)) {
				var list=enumStruct(me,doublelist[i][j]);
				if (list.length>0) {
					recycleStructure(list[0]);
					return;
				}
			}
	
}

// returns the command center owner nearest to (x,y)
function getNearestCCPlayer(x,y,all) {
	var minDist=Infinity, minPlayer;
	for (var ally=0; ally<maxPlayers; ++ally) if (allianceExistsBetween(me,ally) || me==ally || all==1) {
		var list=enumStruct(ally,command);
		for (var i=0; i<list.length; ++i) {
			var d=distBetweenTwoPoints(list[i].x,list[i].y,x,y);
			if (d<minDist) {
				minDist=d;
				minPlayer=ally;
			}
		}
	}
	return minPlayer;
}


// finds out the distance to the nearest structure of a certain type
function distanceToNearestStructure(x,y,structurelist) {
	var minDist=Infinity;
	for (var i=0; i<structurelist.length; ++i) {
		var list=enumStruct(me,structurelist[i]);
		for (var j=0; j<list.length; ++j) {
			var dist=distBetweenTwoPoints(x,y,list[j].x,list[j].y);
			if (dist<minDist)
				minDist=dist;
		}
	}
	return minDist;
}

// protect things
function buildDefenses() {
	if (earlyGame(personality.PEACE_TIME))
		return false;
	if (personality.DEFENSIVENESS == 0)
		queue("buildGateways");
	var oil=findHottestDerrick();
	if (typeof(oil)=="undefined") {
		oil={x:basePosition.x,y:basePosition.y};
	}
	if (isStructureAvailable(repair,me) && UNUSUAL_SITUATION==0 && random(2)==0) {
		if (personality.THIS_AI_MAKES_TANKS || personality.THIS_AI_MAKES_CYBORGS) {
			buildTower(oil.x,oil.y,[repair,],0,0);
			return;
		}
	}
	if (!iHaveCC()) 
		return;
	var success=false;
	if (RATE_AA>0 && (difficulty==HARD || difficulty == INSANE)) {
		success=buildTower(basePosition.x,basePosition.y,personality.antiair,0,0);
	} 
	var myDefenses=countMyDefenses();
	var tooManyDefenses=(personality.DEFENSIVENESS!=0 && myDefenses>30/Math.pow(personality.DEFENSIVENESS,0.5));
	if (personality.DEFENSIVENESS==0)
		tooManyDefenses=(random(100)<myDefenses);
	if (!success) {
		switch(random(5)) {
			case 4:
				success=buildTower(oil.x,oil.y,sensors,2,0);
				if (success) break;
			case 3:
			case 2:
				success=buildTower(basePosition.x,basePosition.y,personality.artillery[random(personality.artillery.length)],0,0);
				if (success) break;
			case 1:
				if (tooManyDefenses) {
					sellOldDefenses(); 
					break;
				}
				success=buildGateways();
				if (success) break;
			case 0:
				if (tooManyDefenses) {
					sellOldDefenses(); 
					break;
				}
				var i=random(personality.defenses.length);
				success=buildTower(oil.x,oil.y,personality.defenses[i],0,0);
				if (success) break;
		}
	}
	if (success)
		queue("buildDefenses",300);
}

// expand the base by building more labs and factories
function keepBuildingThings() {
	var labCount = enumStruct(me, lab).length;
	var factoryCount = enumStruct(me, factory).length;
	var ccCount = enumStruct(me, command).length;
	var genCount = enumStruct(me, generator).length;
	var cbCount = enumStruct(me, cbtower).length;
	var derrickCount = enumStruct(me, derrick).length;
	// force building at least something when build order fails due to danger
	if (genCount == 0 && isStructureAvailable(generator, me))
		return buildBasicStructure(generator);
	if (factoryCount == 0 && isStructureAvailable(factory, me))
		return buildBasicStructure(factory);
	if (labCount == 0 && isStructureAvailable(lab, me))
		return buildBasicStructure(lab);
	if (ccCount == 0 && isStructureAvailable(command, me))
		return buildBasicStructure(command);
	// build generators when necessary
	if (genCount < 2 || derrickCount - 4*genCount > 0)
		if (isStructureAvailable(generator,me))
			if (UNUSUAL_SITUATION==0) 
				if (playerPower(me) < LOW_POWER) {
					buildBasicStructure(generator,0);
					return;
				}
	// build VTOL pads when necessary
	if (personality.THIS_AI_MAKES_VTOLS) {
		if (enumStruct(me, vtolfac).length > 0) {
			var vtolList = enumDroid(me, DROID_WEAPON);
			var vtolCount=0;
			for (var i=0; i<vtolList.length; ++i)
				if (isVTOL(vtolList[i]))
					++vtolCount;
			var padCount = enumStruct(me, vtolpad).length;
			if (NUM_VTOL_PADS*padCount < vtolCount && isStructureAvailable(vtolpad,me)) {
				buildBasicStructure(vtolpad,0);
				return;
			}
		}
	}
	// some advanced build order stuff
	if (cbCount < 1 && isStructureAvailable(cbtower,me)) 
		buildBasicStructure(cbtower,0);
	else if (isStructureAvailable(satlink,me))
		buildBasicStructure(satlink,0);
	else if (isStructureAvailable(lassat,me))
		buildBasicStructure(lassat,0);
	else if (RATE_TANK==0 && isStructureAvailable(lab,me) && enumStruct(me,lab).length<4 && !iHaveHovers()) {
		buildBasicStructure(lab,0); // research hovers faster when having no ground enemies
	} else {
		// build defenses
		if (personality.DEFENSIVENESS==0 || random(personality.DEFENSIVENESS+1) == 0 || playerPower(me) > LOW_POWER) {
			queue("buildDefenses");
		} 
		// use extra power for expansion
		if (playerPower(me) > (personality.DEFENSIVENESS>0?LOW_POWER:2000) && UNUSUAL_SITUATION==0) {
			var list=[];
			if (isStructureAvailable(lab,me))
				list[list.length]=lab;
			if (isStructureAvailable(factory,me) && personality.THIS_AI_MAKES_TANKS)
				list[list.length]=factory;
			if (isStructureAvailable(borgfac,me) && personality.THIS_AI_MAKES_CYBORGS && RATE_TANK>0)
				list[list.length]=borgfac;
			if (isStructureAvailable(vtolfac,me) && personality.THIS_AI_MAKES_VTOLS)
				list[list.length]=vtolfac;
			buildBasicStructure(list[random(list.length)],0);
		}
	}	
}

// the main function responsible for building things
// calls other functions in a particular order
function executeBuildOrder() {
	if (!finishBuildings())
		if (!buildOrder())
			if (!upgradeStructures())
				if (!buildCB())
					if (!buildAA())
						queue("keepBuildingThings");
	
}

// should we panic if this structure is under attack or unfinished?
function isBaseStructure(struct) {
	return (struct.stattype != RESOURCE_EXTRACTOR && struct.stattype != DEFENSE && struct.stattype != REPAIR_FACILITY);
}

// finish buildings that were never finished yet
function finishBuildings() {
	var droidlist; 
	var list = enumStruct(me);
	var found = false;
	for (j=0; j<list.length; ++j) {
		if (list[j].status == BEING_BUILT) {
			if (dangerLevel(list[j].x,list[j].y)<=0) {
				droidlist=enumGroup(oilerGroup);
				var base=isBaseStructure(list[j]);
				for (var i=0; i<droidlist.length && (!found || base); ++i)	
					if (!isTruckBusy(droidlist[i]) && (droidlist[i].order != DORDER_MOVE))
						if (distBetweenTwoPoints(droidlist[i].x,droidlist[i].y,list[j].x,list[j].y)<BASE_SIZE || base)
							orderDroidObj(droidlist[i], DORDER_HELPBUILD, list[j]), found=true;
				if (base) {
					droidlist=enumGroup(builderGroup);
					for (var i=0; i<droidlist.length; ++i)	
						if (!isTruckBusy(droidlist[i]))
							orderDroidObj(droidlist[i], DORDER_HELPBUILD, list[j]), found=true;
				}
			}
		}
		if (found) return true;
	}
	return found;
}

// build different sorts of modules
function upgradeStructures() {
	var list;
	list = enumStruct(me, generator);
	for (var i=0; i<list.length; ++i) 
		if (list[i].modules < 1) 
			if (buildModule(list[i], pmod)) 
				return true;
	// don't build factory and research modules too much
	// if low on power
	if (playerPower(me) < LOW_POWER)
		if (random(5)>0)
			return false;
	list = enumStruct(me, factory);
	for (var i=0; i<list.length; ++i) 
		if (list[i].modules < 2) 
			if (buildModule(list[i], fmod)) 
				return true;
	list = enumStruct(me, lab);
	for (var i=0; i<list.length; ++i) 
		if (list[i].modules < 1) 
			if (buildModule(list[i], rmod)) 
				return true;
	list = enumStruct(me, vtolfac);
	for (var i=0; i<list.length; ++i) 
		if (list[i].modules < 1) 
			if (buildModule(list[i], fmod)) 
				return true;
	return false;
}

// a constructor for Derrick Stat object
function constructDerrickStat(pos) {
	this.x = pos.x; 
	this.y = pos.y;
	this.numBuilders = 0; // >0 if some trucks were sent recently to build derrick here
	this.hotness = -1; // the amount of times this derrick was rebuilt
}

// finds an appropriate derrickStats entry for derrick
function findDerrickStat(obj) {
	if (typeof(derrickStats)=="undefined")
		derrickStats=[];
	for (var i=0; i<derrickStats.length; ++i) {
		if (derrickStats[i].x == obj.x && derrickStats[i].y == obj.y)
			return i;
	}
	var n=derrickStats.length;
	derrickStats[n]=new constructDerrickStat(obj);
	return n;
}

// forgets that a derrick was already ordered
// forgets that the group doesn't currently want to regroup
// forgets that some factories are occupied with producing trucks
function relaxStats() {
	if (typeof(derrickStats)=="undefined")
		derrickStats=[];
	for (var i=0; i<derrickStats.length; ++i) {
		if (derrickStats[i].numBuilders>0)
			--derrickStats[i].numBuilders;
	}
	for (var i=0; i<NUM_GROUPS; ++i) {
		if (groupInfo[i].delayregroup>0) 
			--groupInfo[i].delayregroup;
		if (groupInfo[i].delaystaticregroup>0) 
			--groupInfo[i].delaystaticregroup;
		if (groupInfo[i].targetAge>0) 
			--groupInfo[i].targetAge;
	}
}

function findHottestDerrick() {
	var oils=enumStruct(me,derrick);
	var list=[];
	for (var i=0; i<oils.length; ++i) {
		var h=derrickStats[findDerrickStat(oils[i])].hotness;
		for (var j=0; j<h; ++j) {
			list[list.length]=oils[i];
		}
	}
	if (list.length>0) 
		return list[random(list.length)];
	else
		return oils[random(oils.length)];
}

// tell idle units to pick up oil drums
function pickupOilDrums() {
	if (typeof(pickupOilDrums.lastCall)!="undefined" && gameTime - pickupOilDrums.lastCall < 20000) // don't call this too often
		return;
	pickupOilDrums.lastCall = gameTime;
	if (typeof(DORDER_RECOVER)=="undefined") // HACK: this constant isn't exposed for scripts yet
		DORDER_RECOVER=33;
	if (status == STATUS_PANIC)
		return;
	var drums=enumFeature(-1,oildrum);
	var list=enumGroup(oilerGroup);
	list=list.concat(enumGroup(defendGroup));
	for (var i=0; i<list.length; ++i) {
		var droid=list[i];
		if (isTruckBusy(droid))
			continue;
		var minDist=Infinity, minObj;
		for (var j=0; j<drums.length; ++j) {
			var drum=drums[j];
			if (droidCanReach(droid,drum.x,drum.y)) {
				var dist=distBetweenTwoPoints(drum.x,drum.y,droid.x,droid.y);
				if (dist < minDist) {
					minDist=dist;
					minObj=drum;
				}
			}
		}
		if (typeof(minObj)!="undefined")
			if (dangerLevel(minObj.x,minObj.y)<=0) {
				orderDroidObj(droid,DORDER_RECOVER,minObj);
				return;
			}
	}
}

// order free oil-oriented trucks to build oil derricks
function huntForOil() {
	var droidlist = enumGroup(oilerGroup);
	var list = enumFeature(-1, oilres);
	var oillist = [];
	if (typeof(huntForOil.cachedOilList)=="undefined" || gameTime - huntForOil.cacheUpdateTime > 10000) {
		for (var j=0; j<list.length; ++j) {
			var stat=findDerrickStat(list[j]);
			if (derrickStats[stat].numBuilders<=0) {
				var danger=false;
				if (list.length > 20) {
					if (!safeDest(me,list[j].x,list[j].y))
						danger=true;
				} else {
					if (dangerLevel(list[j].x,list[j].y)>0)
						danger=true;
				}
				if (!danger) {
					var tmp=getNearestCCPlayer(list[j].x,list[j].y,0); // friendly AIs shouldn't capture your oil
					if (typeof(tmp)=="undefined" || tmp==me) {
						var n=oillist.length;
						oillist[n]=list[j];
						oillist[n].stat=stat;
					}
				}
			}
		}
		huntForOil.cachedOilList = oillist;
		huntForOil.cacheUpdateTime = gameTime;
	} else {
		oillist = huntForOil.cachedOilList;
	}
	if (oillist.length <= 0) { // not much to do here ...
		pickupOilDrums();
		return;
	}
	for (var i=0; i<droidlist.length; ++i) if (!isTruckBusy(droidlist[i])) {
		var range1=Infinity;
		var k=-1;
		for (var j=0; j<oillist.length; ++j) {
			if (derrickStats[oillist[j].stat].numBuilders<=0) {
				if (droidCanReach(droidlist[i],oillist[j].x,oillist[j].y)) {
					var range2=distBetweenTwoPoints(droidlist[i].x, droidlist[i].y, oillist[j].x, oillist[j].y);
					if (range2<range1) {
						range1=range2;
						k=j;
					}
				}
			}
		}
		if (k!=-1) {
			orderDroidBuild(droidlist[i], DORDER_BUILD, derrick, oillist[k].x, oillist[k].y);
			//make sure not too many trucks build the same oil
			// 5 here makes sure that at least five calls of relaxStats() occur before the stat is relaxed
			// which takes 40 to 50 seconds
			derrickStats[findDerrickStat(oillist[k])].numBuilders = 5; 
		} //else {
			//return to base if no oils are available for capturing
			/*if (droidlist[i].order != DORDER_MOVE)
				returnToBase(droidlist[i]);*/
		//}
	}

}


/* Researching technologies */


// make a particular lab follow a certain research path
// even though pursueResearch() accepts paths, 
// its behaviour seems to be less deterministic than this
function followResearchPath(lab,path) {
	for (var j=0; j<path.length; ++j)
		if (pursueResearch(lab,path[j])) {
			return true;
		}
	return false;
}

// come up with something
function doResearch() {
	if (!dontRunOutOfPower())
		return;
	var lablist=enumStruct(me, lab);
	for (var i=0; i<lablist.length; ++i) if (structureReady(lablist[i])) {
		if (RATE_TANK==0) { // we seriously need hovers as soon as possible
			if (followResearchPath(lablist[i],["R-Sys-Engineering01","R-Struc-Research-Module","R-Vehicle-Prop-Hover",])) {
				continue;
			}
		}
		if (!followResearchPath(lablist[i],personality.researchPathPrimary)) {
			// pick some branchy research if the primary research path is finished
			var economy=enumStruct(me,derrick).length;
			if (random(100+economy*5)>100) // with 40 oils, it will pick fundamental research in 2/3 labs
				if (followResearchPath(lablist[i],personality.researchPathFundamental)) {
					continue;
				}
			var ret=enemyCountBalance(-1);
			// vtols can be really scary, so give a bit more weight to AA research
			ret.vtol = ret.vtol * 3 + ((earlyGame(20) && !HIGH_TECH_START==1)?0:1);
			// don't forget completely about borgs
			ret.borg += 5;
			if (difficulty == EASY || difficulty == MEDIUM) {
				ret.tank = 4;
				ret.borg = 4;
				ret.vtol = 1;
				ret.defs = 1;
			}
			var total=ret.tank+ret.borg+ret.vtol+ret.defs;
			if (random(total)<ret.borg)
				if (followResearchPath(lablist[i],personality.researchPathAP)) {
					continue;
				}
			total=ret.tank+ret.vtol+ret.defs;
			if (random(total)<ret.tank)
				if (followResearchPath(lablist[i],personality.researchPathAT)) {
					continue;
				}
			total=ret.vtol+ret.defs;
			if (random(total)<ret.vtol)
				if (followResearchPath(lablist[i],personality.researchPathAA)) {
					continue;
				}
			if (!followResearchPath(lablist[i],personality.researchPathAB)) {
				// research crap if nothing to do anyway
				var reslist = enumResearch();
				if (reslist.length > 0)
				{
					var idx = random(reslist.length);
					pursueResearch(lablist[i], reslist[idx].name);
				}
			}
		} 
	}
}


/* Producing droids */	

// not sure this check is necessary ... why not
function structureReady(struct) {
	return structureIdle(struct) && (struct.status == BUILT);
}


// finds out which battle group contains this tank
function groupOfTank(tank) {
	for (i=0; i<NUM_GROUPS; ++i)
		if (tank.group == battleGroup[i])
			return i;
}

// pick a suitable group for a new-born truck
function addTruckToSomeGroup(truck) {
	if (groupSize(builderGroup) < personality.MIN_BUILDERS)
		groupAddDroid(builderGroup,truck);
	else if (groupSize(oilerGroup) < personality.MIN_OILERS)
		groupAddDroid(oilerGroup,truck);
	else if (groupSize(builderGroup) < personality.MAX_BUILDERS)
		groupAddDroid(builderGroup,truck);
	else if (groupSize(oilerGroup) < personality.MAX_OILERS)
		groupAddDroid(oilerGroup,truck);
	else if (groupSize(builderGroup)+groupSize(oilerGroup)>ABS_MAX_TRUCKS)
		groupAddDroid(oilerGroup,truck)
	else if (random(2)==0)
		groupAddDroid(builderGroup,truck);
	else
		groupAddDroid(oilerGroup,truck);
}

// checks if adding a tank of type defined by `at' parameter would
// bring some group closer to its AT/AP weapon balance
// at=true for AT tanks, at=false for AP tanks
function groupNeedsThisSortOfTank(gr,name) {
	var pt=groupCountBalance(gr);
	var tb=enemyCountBalance(groupInfo[gr].enemy);
	var ret=adaptationMatrix(pt.ap, pt.at, pt.aa, pt.ab, tb.tank, tb.borg, tb.vtol, tb.defs);
	if (name.indexOf("AP")>0)
		return ret.ap>0;
	if (name.indexOf("AT")>0)
		return ret.at>0;
	if (name.indexOf("AA")>0)
		return ret.aa>0;
	if (name.indexOf("AB")>0)
		return ret.ab>0;
}

// how many non-empty combat groups do we have?
function listNonEmptyGroups() {
	var ret=[];
	for (var i=0; i<NUM_GROUPS; ++i)
		if (groupSize(battleGroup[i])>0)
			ret[ret.length]=i;
	return ret;
}

// pick a suitable group for a new-born tank
function addTankToSomeGroup(tank) {
	forceReturnToBase(tank);
	if (tank.player!=me) {
		debug("BUG: Player ",me,"trying to put tank",tank.id,"in a group, but the tank belongs to player",tank.player);
		return;
	}
	if (random(100)<countEnemyTeams()*3 && groupSize(defendGroup)<MAX_WARRIORS) {
		groupAddDroid(defendGroup, tank);
		return;
	}
	for (var i=0; i<NUM_GROUPS; ++i)  {
		if (typeof(groupInfo[i].enemy)=="undefined" || !isAnEnemy(groupInfo[i].enemy))
			groupInfo[i].enemy=findEnemy();
	}
	var maxCount=-1, maxIdx=-1;
	// find the biggest non-full group that needs this sort of tank
	for (var i=0; i<NUM_GROUPS; ++i)  {
		if (typeof(groupInfo[i].enemy)=="undefined")
			continue;
		if (!droidCanReach(tank,startPositions[groupInfo[i].enemy].x,startPositions[groupInfo[i].enemy].y))
			continue;
		var count=groupSize(battleGroup[i]);
		if (count<NUM_WARRIORS && count>0 && groupNeedsThisSortOfTank(i,tank.name))
			if (count > maxCount) {
				maxCount = count;
				maxIdx = i;
			}
	}
	if (maxIdx!=-1) {
		groupAddDroid(battleGroup[maxIdx], tank);
		return;
	}
	maxCount=-1, maxIdx=-1;
	// if nobody needs this sort of tank, find the biggest non-full group
	for (var i=0; i<NUM_GROUPS; ++i) {
		if (typeof(groupInfo[i].enemy)=="undefined")
			continue;
		if (!droidCanReach(tank,startPositions[groupInfo[i].enemy].x,startPositions[groupInfo[i].enemy].y))
			continue;
		var count=groupSize(battleGroup[i]);
		if (count<NUM_WARRIORS && count>0)
			if (count > maxCount) {
				maxCount = count;
				maxIdx = i;
			}
	}
	if (maxIdx!=-1) {
		groupAddDroid(battleGroup[maxIdx], tank);
		return;
	}
	// if all groups are full, fill until absolute maximum
	// but still consider AT/AP balance
	for (var i=0; i<NUM_GROUPS; ++i) {
		if (typeof(groupInfo[i].enemy)=="undefined")
			continue;
		if (!droidCanReach(tank,startPositions[groupInfo[i].enemy].x,startPositions[groupInfo[i].enemy].y))
			continue;
		var count=groupSize(battleGroup[i]);
		if (count<MAX_WARRIORS && count>0 && groupNeedsThisSortOfTank(i,tank.name)) {
			groupAddDroid(battleGroup[i], tank);
			return;
		}
	}
	// if considering AT/AP balance failed, stop considering it
	for (var i=0; i<NUM_GROUPS; ++i) {
		if (typeof(groupInfo[i].enemy)=="undefined")
			continue;
		if (!droidCanReach(tank,startPositions[groupInfo[i].enemy].x,startPositions[groupInfo[i].enemy].y))
			continue;
		var count=groupSize(battleGroup[i]);
		if (count<MAX_WARRIORS && count>0) {
			groupAddDroid(battleGroup[i], tank);
			return;
		}
	}
	var ret=listNonEmptyGroups();
	if (ret.length < countEnemies()) {
		// if we just have no groups yet, put the tank into a new group
		for (var i=0; i<NUM_GROUPS; ++i) 
			if (groupSize(battleGroup[i])==0) {
				groupAddDroid(battleGroup[i], tank);
				return;
			}
	} else {
		// huh, still no success? come on, just put it anywhere already
		groupAddDroid(battleGroup[ret[random(ret.length)]], tank);
	}
}

// pick a suitable group for a new-born vtol
function addVtolToSomeGroup(vtol) {
	// find the biggest non-full group
	var maxCount=-1, maxIdx=-1;
	for (var i=0; i<NUM_VTOL_GROUPS; ++i) {
		var count=groupSize(vtolGroup[i])
		if (count<NUM_VTOLS)
			if (count > maxCount) {
				maxCount = count;
				maxIdx = i;
			}
	}
	if (maxIdx!=-1) {
		groupAddDroid(vtolGroup[maxIdx], vtol);
		return;
	}
	// if all groups are full, fill until maximum
	for (var i=0; i<NUM_VTOL_GROUPS; ++i)
		if (groupSize(vtolGroup[i])<MAX_VTOLS) {
			groupAddDroid(vtolGroup[i], vtol);
			return;
		}
	// if all groups are at maximum, add a new group
	if (NUM_VTOL_GROUPS < MAX_VTOL_GROUPS) {
		vtolGroup[NUM_VTOL_GROUPS]=newGroup();
		groupAddDroid(vtolGroup[NUM_VTOL_GROUPS], vtol);
		++NUM_VTOL_GROUPS;
		return;
	}
	// if too many groups, just put it somewhere
	groupAddDroid(vtolGroup[random(NUM_VTOL_GROUPS)], vtol);
}

// checks if we actually need more trucks
function needMoreTrucks() {
	if (status==STATUS_PANIC && personality.THIS_AI_MAKES_TANKS)
		return false;
	if (iHaveHovers()) {
		if (groupSize(oilerGroup)+groupSize(builderGroup)>=ABS_MAX_HOVER_TRUCKS)
			return false;
	} else {
		if (groupSize(oilerGroup)+groupSize(builderGroup)>=ABS_MAX_TRUCKS)
			return false;
	}
	var weHaveHoverTrucks=false;
	var list=enumGroup(oilerGroup);
	for (var i=0; i<list.length; ++i) {
		if (list[i].name.indexOf("Hover")>-1) {
			weHaveHoverTrucks = true;
			break;
		}
	}
	if (iHaveHovers() && !weHaveHoverTrucks)
		return true;
	if (groupSize(oilerGroup) >= personality.MAX_OILERS && groupSize(builderGroup) >= personality.MAX_BUILDERS)
		if (playerPower(me)<LOW_POWER || enumDroid(me,DROID_WEAPON).length < personality.MIN_DEFENDERS)
			return false;
	if (groupSize(oilerGroup) >= personality.MIN_OILERS && groupSize(builderGroup) >= personality.MIN_BUILDERS)
		if (enumDroid(me,DROID_WEAPON).length < personality.MIN_DEFENDERS)
			return false;
	return true;
}

// produce a tank with an anti-personnel weapon in some factory
function produceAPTank(struct) {
	var iap=random(personality.apWeapons.length);
	var ib=random(personality.tankBodies.length);
	var prop = personality.tankPropulsions;
	if (typeof(personality.tankPropulsionsAP)!="undefined")
		prop=personality.tankPropulsionsAP;
	if (random(RATE_TANK+RATE_HOVER) < RATE_HOVER)
		prop=standardTankPropulsionsHover;
	var ip=random(prop.length);
	return buildDroid(struct, "AP Tank", personality.tankBodies[ib], prop[ip], "", DROID_WEAPON, personality.apWeapons[iap])	
}

// produce a tank with an anti-tank weapon in some factory
function produceATTank(struct) {
	var iat=random(personality.atWeapons.length);
	var ib=random(personality.tankBodies.length);
	var prop = personality.tankPropulsions;
	if (typeof(personality.tankPropulsionsAT)!="undefined")
		prop=personality.tankPropulsionsAT;
	if (random(RATE_TANK+RATE_HOVER) < RATE_HOVER)
		prop=standardTankPropulsionsHover;
	var ip=random(prop.length);
	return buildDroid(struct, "AT Tank", personality.tankBodies[ib], prop[ip], "", DROID_WEAPON, personality.atWeapons[iat])	
}

// produce a tank with an anti-building weapon in some factory
function produceABTank(struct) {
	var abtanks=0, sensortanks=0;
	for (var gr=0; gr<NUM_GROUPS; ++gr) {
		var ret=groupCountBalance(gr);
		abtanks += ret.ab;
		sensortanks += ret.se;
	}
	if (personality.THIS_AI_MAKES_TANKS && ((sensortanks*5+1 > abtanks) || enumDroid(me,DROID_WEAPON).length < personality.MIN_DEFENDERS)) {
		var iab=random(personality.abWeapons.length);
		var ib=random(personality.tankBodies.length);
		var prop = personality.tankPropulsions;
		if (typeof(personality.tankPropulsionsAB)!="undefined")
			prop=personality.tankPropulsionsAB;
		if (random(RATE_TANK+RATE_HOVER) < RATE_HOVER)
			prop=standardTankPropulsionsHover;
		var ip=random(prop.length);
		return buildDroid(struct, "AB Tank", personality.tankBodies[ib], prop[ip], "", DROID_WEAPON, personality.abWeapons[iab])	
	} else if (sensortanks < MAX_SENSOR_TANKS) {
		var prop=truckPropulsions;
		if (RATE_TANK == 0)
			prop=["hover01",];
		return buildDroid(struct, "Sensor Tank", truckBodies, prop, "", DROID_SENSOR, sensorTurrets);
	}
}

// produce a tank with an anti-air weapon in some factory
function produceAATank(struct) {
	var iaa=random(personality.aaWeapons.length);
	var ib=random(personality.tankBodies.length);
	var prop = personality.tankPropulsions;
	if (typeof(personality.tankPropulsionsAA)!="undefined")
		prop=personality.tankPropulsionsAA;
	if (random(RATE_TANK+RATE_HOVER) < RATE_HOVER)
		prop=standardTankPropulsionsHover;
	var ip=random(prop.length);
	return buildDroid(struct, "AA Tank", personality.tankBodies[ib], prop[ip], "", DROID_WEAPON, personality.aaWeapons[iaa])	
}

// produce a cyborg with an anti-personnel weapon in some factory
function produceAPCyborg(struct) {
	var iap=random(personality.apCyborgStats.length);
	for (var j=0; j<personality.apCyborgStats[iap].length; ++j) {
		var s = personality.apCyborgStats[iap][j];
		if (buildDroid(struct, "AP Cyborg", s[0], "CyborgLegs", "", DROID_CYBORG, s[1]))
			return true;
	}
	return false;
}

// produce a cyborg with an anti-tank weapon in some factory
function produceATCyborg(struct) {
	var iat=random(personality.atCyborgStats.length);
	for (var j=0; j<personality.atCyborgStats[iat].length; ++j) {
		var s = personality.atCyborgStats[iat][j];
		if (buildDroid(struct, "AT Cyborg", s[0], "CyborgLegs", "", DROID_CYBORG, s[1]))
			return true;
	}
	return false;
}

// get back to work!
function produceDroids() {
	var truckCount=0;
	var factories = enumStruct(me, factory);
	var iHaveFactories=false;
	for (var i=0; i<factories.length; ++i) {
		struct=factories[i];
		if (structureReady(struct)) {
			iHaveFactories = true;
			if (needMoreTrucks() && truckCount<2) {
				if (iHaveHovers()) {
					buildDroid(struct, "Hover Construction Droid", truckBodies, ["hover01",], "", DROID_CONSTRUCT, "Spade1Mk1");
				} else {
					buildDroid(struct, "Construction Droid", truckBodies, truckPropulsions, "", DROID_CONSTRUCT, "Spade1Mk1");
				}
				++truckCount;
				continue;
			}
			if ((!personality.THIS_AI_MAKES_TANKS) && iHaveArty()) {
				produceABTank(struct); // make sensors, actually
				break;
			}
			// we shouldn't produce tanks without a cc to avoid template cheating
			if (!iHaveCC()) 
				break;
			// we shouldn't produce heavy cannon leopard half-tracks because people will laugh at us
			if (HIGH_TECH_START==1) {
				if (struct.modules==0) {
					continue;
				}
			}
			var j=random(RATE_AP+RATE_AT+RATE_AA+RATE_AB);
			if (j < RATE_AA) {
				if (produceAATank(struct))
					continue;
			} 
			j=random(RATE_AP+RATE_AT+RATE_AB);
			if (j < RATE_AB) {
				if (produceABTank(struct)) 
					continue;
			} 
			j=random(RATE_AP+RATE_AT);
			if (j < RATE_AT)
				if (produceATTank(struct))
					continue;
			produceAPTank(struct);
		}
	}
	var borgfacs = enumStruct(me, borgfac);
	for (var i=0; i<borgfacs.length; ++i) {
		struct=borgfacs[i];
		if (structureReady(struct)) {
			if (needMoreTrucks() && truckCount<2 && (!iHaveFactories || !iHaveHovers())) { // don't build borgs when we can build hovers
				// NOTE: cyborg engineers can't be produced at all in v3.1, see http://developer.wz2100.net/ticket/3133
				// also, that's a very dirty version check; but we can't rely on version variable due to the problems described in 
				// http://developer.wz2100.net/ticket/3187 
				if (typeof(DORDER_RTB)!="undefined") {
					buildDroid(struct, "Construction Droid", "Cyb-Bod-ComEng", "CyborgLegs", "", DROID_CONSTRUCT, "CyborgSpade");
					++truckCount;
					continue;
				}
			}
			if (!personality.THIS_AI_MAKES_CYBORGS || RATE_TANK == 0)
				break;
			if (random(RATE_AP+RATE_AT) < RATE_AP)
				produceAPCyborg(struct);
			else
				produceATCyborg(struct);
		}
	}
	var vtolfacs = enumStruct(me, vtolfac);
	if (personality.THIS_AI_MAKES_VTOLS) {
		for (var i=0; i<vtolfacs.length; ++i) {
			var struct=vtolfacs[i];
			if (structureReady(struct)) {
				buildDroid(struct, "VTOL", personality.vtolBodies, vtolPropulsions, "", DROID_WEAPON, personality.vtolWeapons);
			}
		}
	}
}


/* Attacking the enemy */


// checks if a player was not defeated yet
function isAlive(player) {
	for (var i=0; i<allInAttackTargets.length; ++i)
		if (enumStruct(player, allInAttackTargets[i]).length>0)
			return true;
	if (enumDroid(player).length>0)
		return true;
	return false;
}

// checks if the enemy is actually an threat
function isAnEnemy(enemy) {
	if (typeof(enemy)=="undefined")
		return false;
	if (enemy < 0 || enemy >= maxPlayers) {
		// probably scavengers ...
		return false;
	}
	if (allianceExistsBetween(me, enemy)) 
		return false;
	if (isAlive(enemy))
		return true;
	return false;
}

// cycle through enemies
function nextEnemy(enemy) {
	var i=enemy;
	do {
		++i;
		if (i>maxPlayers) 
			i=0;
		if (isAnEnemy(i) && enemyInfo[i].reachable)
			return i;
	} while (i!=enemy);
	do {
		++i;
		if (i>maxPlayers) 
			i=0;
		if (isAnEnemy(i))
			return i;
	} while (i!=enemy);
	return i;
}

// how many enemies left
function countEnemies() {
	var ret=0;
	for (var i=0; i<maxPlayers; ++i)
		if (isAnEnemy(i))
			++ret;
	return ret;
}

// hom many allies do i have
function countAllies() {
	var ret=0;
	for (var i=0; i<maxPlayers; ++i)
		if (allianceExistsBetween(me,i))
			if (isAlive(i))
				++ret;
	return ret;
	
}

// how many teams the enemies are split into
function countEnemyTeams() {
	var forces=[];
	for (var i=0; i<maxPlayers; ++i) if (isAnEnemy(i)) {
		var found=false;
		for (var j=0; j<forces.length; ++j) 
			if (allianceExistsBetween(i,forces[j])) {
				found=true;
				break;
			}
		if (!found)
			forces[forces.length]=i;
	}
	return forces.length;
}

// picks an enemy player to attack
function findEnemy() {
	var enemies=[];
	for (var i=0; i<maxPlayers; ++i)
		if (isAnEnemy(i) && enemyInfo[i].reachable) // prefer ground enemies
			enemies[enemies.length]=i;
	if (enemies.length>0)
		return enemies[random(enemies.length)];
	enemies=[];
	for (var i=0; i<maxPlayers; ++i)
		if (isAnEnemy(i))
			enemies[enemies.length]=i;
	return enemies[random(enemies.length)];
	
}

// checks if we have a huge advantage over certain enemy
function okToAllIn(enemy) {
	if (!isAlive(enemy))
		return false; // this is rather a clean-up, we need to look for derricks *again* when the opponent is almost dead
	if (!earlyGame(personality.PEACE_TIME)) {
		var goodGuys=0, badGuys=0;
		badGuys += enumDroid(enemy, DROID_WEAPON).length;
		badGuys += enumDroid(enemy, DROID_CYBORG).length;
		var enemyStructs=enumStruct(enemy);
		for (var i=0; i<enemyStructs.length; ++i)
			if (enemyStructs[i].stattype == DEFENSE)
				if (!isDedicatedAA(enemyStructs[i]))
					++badGuys;
		for (var i=0; i<NUM_GROUPS; ++i) {
			if (groupInfo[i].enemy==enemy)
				goodGuys+=groupSize(battleGroup[i]);
		}
		if (2*badGuys < goodGuys) {
			return true;
		}
	}
	return false;
}

// finds an appropriate target within enemy (enemy=-1 to choose random enemy)
function findTarget(enemy) {
	if (enemy==-1)
		enemy=findEnemy();
	if (typeof(enemy)=="undefined")
		return;
	var list=allInAttackTargets;
	if (scavengerPlayer!=-1) 
		if (!okToAllIn(enemy)) 
			list=attackTargets.concat(enumStruct(scavengerPlayer, derrick));
	var obj=undefined, danger=Infinity, distance=Infinity;
	for (var i=list.length-1; i>=0; --i) {
		var structs=enumStruct(enemy, list[i]);
		if (structs.length > 0) {
			for (var j=0; j<structs.length; ++j) {
				var _danger = dangerLevel(structs[j].x,structs[j].y)
				var _distance = distBetweenTwoPoints(basePosition.x,basePosition.y,structs[j].x,structs[j].y);
				if ((_danger<danger) || (_danger==danger && _distance<distance)) {
					danger=_danger;
					distance=_distance;
					obj=structs[j];
				}
			}
		}
	}
	if (typeof(obj)!="undefined")
		return obj;
	var droids=enumDroid(enemy);
	if (droids.length > 0) {
		return droids[random(droids.length)];
	}
}

// checks if the target set for a group is already dead
// and updates if necessary
function getGroupTarget(gr) {
	if (groupInfo[gr].targetAge>0) { // if doesn't need updating yet
		// check if the currently selected target is valid
		var x=groupInfo[gr].targetX;
		var y=groupInfo[gr].targetY;
		if (x!=-1 && y!=-1) {
			if (groupInfo[gr].targetType != TARGET_DROID) {
				for (var i=0; i<attackTargets.length; ++i)
					for (var j=0; j<maxPlayers; ++j) if (isAnEnemy(j)) {
						var list=enumStruct(j,attackTargets[i]);
						for (var k=0; k<list.length; ++k)
							if (list[k].x==x && list[k].y==y) {
								return list[k];
							}
					}
			} else {
				for (var j=0; j<maxPlayers; ++j) if (isAnEnemy(j)) {
					var list=enumDroid(j);
					for (var k=0; k<list.length; ++k)
						if (distBetweenTwoPoints(x,y,list[k].x,list[k].y==y)<3) {
							return list[k];
						}
				}
			}
		}
		}
	// pick a new one otherwise
	if (!isAnEnemy(groupInfo[gr].enemy))
		groupInfo[gr].enemy=findEnemy();
	var target=findTarget(groupInfo[gr].enemy);
	if (typeof(target)!="undefined") {
		groupInfo[gr].targetX = target.x;
		groupInfo[gr].targetY = target.y;
		groupInfo[gr].targetType = TARGET_STRUCTURE;
		groupInfo[gr].targetAge = 20; // around 3 minutes before target update
		if (target.type == DROID)
			groupInfo[gr].targetType = TARGET_DROID;
		return target;
	}
	
}

// a very naive way of finding where our units mostly are
function naiveFindClusters(list,size) {
	this.clusters=[];
	this.xav=[];
	this.yav=[];
	this.maxIdx=0;
	this.maxCount=0;
	for (i=list.length-1; i>=0; --i) {
		var x=list[i].x,y=list[i].y;
		var found=false;
		for (var j=0; j<this.clusters.length; ++j) {
			if (distBetweenTwoPoints(this.xav[j],this.yav[j],x,y)<size) {
				var n=this.clusters[j].length;
				this.clusters[j][n]=i;
				this.xav[j]=(n*this.xav[j]+x)/(n+1);
				this.yav[j]=(n*this.yav[j]+y)/(n+1);
				if (this.clusters[j].length>this.maxCount) {
					this.maxIdx=j;
					this.maxCount=this.clusters[j].length;
				}
				found=true;
				break;
			}
		}
		if (!found) {
			var n=this.clusters.length;
			this.clusters[n]=[i];
			this.xav[n]=x;
			this.yav[n]=y;
			if (1>this.maxCount) {
				this.maxIdx=n;
				this.maxCount=1
			}
		}
	}
}

// order a droid to stop moving
function stopDroid(droid) {
	if (typeof(orderDroid)=="undefined")
		orderDroidLoc(droid, DORDER_MOVE, droid.x, droid.y);
	else
		orderDroid(droid, DORDER_STOP);
}

// for formation movement of groups
// returns false iff already in formation
function regroupWarriors(gr) {
	var list=enumGroup(battleGroup[gr]);
	if (DEF_LIGHT==1) {
		if (groupSize(battleGroup[gr])<MIN_WARRIORS) {
			for (var i=0; i<list.length; ++i)
				if (!sendForRepair(list[i])) {
					//orderDroidLoc(list[i], DORDER_MOVE, list[ret.maxIdx].x,list[ret.maxIdx].y);
					if (groupInfo[gr].delaystaticregroup > 0)
						returnToBase(list[i]);
					else
						stopDroid(list[i]);
				}
		}
		return groupSize(battleGroup[gr])<MIN_WARRIORS;
	}
	if (typeof(regroupWarriors.lastUnstuck)==undefined || isNaN(regroupWarriors.lastUnstuck))
		regroupWarriors.lastUnstuck=maxPlayers;
	if (typeof(regroupWarriors.lastReturn)==undefined)
		regroupWarriors.lastReturn=false;
	// sometimes groups have a right to be dispersed
	if (groupInfo[gr].delayregroup>0)
		return regroupWarriors.lastReturn;
	// don't bother doing this sort of thing in case of emergency
	if (status!=STATUS_NORMAL)
		return false;
	// turtle AI doesn't really need to regroup his sensors
	if (!personality.THIS_AI_MAKES_TANKS && !personality.THIS_AI_MAKES_CYBORGS)
		return false;
	// early rushy tanks need to be fast
	if (earlyGame(personality.PEACE_TIME))
		return false;
	var min = groupSize(battleGroup[gr])*0.6;
	if (min < MIN_WARRIORS)
		min = MIN_WARRIORS;
	var ret=new naiveFindClusters(list,MAX_DISPERSE + list.length/8);
	if (ret.maxCount >= min) {
		if (distBetweenTwoPoints(ret.xav[ret.maxIdx],ret.yav[ret.maxIdx],groupInfo[gr].lastX,groupInfo[gr].lastY)<5) {
			++groupInfo[gr].idleTime;
		} else {
			groupInfo[gr].idleTime=0;
		}
	} else {
		groupInfo[gr].idleTime=0;
	}
	groupInfo[gr].lastX=ret.xav[ret.maxIdx];
	groupInfo[gr].lastY=ret.yav[ret.maxIdx];
	if (groupInfo[gr].idleTime<5) {
		if (
			(groupInfo[gr].delaystaticregroup == 0) 
				|| 
			(ret.maxCount > min)
		) {
			for (var i=0; i<ret.clusters.length; ++i) 
				if (i!=ret.maxIdx)  {
					for (j=0; j<ret.clusters[i].length; ++j) 
						if (!sendForRepair(list[ret.clusters[i][j]]))
							orderDroidLoc(list[ret.clusters[i][j]], DORDER_MOVE, ret.xav[ret.maxIdx], ret.yav[ret.maxIdx]);
				} else if (ret.maxCount < min || ret.maxCount < list.length/2) { // note: i==ret.maxIdx here
					for (j=0; j<ret.clusters[i].length; ++j) 
						if (!sendForRepair(list[ret.clusters[i][j]]))
							stopDroid(list[ret.clusters[i][j]]);
				}
		} else {
			for (var i=0; i<list.length; ++i)
				if (!sendForRepair(list[i])) {
					//orderDroidLoc(list[i], DORDER_MOVE, list[ret.maxIdx].x,list[ret.maxIdx].y);
					if (groupInfo[gr].delaystaticregroup > 0)
						returnToBase(list[i]);
					else
						stopDroid(list[i]);
				}
		}
		groupInfo[gr].delayregroup = 1; // don't regroup too often
		regroupWarriors.lastReturn = (ret.maxCount < min);
		return regroupWarriors.lastReturn;
	} else if (countEnemies()>0) { // anti-stuck mechanism below
		var rx,ry;
		++regroupWarriors.lastUnstuck;
		if (regroupWarriors.lastUnstuck>=maxPlayers) {
			regroupWarriors.lastUnstuck=0;
			rx=random(mapWidth-2)+1;
			ry=random(mapHeight-2)+1;
		} else {
			rx=startPositions[regroupWarriors.lastUnstuck].x;
			ry=startPositions[regroupWarriors.lastUnstuck].y;
		}
		list=enumDroid(me,DROID_WEAPON);
		list=list.concat(enumDroid(me,DROID_CYBORG));
		for (var i=0; i<list.length; ++i) {
			orderDroidLoc(list[i], DORDER_SCOUT, rx, ry);
		}
		for (var i=0; i<NUM_GROUPS; ++i) {
			groupInfo[i].delayregroup = 4; // for around half a minute, shouldn't be enough to reach too far
		}
		setStatus(STATUS_UNSTUCK);
		regroupWarriors.lastReturn = true;
		return regroupWarriors.lastReturn;
	}
}

function findNearestTarget(x,y) {
	var minDist=Infinity, minObj;
	for (var i=0; i<maxPlayers; ++i) if (isAnEnemy(i)) {
		var list=enumStruct(i);
		list=list.concat(enumDroid(i,DROID_WEAPON));
		for (var j=0; j<list.length; ++j) {
			var dist=distBetweenTwoPoints(list[j].x,list[j].y,x,y);
			if (dist<minDist) {
				minDist = dist;
				minObj = list[j];
			}
		}
	}
	return minObj;
}

// find the next non-empty group
function nextGroup(gr) {
	var next=gr;
	while (true) {
		++next;
		if (next>=NUM_GROUPS)
			next=0;
		if (next==gr)
			return gr;
		if (groupSize(battleGroup[next])>0)
			return next;
	}
}

// the attack
function attackStuff() {
	if (typeof(DORDER_OBSERVE)=="undefined")
		var DORDER_OBSERVE = 9; // HACK: waiting until this constant is exported to scripts ...
	if (typeof(attackStuff.groupNumber)=="undefined")
		attackStuff.groupNumber = 0;
	attackStuff.groupNumber = nextGroup(attackStuff.groupNumber);
	// don't bother doing this sort of thing in case of emergency
	if (status==STATUS_PANIC)
		return;
	var sensorAttack = (!personality.THIS_AI_MAKES_TANKS && !personality.THIS_AI_MAKES_CYBORGS);
	var gr=attackStuff.groupNumber;
	var attackers=enumGroup(battleGroup[gr]);
	if (sensorAttack || groupSize(battleGroup[gr]) >= MIN_WARRIORS || (groupSize(battleGroup[gr]) >= NO_WARRIORS_AT_ALL && earlyGame(personality.PEACE_TIME))) {
		if (regroupWarriors(gr))
			return;
		var target=getGroupTarget(gr);
		if (typeof(target) != "undefined") {
			for (var i=0; i<attackers.length; ++i) {
				droid=attackers[i];
				if (sendForRepair(droid))
					continue;
				if (!droidCanReach(droid,target.x,target.y)) { // useless droid ...
					groupAddDroid(defendGroup,droid);
					continue;
				}
				if (droid.order != DORDER_ATTACK && droid.order != DORDER_SCOUT) {
					if (droid.name.indexOf("Sensor")> -1) {
						var sensorTarget=findNearestTarget(droid.x,droid.y);
						if (typeof(sensorTarget)!="undefined")
							orderDroidObj(droid, DORDER_OBSERVE, sensorTarget);
						else 
							orderDroidObj(droid, DORDER_OBSERVE, target);
					} else orderDroidLoc(droid, DORDER_SCOUT, target.x+random(2)-1, target.y+random(2)-1);
				}
			}
		}
	}
}

// attack things with VTOLs
function vtolAttack() {
	if (!personality.THIS_AI_MAKES_VTOLS)
		return;
	for (var gr=0; gr<NUM_VTOL_GROUPS; ++gr) if (groupSize(vtolGroup[gr])>=MIN_VTOLS) {
		var target=findTarget(-1);
		if (typeof(target) != "undefined") {
			var vtols=enumGroup(vtolGroup[gr]);
			var list=[];
			for (var i=0; i<vtols.length; ++i) {
				var vtol=vtols[i];
				if (isNaN(vtol.armed)) {
					if (vtolAttack.debugSent!=true) {
						debug("vtol.armed is NaN, assuming fully rearmed - known v3.1 bug, see http://developer.wz2100.net/ticket/3178");
						vtolAttack.debugSent=true;
					}
				} 
				if (vtol.armed >= 99 || isNaN(vtol.armed)) {
					if (vtol.order != DORDER_ATTACK)
						list[list.length]=i;
				}
			}
			if (list.length>=MIN_VTOLS) {
				for (var i=0; i<list.length; ++i) 
					orderDroidObj(vtols[list[i]], DORDER_ATTACK, target);
			} 
		} 
	}
}

// make sure the defenders of the base are somewhere near the base
function returnDefendersToBase() {
	var list=enumGroup(defendGroup);
	for (var i=0; i<list.length; ++i)
		returnToBase(list[i]);
}

function callUpdateEnemyInfo() {
	var noReturn=(typeof(enemyInfo)=="undefined");  // this usually happens on loading a game, need a full update in that case
	for (var i=-1; i<maxPlayers; ++i) {
		if (typeof(enemyInfo[i])=="undefined")
			noReturn=true;
	}
	if (noReturn) {
		global.enemyInfo = [];
		enemyInfo[-1] = new Object();
	}
	for (var i=0; i<maxPlayers; ++i) if (isAnEnemy(i)) {
		if (typeof(enemyInfo[i])=="undefined" || gameTime - enemyInfo[i].lastUpdate > 30000) {
			enemyCountBalance(i);
			if (!noReturn) return;
		}
	}
	if (typeof(enemyInfo[-1])=="undefined" || gameTime - enemyInfo[-1].lastUpdate > 30000) {
		enemyInfo[-1].tank = 0;
		enemyInfo[-1].borg = 0;
		enemyInfo[-1].vtol = 0;
		enemyInfo[-1].defs = 0;
		enemyInfo[-1].aa = 0;
		for (var i=0; i<maxPlayers; ++i) if (isAnEnemy(i)) {
			enemyInfo[-1].tank += enemyInfo[i].tank;
			enemyInfo[-1].borg += enemyInfo[i].borg;
			enemyInfo[-1].vtol += enemyInfo[i].vtol;
			enemyInfo[-1].defs += enemyInfo[i].defs;
			enemyInfo[-1].aa   += enemyInfo[i].aa;
		}
		enemyInfo[-1].lastUpdate = gameTime;
	}
}

function rnd() {
	return random(201)-100;
}

function setTimers() {
	setTimer("updateStatus",		10000+rnd());
	setTimer("huntForOil", 		7000+rnd());
	setTimer("doResearch", 		12000+rnd());
	setTimer("executeBuildOrder",	5000+rnd());
	setTimer("attackStuff",		3000+rnd());
	setTimer("vtolAttack",		5000+rnd());
	setTimer("cheat",			30000+rnd());
	setTimer("becomeHarder",		480000+rnd());
	setTimer("adapt",			30000+rnd());
	setTimer("sendAllForRepair",		5000+rnd());
	setTimer("produceDroids",		3000+rnd());
	setTimer("relaxStats",		10000+rnd());
	setTimer("balanceTrucks",		10000+rnd());
	setTimer("returnDefendersToBase",		20000+rnd());
	setTimer("callUpdateEnemyInfo",		2000+rnd());
}

function fallBack(droid,threat) {
	if (droid.order==DORDER_MOVE)
		return;
	var x=droid.x-3*(threat.x-droid.x)/distBetweenTwoPoints(threat.x,threat.y,droid.x,droid.y);
	var y=droid.y-3*(threat.y-droid.y)/distBetweenTwoPoints(threat.x,threat.y,droid.x,droid.y);
	if (x<1 || y<1 || x>mapWidth-2 || y>mapHeight-2)
		return;
	orderDroidLoc(droid,DORDER_MOVE,x,y);
}

	/**********/
	/* Events */
	/**********/
	
// work harder
function eventDroidIdle(droid) {
	if (droid.droidType == DROID_CONSTRUCT)
		queue("huntForOil");
}

// something was just researched
function eventResearched(tech, labparam) {
	queue("doResearch");
	queue("executeBuildOrder");
	queue("produceDroids");
}

// something was just built
function eventStructureBuilt(struct, droid) {
	if (struct.stattype == RESOURCE_EXTRACTOR) {
		++derrickStats[findDerrickStat(struct)].hotness;
	}
	queue("executeBuildOrder");
	queue("huntForOil");
	queue("produceDroids");
	queue("doResearch");
}

// something was just produced
function eventDroidBuilt(droid, struct) {
	if (droid.player != me)
		return;
	if (isVTOL(droid))
		addVtolToSomeGroup(droid);
	else if (droid.droidType == DROID_CONSTRUCT) {
			addTruckToSomeGroup(droid);
			queue("executeBuildOrder");
			queue("huntForOil");
	} else {
		if (tankReachibilityCheck(droid))
			RATE_TANK=0;
		addTankToSomeGroup(droid);
	}
	queue("produceDroids");
}

// something was just attacked
function eventAttacked(victim, attacker) {
	if (typeof(victim)=="undefined" || typeof(attacker)=="undefined") 
		return;
	if (typeof(eventAttacked.lastCall)!="undefined" && gameTime - eventAttacked.lastCall < 200) {
		if (victim.type == DROID && !isVTOL(victim))
			fallBack(victim,attacker);
		return;
	}
	if (allianceExistsBetween(attacker.player,me)) // don't respond to friendly splash damage
		return;
	eventAttacked.lastCall = gameTime;
	personality.PEACE_TIME = 0;
	if (status==STATUS_EARLYGAME)
		setStatus(STATUS_NORMAL);
	var dist=distBetweenTwoPoints(attacker.x,attacker.y,victim.x,victim.y);
	var arty=false;
	if (typeof(attacker.hasIndirect) == "undefined") {
		if (dist>20)
			arty=true;
	} else {
		if (attacker.hasIndirect)
			arty=true;
	}
	if (attacker.type == DROID) 
		if (isVTOL(attacker)) {
			// Build anti-air defenses at spots of enemy VTOL attacks
			lastAirAttackX = victim.x;
			lastAirAttackY = victim.y;
			return;
		}
	if (victim.type == DROID) {
		if (isVTOL(victim)) {
			// VTOLs fire back when attacked by AA
			if (!needsRepair(victim))
				if (victim.health > 50)
					orderDroidObj(victim, DORDER_ATTACK, attacker);
			return;
		}
		if (victim.droidType == DROID_WEAPON || victim.droidType == DROID_CYBORG || victim.droidType == DROID_SENSOR) {
			sendForRepair(victim);
			var gr=groupOfTank(victim);
			if (typeof(gr)=="undefined") { // defendGroup
				var list=enumGroup(defendGroup);
				for (var i=0; i<list.length; ++i)
					orderDroidObj(list[i], DORDER_ATTACK, attacker);
				return;
			}
			groupInfo[gr].idleTime=0;
			if (!arty)
				groupInfo[gr].delaystaticregroup=2;
			if (regroupWarriors(gr))
				return;
			// group comes for help when one of its droids is under attack
			var attackers=enumGroup(battleGroup[gr]);
			for (var i=0; i<attackers.length; ++i) 
				if (attackers[i].id!=victim.id)
					if (!sendForRepair(attackers[i]))
						if (attackers[i].order!=DORDER_MOVE && attackers[i].order!=DORDER_ATTACK)
							orderDroidObj(attackers[i], DORDER_ATTACK, attacker);
			return;
		} else {
			// non-combat units retreat when attacked
			if (victim.order != DORDER_BUILD)
				returnToBase(victim);
		}
	} else if (victim.type == STRUCTURE) {
		if (arty) {
			lastArtyAttackX = victim.x;
			lastArtyAttackY = victim.y;
		}
		if (isBaseStructure(victim) && !arty)
			setStatus(STATUS_PANIC);
		// when a structure is attacked, all groups responsible for attacker's player react
		// unless the base itself is under attack, when all groups respond
		for (var gr=0; gr<NUM_GROUPS; ++gr) 
			if (groupInfo[gr].enemy == attacker.player || status==STATUS_PANIC) {
				if (regroupWarriors(gr) && status==STATUS_NORMAL) 
					continue;
				var attackers=enumGroup(battleGroup[gr]);
				for (var i=0; i<attackers.length; ++i)
					if (!sendForRepair(attackers[i]))
						if (attackers[i].order!=DORDER_ATTACK)
							orderDroidObj(attackers[i], DORDER_ATTACK, attacker);
			}
		// defendGroup responds to all attacks as well
		var list=enumGroup(defendGroup);
		for (var i=0; i<list.length; ++i)
			orderDroidObj(list[i], DORDER_ATTACK, attacker);
		// use VTOLs for defense as well
		for (var gr=0; gr<NUM_VTOL_GROUPS; ++gr) {
			var attackers=enumGroup(vtolGroup[gr]);
			for (var i=0; i<attackers.length; ++i)
				if (!sendForRepair(attackers[i]) && !isNaN(attackers[i].armed) && attackers[i].armed>=99)
					orderDroidObj(attackers[i], DORDER_ATTACK, attacker);
		}
	}
}



// laser satellite strike
function eventStructureReady(structure) {
	list=[];
	for (var i=0; i<maxPlayers; ++i) if (isAnEnemy(i)) {
		list = list.concat(enumStruct(i),enumDroid(i));
	}
	ret=new naiveFindClusters(list,4); // 4 is the laser satellite splash radius
	// find an object closest to the middle of the biggest enemy cluster
	var minIdx=0,minDist=Infinity;
	for (var i=0; i<ret.maxCount; ++i) {
		var x1=ret.clusters[ret.maxIdx][i].x;
		var y1=ret.clusters[ret.maxIdx][i].y;
		var x2=ret.xav[ret.maxIdx];
		var y2=ret.yav[ret.maxIdx];
		var d=distBetweenTwoPoints(x1,y1,x2,y2);
		if (d<minDist) {
			minIdx=i;
			minDist=d;
		}
	}
	activateStructure(structure,list[ret.clusters[ret.maxIdx][minIdx]]);
}



// respond correctly on unit transfers
function eventObjectTransfer(object, from) {
	if (object.type == DROID) {
		eventDroidBuilt(object,null);
	} else {
		eventStructureBuilt(object,null);
	}
}

function eventGameInit() {
	setTimers();
}

// all initialization happens here, apart from personality initialization
function eventStartLevel() {
	if (typeof(global.scavengerPlayer)=="undefined") // HACK: until scavengerPlayer is exposed to scripts.
		global.scavengerPlayer=Math.max(7,maxPlayers)
	personality=new constructPersonality();
	basePosition=startPositions[me];
	if (isStructureAvailable(fmod,me)) {
		HIGH_TECH_START = 1;
		EXTREME_LOW_POWER = 350;
	}
	// HACK: find a better way to find out if we are in challenge
	if (playerPower(me)<1000 || playerPower(me) > 3300) 
		UNUSUAL_SITUATION = 1;
	if (baseType != CAMP_CLEAN)
		personality.PEACE_TIME=0;
	var list = enumDroid(me);
	enemyInfo[-1]=new constructEnemyInfo();
	for (var i=0; i<maxPlayers; ++i) {
		enemyInfo[i]=new constructEnemyInfo();
		for (var j=0; j<list.length; ++j)
			if (!droidCanReach(list[j],startPositions[i].x,startPositions[i].y))
				enemyInfo[i].reachable=false;
	}
	builderGroup=newGroup();
	oilerGroup=newGroup();
	defendGroup=newGroup();
	for (i=0; i<NUM_GROUPS; ++i)
		battleGroup[i]=newGroup();
	groupInfo[0]=new constructGroupInfo(findEnemy());
	for (i=1; i<NUM_GROUPS; ++i)
		groupInfo[i]=new constructGroupInfo(nextEnemy(groupInfo[i-1].enemy));
	NUM_VTOL_GROUPS = MIN_VTOL_GROUPS;
	for (i=0; i<MAX_VTOL_GROUPS; ++i)
		vtolGroup[i]=newGroup();
	for (var i=0; i<list.length; ++i) {
		var droid=list[i];
		if (droid.droidType == DROID_CONSTRUCT || droid.name.indexOf("Engineer")>-1) {
			addTruckToSomeGroup(droid);
			// the next line helps avoiding some strange problem when droids that are initially
			// buried into the ground don't move out of the way when a building is being
			// placed right above them
			orderDroidLoc(droid,DORDER_MOVE,droid.x+random(5)-2,droid.y+random(5)-2); 
		}
		else if (isVTOL(droid))
			addVtolToSomeGroup(droid);
		else if (droid.droidType == DROID_WEAPON || (droid.droidType == DROID_CYBORG && droid.name.indexOf("Mechanic")==-1))
			addTankToSomeGroup(droid);
	}
	setStatus(STATUS_EARLYGAME);
	becomeHarder();
	adapt();
	queue("executeBuildOrder",200);
}

// currently unused events
function eventObjectSeen(viewer, seen) {}
function eventDestroyed(object) {}
function eventChat(from, to, message) {}
