const research_test = ["R-Wpn-MG1Mk1"];

var research_primary = [
"R-Wpn-MG1Mk1",
"R-Wpn-MG-Damage02",			//APDSB MG Bullets
"R-Defense-Tower01",			//Оборонная вышка / пулемётная башня (старт)
"R-Wpn-MG2Mk1",					//Спаренный лёгкий пулемёт
"R-Vehicle-Engine01",			//Fuel Injection Engine
"R-Sys-Sensor-Turret01",		//Сенсор
"R-Struc-CommandRelay",
"R-Struc-PowerModuleMk1",		//Модуль на генератор
"R-Struc-Research-Module",		//Модуль для лаборотории
"R-Wpn-MG-Damage03",			//APDSB MG Bullets Mk2
"R-Wpn-MG3Mk1",					//Heavy Machinegun
"R-Struc-Power-Upgrade01",
"R-Struc-Power-Upgrade01b",
"R-Struc-Power-Upgrade01c",
"R-Struc-Power-Upgrade02",
"R-Struc-Power-Upgrade03",
"R-Struc-Power-Upgrade03a",
//"R-Struc-VTOLFactory",			//Авиазавод
//"R-Struc-VTOLPad-Upgrade06",	//Заправка авиации A0VtolPad
];


const research_way_1 = [
"R-Wpn-MG1Mk1",
//"R-Struc-VTOLFactory",			//Авиазавод(temp)
"R-Sys-Engineering01",			//Инженерия
"R-Struc-PowerModuleMk1",
"R-Vehicle-Prop-Halftracks",	//Полугусенецы
"R-Sys-MobileRepairTurret01",	//Паяльник
"R-Wpn-Cannon1Mk1",				//Лёгкая пушка
"R-Struc-Factory-Cyborg",		//Завод киборгов
"R-Wpn-Flamer01Mk1",
"R-Wpn-Rocket05-MiniPod",
"R-Defense-WallTower02",
"R-Vehicle-Body05",				//Средняя начальная броня
"R-Vehicle-Body04",				//Лёгкая броня Bug
//	"R-Wpn-MG-Damage03",
"R-Wpn-MG-Damage04",			//APDSB MG Bullets Mk3
//	"R-Sys-Engineering01",			//Инженерия (старт)
//	"R-Sys-Sensor-Turret01",		//Сенсорная башня (для лидера)
"R-Wpn-MG3Mk1",					//Тяжолопулемётная башня
"R-Vehicle-Prop-Hover",			//Ховер для строителей
"R-Sys-MobileRepairTurretHvy",	//Тяжёлый паяльник
"R-Vehicle-Body08",				//Medium Body - Scorpion
"R-Struc-VTOLFactory",			//Авиазавод
"R-Struc-VTOLPad",				//Заправка авиации A0VtolPad
"R-Defense-AASite-QuadMg1",		//Hurricane AA Site
"R-Wpn-Rocket01-LtAT",			//Лансер
"R-Sys-Autorepair-General",		//Автопочинка
"R-Wpn-Bomb03",					//Фосфорные бомбы
"R-Vehicle-Body12",				//Heavy Body - Mantis
"R-Wpn-Rocket05-MiniPod",		//Скорострельная ракетница
"R-Wpn-Cannon1Mk1",				//Пушечная башня
"R-Wpn-Flamer01Mk1",			//Огнемётная башня
"R-Wpn-Mortar01Lt",				//Гранатное орудие
"R-Wpn-Rocket02-MRL",			//Ракетная батарея
"R-Wpn-MG4",					//Штурмовой пулемёт
"R-Vehicle-Body11",				//Тяжёлая начальная броня
"R-Defense-WallTower-QuadRotAA",//Whirlwind Hardpoint
"R-Struc-VTOLPad-Upgrade06",	//Заправка авиации A0VtolPad
"R-Vehicle-Prop-Tracks",		//Гусенецы
"R-Wpn-Flame2",					//Горячий напалм
"R-Wpn-Bomb04",					// Термитные бомы
"R-Vehicle-Body09",				//Броня "Tiger"
"R-Wpn-Mortar3",				//Скорострельная мортира "Pepperpot"
"R-Wpn-Flamer-Damage09",		//Самый последний огнемёт (финал)
//	"Emplacement-RotHow",			//Самая последняя артиллерия (финал)
"R-Sys-Sensor-UpLink",			//Открыть всю карту
];

var research_way_ng = [
"R-Sys-Sensor-Upgrade03",
];

/*
if(version != "3.1"){
	research_way_ng.unshift("R-Sys-ECM-Upgrade01");
	research_way_ng.pop("R-Sys-ECM-Upgrade02");
}
*/

const research_way_2 = [
"R-Wpn-MG1Mk1",
//"R-Struc-Factory-Cyborg",		//Казармы
"R-Struc-PowerModuleMk1",		//Модуль на генератор
"R-Sys-MobileRepairTurret01",	//Паяльник
"R-Struc-Research-Upgrade09",
"R-Vehicle-Engine09",
"R-Cyborg-Metals09",			//Кинетическая броня киборгов (финал)
];

const research_way_armor = [
"R-Vehicle-Metals09",			//Superdense Composite Alloys Mk3 (финал) 
"R-Vehicle-Armor-Heat09"		//Vehicle Superdense Thermal Armor Mk3
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
"R-Wpn-Missile-Damage03",
];

const research_way_power = [
"R-Struc-Power-Upgrade03a",
];

const research_way_defence = [
//"R-Defense-Pillbox01",
"R-Defense-Tower06",
"R-Defense-WallTower01",
"R-Defense-WallTower-HPVcannon",
"R-Defense-Emplacement-HPVcannon",
"R-Defense-MRL",
"R-Defense-MortarPit",
"R-Defense-RotMor",
"R-Defense-IDFRocket",
"R-Defense-MortarPit-Incenediary",
"R-Defense-Pillbox06",
"R-Defense-WallTower-TwinAGun",
"R-Defense-HvyHowitzer",
];

const research_way_mg = [
"R-Wpn-MG-ROF03",				//Hyper Fire Chaingun Upgrade
"R-Wpn-MG5",					//Twin Assault Gun
"R-Wpn-MG-Damage08",			//Depleted Uranium MG Bullets
"R-Defense-WallTower-QuadRotAA",//Whirlwind Hardpoint
"R-Wpn-AAGun-Damage06",
"R-Wpn-AAGun-ROF06",
];

const research_way_cannon = [
"R-Wpn-Cannon4AMk1",			//Hyper Velocity Cannon
"R-Wpn-Cannon-ROF01",			//Cannon auto loader
"R-Wpn-Cannon6TwinAslt",		//Twin Assault Cannon
"R-Wpn-Cannon-ROF06",			//Cannon Rapid Loader Mk3
"R-Wpn-Cannon-Accuracy02",		//Cannon Laser Designator
"R-Wpn-Cannon-Damage09"		//HVAPFSDS Cannon Rounds Mk3
];


const research_way_5 = [
"R-Cyborg-Armor-Heat09",		//Термостойкая броня киборгов (финал)
"R-Defense-MassDriver",
"R-Vehicle-Body14",
];

//Переменная приоритетов путей исследований
research_way = [
	research_way_1,
	research_way_mg,
	research_way_power,
	research_way_ng,
	research_way_2,
	research_way_defence,
	research_way_armor,
	research_way_cannon,
	research_rockets,
	research_way_5
];
