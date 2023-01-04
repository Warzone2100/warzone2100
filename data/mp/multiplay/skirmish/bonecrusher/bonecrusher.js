namespace("bc_");
const vernum    = "bonecrusher"; //v1.1.1
const verdate   = "12.01.2021";
const vername   = "BoneCrusher!";
const shortname = "bc";
const release	= true;


///////\\\\\\\
//v1.00 - 10.10.2019 (Встроенная версия)
//		Remove old game version dependency (if version)
//		Remove old nfAlgorythm for game v3.1
//		Slightly improve performance, set order limits
//v1.01 - 10.12.2019 Big update
//
//v1.02 - 03.07.2020 Cosmetic update
//		Не читерить без явных указаний на это
//v1.1 - Big update
//v1.1.1 - Fix some errors
/*
+ Проверить чит-чат для INSANE (как у встроенного)
=== Строители
+ Удалять предпостроенные заборы (Есть проблема в движке, статус сносимой структуры становится "Структура возводится" и другие конструкторы едут её "Чинить")
+ Строить ремцех
++ Строить кучно, возле ближайшей нефтеточке, на краю границы базы (как то доработать, строят криво)
+ Исправлено застревание строителя возле Бочки или Артифакта
+ Строители должны строить защитное сооружение возле любой нефтеточки
+ почему то строители стали маятся, ездят по 2 по точкам, так не должно быть (вроде испрвил)
=== Армия
+ Если у мусорщиков больше ОЗ - не нападать (доработать отход)
+ Может быть изменить поведение армии, при захвате множества вышек переключать на "Rich" (Изменил, на FFA и больших ландшафтных картах не очень хорошо)
+ Продавать сильно раненых юнитов, если нет ремцехов и армия больше 10
+ Сенсор объезжает все неразведанные нефтеточки
+ Переключать на режим "Островной", когда выбиты противники на континенте, а оставшиеся недосегаемы по земле
+ Подтюнить островной режим
=== Прочее
+ Заменены устаревшие функции с groupAddDroid на groupAdd
+ Добавлен namespace на все event функции
+ Исправлены некоторые недочёты в матчах на островных картах
+ на островной карте криво строит оборонительные сооружения (починил)
+ Очередная чистка от старых алгоритмов, расчитаных на 3.1.5 версию, замена новыми
+ Исправлено, теперь при передаче боту ремонтного дроида, бот его использует
+ Делится излишками ресурсов с любым союзником
+ При выключении бота во время матча (kick), отдаёт всю энергию первому союзнику.
=== В долгосрочных планах
Отдавать приказ военным, очистить загаженный ресурс от дереьев и мусора
NTW В командной игре армии кучкует по отдельности, сделать что бы сообща играли
NTW Авиация исследует кластерные бомбы и применяет их на вражескую армию
Авиация более разумно и полностью должна использовать запас снарядов
Доработать переезд базы
Строить защитные башни вблизи занятых нефтеточек (совсем близко)
*/

//DEBUG: количество вывода, закоментить перед релизом
var debugLevels = ['error'];

//var debugLevels = ['init', 'end', 'stats', 'temp', 'production', 'group', 'events', 'error', 'research', 'builders', 'targeting'];



var debugName = me;


//Массив конкретных технологий (tech.js)
var tech = [];

include("multiplay/skirmish/"+vernum+"/names.js");

//инфа
debugName = colors[playerData[me].colour];

include("multiplay/skirmish/"+vernum+"/functions.js");

//new 3.3+
var research_path = [];
include("multiplay/skirmish/"+vernum+"/research-paths.js");
include("multiplay/skirmish/"+vernum+"/research.js");

include("multiplay/skirmish/"+vernum+"/builders.js");
include("multiplay/skirmish/"+vernum+"/targeting.js");
include("multiplay/skirmish/"+vernum+"/events.js");
include("multiplay/skirmish/"+vernum+"/produce.js");
include("multiplay/skirmish/"+vernum+"/performance.js");
include("multiplay/skirmish/"+vernum+"/chatting.js");
include("multiplay/skirmish/"+vernum+"/tech.js");
include("multiplay/skirmish/"+vernum+"/weapons.js");
include("multiplay/skirmish/"+vernum+"/build-normal.js");

/*
 *
 * НАСТРОЙКИ
 *
 */


//Hard CPU-load algorythms
var weakCPU = false;

var base_range = 20; // В каких пределах работают основные строители (не охотники)

var buildersTimer = 25000;		//Триггер для заказа строителей (что бы не выходили пачкой сразу)
var fixersTimer = 50000;		//Триггер для заказа рем.инженеров
var scannersTimer = 300000;		//Триггер для заказа сенсоров
var checkRegularArmyTimer = 10000;
var reactRegularArmyTimer = 10000;
var reactWarriorsTimer = 5000;
var reactPartisanTimer = 20000;
var fullBaseTimer = 60000;

var minBuilders = 5;

var builderPts = 750; //Необходимость энергии для постройки "лишнего" строителя

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
//const DORDER_RECOVER = 33;


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

//Массив всех приказов юнитам
var _globalOrders = [];

var build_rich = 26; //Сколько должно быть рядом нефтеточек, что бы изменить механизм постройки на rich
var army_rich = 28; //Сколько должно быть занято нефтеточек, что бы изменить механизм армии на rich

var bc_ally=[]; //Союзные ИИ BoneCrusher-ы

var avail_research = [];	//Массив возможных исследований, заполняется в функции doResearch();

//var scavengerPlayer = -1;

var rage = difficulty;

if (typeof asPlayer === "undefined") asPlayer = false;
else rage = HARD;

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
var armyCyborgs = newGroup();
var armyFixers = newGroup();
var armyJammers = newGroup();
var armyScanners = newGroup();
var VTOLAttacker = newGroup();
var droidsRecycle = newGroup();
var droidsBroken = newGroup();
var droidsFleet = newGroup();

var maxFactories, maxFactoriesCyb, maxFactoriesVTOL, maxLabs, maxPads;

//Triggers
var buildersTrigger = 0;
var fixersTrigger = 0;
var scannersTrigger = 0;
var checkRegularArmyTrigger = 0;
var reactRegularArmyTrigger = 0;
var reactWarriorsTrigger = 0;
var fullBaseTrigger = 0;
var partisanTrigger = 0;
var fleetTrigger = 0;

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




//old 3.2-
//Предустановки на исследование
var research_way = []; //Главный путь развития, компануется далее, в функциях, в зависимости от уровня сложности и др. настроек
var research_primary = []; //Первичный, один из главных под-путей развития, к которому задаётся режим его исследований(строгий, размазанный или случайный)
const research_synapse = ["R-Struc-Research-Upgrade09"];
const research_power = ["R-Struc-Power-Upgrade03a"];
const research_armor = ["R-Vehicle-Metals09"];
const research_sensor = ["R-Sys-Sensor-UpLink"];

//Переназначаются в функции prepeareProduce() что бы не читерить.
//var light_bodies=["Body3MBT","Body2SUP","Body4ABT","Body1REC"];
var light_bodies=["Body3MBT","Body2SUP","Body1REC"];
var medium_bodies=["Body7ABT","Body6SUPP","Body8MBT","Body5REC"];
var heavy_bodies=["Body13SUP","Body10MBT","Body9REC","Body12SUP","Body11ABT"];
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
['R-Defense-MortarPit-Incendiary' , 'Emplacement-MortarPit-Incendiary'],	//Адская мортира
];

var AA_defence = [];
var AA_queue = [];
var AA_towers=[
['R-Defense-AASite-QuadMg1', 'AASite-QuadMg1'],					//Hurricane AA Site
['R-Defense-AASite-QuadBof', 'AASite-QuadBof'],					//AA Cyclone Flak Cannon Emplacement
['R-Defense-WallTower-DoubleAAgun', 'WallTower-DoubleAAGun'],	//AA Cyclone Flak Cannon Hardpoint
['R-Defense-Sunburst', 'P0-AASite-Sunburst'],					//Sunburst AA Site
['R-Defense-SamSite1', 'P0-AASite-SAM1'],						//Avenger SAM Site
['R-Defense-SamSite2', 'P0-AASite-SAM2'],						//Vindicator SAM Site
['R-Defense-WallTower-QuadRotAA', 'WallTower-QuadRotAAGun'],	//Whirlwind Hardpoint
['R-Defense-AA-Laser', 'P0-AASite-Laser'],						//Stormbringer Emplacement
];

//Инициализация
function init(){

	if (isHumanOverride()) {debugMsg("Human override detected..", 'init');rage=HARD;}

	debugMsg("ИИ №"+me+" "+vername+" "+vernum+"("+verdate+") difficulty="+rage, "init");
	debugMsg("Warzone2100 "+version, "init");

	//Определеяем моды
	debugMsg("MODS: "+modList, "init");

	if (modList.indexOf('oilfinite') !== -1) {
		nf['oilfinite'] = true;
		debugMsg('Consider oilfinite mod', "init");
	}

	//Определяем мусорщиков
	//Больше не требуется, игра сама предоставляет эту переменную
//	scavengerPlayer = (scavengers) ? Math.max(7,maxPlayers) : -1;
	if (scavengers !== NO_SCAVENGERS) debugMsg("На карте присудствуют гопники! {"+scavengerPlayer+"}", "init");
	else debugMsg("На карте отсутствуют гопники", "init");

//	base = startPositions[me];
	initBase();
	startPos = base;

	var technology = enumResearch();
	if (technology.length) debugMsg("Доступных исследований: "+technology.length, "init");
	else debugMsg("ВНИМАНИЕ: Нет доступных исследований", "init");

	debugMsg('Is Multiplayer: '+isMultiplayer, 'init');
	debugMsg('Is Human in Ally: '+isHumanAlly(), 'init');
	debugMsg('Num Enemies: '+getNumEnemies(), 'init');

	//Получаем координаты всех ресурсов и занятых и свободных
	var freeResources = getFreeResources();
	var nearResources = freeResources.filter((e) => (distBetweenTwoPoints_p(base.x,base.y,e.x,e.y) < base_range));
	nearResources = nearResources.concat(enumStruct(me, "A0ResourceExtractor").filter((e) => (distBetweenTwoPoints_p(base.x,base.y,e.x,e.y) < base_range)));
	debugMsg("На карте "+freeResources.length+" свободных ресурсов", 'init');

	allResources = getAllResources();

	debugMsg("На карте "+allResources.length+" всего ресурсов, рядом "+nearResources.length, 'init');

	_builders = enumDroid(me,DROID_CONSTRUCT);

	debugMsg("Игроков на карте: "+maxPlayers,2);
	var access=false;
	playerData.forEach((data, player) => {
		var msg = "Игрок №"+player+" "+colors[data.colour];
		var dist = distBetweenTwoPoints_p(base.x,base.y,startPositions[player].x,startPositions[player].y);

		if (player === me) {
			msg+=" я сам ИИ";
			bc_ally.push(player);
//			debugMsg("TEST: "+bc_ally.length, 'research');
			//			debugName = colors[data.colour];
		}
		else if (playerLoose(player)) {msg+=" отсутствует";}
		else if (playerSpectator(player)) {msg+=" наблюдатель";}
		else if (allianceExistsBetween(me,player)) {
			msg+=" мой союзник ";
			ally.push(player);
			if (data.name === 'bc-master' || data.name.substr(0,11) === "BoneCrusher") { msg+="BC!"; bc_ally.push(player);}
			else {msg+=data.name;}
		}
		else {
			msg+=" мой враг";
			enemy.push(player);
			if (propulsionCanReach('wheeled01', base.x, base.y, startPositions[player].x, startPositions[player].y)) { msg+= ", по земле"; access = 'land';}
			else if (propulsionCanReach('hover01', base.x, base.y, startPositions[player].x, startPositions[player].y)) { msg+= ", по воде"; access = 'island';}
			else if (propulsionCanReach('V-Tol', base.x, base.y, startPositions[player].x, startPositions[player].y)) { msg+= ", по воздуху"; access = 'air';}
			else {msg+= ", не доступен!"; access = 'island';}
			if (!nf['policy'] || nf['policy'] === 'island' || nf['policy'] === 'air') {nf['policy'] = access;}
		}

		msg+=" ["+startPositions[player].x+"x"+startPositions[player].y+"]";
		msg+=" дист. "+dist;
		debugMsg(msg,"init");

	});
	debugMsg('bc_ally.length: '+bc_ally.length, 'init');

	if (ally.length === 0) {
		debugMsg("Союзников нет" , 'init');
	}
	if (ally.length === 1) {
		debugMsg("Имеется союзник" , 'init');
	}
	if (ally.length > 1) {
		debugMsg("Имеются союзники" , 'init');
	}
	if (ally.length > 0) {
		if (alliancesType === 2) debugMsg("Исследования общие", 'init');
		if (alliancesType === 3) debugMsg("Исследования раздельные", 'init');
	}
	if (nearResources.length >= build_rich) {
		policy['build'] = 'rich';
		initBase();
	} else {
		policy['build'] = 'standart';
	}

	debugMsg("Policy build order = "+policy['build'], 'init');
	debugMsg("nf Policy = "+nf['policy'], 'init');

	if (policy['build'] === 'rich') {

		//Если есть союзники бонкрашеры
		if (bc_ally.length > 1) {
			var researches = [research_rich2, research_fire1, research_cannon, research_fire2, research_rich, research_rockets];
			var r = bc_ally.indexOf(me)%researches.length;
			debugMsg('Get research path #'+r+', from ally researches array', 'init');
			research_path = researches[r];
		} else {
			var researches = [
				research_rich2, research_rich2, research_rich2, research_rich2, research_rich2,
				research_cannon, research_cannon,
				research_fire2,
				research_rich, research_rich, research_rich,
				research_fire1, research_fire1,
				research_fire3, research_fire3, research_fire3,
				research_rockets];
			var r = Math.floor(Math.random()*researches.length);
			debugMsg('Get research path #'+r+', from solo researches array', 'init');
			research_path = researches[r];
		}

		if (technology.length)cyborgs.unshift(["R-Wpn-MG1Mk1", "CyborgLightBody", "CyborgChaingun"]);

		buildersTimer = 7000;
		minBuilders = 10;
		minPartisans = 1;
		maxPartisans = 4;
		builderPts = 150;
		maxRegular = 100;
		scannersTimer = 120000;
	} else {
		//Если есть союзники бонкрашеры
		if (bc_ally.length > 1) {
			var researches = [research_fire1, research_cannon, research_fire2, research_rich, research_rockets];
			var r = bc_ally.indexOf(me)%researches.length;
			debugMsg('Get research path #'+r+', from ally researches array', 'init');
			research_path = researches[r];
		} else {

			var researches = [
				research_rich2,
				research_cannon, research_cannon, research_cannon, research_cannon, research_cannon,
				research_fire2,
				research_rich,
				research_fire1,
				research_fire3, research_fire3,
				research_rockets];

//			var researches = [research_green];
			var r = Math.floor(Math.random()*researches.length);
			debugMsg('Get research path #'+r+', from solo researches array', 'init');
			research_path = researches[r];
		}
	}

	if (nf['oilfinite'])research_path = research_earlygame.concat(["R-Sys-MobileRepairTurret01"]).concat(research_path).concat(research_lasttech);
	else research_path = research_earlygame.concat(research_path).concat(research_lasttech);

	//Лимиты:
	maxFactories = getStructureLimit("A0LightFactory", me);
	maxLabs = getStructureLimit("A0ResearchFacility", me);
	maxGenerators = getStructureLimit("A0PowerGenerator", me);
	maxFactoriesCyb = getStructureLimit("A0CyborgFactory", me);
	maxFactoriesVTOL = getStructureLimit("A0VTolFactory1", me);
	maxPads = getStructureLimit("A0VtolPad", me);


	//Лёгкий режим
	if (rage === EASY) {
		debugMsg("Похоже я играю с нубами, будем поддаваться:", 'init');

		//Забываем все предустановленные исследования
		//Исследуем оружия и защиту в полном рандоме.
//		research_way=[["R-Wpn-MG1Mk1"],["R-Struc-Research-Upgrade09"],["R-Struc-Power-Upgrade03a"]];
		research_path = research_earlygame;

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



	} else if (rage === MEDIUM) {
		buildersTimer = buildersTimer + Math.floor(Math.random()*5000 - 2000);
		minBuilders = minBuilders + Math.floor(Math.random() * 5 - 2 );
		builderPts = builderPts + Math.floor(Math.random() * 200 - 150);
		minPartisans = minPartisans + Math.floor(Math.random() * 6 - 4);

		//Если в союзниках человек и исслоедования общий, а мы на средней сложности, то просто поддерживаем исследования (без особой ветки)
		if (alliancesType === 2 && isHumanAlly()) {research_path = research_earlygame.concat(research_lasttech);}

	}
	debugMsg("minPartisans="+minPartisans+", minBuilders="+minBuilders+", builderPts="+builderPts+", buildersTimer="+buildersTimer, "init");
	debugMsg("Лимиты базы: maxFactories="+maxFactories+"; maxFactoriesCyb="+maxFactoriesCyb+"; maxFactoriesVTOL="+maxFactoriesVTOL+"; maxPads="+maxPads+"; maxLabs="+maxLabs+"; maxGenerators="+maxGenerators+"; maxExtractors="+maxExtractors, 'init');
	debugMsg("Лимиты юнитов: maxPartisans="+maxPartisans+"; maxRegular="+maxRegular+"; maxCyborgs="+maxCyborgs+"; maxVTOL="+maxVTOL+"; maxFixers="+maxFixers+"; maxConstructors="+maxConstructors, 'init');




	if (nf['policy'] === 'island') {
		debugMsg("Тактика игры: "+nf['policy'], 'init');
		switchToIsland();
	}

	if (!release)research_path.forEach((e) => {debugMsg(e, 'init');});

	if (!release) for (let p = 0; p < maxPlayers; ++p) {debugMsg("startPositions["+p+"] "+startPositions[p].x+"x"+startPositions[p].y, 'init');}

	//Просто дебаг информация
	var oilDrums = enumFeature(ALL_PLAYERS, "OilDrum");
	debugMsg("На карте "+oilDrums.length+" бочек с нефтью", 'init');

	queue("welcome", 3000+me*(Math.floor(Math.random()*2000)+1500) );
	queue("checkAlly", 2000);
	letsRockThisFxxxingWorld(true); // <-- Жжём плазмитом сцуко!
}

function welcome(){
	playerData.forEach((data, player) => {
		if (!asPlayer)chat(player, ' from '+debugName+': '+chatting('welcome'));
	});
}

//Старт
function letsRockThisFxxxingWorld(init){
	debugMsg("Старт/Run", 'init');

	include("multiplay/skirmish/"+vernum+"/weap-init.js");

	//Remove chaingun and flamer cyborgs if better available
	cyborgs = cyborgs.filter((e) => (!((e[2] === 'CyborgChaingun' && getResearch('R-Wpn-MG4').done) || (e[2] === 'CyborgFlamer01' && getResearch('R-Wpn-Flame2').done))));

	//Первых военных в группу
	enumDroid(me,DROID_CYBORG).forEach((e) => {groupAdd(armyCyborgs, e);});
	enumDroid(me,DROID_WEAPON).forEach((e) => {groupAdd(armyCyborgs, e);}); // <-- Это не ошибка, первых бесплатных определяем как киборгов (работа у них будет киборгская)

	setTimer("secondTick", 1000);
	queue("buildersOrder", 1000);
	queue("prepeareProduce", 2000);
	queue("produceDroids", 3000);
	queue("doResearch", 3000);
	setTimer("longCycle", 120000);

	running = true;
	if (init) {
		if (rage === EASY) {

			setTimer("produceDroids", 10000+me*100);
			setTimer("produceVTOL", 12000+me*100);
			setTimer("checkEventIdle", 60000+me*100);	//т.к. eventDroidIdle глючит, будем дополнительно отслежвать.
			setTimer("doResearch", 60000+me*100);
			setTimer("defenceQueue", 60000+me*100);
			setTimer("produceCyborgs", 25000+me*100);
//			setTimer("buildersOrder", 120000+me*100);
			setTimer("targetVTOL", 120000+me*100); //Не раньше 30 сек.


		} else if (rage === MEDIUM) {

			setTimer("produceDroids", 7000+me*100);
			setTimer("produceVTOL", 8000+me*100);
			setTimer("produceCyborgs", 9000+me*100);
//			if (policy['build'] === 'rich') setTimer("buildersOrder", 5000+me*100);
//			else setTimer("buildersOrder", 120000+me*100);
			setTimer("checkEventIdle", 30000+me*100);	//т.к. eventDroidIdle глючит, будем дополнительно отслежвать.
			setTimer("doResearch", 30000+me*100);
			setTimer("defenceQueue", 60000+me*100);
			setTimer("targetVTOL", 56000+me*100); //Не раньше 30 сек.
			setTimer("targetRegular", 65000+me*100);

			if (policy['build'] === 'rich') func_buildersOrder_timer = 5000+me*100;
		} else if (rage === HARD || rage === INSANE) {

//			research_way.unshift(["R-Defense-MortarPit-Incendiary"]);

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
			func_buildersOrder_timer = 2000+me*100;
		}

		if (!release) {
			setTimer("stats", 10000); // Отключить в релизе
		}
		setTimer("checkProcess", 60000+me*100);
//		setTimer("perfMonitor", 5000);

	}
}

function initBase(){
	//Первых строителей в группу
	checkBase();
	var _builders = enumDroid(me,DROID_CONSTRUCT);

	//Получаем свои координаты
	var _r = Math.floor(Math.random()*_builders.length);
	if (_builders.length > 0) base = {x:_builders[_r].x, y:_builders[_r].y};

	_builders.forEach((e) => {groupBuilders(e);});

	if (policy['build'] === 'rich' && _builders.length > 4) {
		groupAdd(buildersHunters, _builders[0]);
		debugMsg('Builder --> Hunter +1', 'group');
	}



	debugMsg("Тут будет моя база: ("+base.x+","+base.y+")", 'init');
	if (!release)mark(base.x,base.y);
}

function debugMsg(msg,level){
	if (typeof level === "undefined") return;
//	if (debugName === "Grey") return; //Временно
	if (debugLevels.indexOf(level) === -1) return;
	var timeMsg = Math.floor(gameTime / 1000);
	debug(shortname+"["+timeMsg+"]{"+debugName+"}("+level+"): "+msg);
}

function bc_eventStartLevel() {
	if (version !== '3.3.0')
	queue("init", 1000);
}

function bc_eventGameLoaded(){
	queue("init", 1000);
}

function bc_eventGameSaving(){
	running = false;
}

function bc_eventGameSaved(){
	running = true;
	playerData.forEach((data, player) => {
		if (!asPlayer)chat(player, ' from '+debugName+': '+chatting('saved'));
	});
}
