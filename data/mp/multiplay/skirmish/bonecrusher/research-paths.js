debugMsg('Module: research-paths.js','init');

const research_lasttech = [
	"R-Struc-Research-Upgrade09",
	"R-Wpn-LasSat",
	"R-Sys-Sensor-UpLink",
	"R-Struc-Power-Upgrade03a",
	"R-Sys-Autorepair-General",
	"R-Vehicle-Body14",
	"R-Vehicle-Metals09",
	"R-Cyborg-Metals09",
	"R-Struc-Materials03",
	"R-Vehicle-Armor-Heat09",
	"R-Cyborg-Armor-Heat09"
];

/*
const research_earlygame = [
	"R-Defense-Tower01", //защита
	"R-Sys-Engineering01",
	"R-Vehicle-Prop-Halftracks",
	"R-Wpn-MG3Mk1",		// #38  Heavy Machinegun
	"R-Struc-PowerModuleMk1",
	"R-Struc-Research-Module",
	"R-Vehicle-Prop-Hover"
];
*/
const research_earlygame = [
	"R-Defense-Tower01",
	"R-Vehicle-Prop-Halftracks",
	"R-Struc-PowerModuleMk1",
	"R-Sys-MobileRepairTurret01",   //Паяльник
	"R-Vehicle-Prop-Hover"
//	"R-Struc-RepairFacility"
];


const research_test = [
	"R-Vehicle-Prop-Hover"
];

/*const research_cannon = [
"R-Defense-Tower01",
"R-Wpn-Cannon1Mk1",		// #6  Light Cannon
//"R-Defense-WallTower02", //защита лёгкая пушка
"R-Struc-Power-Upgrade03a",
"R-Struc-Research-Upgrade09",
//"R-Sys-MobileRepairTurretHvy",
"R-Sys-Autorepair-General",             //Автопочинка
//"R-Sys-ECM-Upgrade01",	//Глушилка
"R-Wpn-MG-ROF03",				//Hyper Fire Chaingun Upgrade
"R-Vehicle-Body11",		// #101  Heavy Body - Python
"R-Wpn-MG5",					//Twin Assault Gun
"R-Wpn-MG3Mk1",
"R-Wpn-Cannon4AMk1",			//Hyper Velocity Cannon
//"R-Sys-ECM-Upgrade02",
"R-Cyborg-Metals04",            // #16  Cyborg Dense Composite Alloys
"R-Vehicle-Metals03",           // #11  Composite Alloys Mk3
'R-Defense-Emplacement-HPVcannon',
"R-Wpn-Cannon6TwinAslt",		////Спаренная штурмовая пушка
"R-Cyborg-Hvywpn-Acannon",		// #46  Super Auto-Cannon Cyborg
"R-Wpn-Plasmite-Flamer", // plasmite
"R-Wpn-Cannon-ROF06",			//Cannon Rapid Loader Mk3
"R-Wpn-Cannon-Accuracy02",		//Cannon Laser Designator
"R-Wpn-MG-Damage04",            // #32  APDSB MG Bullets Mk3
"R-Wpn-Cannon-Damage09",		//HVAPFSDS Cannon Rounds Mk3
"R-Sys-MobileRepairTurretHvy",
'R-Defense-WallTower-HPVcannon',
"R-Wpn-MG5",					//Twin Assault Gun
"R-Wpn-MG-Damage08",			//Depleted Uranium MG Bullets
"R-Vehicle-Metals09",			//Superdense Composite Alloys Mk3 (финал)
"R-Vehicle-Armor-Heat09",		//Vehicle Superdense Thermal Armor Mk3
"R-Cyborg-Metals09",
"R-Struc-VTOLPad",
"R-Wpn-Bomb04",
"R-Defense-WallTower-DoubleAAgun"		// #107  AA Cyclone Flak Cannon Hardpoint
];*/

const research_rich2 = [
"R-Vehicle-Engine01", 
"R-Wpn-Cannon1Mk1", 
"R-Vehicle-Prop-Halftracks", 
"R-Vehicle-Body05", 
"R-Vehicle-Body11", 
"R-Struc-RepairFacility",
"R-Struc-Research-Upgrade09",
"R-Wpn-Cannon4AMk1",
"R-Wpn-RailGun03", 
"R-Wpn-Cannon6TwinAslt", 
"R-Vehicle-Metals04", 
"R-Cyborg-Metals04", 
"R-Cyborg-Hvywpn-Mcannon",
"R-Wpn-MG2Mk1",
"R-Wpn-Rail-Damage02",
"R-Cyborg-Hvywpn-RailGunner",
"R-Vehicle-Prop-Tracks",
"R-Vehicle-Body10",
"R-Sys-Autorepair-General",
"R-Struc-Materials03",
"R-Wpn-LasSat",
"R-Sys-Sensor-UpLink"
];

const research_rockets = [
"R-Wpn-MG2Mk1",					//Спаренный лёгкий пулемёт
"R-Wpn-MG-ROF03",				//Hyper Fire Chaingun Upgrade
"R-Sys-Engineering01",
"R-Struc-Research-Module",
"R-Defense-Tower01",
"R-Struc-PowerModuleMk1",
"R-Defense-MRL",
"R-Vehicle-Prop-Halftracks",	//Полугусенецы
"R-Wpn-Rocket-ROF03",
"R-Struc-RepairFacility",
"R-Sys-MobileRepairTurret01",   //Паяльник
"R-Wpn-MG-Damage08",			//Depleted Uranium MG Bullets
"R-Wpn-Rocket01-LtAT",
"R-Vehicle-Body05",				//Средняя начальная броня
"R-Struc-Research-Upgrade09",
"R-Sys-MobileRepairTurretHvy",
"R-Vehicle-Metals09",			//Superdense Composite Alloys Mk3 (финал) 
"R-Sys-Autorepair-General",		//Автопочинка
//"R-Sys-ECM-Upgrade02",	//Глушилка
"R-Wpn-MG5",					//Twin Assault Gun
"R-Wpn-Rocket-Accuracy02",
"R-Sys-MobileRepairTurretHvy",
"R-Defense-MRLHvy",
"R-Wpn-Rocket-Damage09",
"R-Struc-VTOLPad",				//Заправка авиации A0VtolPad
"R-Wpn-Rocket07-Tank-Killer",
"R-Struc-Power-Upgrade03a",
"R-Wpn-RocketSlow-Accuracy02",
"R-Wpn-Missile2A-T",
"R-Vehicle-Armor-Heat09",		//Vehicle Superdense Thermal Armor Mk3
"R-Cyborg-Hvywpn-A-T",
];

const research_fire1 = [
"R-Defense-Tower01",
"R-Sys-Engineering01",
"R-Struc-Research-Module",
"R-Vehicle-Prop-Halftracks",	//Полугусенецы
"R-Wpn-Mortar01Lt",
"R-Wpn-Mortar-Damage03",
"R-Vehicle-Body05",                             //Средняя начальная броня
"R-Wpn-Flamer01Mk1",
"R-Struc-PowerModuleMk1",
"R-Struc-Power-Upgrade03a",
"R-Defense-MortarPit-Incendiary",
"R-Struc-Factory-Cyborg",
"R-Wpn-Flamer-ROF03",
"R-Struc-Research-Upgrade09",
"R-Wpn-Plasmite-Flamer", // plasmite
"R-Struc-Power-Upgrade03a",
"R-Struc-RepairFacility",
"R-Cyborg-Metals09",
"R-Wpn-MG-ROF03",
//"R-Sys-ECM-Upgrade02",	//Глушилка
"R-Sys-MobileRepairTurretHvy",
"R-Vehicle-Body08",
"R-Sys-Autorepair-General",             //Автопочинка
"R-Wpn-Flamer-Damage09",
"R-Wpn-Mortar-ROF04",
"R-Wpn-Mortar-Damage06",
"R-Wpn-Mortar-Acc03",
"R-Vehicle-Body12",
"R-Struc-VTOLPad",
"R-Vehicle-Prop-Tracks",		// #98  Tracked Propulsion
"R-Wpn-Bomb04",
"R-Wpn-Howitzer-Incendiary",
"R-Wpn-Howitzer-ROF04",
"R-Wpn-HvyHowitzer",
"R-Wpn-Howitzer-Accuracy03",
"R-Wpn-Howitzer-Damage06",
"R-Wpn-Howitzer03-Rot",
"R-Sys-Sensor-UpLink"
];

const research_fire2 = [
"R-Defense-Tower01",
"R-Sys-Engineering01",
"R-Vehicle-Prop-Halftracks",	//Полугусенецы
"R-Wpn-MG5",					//Twin Assault Gun
"R-Vehicle-Body08",
"R-Struc-PowerModuleMk1",
"R-Struc-Power-Upgrade03a",
"R-Wpn-Cannon1Mk1",		// #6  Light Cannon
"R-Wpn-Flamer01Mk1",
"R-Struc-Factory-Cyborg",
"R-Struc-Research-Module",
"R-Wpn-MG2Mk1",					//Спаренный лёгкий пулемёт
"R-Wpn-Flamer-ROF03",
"R-Struc-RepairFacility",
"R-Wpn-Plasmite-Flamer", // plasmite
"R-Struc-Research-Upgrade09",
"R-Vehicle-Prop-Halftracks",	//Полугусенецы
"R-Defense-WallTower-QuadRotAA",
"R-Struc-VTOLPad",
"R-Wpn-Bomb04",
"R-Struc-Power-Upgrade03a",
//"R-Sys-ECM-Upgrade02",	//Глушилка
"R-Vehicle-Body05",				//Средняя начальная броня
"R-Cyborg-Metals09",
"R-Wpn-Cannon6TwinAslt",		////Спаренная штурмовая пушка
"R-Sys-MobileRepairTurretHvy",
"R-Vehicle-Body08",
"R-Sys-Autorepair-General",		//Автопочинка
"R-Wpn-Flamer-Damage09",
"R-Defense-MortarPit-Incendiary",
//"R-Wpn-Cannon4AMk1",			//Hyper Velocity Cannon
"R-Wpn-Cannon-ROF01",			//Cannon auto loader
"R-Wpn-Cannon6TwinAslt",		//Twin Assault Cannon
"R-Wpn-Cannon-ROF06",			//Cannon Rapid Loader Mk3
"R-Wpn-Cannon-Accuracy02",		//Cannon Laser Designator
"R-Wpn-Cannon-Damage09",		//HVAPFSDS Cannon Rounds Mk3
"R-Wpn-Mortar-ROF04",
"R-Wpn-Mortar-Damage06",
"R-Wpn-Mortar-Acc03",
"R-Vehicle-Body12",
"R-Defense-AASite-QuadMg1",		//Hurricane AA Site
"R-Wpn-Howitzer-Incendiary",
"R-Wpn-Howitzer-ROF04",
"R-Wpn-HvyHowitzer",
"R-Wpn-Howitzer-Accuracy03",
"R-Defense-WallTower-QuadRotAA",//Whirlwind Hardpoint
"R-Wpn-Howitzer-Damage06",
"R-Wpn-Howitzer03-Rot",
"R-Sys-Sensor-UpLink",

];

const research_rich = [
"R-Sys-Engineering01",		// #1  Engineering
"R-Sys-Sensor-Turret01",		// #3  Sensor Turret
"R-Wpn-MG1Mk1",		// #0  Machinegun
"R-Wpn-MG-Damage01",		// #2  Hardened MG Bullets
"R-Wpn-Cannon1Mk1",		// #4  Light Cannon
"R-Vehicle-Engine01",		// #5  Fuel Injection Engine
"R-Struc-PowerModuleMk1",		// #6  Power Module
"R-Vehicle-Prop-Halftracks",		// #7  Half-tracked Propulsion
"R-Defense-HardcreteWall",		// #8  Hardcrete
"R-Defense-WallTower02",		// #9  Light Cannon Hardpoint
"R-Sys-Sensor-Tower01",		// #10  Sensor Tower
"R-Struc-CommandRelay",		// #11  Command Relay Post
"R-Struc-Research-Module",		// #12  Research Module
"R-Struc-Research-Upgrade01",		// #13  Synaptic Link Data Analysis
"R-Struc-Research-Upgrade02",		// #14  Synaptic Link Data Analysis Mk2
"R-Struc-Factory-Cyborg",		// #15  Cyborg Factory
"R-Struc-Factory-Module",		// #16  Factory Module
"R-Cyborg-Metals01",		// #17  Cyborg Composite Alloys
"R-Wpn-Cannon-Damage01",		// #18  HEAT Cannon Shells
"R-Struc-Research-Upgrade03",		// #19  Synaptic Link Data Analysis Mk3
"R-Wpn-Cannon-Damage02",		// #20  HEAT Cannon Shells Mk2
"R-Wpn-Cannon-Accuracy01",		// #21  Cannon Laser Rangefinder
"R-Struc-Research-Upgrade04",		// #22  Dedicated Synaptic Link Data Analysis
"R-Struc-Power-Upgrade01",		// #23  Gas Turbine Generator
"R-Struc-Power-Upgrade01b",		// #24  Gas Turbine Generator Mk2
"R-Vehicle-Metals01",		// #25  Composite Alloys
"R-Cyborg-Metals02",		// #26  Cyborg Composite Alloys Mk2
"R-Vehicle-Body08",
"R-Struc-RepairFacility",
"R-Struc-Power-Upgrade01c",		// #27  Gas Turbine Generator Mk3
"R-Struc-Research-Upgrade05",		// #28  Dedicated Synaptic Link Data Analysis Mk2
"R-Struc-Research-Upgrade06",		// #29  Dedicated Synaptic Link Data Analysis Mk3
"R-Vehicle-Body05",		// #30  Medium Body - Cobra
"R-Struc-Research-Upgrade07",		// #31  Neural Synapse Research Brain
"R-Struc-Power-Upgrade02",		// #32  Vapor Turbine Generator
"R-Wpn-MG-Damage02",		// #33  APDSB MG Bullets
"R-Wpn-MG2Mk1",		// #34  Twin Machinegun
"R-Wpn-Flamer01Mk1",		// #35  Flamer
"R-Wpn-Flamer-Damage01",		// #36  High Temperature Flamer Gel
"R-Wpn-MG-Damage03",		// #37  APDSB MG Bullets Mk2
"R-Wpn-MG3Mk1",		// #38  Heavy Machinegun
"R-Struc-Power-Upgrade03",		// #39  Vapor Turbine Generator Mk2
"R-Struc-Power-Upgrade03a",		// #40  Vapor Turbine Generator Mk3
"R-Struc-Research-Upgrade08",		// #41  Neural Synapse Research Brain Mk2
"R-Struc-Research-Upgrade09",		// #42  Neural Synapse Research Brain Mk3
"R-Sys-Engineering02",		// #43  Improved Engineering
"R-Sys-MobileRepairTurretHvy",		// #44  Heavy Repair Turret
"R-Sys-Autorepair-General",		// #45  Auto-Repair
"R-Wpn-Flamer-Damage02",		// #46  High Temperature Flamer Gel Mk2
"R-Sys-Sensor-Upgrade01",		// #47  Sensor Upgrade
"R-Wpn-Cannon-Damage03",		// #48  HEAT Cannon Shells Mk3
"R-Wpn-Cannon-Damage04",		// #49  APFSDS Cannon Rounds
"R-Wpn-Cannon-ROF01",		// #50  Cannon Autoloader
"R-Wpn-Cannon5",		// #51  Assault Cannon
"R-Wpn-Flamer-Damage03",		// #52  High Temperature Flamer Gel Mk3
"R-Wpn-Flamer-Damage04",		// #53  Superhot Flamer Gel
"R-Wpn-Flame2",		// #54  Heavy Flamer - Inferno
//"R-Sys-ECM-Upgrade01",		// #55  Electronic Countermeasures
"R-Struc-Factory-Upgrade01",		// #56  Automated Manufacturing
"R-Wpn-MG-ROF01",		// #57  Chaingun Upgrade
"R-Wpn-MG-ROF02",		// #58  Rapid Fire Chaingun
"R-Wpn-MG-ROF03",		// #59  Hyper Fire Chaingun Upgrade
"R-Vehicle-Metals02",		// #60  Composite Alloys Mk2
"R-Vehicle-Body11",		// #61  Heavy Body - Python
"R-Sys-Sensor-Upgrade02",		// #62  Sensor Upgrade Mk2
"R-Sys-Sensor-Upgrade03",		// #63  Sensor Upgrade Mk3
//"R-Sys-ECM-Upgrade02",		// #64  Electronic Countermeasures Mk2
"R-Wpn-MG4",		// #65  Assault Gun
"R-Wpn-MG5",		// #66  Twin Assault Gun
"R-Cyborg-Metals03",		// #67  Cyborg Composite Alloys Mk3
"R-Cyborg-Metals04",		// #68  Cyborg Dense Composite Alloys
"R-Vehicle-Metals03",		// #69  Composite Alloys Mk3
"R-Wpn-Cannon4AMk1",		// #70  Hyper Velocity Cannon
"R-Defense-Emplacement-HPVcannon",		// #71  Hyper Velocity Cannon Emplacement
"R-Wpn-Cannon6TwinAslt",		// #72  Twin Assault Cannon
"R-Wpn-Cannon2Mk1",		// #73  Medium Cannon
"R-Cyborg-Hvywpn-Mcannon",		// #74  Super Heavy-Gunner
"R-Cyborg-Hvywpn-HPV",		// #75  Super HPV Cyborg
"R-Cyborg-Hvywpn-Acannon",		// #76  Super Auto-Cannon Cyborg
"R-Wpn-Plasmite-Flamer",		// #77  Plasmite Flamer
"R-Struc-Factory-Upgrade04",		// #78  Robotic Manufacturing
"R-Wpn-Cannon-ROF02",		// #79  Cannon Autoloader Mk2
"R-Wpn-Cannon-ROF03",		// #80  Cannon Autoloader Mk3
"R-Wpn-Cannon-Damage05",		// #81  APFSDS Cannon Rounds Mk2
"R-Wpn-Cannon-Damage06",		// #82  APFSDS Cannon Rounds Mk3
"R-Wpn-Cannon-Damage07",		// #83  HVAPFSDS Cannon Rounds
"R-Wpn-Cannon-ROF04",		// #84  Cannon Rapid Loader
"R-Wpn-Cannon-ROF05",		// #85  Cannon Rapid Loader Mk2
"R-Wpn-Cannon-ROF06",		// #86  Cannon Rapid Loader Mk3
"R-Wpn-Cannon-Accuracy02",		// #87  Cannon Laser Designator
"R-Wpn-MG-Damage04",		// #88  APDSB MG Bullets Mk3
"R-Wpn-Cannon-Damage08",		// #89  HVAPFSDS Cannon Rounds Mk2
"R-Wpn-Cannon-Damage09",		// #90  HVAPFSDS Cannon Rounds Mk3
"R-Wpn-MG-Damage05",		// #91  Tungsten-Tipped MG Bullets
"R-Wpn-MG-Damage06",		// #92  Tungsten-Tipped MG Bullets Mk2
"R-Wpn-MG-Damage07",		// #93  Tungsten-Tipped MG Bullets Mk3
"R-Wpn-MG-Damage08",		// #94  Depleted Uranium MG Bullets
"R-Vehicle-Metals04",		// #95  Dense Composite Alloys
"R-Vehicle-Metals05",		// #96  Dense Composite Alloys Mk2
"R-Vehicle-Metals06",		// #97  Dense Composite Alloys Mk3
"R-Vehicle-Metals07",		// #98  Superdense Composite Alloys
"R-Vehicle-Metals08",		// #99  Superdense Composite Alloys Mk2
"R-Vehicle-Metals09",		// #100  Superdense Composite Alloys Mk3
"R-Vehicle-Armor-Heat01",		// #101  Thermal Armor
"R-Vehicle-Armor-Heat02",		// #102  Thermal Armor Mk2
"R-Vehicle-Armor-Heat03",		// #103  Thermal Armor Mk3
"R-Vehicle-Armor-Heat04",		// #104  High Intensity Thermal Armor
"R-Vehicle-Armor-Heat05",		// #105  High Intensity Thermal Armor Mk2
"R-Vehicle-Armor-Heat06",		// #106  High Intensity Thermal Armor Mk3
"R-Vehicle-Armor-Heat07",		// #107  Vehicle Superdense Thermal Armor
"R-Vehicle-Armor-Heat08",		// #108  Vehicle Superdense Thermal Armor Mk2
"R-Vehicle-Armor-Heat09",		// #109  Vehicle Superdense Thermal Armor Mk3
"R-Cyborg-Metals05",		// #110  Cyborg Dense Composite Alloys Mk2
"R-Cyborg-Metals06",		// #111  Cyborg Dense Composite Alloys Mk3
"R-Cyborg-Metals07",		// #112  Cyborg Superdense Composite Alloys
"R-Cyborg-Metals08",		// #113  Cyborg Superdense Composite Alloys Mk2
"R-Cyborg-Metals09",		// #114  Cyborg Superdense Composite Alloys Mk3
"R-Vehicle-Engine02",		// #115  Fuel Injection Engine Mk2
"R-Vehicle-Engine03",		// #116  Fuel Injection Engine Mk3
"R-Vehicle-Prop-Hover",		// #117  Hover Propulsion
"R-Vehicle-Prop-VTOL",		// #118  VTOL Propulsion
"R-Struc-VTOLFactory",		// #119  VTOL Factory
"R-Struc-VTOLPad",		// #120  VTOL Rearming Pad
"R-Wpn-Bomb01",		// #121  Cluster Bomb Bay
"R-Wpn-Bomb03",		// #122  Phosphor Bomb Bay
"R-Wpn-Flamer-Damage05",		// #123  Superhot Flamer Gel Mk2
"R-Wpn-Bomb04",		// #124  Thermite Bomb Bay
"R-Wpn-AAGun01",		// #125  AA Cyclone Flak Cannon
"R-Defense-WallTower-DoubleAAgun",		// #126  AA Cyclone Flak Cannon Hardpoint
];

const research_cannon = [
"R-Wpn-MG1Mk1",
"R-Sys-Engineering01",
"R-Defense-Tower01",
"R-Wpn-Cannon1Mk1",		// #6  Light Cannon
"R-Struc-PowerModuleMk1",
"R-Vehicle-Prop-Halftracks",    //Полугусенецы
"R-Defense-WallTower02", //защита лёгкая пушка
"R-Struc-Power-Upgrade03a",
"R-Struc-Research-Upgrade09",
"R-Vehicle-Prop-Hover",
"R-Sys-MobileRepairTurretHvy",
"R-Sys-Autorepair-General",             //Автопочинка
//"R-Sys-ECM-Upgrade01",	//Глушилка
"R-Wpn-MG-ROF03",				//Hyper Fire Chaingun Upgrade
"R-Vehicle-Body11",		// #101  Heavy Body - Python
"R-Wpn-MG5",					//Twin Assault Gun
"R-Struc-RepairFacility",
"R-Wpn-MG3Mk1",
"R-Wpn-Cannon4AMk1",			//Hyper Velocity Cannon
//"R-Sys-ECM-Upgrade02",
"R-Cyborg-Metals04",            // #16  Cyborg Dense Composite Alloys
"R-Vehicle-Metals03",           // #11  Composite Alloys Mk3
'R-Defense-Emplacement-HPVcannon',
"R-Wpn-Cannon6TwinAslt",		////Спаренная штурмовая пушка
"R-Cyborg-Hvywpn-Acannon",		// #46  Super Auto-Cannon Cyborg
"R-Wpn-Plasmite-Flamer", // plasmite
"R-Wpn-Cannon-ROF06",			//Cannon Rapid Loader Mk3
"R-Wpn-Cannon-Accuracy02",		//Cannon Laser Designator
"R-Wpn-MG-Damage04",            // #32  APDSB MG Bullets Mk3
"R-Wpn-Cannon-Damage09",		//HVAPFSDS Cannon Rounds Mk3
//"R-Sys-MobileRepairTurretHvy",
'R-Defense-WallTower-HPVcannon',
"R-Wpn-MG5",					//Twin Assault Gun
"R-Wpn-MG-Damage08",			//Depleted Uranium MG Bullets
"R-Vehicle-Metals09",			//Superdense Composite Alloys Mk3 (финал)
"R-Vehicle-Armor-Heat09",		//Vehicle Superdense Thermal Armor Mk3
"R-Cyborg-Metals09",
"R-Struc-VTOLPad",
"R-Wpn-Bomb04",
"R-Defense-WallTower-DoubleAAgun"		// #107  AA Cyclone Flak Cannon Hardpoint
];

//Flamer-cannon-vtols
const research_fire3 = [
"R-Wpn-MG1Mk1",
"R-Defense-Tower01",
"R-Sys-Engineering01",
"R-Vehicle-Prop-Halftracks",	//Полугусенецы
"R-Wpn-MG5",					//Twin Assault Gun
"R-Vehicle-Body08",
"R-Struc-PowerModuleMk1",
"R-Struc-Power-Upgrade03a",
"R-Wpn-Cannon1Mk1",		// #6  Light Cannon
"R-Wpn-Flamer01Mk1",
"R-Struc-Factory-Cyborg",
"R-Struc-RepairFacility",
"R-Struc-Research-Module",
"R-Wpn-MG2Mk1",					//Спаренный лёгкий пулемёт
"R-Wpn-Flamer-ROF03",
"R-Wpn-Plasmite-Flamer", // plasmite
"R-Struc-Research-Upgrade09",
"R-Vehicle-Prop-Halftracks",	//Полугусенецы
"R-Defense-WallTower-QuadRotAA",
"R-Struc-VTOLPad",
"R-Wpn-Bomb04",
"R-Struc-Power-Upgrade03a",
"R-Vehicle-Prop-Hover",
//"R-Sys-ECM-Upgrade02",	//Глушилка
"R-Vehicle-Body05",				//Средняя начальная броня
"R-Cyborg-Metals09",
"R-Wpn-Cannon6TwinAslt",		////Спаренная штурмовая пушка
"R-Sys-MobileRepairTurretHvy",
"R-Vehicle-Body08",
"R-Sys-Autorepair-General",		//Автопочинка
"R-Wpn-Flamer-Damage09",
"R-Defense-MortarPit-Incendiary",
//"R-Wpn-Cannon4AMk1",			//Hyper Velocity Cannon
"R-Wpn-Cannon-ROF01",			//Cannon auto loader
"R-Wpn-Cannon6TwinAslt",		//Twin Assault Cannon
"R-Wpn-Cannon-ROF06",			//Cannon Rapid Loader Mk3
"R-Wpn-Cannon-Accuracy02",		//Cannon Laser Designator
"R-Wpn-Cannon-Damage09",		//HVAPFSDS Cannon Rounds Mk3
"R-Wpn-Mortar-ROF04",
"R-Wpn-Mortar-Damage06",
"R-Wpn-Mortar-Acc03",
"R-Vehicle-Body12",
"R-Defense-AASite-QuadMg1",		//Hurricane AA Site
"R-Wpn-Howitzer-Incendiary",
"R-Wpn-Howitzer-ROF04",
"R-Wpn-HvyHowitzer",
"R-Wpn-Howitzer-Accuracy03",
"R-Defense-WallTower-QuadRotAA",//Whirlwind Hardpoint
"R-Wpn-Howitzer-Damage06",
"R-Wpn-Howitzer03-Rot",
"R-Sys-Sensor-UpLink",

];

research_path = research_cannon;
