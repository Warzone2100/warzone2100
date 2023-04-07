debugMsg('Module: weap-init.js','init');

var guns=[
//	===== Пулемёты
["R-Wpn-MG1Mk1", "MG1Mk1"],						//Лёгкий пулемёт
["R-Wpn-MG2Mk1", "MG2Mk1"],						//Лёгкий спаренный пулемёт
["R-Wpn-MG3Mk1", "MG3Mk1"],						//Тяжёлый пулемёт
["R-Wpn-MG4", "MG4ROTARYMk1"],					//Штурмовой пулемёт
["R-Wpn-MG5", "MG5TWINROTARY"],					//Спаренный штурмовой пулемёт
//	===== Пушки
["R-Wpn-Cannon1Mk1", "Cannon1Mk1"],				//Лёгкая пушка
["R-Wpn-Cannon2Mk1", "Cannon2A-TMk1"],			//Средняя пушка
["R-Wpn-Cannon5", "Cannon5VulcanMk1"],			//Штурмовая пушка
["R-Wpn-Cannon4AMk1", "Cannon4AUTOMk1"],		//Гипер скоростная штурмовая пушка
["R-Wpn-Cannon3Mk1", "Cannon375mmMk1"],			//Тяжёлая пушка
["R-Wpn-Cannon6TwinAslt", "Cannon6TwinAslt"],	//Спаренная штурмовая пушка
["R-Wpn-PlasmaCannon", "Laser4-PlasmaCannon"],	//Плазма-пушка
//	===== Огнемёты
["R-Wpn-Flamer01Mk1", "Flame1Mk1"],
//["R-Wpn-Flame2", "Flame2"],						//Heavy Flamer - Inferno
["R-Wpn-Plasmite-Flamer", "PlasmiteFlamer"],
//	===== Ракеты прямого наведения
["R-Wpn-Rocket05-MiniPod", "Rocket-Pod"],			//Скорострельная лёгкая ракета прямого наведения
["R-Wpn-Rocket03-HvAT", "Rocket-BB"],				//Медленная ракета против строений и танков (в общей армии не очень, только если против укреплений)
["R-Wpn-Rocket01-LtAT", "Rocket-LtA-T"],			//Противотанковая пара ракет прямого наведения "Лансер"
["R-Wpn-Rocket07-Tank-Killer", "Rocket-HvyA-T"],	//Улучшенная противотанковая пара ракет прямого наведения
["R-Wpn-Missile2A-T", "Missile-A-T"],				//Тяжёлая противотанковая пара ракет прямого наведения "scourge"
//	===== Ракеты артиллерии
["R-Wpn-Rocket02-MRL", "Rocket-MRL"],				//Лёгкая артиллерийская ракетная баттарея
["R-Wpn-Rocket02-MRLHvy", "Rocket-MRL-Hvy"],
["R-Wpn-Rocket06-IDF", "Rocket-IDF"],				//Дальнобойная артиллерийская ракетная баттарея Ripple
["R-Wpn-MdArtMissile", "Missile-MdArt"],			//Улучшенная артиллерийская ракетная баттарея Seraph
["R-Wpn-HvArtMissile", "Missile-HvyArt"],			//Улучшенная дальнобойная артиллерийская ракетная баттарея Archangel
//	===== Мортиры
["R-Wpn-Mortar01Lt", "Mortar1Mk1"],					//Mortar
["R-Wpn-Mortar-Incendiary", "Mortar-Incendiary"],	//Incendiary Mortar
["R-Wpn-Mortar02Hvy", "Mortar2Mk1"],				//Heavy Mortar - Bombard
["R-Wpn-Mortar3", "Mortar3ROTARYMk1"],				//Rotary Mortar - Pepperpot
//	===== Гаубицы
["R-Wpn-HowitzerMk1", "Howitzer105Mk1"],				//Howitzer
["R-Wpn-HvyHowitzer", "Howitzer150Mk1"],				//Heavy Howitzer - Ground Shaker
["R-Wpn-Howitzer03-Rot", "Howitzer03-Rot"],				//Rotary Howitzer - Hellstorm
["R-Wpn-Howitzer-Incendiary", "Howitzer-Incendiary"],	//Incendiary Howitzer
//	===== Лазеры
["R-Wpn-HvyLaser", "HeavyLaser"],					//Heavy Laser
["R-Wpn-Laser02", "Laser2PULSEMk1"],				//Pulse Laser
["R-Wpn-Laser01", "Laser3BEAMMk1"],					//Laser - Flashlight
//	===== Rails
["R-Wpn-RailGun01", "RailGun1Mk1"],					//Needle Gun
["R-Wpn-RailGun02", "RailGun2Mk1"],					//Rail Gun
["R-Wpn-RailGun03", "RailGun3Mk1"],					//Gauss Cannon

];


//Типы пушек, по приоритету
var guns_type = {};


//Пулемёты
guns_type['mg'] = [
["R-Wpn-MG5", "MG5TWINROTARY"],					//Спаренный штурмовой пулемёт
["R-Wpn-MG4", "MG4ROTARYMk1"],					//Штурмовой пулемёт
["R-Wpn-MG3Mk1", "MG3Mk1"],						//Тяжёлый пулемёт
["R-Wpn-MG2Mk1", "MG2Mk1"],						//Лёгкий спаренный пулемёт
["R-Wpn-MG1Mk1", "MG1Mk1"],						//Лёгкий пулемёт
];

//Пушки
guns_type['cn'] = [
["R-Wpn-Cannon6TwinAslt", "Cannon6TwinAslt"],	//Спаренная штурмовая пушка
["R-Wpn-Cannon5", "Cannon5VulcanMk1"],			//Штурмовая пушка
["R-Wpn-Cannon1Mk1", "Cannon1Mk1"],				//Лёгкая пушка
["R-Wpn-Cannon4AMk1", "Cannon4AUTOMk1"],		//Гипер скоростная штурмовая пушка
["R-Wpn-Cannon2Mk1", "Cannon2A-TMk1"],			//Средняя пушка
["R-Wpn-Cannon3Mk1", "Cannon375mmMk1"],			//Тяжёлая пушка
["R-Wpn-PlasmaCannon", "Laser4-PlasmaCannon"],	//Плазма-пушка
];

//Огнемёты
guns_type['fl'] = [
["R-Wpn-Plasmite-Flamer", "PlasmiteFlamer"],
["R-Wpn-Flame2", "Flame2"],						//Heavy Flamer - Inferno
["R-Wpn-Flamer01Mk1", "Flame1Mk1"],
];

//Рокеты
guns_type['rk'] = [
["R-Wpn-Missile2A-T", "Missile-A-T"],				//Тяжёлая противотанковая пара ракет прямого наведения "scourge"
["R-Wpn-Rocket07-Tank-Killer", "Rocket-HvyA-T"],	//Улучшенная противотанковая пара ракет прямого наведения
["R-Wpn-Rocket01-LtAT", "Rocket-LtA-T"],			//Противотанковая пара ракет прямого наведения "Лансер"
["R-Wpn-Rocket03-HvAT", "Rocket-BB"],				//Медленная ракета против строений и танков (в общей армии не очень, только если против укреплений)
["R-Wpn-Rocket05-MiniPod", "Rocket-Pod"],			//Скорострельная лёгкая ракета прямого наведения
];

//Рокеты артиллерии
guns_type['ra'] = [
["R-Wpn-HvArtMissile", "Missile-HvyArt"],			//Улучшенная дальнобойная артиллерийская ракетная баттарея Archangel
["R-Wpn-MdArtMissile", "Missile-MdArt"],			//Улучшенная артиллерийская ракетная баттарея Seraph
["R-Wpn-Rocket06-IDF", "Rocket-IDF"],				//Дальнобойная артиллерийская ракетная баттарея Ripple
["R-Wpn-Rocket02-MRLHvy", "Rocket-MRL-Hvy"],
["R-Wpn-Rocket02-MRL", "Rocket-MRL"],				//Лёгкая артиллерийская ракетная баттарея
];

//Мортиры
guns_type['mr'] = [
["R-Wpn-Mortar3", "Mortar3ROTARYMk1"],				//Rotary Mortar - Pepperpot
["R-Wpn-Mortar-Incendiary", "Mortar-Incendiary"],	//Incendiary Mortar
["R-Wpn-Mortar02Hvy", "Mortar2Mk1"],				//Heavy Mortar - Bombard
["R-Wpn-Mortar01Lt", "Mortar1Mk1"],					//Mortar
];

//Гаубицы
guns_type['hw'] = [
["R-Wpn-Howitzer-Incendiary", "Howitzer-Incendiary"],	//Incendiary Howitzer
["R-Wpn-Howitzer03-Rot", "Howitzer03-Rot"],				//Rotary Howitzer - Hellstorm
["R-Wpn-HvyHowitzer", "Howitzer150Mk1"],				//Heavy Howitzer - Ground Shaker
["R-Wpn-HowitzerMk1", "Howitzer105Mk1"],				//Howitzer
];

//Лазеры
guns_type['ls'] = [
["R-Wpn-HvyLaser", "HeavyLaser"],					//Heavy Laser
["R-Wpn-Laser02", "Laser2PULSEMk1"],				//Pulse Laser
["R-Wpn-Laser01", "Laser3BEAMMk1"],					//Laser - Flashlight
];

//Рельсы
guns_type['rl'] = [
["R-Wpn-RailGun01", "RailGun1Mk1"],					//Needle Gun
["R-Wpn-RailGun02", "RailGun2Mk1"],					//Rail Gun
["R-Wpn-RailGun03", "RailGun3Mk1"],					//Gauss Cannon
];

var guns_pts = {};
guns_pts['mg'] = [
'R-Wpn-MG3Mk1',
'R-Wpn-MG4',
'R-Wpn-MG5',
'R-Wpn-MG-Damage01',
'R-Wpn-MG-Damage02',
'R-Wpn-MG-Damage03',
'R-Wpn-MG-Damage04',
'R-Wpn-MG-Damage05',
'R-Wpn-MG-Damage06',
'R-Wpn-MG-Damage07',
'R-Wpn-MG-Damage08',
//'R-Wpn-MG-Damage09',
'R-Wpn-MG-ROF01',
'R-Wpn-MG-ROF02',
'R-Wpn-MG-ROF03'
];

guns_pts['cn'] = [
'R-Wpn-Cannon1Mk1',
'R-Wpn-Cannon5',
'R-Wpn-Cannon6TwinAslt',
'R-Wpn-Cannon-Damage01',
'R-Wpn-Cannon-Damage02',
'R-Wpn-Cannon-Damage03',
'R-Wpn-Cannon-Damage04',
'R-Wpn-Cannon-Damage05',
'R-Wpn-Cannon-Damage06',
'R-Wpn-Cannon-Damage07',
'R-Wpn-Cannon-Damage08',
'R-Wpn-Cannon-Damage09',
'R-Wpn-Cannon-Accuracy01',
'R-Wpn-Cannon-Accuracy02',
'R-Wpn-Cannon-ROF01',
'R-Wpn-Cannon-ROF02',
'R-Wpn-Cannon-ROF03',
'R-Wpn-Cannon-ROF04',
'R-Wpn-Cannon-ROF05',
'R-Wpn-Cannon-ROF06'
];

guns_pts['fl'] = [
'R-Wpn-Flame2',
'R-Wpn-Plasmite-Flamer',
'R-Wpn-Flamer-Damage01',
'R-Wpn-Flamer-Damage02',
'R-Wpn-Flamer-Damage03',
'R-Wpn-Flamer-Damage04',
'R-Wpn-Flamer-Damage05',
'R-Wpn-Flamer-Damage06',
'R-Wpn-Flamer-Damage07',
'R-Wpn-Flamer-Damage08',
'R-Wpn-Flamer-Damage09',
'R-Wpn-Flamer-ROF01',
'R-Wpn-Flamer-ROF02',
'R-Wpn-Flamer-ROF03'
];

guns_pts['rk'] = [
'R-Wpn-Rocket-Damage01',
'R-Wpn-Rocket-Damage02',
'R-Wpn-Rocket-Damage03',
'R-Wpn-Rocket-Damage04',
'R-Wpn-Rocket-Damage05',
'R-Wpn-Rocket-Damage06',
'R-Wpn-Rocket-Damage07',
'R-Wpn-Rocket-Damage08',
'R-Wpn-Rocket-Damage09',
'R-Wpn-Rocket-ROF01',
'R-Wpn-Rocket-ROF02',
'R-Wpn-Rocket-ROF03',
//'R-Wpn-Rocket-ROF04',
//'R-Wpn-Rocket-ROF05',
//'R-Wpn-Rocket-ROF06',
'R-Wpn-Rocket-Accuracy01',
'R-Wpn-Rocket-Accuracy02',
'R-Wpn-RocketSlow-Accuracy01',
'R-Wpn-RocketSlow-Accuracy02'
];

guns_pts['ra'] = guns_pts['rk'];

guns_pts['mr'] = [
'R-Wpn-Mortar-Damage01',
'R-Wpn-Mortar-Damage02',
'R-Wpn-Mortar-Damage03',
'R-Wpn-Mortar-Damage04',
'R-Wpn-Mortar-Damage05',
'R-Wpn-Mortar-Damage06',
'R-Wpn-Mortar-ROF01',
'R-Wpn-Mortar-ROF02',
'R-Wpn-Mortar-ROF03',
'R-Wpn-Mortar-ROF04',
'R-Wpn-Mortar-Acc01',
'R-Wpn-Mortar-Acc02',
'R-Wpn-Mortar-Acc03'
];

guns_pts['hw'] = [
'R-Wpn-Howitzer-Damage01',
'R-Wpn-Howitzer-Damage02',
'R-Wpn-Howitzer-Damage03',
'R-Wpn-Howitzer-Damage04',
'R-Wpn-Howitzer-Damage05',
'R-Wpn-Howitzer-Damage06',
'R-Wpn-Howitzer-Accuracy01',
'R-Wpn-Howitzer-Accuracy02',
'R-Wpn-Howitzer-Accuracy03',
'R-Wpn-Howitzer-ROF01',
'R-Wpn-Howitzer-ROF02',
'R-Wpn-Howitzer-ROF03',
'R-Wpn-Howitzer-ROF04'
];

guns_pts['ls'] = guns_pts['fl'];

guns_pts['rl'] = [
'R-Wpn-Rail-Damage01',
'R-Wpn-Rail-Damage02',
'R-Wpn-Rail-Damage03',
'R-Wpn-Rail-Accuracy01',
'R-Wpn-Rail-ROF01',
'R-Wpn-Rail-ROF02',
'R-Wpn-Rail-ROF03'
];
