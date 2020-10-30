const vernum    = "1.02";
const verdate   = "03.07.2020";
const vername   = "WZ-BoneCrusher!";
const shortname = "wzbc";
const release	= true;


///////\\\\\\\

/* ---=== For buildIn AI ===---

Clean code
+ Remove nastyFeatures algorythms
- Remove most useless comments
+ Remove "if version"

*/

///////\\\\\\\
//v1.00 (Встроенная версия)
//		Remove old game version dependency (if version)
//		Remove old nfAlgorythm for game v3.1
//		Slightly improve performance, set order limits


//DEBUG: количество вывода, закоментить перед релизом
var debugLevels = new Array('error');

//var debugLevels = new Array('init', 'end', 'stats', 'temp', 'production', 'group', 'events', 'error', 'research', 'builders', 'targeting');



var debugName;


//Массив конкретных технологий (tech.js)
var tech = [];

include("multiplay/skirmish/bonecrusher/functions.js");
include("multiplay/skirmish/bonecrusher/builders.js");
include("multiplay/skirmish/bonecrusher/targeting.js");
include("multiplay/skirmish/bonecrusher/events.js");
include("multiplay/skirmish/bonecrusher/names.js");
include("multiplay/skirmish/bonecrusher/produce.js");
include("multiplay/skirmish/bonecrusher/performance.js");
include("multiplay/skirmish/bonecrusher/chatting.js");
include("multiplay/skirmish/bonecrusher/tech.js");
include("multiplay/skirmish/bonecrusher/weapons.js");
include("multiplay/skirmish/bonecrusher/build-normal.js");

/*
 * 
 * НАСТРОЙКИ
 * 
 */


//Hard CPU-load algorythms
var weakCPU = true;


//Стратегия исследования пути
//"Strict" - строгая, primary_way исcледуется один за другим
//"Smudged" - не строгая, по мере возможности забегает вперёд, но доисследует весь primary_way
//"Random" - случайная, выборочно по заданному пути, но доиследует весь primary_way
//"RoundRobin" - пока не реализована..
var researchStrategy = 'Strict'; //По умолчанию, можно изменить в файле настроек вынесенный за предел мода

var base_range = 20; // В каких пределах работают основные строители (не охотники)

var buildersTimer = 25000;		//Триггер для заказа строителей (что бы не выходили пачкой сразу)
var fixersTimer = 50000;		//Триггер для заказа рем.инженеров
var scannersTimer = 300000;		//Триггер для заказа сенсоров
var checkRegularArmyTimer = 10000;
var reactRegularArmyTimer = 10000;
var reactWarriorsTimer = 5000;
var fullBaseTimer = 60000;

var minBuilders = 5;

var builderPts = 750;

var maxConstructors = 15;

var minPartisans = 7;
var maxPartisans = 15;
var minRegular = 10;
var maxRegular = 50;
var maxVTOL = 40;
var minCyborgs = 20;
var maxCyborgs = 30;
var maxFixers = 5;
var maxJammers = 2;
var maxScouts = 2;

var maxExtractors = 40;
var maxGenerators = 10;

//Performance limits
var ordersLimit = 100;

//functions controller for performance purpose
var func_buildersOrder = true;
var func_buildersOrder_timer = 5000+me*100;
var func_buildersOrder_trigger = 0;

/*
 * 
 * -=-=-=-
 * 
 */

// --- CONSTANTS --- \\

//Отсутствующие переменные
const DORDER_NONE = 0;


// --- TRIGGERS --- \\

var fullBase = false;
var earlyGame = true;
var running = false;	//Работаем?

var produceTrigger=[];

var armyToPlayer = false;	//Передавать всю новую армию игроку №№
var vtolToPlayer = false;


// --- VARIABLES --- \\


//Координаты всех ресурсов, свободных и занятых
var allResources;

//Координаты нашей базы
var base		= {x:0,y:0};
var startPos	= {x:0,y:0};

//Массив для союзников
var ally=[];

var enemy=[];

var rWay;

//Массив всех приказов юнитам
var _globalOrders = [];


var bc_ally=[]; //Союзные ИИ BoneCrusher-ы

var avail_research = [];	//Массив возможных исследований, заполняется в функции doResearch();

//var scavengerPlayer = -1;

var buildersMain = newGroup();
var buildersHunters = newGroup();

var policy = [];

//Фитчи, не совместимые с 3.1.5
var nf = [];
nf['policy'] = false;

var enemyDist = 0;

var armyPartisans = newGroup();
var armyRegular = newGroup();
var targRegular={x:0,y:0};
var lastImpact=false;
var pointRegular=false;
var lastEnemiesSeen = 0;
var armySupport = newGroup();
var armyCyborgs = newGroup();
var armyFixers = newGroup();
var armyJammers = newGroup();
var armyScanners = newGroup();
var armyScouts = newGroup();
var partJammers = newGroup();
var VTOLAttacker = newGroup();

var maxFactories, maxFactoriesCyb, maxFactoriesVTOL, maxLabs, maxPads;

//Triggers
var buildersTrigger = 0;
var fixersTrigger = 0;
var scannersTrigger = 0;
var checkRegularArmyTrigger = 0;
var reactRegularArmyTrigger = 0;
var reactWarriorsTrigger = 0;
var fullBaseTrigger = 0;

var berserk = false;
var seer = false;
var credit = 0;

var lassat_charged = false;


var eventsRun=[];
eventsRun['targetCyborgs'] = 0;
eventsRun['targetArmy'] = 0;
eventsRun['targetRegular'] = 0;
eventsRun['targetJammers'] = 0;
eventsRun['targetFixers'] = 0;
eventsRun['buildersOrder'] = 0;
eventsRun['victimCyborgs'] = 0;
eventsRun['targetSensors'] = 0;


//Предустановки на исследование
var research_way = []; //Главный путь развития, компануется далее, в функциях, в зависимости от уровня сложности и др. настроек
var research_primary = []; //Первичный, один из главных под-путей развития, к которому задаётся режим его исследований(строгий, размазанный или случайный)
//Предустановки, на случай researchCustom
var researchCustom = false; //Задаётся в файле настроек, вынесенный за пределы мода
const research_synapse = ["R-Struc-Research-Upgrade09"];
const research_power = ["R-Struc-Power-Upgrade03a"];
const research_armor = ["R-Vehicle-Metals09"];
const research_sensor = ["R-Sys-Sensor-UpLink"];

//Переназначаются в функции prepeareProduce() что бы не читерить.
var light_bodies=["Body3MBT","Body2SUP","Body4ABT","Body1REC"];
var medium_bodies=["Body7ABT","Body6SUPP","Body8MBT","Body5REC"];
var heavy_bodies=["Body13SUP","Body10MBT","Body9REC","Body12SUP","Body11ABT"];
/*
var light_bodies=[
"Body8MBT",  // Scorpion (жёлтое 2)
"Body1REC"   // Viper (серое 1)
//"Body4ABT"  // Bug (жёлтое 1)
];
var medium_bodies=[
"Body12SUP", // Mantis (жёлтое 3)
"Body7ABT",  // Retribution (чёрное 2)
"Body6SUPP", // Panther (зелёное 2)
"Body3MBT",  // Retaliation (чёрное 1)
"Body2SUP",  // Leopard (зелёное 1)
"Body5REC"   // Cobra (серое 2)
];
var heavy_bodies=[
"Body13SUP", // Wyvern (красное 1)
"Body10MBT", // Vengeance (чёрное 3)
"Body9REC",  // Tiger (зелёное 3)
"Body11ABT"  // Python (серое 3)
];
*/
var avail_cyborgs=[];


// Research, Body, Weapon
var cyborgs=[
//	["R-Wpn-MG1Mk1",			"CyborgLightBody",		"CyborgChaingun"],			// легкий пулемёт
["R-Wpn-Flamer01Mk1",			"CyborgLightBody",		"CyborgFlamer01"],			// лёгкий огнемёт
["R-Wpn-MG4",					"CyborgLightBody",		"CyborgRotMG"],				// тяжёлый пулемёт
["R-Wpn-Flame2",				"CyborgLightBody",		"Cyb-Wpn-Thermite"],		// горячий напалм
["R-Wpn-Cannon1Mk1",			"CyborgLightBody",		"CyborgCannon"],			// лёгкая пушка
["R-Wpn-Mortar01Lt",			"CyborgLightBody",		"Cyb-Wpn-Grenade"],			// гранатамёт
["R-Wpn-Rocket01-LtAT",			"CyborgLightBody",		"CyborgRocket"],			// Lancer
["R-Wpn-Missile2A-T",			"CyborgLightBody",		"Cyb-Wpn-Atmiss"],			// scourge
["R-Wpn-Laser01",				"CyborgLightBody",		"Cyb-Wpn-Laser"],			// Flashlight Gunner
["R-Wpn-RailGun01",				"CyborgLightBody",		"Cyb-Wpn-Rail1"],			// Needle Gunner
["R-Cyborg-Hvywpn-A-T",			"CyborgHeavyBody",		"Cyb-Hvywpn-A-T"],			//Super scourge
["R-Cyborg-Hvywpn-Mcannon",		"CyborgHeavyBody",		"Cyb-Hvywpn-Mcannon"],		//Super Heavy Gunner
["R-Cyborg-Hvywpn-HPV",			"CyborgHeavyBody",		"Cyb-Hvywpn-HPV"],			//Super Hyper velocity
["R-Cyborg-Hvywpn-Acannon",		"CyborgHeavyBody",		"Cyb-Hvywpn-Acannon"],		//Super autocannon
["R-Cyborg-Hvywpn-PulseLsr",	"CyborgHeavyBody",		"Cyb-Hvywpn-PulseLsr"],		//Super pulse laser
["R-Cyborg-Hvywpn-TK",			"CyborgHeavyBody",		"Cyb-Hvywpn-TK"],			//Super tank killer
["R-Cyborg-Hvywpn-RailGunner",	"CyborgHeavyBody",		"Cyb-Hvywpn-RailGunner"],	//Super Rail-Gunner

];
/*var cyborgs=[
["R-Wpn-MG1Mk1", "CyborgChain1Ground", "CyborgChaingun"],			// легкий пулемёт
["R-Wpn-Flamer01Mk1", "CyborgFlamerGrd", "CyborgFlamer01"],			// лёгкий огнемёт
["R-Wpn-MG4", "CybRotMgGrd","CyborgRotMG"],							// тяжёлый пулемёт
["R-Wpn-Flame2", "Cyb-Bod-Thermite", "Cyb-Wpn-Thermite"],			// горячий напалм
["R-Wpn-Cannon1Mk1", "CyborgCannonGrd", "CyborgCannon"],			// лёгкая пушка
["R-Wpn-Mortar01Lt", "Cyb-Bod-Grenade", "Cyb-Wpn-Grenade"],			// гранатамёт
["R-Wpn-Rocket01-LtAT", "CyborgRkt1Ground", "CyborgRocket"],		//Lancer
["R-Wpn-Missile2A-T", "Cyb-Bod-Atmiss", "Cyb-Wpn-Atmiss"],			//scourge
["R-Cyborg-Hvywpn-A-T", "Cyb-Hvybod-A-T", "Cyb-Hvywpn-A-T"],			//super scourge
["R-Cyborg-Hvywpn-HPV", "Cyb-Hvybod-HPV", "Cyb-Hvywpn-HPV"],			//Super Hyper velocity
["R-Cyborg-Hvywpn-Acannon", "Cyb-Hvybod-Acannon", "Cyb-Hvywpn-Acannon"],	//Super autocannon
["R-Cyborg-Hvywpn-PulseLsr", "Cyb-Hvybod-PulseLsr","Cyb-Hvywpn-PulseLsr"],	//super pulse laser
];*/


var bodies=[
//	===== Средняя броня (металическая)
["R-Vehicle-Body01", "Body1REC"],	//Стартовая броня лёгкой защиты "Вайпер" (уже есть)
["R-Vehicle-Body05", "Body5REC"],	//Средняя защита "Кобра"
["R-Vehicle-Body11", "Body11ABT"],	//Улучгенная защита "Питон"
//	===== Лёгкая броня (жёлтая)
["R-Vehicle-Body04", "Body4ABT"],	//bug
["R-Vehicle-Body08", "Body8MBT"],	//scorpion
["R-Vehicle-Body12", "Body12SUP"],	//mantis
//	===== Улучшенная броня (зелёная)
["R-Vehicle-Body02", "Body2SUP"],	//Легковестная защита "Leopard"
["R-Vehicle-Body06", "Body6SUPP"],	//Средняя защита "Panther"
["R-Vehicle-Body09", "Body9REC"],	//Улучшенная защита "Tiger"
//	===== Тяжёлая броня (чёрная)
["R-Vehicle-Body03", "Body3MBT"],	//retaliation
["R-Vehicle-Body07", "Body7ABT"],	//retribution
["R-Vehicle-Body10", "Body10MBT"],	//vengeance
//	===== Особая броня (красная)
["R-Vehicle-Body13", "Body13SUP"],	//Wyvern
["R-Vehicle-Body14", "Body14SUP"],	//Dragon (двухпушечная)
];
var propulsions=[
[true,"wheeled01"],								//Стартовые колёса (уже есть)
["R-Vehicle-Prop-Halftracks", "HalfTrack"],		//Полугусенецы
["R-Vehicle-Prop-Tracks", "tracked01"],			//Гусенецы
["R-Vehicle-Prop-Hover", "hover01"],			//Ховер
["R-Vehicle-Prop-VTOL", "V-Tol"]				//СВВП
];

//Плохая идея для 3.1, нет возможности узнать какое орудие несёт дроид.
//var partisanGuns=["Cannon4AUTOMk1","MG4ROTARYMk1","MG3Mk1","Cannon1Mk1","MG2Mk1","MG1Mk1"];
//var regularGuns=["PlasmiteFlamer","Rocket-MRL","Rocket-LtA-T","Flame2","MG4ROTARYMk1","Cannon4AUTOMk1","MG3Mk1","Flame1Mk1","Rocket","Cannon1Mk1","MG2Mk1","MG1Mk1"];
//var supportGuns=["Mortar1Mk1","Rocket-LtA-T","MG3Mk1"];

//Переназначаются в функции prepeareProduce() что бы не читерить.
var avail_vtols=["MG3-VTOL"];
var vtols=[
["R-Wpn-MG3Mk1","MG3-VTOL"],					//VTOL Heavy Machinegun
["R-Wpn-MG4","MG4ROTARY-VTOL"],					//VTOL Assault Gun
["R-Wpn-Cannon4AMk1","Cannon4AUTO-VTOL"],		//VTOL Hyper Velocity Cannon
["R-Wpn-Rocket01-LtAT","Rocket-VTOL-LtA-T"],	//VTOL Lancer
//["Bomb3-VTOL-LtINC","Bomb3-VTOL-LtINC"],		//VTOL Phosphor Bomb Bay
//["Bomb4-VTOL-HvyINC","Bomb4-VTOL-HvyINC"],		//VTOL Thermite Bomb Bay
];

var avail_guns=[];


var defence = [];
var towers=[
['R-Defense-Tower01', 'GuardTower1'],									//Пулемётная вышка
['R-Defense-Pillbox01', 'PillBox1'],									//Пулемётный бункер
['R-Defense-WallTower01', 'WallTower01'],								//Укреплённый пулемёт
['R-Defense-WallTower02','WallTower02'],								//Укреплённая лёгкая пушка
['R-Defense-Tower06', 'GuardTower6'],									//Вышка минирокет
['R-Defense-Pillbox06', 'GuardTower5'],									//Лансер
['R-Defense-WallTower-HPVcannon','WallTower-HPVcannon'],				//Гиперскоростная защита
['R-Defense-Emplacement-HPVcannon', 'Emplacement-HPVcannon'],			//Гиперскоростная защита окоп
['R-Defense-MRL', 'Emplacement-MRL-pit'],								//Окоп рокетных батарей
['R-Defense-IDFRocket','Emplacement-Rocket06-IDF'],							//Окоп дальнобойных рокетных батарей
['R-Defense-MortarPit', 'Emplacement-MortarPit01'], 						//Мортира
['R-Defense-RotMor', 'Emplacement-RotMor'],									//Пепперпот
['R-Defense-WallTower-TwinAGun', 'WallTower-TwinAssaultGun'],				//Спаренный пулемёт
['R-Defense-MortarPit-Incenediary' , 'Emplacement-MortarPit-Incenediary'],	//Адская мортира
];

var AA_defence = [];
var AA_queue = [];
var AA_towers=[
['R-Defense-AASite-QuadMg1', 'AASite-QuadMg1'],					//Hurricane AA Site
['R-Defense-AASite-QuadBof', 'AASite-QuadBof'],					//AA Flak Cannon Emplacement
['R-Defense-WallTower-DoubleAAgun', 'WallTower-DoubleAAGun'],	//AA Flak Cannon Hardpoint
['R-Defense-Sunburst', 'P0-AASite-Sunburst'],					//Sunburst AA Site
['R-Defense-SamSite1', 'P0-AASite-SAM1'],						//Avenger SAM Site
['R-Defense-SamSite2', 'P0-AASite-SAM2'],						//Vindicator SAM Site
['R-Defense-WallTower-QuadRotAA', 'WallTower-QuadRotAAGun'],	//Whirlwind Hardpoint
['R-Defense-AA-Laser', 'P0-AASite-Laser'],						//Stormbringer Emplacement
];

//Инициализация
function init(){
	//инфа
	debugName = colors[playerData[me].colour];
	
	debugMsg("ИИ №"+me+" "+vername+" "+vernum+"("+verdate+") difficulty="+difficulty, "init");
	debugMsg("Warzone2100 "+version, "init");
	
	//Определяем мусорщиков
//	scavengerPlayer = (scavengers) ? Math.max(7,maxPlayers) : -1;
	if(scavengers)debugMsg("На карте присудствуют гопники! {"+scavengerPlayer+"}", "init");
	else debugMsg("На карте отсутствуют гопники", "init");
	
//	base = startPositions[me];
	initBase();
	startPos = base;
	
	var technology = enumResearch();
	if(technology.length) debugMsg("Доступных исследований: "+technology.length, "init");
	else debugMsg("ВНИМАНИЕ: Нет доступных исследований", "init");
	
	debugMsg('Is Multiplayer: '+getIsMultiplayer(), 'init');
	debugMsg('Is Human in Ally: '+isHumanAlly(), 'init');
	debugMsg('Num Enemies: '+getNumEnemies(), 'init');
	
	//Получаем координаты всех ресурсов и занятых и свободных
	allResources = enumFeature(ALL_PLAYERS, "OilResource");
	var nearResources = allResources.filter(function(e){if(distBetweenTwoPoints_p(base.x,base.y,e.x,e.y) < base_range) return true; return false;});
	nearResources = nearResources.concat(enumStruct(me, "A0ResourceExtractor").filter(function(e){if(distBetweenTwoPoints_p(base.x,base.y,e.x,e.y) < base_range) return true; return false;}));
	debugMsg("На карте "+allResources.length+" свободных ресурсов", 'init');
	
	for ( var e = 0; e < maxPlayers; ++e ) allResources = allResources.concat(enumStruct(e,RESOURCE_EXTRACTOR));
	if(scavengers == true){
		allResources = allResources.concat(enumStruct(scavengerPlayer, "A0ResourceExtractor"));
	}
	debugMsg("На карте "+allResources.length+" всего ресурсов, рядом "+nearResources.length, 'init');
	
	_builders = enumDroid(me,DROID_CONSTRUCT);
	
	debugMsg("Игроков на карте: "+maxPlayers,2);
	playerData.forEach( function(data, player) {
		var msg = "Игрок №"+player+" "+colors[data.colour];
		var dist = distBetweenTwoPoints_p(base.x,base.y,startPositions[player].x,startPositions[player].y);
		
		if (player == me) {
			msg+=" я сам ИИ";
			bc_ally.push(player);
//			debugMsg("TEST: "+bc_ally.length, 'research');
			//			debugName = colors[data.colour];
		}
		else if(playerLoose(player)){msg+=" отсутствует";}
		else if(playerSpectator(player)){msg+=" наблюдатель";}
		else if(allianceExistsBetween(me,player)){
			msg+=" мой союзник ";
			ally.push(player);
			if(data.name == 'bc-master' || data.name.substr(0,11) == "BoneCrusher"){ msg+="BC!"; bc_ally.push(player);}
			else{msg+=data.name;}
		}
		else{
			msg+=" мой враг";
			enemy.push(player);
			if(_builders.length != 0){
				
				//Знаю, в 3.2 можно тщательнее всё проверить, но у нас совместимость с 3.1.5
				//поэтому логика аналогична
				//Надеемся, что строитель №0 не ховер, проверяем может ли он добраться до врага
				//Если нет, надеемся, что враг недоступен по земле, но доступен по воде
				if(!droidCanReach(_builders[0], startPositions[player].x, startPositions[player].y)){
					if(!nf['policy']){nf['policy'] = 'island';}
					else if(nf['policy'] == 'land'){ nf['policy'] = 'land';}
				}else{
					
					/*
					Перенесено в checkAlly(), в functions.js
					if(Math.floor(dist/2) < base_range){
						debugMsg('base_range снижено: '+base_range+'->'+Math.floor(dist/2), 'init');
						base_range = Math.floor(dist/2);
					}
					*/
					
					if(!nf['policy']){nf['policy'] = 'land';}
					else if(nf['policy'] == 'island'){ nf['policy'] = 'land';}
				}
			}
		}
		
		msg+=" ["+startPositions[player].x+"x"+startPositions[player].y+"]";
		msg+=" дист. "+dist;
		debugMsg(msg,"init");
	});
	


	if(ally.length == 1){
		debugMsg("Имеется союзник" , 'init');
	}
	if(ally.length > 1){
		debugMsg("Имеются союзники" , 'init');
	}
	if(ally.length != 0){
		if(alliancesType == 2) debugMsg("Исследования общие", 'init');
		if(alliancesType == 3) debugMsg("Исследования раздельные", 'init');
	}
	if(nearResources.length >= 24){
		policy['build'] = 'rich';
		initBase();
	}else{
		policy['build'] = 'standart';
	}
	
	debugMsg("Policy build order = "+policy['build'], 'init');
	debugMsg("nf Policy = "+nf['policy'], 'init');

	
	//Research way
	if(Math.round(Math.random()*3) != 0) researchCustom = true;
	if(difficulty == HARD || difficulty == INSANE) researchCustom = true;
	if(researchCustom){
		researchStrategy = 'Smudged';
		debugMsg("initializing custom research_primary", 'init');
		include("multiplay/skirmish/bonecrusher/research-test.js");
		chooseResearch();
		//fixResearchWay(research_rocket_mg);
		//fixResearchWay(research_vtol_laser_flamer);
		//fixResearchWay(research_cannon_mg);
		//fixResearchWay(research_mortar_flamer);
		var _primary = fixResearchWay(research_primary);
		if(_primary == false){
			debugMsg('Cannot check research way', 'error');
			debugMsg("initializing standart research_primary", 'init');
			include("multiplay/skirmish/bonecrusher/research-normal.js");
			researchCustom = false;
			researchStrategy = 'Strict';
		}
		else{
			research_primary = _primary;
			//Переменная приоритетов путей исследований
			research_way = [
				research_synapse,
				research_power,
				research_armor,
				research_sensor
			];
		}
	}else{
		debugMsg("initializing standart research_primary", 'init');
		include("multiplay/skirmish/bonecrusher/research-normal.js");
	}
	if(!addPrimaryWay()){debugMsg("research_primary не добавлен в research_way!", 'error');}
	

	
	if(policy['build'] == 'rich'){

		research_way.unshift(
			["R-Wpn-MG1Mk1"],
			["R-Sys-Engineering01"],
			["R-Struc-Research-Module"],
			["R-Struc-Factory-Cyborg"],
			["R-Vehicle-Prop-Halftracks"],
			["R-Struc-Factory-Module"],
			["R-Struc-PowerModuleMk1"],
			["R-Vehicle-Body11"]
//			["R-Vehicle-Prop-Tracks"]
		);
		
		if(technology.length)cyborgs.unshift(["R-Wpn-MG1Mk1", "CyborgLightBody", "CyborgChaingun"]);
		
		buildersTimer = 7000;
		minBuilders = 10;
		minPartisans = 1;
		maxPartisans = 4;
		builderPts = 150;
		maxRegular = 100;
		scannersTimer = 120000
	}
	
//	if(policy['build'] == 'cyborgs') cyborgs.unshift(["R-Wpn-MG1Mk1", "CyborgChain1Ground", "CyborgChaingun"]);
	
	
	
	//Лимиты:
	maxFactories = getStructureLimit("A0LightFactory", me);
	maxLabs = getStructureLimit("A0ResearchFacility", me);
	maxGenerators = getStructureLimit("A0PowerGenerator", me);
	maxFactoriesCyb = getStructureLimit("A0CyborgFactory", me);
	maxFactoriesVTOL = getStructureLimit("A0VTolFactory1", me);
	maxPads = getStructureLimit("A0VtolPad", me);
	
	
	//Лёгкий режим
	if(difficulty == EASY){
		debugMsg("Похоже я играю с нубами, будем поддаваться:", 'init');
		
		//Забываем все предустановленные исследования
		//Исследуем оружия и защиту в полном рандоме.
		research_way=[["R-Wpn-MG1Mk1"],["R-Struc-Research-Upgrade09"],["R-Struc-Power-Upgrade03a"]];
		
		//Уменьшаем размеры армий
		(maxPartisans > 7)?maxPartisans = 7:{};
		maxRegular = 0;
		(maxVTOL > 5)?maxVTOL = 5:{};
		(maxCyborgs > 5)?maxCyborgs = 5:{};
		(maxFixers > 2)?maxFixers = 2:{};
		
		//Уменьшаем кол-во строителей
		(maxConstructors > 7)?maxConstructors = 7:{};
		(minBuilders > 3)?minBuilders = 3:{};
		
		//Уменьшаем размер базы
		(maxFactories > 2)?maxFactories = 2:{};
		(maxFactoriesCyb > 1)?maxFactoriesCyb = 1:{};
		(maxFactoriesVTOL > 1)?maxFactoriesVTOL = 1:{};
		(maxPads > 2)?maxPads = 2:{};
		
		maxJammers = 0;
		
		//Производим строителя раз в минуту, не раньше
		buildersTimer = 60000;
	}else if(difficulty == MEDIUM){
		buildersTimer = buildersTimer + Math.floor(Math.random()*5000 - 2000);
		minBuilders = minBuilders + Math.floor(Math.random() * 5 - 2 );
		builderPts = builderPts + Math.floor(Math.random() * 200 - 150);
		minPartisans = minPartisans + Math.floor(Math.random() * 6 - 4);
	}
	debugMsg("minPartisans="+minPartisans+", minBuilders="+minBuilders+", builderPts="+builderPts+", buildersTimer="+buildersTimer, "init");
	debugMsg("Лимиты базы: maxFactories="+maxFactories+"; maxFactoriesCyb="+maxFactoriesCyb+"; maxFactoriesVTOL="+maxFactoriesVTOL+"; maxPads="+maxPads+"; maxLabs="+maxLabs+"; maxGenerators="+maxGenerators+"; maxExtractors="+maxExtractors, 'init');
	debugMsg("Лимиты юнитов: maxPartisans="+maxPartisans+"; maxRegular="+maxRegular+"; maxCyborgs="+maxCyborgs+"; maxVTOL="+maxVTOL+"; maxFixers="+maxFixers+"; maxConstructors="+maxConstructors, 'init');
	
	
	if(!release)research_way.forEach(function(e){debugMsg(e, 'research_way');});
	
	
	if(nf['policy'] == 'island'){
		debugMsg("Тактика игры: "+nf['policy'], 'init');
		var _msg='Change policy form: '+policy['build'];
		if(policy['build'] != 'rich') policy['build'] = 'island';
		debugMsg(_msg+' to '+policy['build'], 'init');
		debugMsg('Change maxCyborgs='+maxCyborgs+' to 0', 'init');
		maxCyborgs = 0;
		debugMsg('Change minBuilders='+minBuilders+' to 3', 'init');
		minBuilders = 3;
		buildersTimer = 35000;
		minPartisans = 2;
		
		research_way.unshift(
			["R-Vehicle-Prop-Hover"],
			["R-Struc-PowerModuleMk1"],
			["R-Struc-Research-Module"],
			["R-Struc-Factory-Module"]
		);
		
		debugMsg("Убираем все исследования связанные с киборгами", 'init');
		
		research_way = excludeTech(research_way, tech['cyborgs']);
		
		research_way = excludeTech(research_way, tech['tracks']);
		
		if(!release)research_way.forEach(function(e){debugMsg(e, 'research_way');});
		
	}
	
	if(!release) for ( var p = 0; p < maxPlayers; ++p ) {debugMsg("startPositions["+p+"] "+startPositions[p].x+"x"+startPositions[p].y, 'temp');}
	
	//Просто дебаг информация
	var oilDrums = enumFeature(ALL_PLAYERS, "OilDrum");
	debugMsg("На карте "+oilDrums.length+" бочек с нефтью", 'init');
	
	queue("welcome", 3000+me*(Math.floor(Math.random()*2000)+1500) );
	queue("checkAlly", 2000);
	letsRockThisFxxxingWorld(true); // <-- Жжём плазмитом сцуко!	
}

function welcome(){
	playerData.forEach( function(data, player) {
		chat(player, ' from '+debugName+': '+chatting('welcome'));
	});
}

//Старт
function letsRockThisFxxxingWorld(init){
	debugMsg("Старт/Run", 'init');
	
	include("multiplay/skirmish/bonecrusher/weap-init.js");
	
	//Remove chaingun and flamer cyborgs if better available
	cyborgs = cyborgs.filter(function(e){if( (e[2] == 'CyborgChaingun' && getResearch('R-Wpn-MG4').done) || (e[2] == 'CyborgFlamer01' && getResearch('R-Wpn-Flame2').done) )return false;return true;});
	
	//Первых военных в группу
	enumDroid(me,DROID_CYBORG).forEach(function(e){groupAddDroid(armyCyborgs, e);});
	enumDroid(me,DROID_WEAPON).forEach(function(e){groupAddDroid(armyCyborgs, e);}); // <-- Это не ошибка, первых бесплатных определяем как киборгов (работа у них будет киборгская)

	setTimer("secondTick", 1000);
	queue("buildersOrder", 1000);
	queue("prepeareProduce", 2000);
	queue("produceDroids", 3000);
	queue("doResearch", 3000);
	
	running = true;
	if(init){
		if(difficulty == EASY){
	
			setTimer("produceDroids", 10000+me*100);
			setTimer("produceVTOL", 12000+me*100);
			setTimer("checkEventIdle", 60000+me*100);	//т.к. eventDroidIdle глючит, будем дополнительно отслежвать.
			setTimer("doResearch", 60000+me*100);
			setTimer("defenceQueue", 60000+me*100);
			setTimer("produceCyborgs", 25000+me*100);
//			setTimer("buildersOrder", 120000+me*100);
			setTimer("targetVTOL", 120000+me*100); //Не раньше 30 сек.
		
	
		} else if(difficulty == MEDIUM){

			setTimer("produceDroids", 7000+me*100);
			setTimer("produceVTOL", 8000+me*100);
			setTimer("produceCyborgs", 9000+me*100);
//			if(policy['build'] == 'rich') setTimer("buildersOrder", 5000+me*100);
//			else setTimer("buildersOrder", 120000+me*100);
			setTimer("checkEventIdle", 30000+me*100);	//т.к. eventDroidIdle глючит, будем дополнительно отслежвать.
			setTimer("doResearch", 30000+me*100);
			setTimer("defenceQueue", 60000+me*100);
			setTimer("targetVTOL", 56000+me*100); //Не раньше 30 сек.
			setTimer("targetRegular", 65000+me*100);

			if(policy['build'] == 'rich') func_buildersOrder_timer = 5000+me*100;
			
		} else if(difficulty == HARD){
		
			setTimer("targetPartisan", 5000+me*100);
//			setTimer("buildersOrder", 5000+me*100);
//			setTimer("targetJammers", 5500+me*100);
			setTimer("targetCyborgs", 7000+me*100);
			setTimer("produceDroids", 7000+me*100);
			setTimer("produceVTOL", 8000+me*100);
			setTimer("targetFixers", 8000+me*100);
			setTimer("produceCyborgs", 9000+me*100);
			setTimer("doResearch", 30000+me*100);
			setTimer("defenceQueue", 60000+me*100);
			setTimer("targetRegular", 32000+me*100);
			setTimer("targetVTOL", 56000+me*100); //Не раньше 30 сек.
			
			reactRegularArmyTimer = 5000;
			checkRegularArmyTimer = 5000;
			reactWarriorsTimer = 2000;
			func_buildersOrder_timer = 5000+me*100;
		
		} else if(difficulty == INSANE){
		
//			research_way.unshift(["R-Defense-MortarPit-Incenediary"]);
			
			setTimer("targetPartisan", 5000+me*100);
//			setTimer("buildersOrder", 5000+me*100);
//			setTimer("targetJammers", 5500+me*100);
			setTimer("produceDroids", 6000+me*100);
			setTimer("produceVTOL", 6500+me*100);
			setTimer("produceCyborgs", 7000+me*100);
			setTimer("targetCyborgs", 7000+me*100);
			setTimer("targetFixers", 8000+me*100);
			setTimer("targetRegular", 10000+me*100);
			setTimer("doResearch", 12000+me*100);
			setTimer("defenceQueue", 30000+me*100);
			setTimer("targetVTOL", 56000+me*100); //Не раньше 30 сек.
			
			reactRegularArmyTimer = 5000;
			checkRegularArmyTimer = 5000;
			reactWarriorsTimer = 2000;
			func_buildersOrder_timer = 5000+me*100;
			
			/* NO Cheat Anymore!
			if(!getIsMultiplayer() && ( !isHumanAlly() || !release ) ){
				berserk = true;
				debugMsg('Berserk activated', 'init');
				if(getNumEnemies() > 1){
					debugMsg('Big army activated', 'init');
					minPartisans = 20;
					maxPartisans = 25;
					minRegular = 30;
					maxRegular = 70;
					minCyborgs = 40;
					maxCyborgs = 50;
				}
				if(getNumEnemies() > 2){
					debugMsg('Seer activated', 'init');
					seer = true;
				}
			}
			*/
		}
	
		if(!release){
			setTimer("stats", 10000); // Отключить в релизе
		}
		setTimer("checkProcess", 60000+me*100);
		setTimer("perfMonitor", 5000);

	}
}

function initBase(){
	//Первых строителей в группу
	checkBase();
	var _builders = enumDroid(me,DROID_CONSTRUCT);
	
	//Получаем свои координаты
	var _r = Math.floor(Math.random()*_builders.length);
	if(_builders.length != 0) base = {x:_builders[_r].x, y:_builders[_r].y};
	
	_builders.forEach(function(e){groupBuilders(e);});

	if(policy['build'] == 'rich' && _builders.length > 4){
		groupAddDroid(buildersHunters, _builders[0]);
		debugMsg('Builder --> Hunter +1', 'group');
	}



	debugMsg("Тут будет моя база: ("+base.x+","+base.y+")", 'init');
	if(!release)mark(base.x,base.y);
}

function debugMsg(msg,level){
	if (typeof level == 'undefined') return;
//	if (debugName == "Grey" ) return; //Временно
	if(debugLevels.indexOf(level) == -1) return;
	var timeMsg = Math.floor(gameTime / 1000);
	debug(shortname+"["+timeMsg+"]{"+debugName+"}("+level+"): "+msg);
}

function eventStartLevel() {
	queue("init", 1000);
}

function eventGameLoaded(){
	queue("init", 1000);
}

function eventGameSaving(){
	running = false;
}

function eventGameSaved(){
	running = true;
	playerData.forEach( function(data, player) {
		chat(player, ' from '+debugName+': '+chatting('saved'));
	});
}
