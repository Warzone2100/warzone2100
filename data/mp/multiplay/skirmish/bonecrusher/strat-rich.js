

/*
 * OFFLINE
 * Это часть скрипта пока не используется
 * 
 * */




const research_primary = [
"R-Wpn-MG1Mk1",					//Лёгкий пулемёт (старт)
"R-Wpn-MG-Damage02",
"R-Wpn-Cannon1Mk1",				//Пушечная башня
"R-Struc-Research-Module",		//Модуль для лаборотории
//"R-Defense-Tower01",			//Оборонная вышка / пулемётная башня (старт)
"R-Vehicle-Prop-Halftracks",	//Полугусенецы
"R-Vehicle-Body05",				//Средняя начальная броня
//"R-Defense-Pillbox01"			//Пулемётный бункер
"R-Struc-Research-Upgrade09",	//Neural Synapse Research Brain Mk3 (финал)
"R-Vehicle-Metals09"			//Superdense Composite Alloys Mk3

];

const research_rockets = [
"R-Wpn-Rocket-Damage02",		//HE Rockets Mk2
"R-Wpn-Rocket02-MRL",			//Mini-Rocket Array
"R-Wpn-Rocket-Accuracy01",
"R-Wpn-Rocket-ROF03",
"R-Wpn-Rocket-Damage09",
"R-Wpn-Rocket-Accuracy02",
"R-Cyborg-Hvywpn-TK",
"R-Cyborg-Hvywpn-A-T",
"R-Wpn-HvArtMissile",
"R-Wpn-Missile-ROF03",
"R-Wpn-Missile-Damage03"
];

const research_way_1 = [
"R-Wpn-MG2Mk1",
//	"R-Wpn-MG-Damage03",
"R-Struc-Factory-Cyborg",		//Завод киборгов
"R-Wpn-MG-Damage04",			//APDSB MG Bullets Mk3
//	"R-Sys-Engineering01",			//Инженерия (старт)
//	"R-Sys-Sensor-Turret01",		//Сенсорная башня (для лидера)
"R-Wpn-MG2Mk1",					//Спаренный лёгкий пулемёт
"R-Wpn-MG3Mk1",					//Тяжолопулемётная башня
"R-Vehicle-Prop-Halftracks",	//Полугусенецы
"R-Vehicle-Body05",				//Средняя начальная броня
"R-Wpn-Cannon4AMk1",			//Hyper Velocity Cannon
//"R-Vehicle-Prop-Hover",			//Ховер для строителей
//"R-Wpn-Rocket05-MiniPod",		//Скорострельная ракетница
//"R-Wpn-Flamer01Mk1",			//Огнемётная башня
"R-Wpn-Mortar01Lt",				//Гранатное орудие
//"R-Wpn-Rocket02-MRL",			//Ракетная батарея
"R-Wpn-MG4",					//Штурмовой пулемёт
"R-Vehicle-Body11",				//Тяжёлая начальная броня
"R-Vehicle-Prop-Tracks",		//Гусенецы
"R-Wpn-Cannon6TwinAslt",		//Twin Assault Cannon
//"R-Wpn-Flame2",					//Горячий напалм
"R-Vehicle-Body09",				//Броня "Tiger"
"R-Wpn-Mortar3",				//Скорострельная мортира "Pepperpot"
//"R-Wpn-Flamer-Damage09",		//Самый последний огнемёт (финал)
//"Emplacement-RotHow",			//Самая последняя артиллерия (финал)
"R-Sys-Sensor-UpLink",			//Открыть всю карту
];

const research_way_power = [
"R-Struc-Power-Upgrade03a",
"R-Vehicle-Engine09"			//Gas Turbine Engine Mk3 (финал)

];

const research_way_defence = [
"R-Defense-Pillbox01",
"R-Defense-Tower06",
"R-Defense-WallTower-HPVcannon",
"R-Defense-MRL",
"R-Defense-HvyHowitzer"
];

const research_way_armor = [
"R-Struc-Research-Upgrade09",	//Neural Synapse Research Brain Mk3 (финал)
"R-Vehicle-Metals09",			//Superdense Composite Alloys Mk3 (финал) 
"R-Cyborg-Metals09",			//Кинетическая броня киборгов (финал)
"R-Vehicle-Engine09"			//Gas Turbine Engine Mk3 (финал)
];
const research_way_3 = [
"R-Sys-Autorepair-General",		//Автопочинка
//	"R-Defense-Wall-RotMg",
];

const research_way_mg = [
"R-Wpn-MG-ROF03",				//Hyper Fire Chaingun Upgrade
"R-Wpn-MG-Damage08",			//Depleted Uranium MG Bullets
"R-Wpn-MG5"						//Twin Assault Gun
];

const research_way_cannon = [
"R-Wpn-Cannon-ROF06",			//Cannon Rapid Loader Mk3
"R-Wpn-Cannon-Accuracy02",		//Cannon Laser Designator
"R-Wpn-Cannon-Damage09"		//HVAPFSDS Cannon Rounds Mk3
];

const research_way_5 = [
"R-Cyborg-Armor-Heat09",		//Термостойкая броня киборгов (финал)
"R-Defense-MassDriver",
"R-Vehicle-Body14"
];

//Переменная приоритетов путей исследований
var research_way = [
research_primary,
research_way_1,
research_way_armor,
research_way_cannon,
research_way_power,
research_way_mg
];


function mainBuilders(rotation){
	enumGroup(buildersMain).forEach( function(obj, iter){
		if(builderBusy(obj) == true) {debugMsg("buildersOrder(): Строитель занят "+iter);return;}
		
		
		//Модули на здания
		if(getResearch("R-Struc-Factory-Module").done && iter <=3) { factory.forEach( function(e){ if(e.modules < 1) orderDroidBuild(obj, DORDER_BUILD, "A0FacMod1", e.x, e.y);});}
		if(getResearch("R-Struc-Factory-Module").done && getResearch("R-Vehicle-Metals02").done && iter <=3) { factory.forEach( function(e){ if(e.modules < 2) orderDroidBuild(obj, DORDER_BUILD, "A0FacMod1", e.x, e.y);});}
		if(getResearch("R-Struc-PowerModuleMk1").done && iter <= 2) { power_gen.forEach( function(e){ if(e.modules < 1) orderDroidBuild(obj, DORDER_BUILD, "A0PowMod1", e.x, e.y);});}
		if(getResearch("R-Struc-Research-Module").done && iter <= 1) { research_lab.forEach( function(e){ if(e.modules < 1) orderDroidBuild(obj, DORDER_BUILD, "A0ResearchModule1", e.x, e.y);});}
		
		//Если строители свободны, ищем чего-бы достроить или починить
		var myBase = enumStruct(me);
		for ( var b in myBase ){
			if(distBetweenTwoPoints_p(base.x,base.y,myBase[b].x,myBase[b].y) > base_range)continue;
			if(myBase[b].status == BEING_BUILT) {orderDroidObj_p(obj, DORDER_HELPBUILD, myBase[b]); return;}
			if(myBase[b].health < 100) {orderDroidObj_p(obj, DORDER_REPAIR, myBase[b]); return;}
		}
		
		//Завод, лаборатория,генератор,ком-центр! - вот залог хорошего пионера!
		if(factory_ready.length < 2) { builderBuild(obj, "A0LightFactory", rotation); return; }
		else if(research_lab_ready.length < 1) { builderBuild(obj, "A0ResearchFacility", rotation); return; }
		//Ком центр
		else if(hq_ready.length == 0) { builderBuild(obj, "A0CommandCentre", rotation); return; }
		//Генератор энергии
		else if(power_gen_ready.length == 0) { builderBuild(obj, "A0PowerGenerator", rotation); return; }
		else if(research_lab_ready.length < 3 && playerPower(me) > 200) { builderBuild(obj, "A0ResearchFacility", rotation); return; }
		else if(factory_ready.length < 4 && playerPower(me) > 300) { builderBuild(obj, "A0LightFactory", rotation); return; }
		else if(research_lab_ready.length < 4 && playerPower(me) > 300) { builderBuild(obj, "A0ResearchFacility", rotation); return; }
		else if(factory_ready.length < 5 && playerPower(me) > 800) { builderBuild(obj, "A0LightFactory", rotation); return; }
		else if(research_lab_ready.length < 5 && playerPower(me) > 500) { builderBuild(obj, "A0ResearchFacility", rotation); return; }
		else if(isStructureAvailable("A0CyborgFactory") && cyborg_factory_ready.length == 2 && playerPower(me) > 300) { builderBuild(obj, "A0CyborgFactory", rotation); return; }
		else if(isStructureAvailable("A0CyborgFactory") && cyborg_factory_ready.length < 5 && playerPower(me) > 800) { builderBuild(obj, "A0CyborgFactory", rotation); return; }
		else if( (power_gen_ready.length * 4) <= resource_extractor.length && (power_gen.length < getStructureLimit("A0PowerGenerator")) ) { builderBuild(obj, "A0PowerGenerator", rotation); return; }
		//		else { debugMsg("buildersOrder: Строителям нефиг делать"); }
		
		
		
		
		if(oilHunt(obj, true)) return;
		if(rigDefence(obj)) return;

		//Если свободны, и далеко от базы - отправляем домой
		if(distBetweenTwoPoints_p(base.x,base.y,obj.x,obj.y) > 10 && !builderBusy(obj)) { orderDroidLoc_p(obj,DORDER_MOVE,base.x,base.y); return; }
		debugMsg("buildersOrder(): Строители бездельничают "+iter, 'builders');
		
		
	});
}
