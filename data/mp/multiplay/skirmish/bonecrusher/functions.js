// debugMsg('Module: functions.js','init');

//Функция проверяет объекты и возвращает значение
//Задача стоит в обработке тяжёлых данных, и работа данной функции
//в цикле, функция должна будет отработать один раз, и прокешировать
//результат, что бы быстро отдавать циклам нужную информация, не
//забивая процессор всякой повторяющейся хернёй
//Параметры:
//x,y - координаты
//command - что-то конкретное ищем, задание для функции
//range - передаём, если что-то нужно искать в радиусе (примичание, радиус так же кешируется, следующий запрос по таким же координатам и коммандой дадут результат с прошлого радиуса, пока не пройдёт время updateIn)
//time - сколько секунд хранить информацию
//obj - передаём, если нужно сравнить что-то с объектом(например propulsion is reach для droid)
//cheat - true(читерим, видя через туман войны), null/false/undefined(не читерим, возвращем только то, что можем видеть)
//inc - true(прибавляем value+1 и возвращаем), null/false/undefined(просто возвращаем value)

var _globalInfoNear = [];
function getInfoNear(x,y,command,range,time,obj,cheat,inc){
	/*
	if (command === 'buildRig') {
		debugMsg('DEBUG: '+x+','+y+','+command+','+time+','+inc, 'temp');
		if (typeof _globalInfoNear[x+'_'+y+'_'+command] !== "undefined") {
			debugMsg((gameTime-_globalInfoNear[x+'_'+y+'_'+command].setTime)+' left', 'temp');
		}
	}*/

	debugMsg(x+'x'+y+' '+command, 'gi');

	if (typeof _globalInfoNear[x+'_'+y+'_'+command] !== "undefined"
	&& gameTime < (_globalInfoNear[x+'_'+y+'_'+command].setTime + _globalInfoNear[x+'_'+y+'_'+command].updateIn)) {
		if (inc) {
			_globalInfoNear[x+'_'+y+'_'+command].value++;
		}
		return _globalInfoNear[x+'_'+y+'_'+command];
	} else {
		if (typeof time === "undefined") time = 30000;
		if (typeof range === "undefined") range = 7;
		if (typeof cheat === "undefined") var view = me;
		else if (cheat) var view = -1;
//		_globalInfoNear[x+'_'+y+'_'+command] = [];
		_globalInfoNear[x+'_'+y+'_'+command] = { setTime: gameTime, updateIn: time };

		if (command === 'safe') {
			var danger = [];
			for (let e = 0; e < maxPlayers; ++e) {
				if (allianceExistsBetween(me,e)) continue;
				danger = danger.concat(enumDroid(e, DROID_WEAPON, view));
				danger = danger.concat(enumDroid(e, DROID_CYBORG, view));
				danger = danger.concat(enumStruct(e, DEFENSE, view));
			}
			if (scavengerPlayer !== -1) {
				danger = danger.concat(enumDroid(scavengerPlayer, DROID_WEAPON, view));
				danger = danger.concat(enumDroid(scavengerPlayer, DROID_CYBORG, view));
				danger = danger.concat(enumStruct(scavengerPlayer, DEFENSE, view));
			}

			for (const d in danger) {
				if (distBetweenTwoPoints_p(x,y,danger[d].x,danger[d].y) < range) {
					_globalInfoNear[x+'_'+y+'_'+command].value = false;
					return _globalInfoNear[x+'_'+y+'_'+command];
				}
			}
			_globalInfoNear[x+'_'+y+'_'+command].value = true;
			return _globalInfoNear[x+'_'+y+'_'+command];
		} else if (command === 'defended') {

			var defenses = enumStruct(me, DEFENSE).filter((e) => (e.status === BUILT));
			for (const d in defenses) {
				if (distBetweenTwoPoints_p(x,y,defenses[d].x,defenses[d].y) < range) {
					_globalInfoNear[x+'_'+y+'_'+command].value = true;
					return _globalInfoNear[x+'_'+y+'_'+command];
				}
			}
			_globalInfoNear[x+'_'+y+'_'+command].value = false;
			return _globalInfoNear[x+'_'+y+'_'+command];

		} else if (command === 'buildDef') {

			var _builder = enumGroup(buildersHunters);
			if (_builder.length === 0) _builder = enumDroid(me,DROID_CONSTRUCT);
			if (_builder.length === 0) { //Невозможно в данный момент проверить, запоминаем на 10 секунд
				_globalInfoNear[x+'_'+y+'_'+command].updateIn = 10000;
				_globalInfoNear[x+'_'+y+'_'+command].value = false;
				return _globalInfoNear[x+'_'+y+'_'+command];
			}
			var toBuild = defence[Math.floor(Math.random()*defence.length)];
			var pos = pickStructLocation(_builder[0],toBuild,x,y);
			if (pos && distBetweenTwoPoints_p(x,y,pos.x,pos.y) < range) {
				debugMsg('gi: try: '+x+'x'+y+', build: '+pos.x+'x'+pos.y+' - true', 'defence');
				_globalInfoNear[x+'_'+y+'_'+command].value = true;
				_globalInfoNear[x+'_'+y+'_'+command].x = pos.x;
				_globalInfoNear[x+'_'+y+'_'+command].y = pos.y;
			} else {
				debugMsg('gi: try: '+x+'x'+y+' - false', 'defence');
				_globalInfoNear[x+'_'+y+'_'+command].value = false;
			}
			return _globalInfoNear[x+'_'+y+'_'+command];
		} else if (command === 'actionTry') {
			if (typeof _globalInfoNear[x+'_'+y+'_'+command].value === "undefined") {
				_globalInfoNear[x+'_'+y+'_'+command].value = 0;
//				debugMsg('getInfoNear set 0','temp');

			}
			return _globalInfoNear[x+'_'+y+'_'+command];
		}
	}
}

//Проверяем есть ли союзники, не спектаторы ли они
//Выбираем одного для поддержки 3.2+
function checkAlly(){
//	return;
	debugMsg('me='+me, 'ally');
	playerData.forEach((data, player) => {
		var dist = distBetweenTwoPoints_p(base.x,base.y,startPositions[player].x,startPositions[player].y);
		var ally = allianceExistsBetween(me, player);

		if (playerSpectator(player)) {
			debugMsg("#"+player+" name="+data.name+", human="+data.isHuman+", ai="+data.isAI
			+", ally=spectator, dist="+dist, "ally");
			return;
		}
		if (playerLoose(player)) {
			debugMsg("#"+player+" name=\"Empty Slot\""
			+", ally=empty, dist="+dist, "ally");
			return;
		}
		if (player === me) {
			debugMsg("#"+player+" name="+vername+", ai=ofcource"
			+", ally=me, dist=0", "ally");
			return;
		}
		debugMsg("#"+player+" name="+data.name+", human="+data.isHuman+", ai="+data.isAI
		+", ally="+ally+", dist="+dist, "ally");

		//Если враг
		if (!ally) {
			if (Math.floor(dist/2) < base_range) {
				debugMsg('base_range снижено: '+base_range+'->'+Math.floor(dist/2), 'init');
				base_range = Math.floor(dist/2);
			}
			if (!enemyDist || enemyDist > dist) enemyDist = dist;
		}

	});
//	playerSpectator();
}

function checkDonate(obj){
	if (!getInfoNear(base.x,base.y,'safe',base_range).value) return false;
	if (groupSize(armyPartisans) < (minPartisans/2)) return false;
//	if ()
//	debugMsg(obj.name+" to "+armyToPlayer, 'donate');
	if (obj.droidType === DROID_WEAPON && armyToPlayer !== false) {
//		debugMsg(obj.name+" -> "+armyToPlayer, 'donate');
		donateObject(obj, armyToPlayer);
		return true;
	}
	return false;
}

function groupArmy(droid, type){

	if (typeof type === "undefined") type = false;

	if (type === 'jammer') {
//		debugMsg("armyJammers +1", 'group');
		groupAdd(armyJammers, droid);
		return;
	}

	if (droid.droidType === DROID_REPAIR) {
//		debugMsg("armyFixers +1", 'group');
		groupAdd(armyFixers, droid);
		return;
	}

	//Забираем киборгов под общее коммандование
	if (droid.droidType === DROID_CYBORG && se_r >= army_rich && (rage === HARD || rage === INSANE)) {
//		debugMsg("armyRegular +1", 'group');
		groupAdd(armyRegular, droid);
		return;
	}

	//Если армия партизан меньше 7 -ИЛИ- нет среднего Body -ИЛИ- основная армия достигла лимитов
//	if (groupSize(armyPartisans) < 7 || !getResearch("R-Vehicle-Body05").done || groupSize(armyRegular) >= maxRegular) {
	if (groupSize(armyPartisans) <= minPartisans || groupSize(armyRegular) >= maxRegular) {
//		debugMsg("armyPartisans +1", 'group');
		groupAdd(armyPartisans, droid);
	} else {

		if (droid.droidType === DROID_CYBORG || groupSize(armyCyborgs) === 0) {
//			debugMsg("armyCyborgs +1", 'group');
			groupAdd(armyCyborgs, droid);
			return;
		}

//		debugMsg("armyRegular +1", 'group');
		groupAdd(armyRegular, droid);
	}

	//Перегрупировка
	if (groupSize(armyPartisans) < minPartisans && groupSize(armyRegular) > 1 && !(se_r >= army_rich && (rage === HARD || rage === INSANE))) {
		var regroup = enumGroup(armyRegular);
		regroup.forEach((e) => {
//			debugMsg("armyRegular --> armyPartisans +1", 'group');
			groupAdd(armyPartisans, e);
		});
	}

}


function stats(){
	if (!running)return;
//	if (release) return;

	var _rigs = enumStruct(me, "A0ResourceExtractor").filter((e) => (e.status === BUILT)).length;
	var _gens = enumStruct(me, "A0PowerGenerator").filter((e) => (e.status === BUILT)).length*4;
	if (_gens > _rigs) _gens = _rigs;

	debugMsg("---===---", 'stats');
	debugMsg("Power: "+playerPower(me)+"; rigs="+_gens+"/"+_rigs+"; free="+enumFeature(me, "OilResource").length+"/"+allResources.length+"; unknown="+getUnknownResources().length+"; enemy="+getEnemyResources().length, 'stats');
	debugMsg("Hold: me="+Math.round(_rigs*100/allResources.length)+"%; enemy~="+Math.round((getEnemyResources().length+getUnknownResources().length)*100/allResources.length)+"%", 'stats');
	debugMsg("Army: "+(groupSize(armyPartisans)+groupSize(armyRegular)+groupSize(armyCyborgs)+groupSize(VTOLAttacker))+"; Partisans="+groupSize(armyPartisans)+"; Regular="+groupSize(armyRegular)+"; Borgs="+groupSize(armyCyborgs)+"; VTOL="+groupSize(VTOLAttacker), 'stats');
//	debugMsg("Units: Builders="+groupSize(buildersMain)+"; Hunters="+groupSize(buildersHunters)+"; Repair="+groupSize(armyFixers)+"; Jammers="+groupSize(armyJammers)+"; targets="+builder_targets.length, 'stats');
	debugMsg("Research: avail="+avail_research.length+"; Ways="+research_way.length, 'stats');
	debugMsg("Weapons: "+guns.length+"; known="+avail_guns.length+"; cyborgs="+avail_cyborgs.length+"; vtol="+avail_vtols.length, 'stats');
	debugMsg("Base: "+base.x+"x"+base.y+", r="+base_range+", safe="+getInfoNear(base.x,base.y,'safe',base_range).value+"; defense="+enumStruct(me, DEFENSE).length+"; labs="+enumStruct(me, RESEARCH_LAB).length+"; factory="+enumStruct(me, FACTORY).length+"; cyb_factory="+enumStruct(me, CYBORG_FACTORY).length+"; vtol="+enumStruct(me, VTOL_FACTORY).length+", full="+isFullBase(me), 'stats');
	debugMsg("Bodies: light="+light_bodies.length+"; medium="+medium_bodies.length+"; heavy="+heavy_bodies.length, 'stats');
	debugMsg("Misc: enemyDist="+enemyDist+"; barrels="+enumFeature(ALL_PLAYERS, "").filter((e) => (e.player === 99)).length
		+"; known defence="+defence.length+"; known AA="+AA_defence.length+"; AA_queue="+AA_queue.length, 'stats');
	debugMsg("Ally="+ally.length+"; enemy="+enemy.length, 'stats');
	debugMsg("Produce: "+produceTrigger.length, 'stats');
	debugMsg('defQueue: '+defQueue.length, 'stats');

	debugMsg("_globalInfoNear level1 = "+Object.keys(_globalInfoNear).length, 'stats');
}

//Функция определяет подвергается ли ремонту наша цель
//Если да, возвращяем объект, кто ремонтирует нашу цель
function isBeingRepaired(who){
	if (typeof who === "undefined") {
//		debugMsg("Атакующий неизвестен",5);
		return false;
	}

	if (allianceExistsBetween(me, who.player)) {
//		debugMsg("Атакующий мой союзник, прощаю",5);
		return false;
	}

	switch (who.type) {
		case DROID: {
//			debugMsg("Нас атаковал вражеский дроид ["+who.player+"]",5);
//TODO
//			Тут нужно доработать
			return false;

		}
		case STRUCTURE: {
//			debugMsg("Нас атакует вражесая башня ["+who.player+"]",5);
			var droids = enumDroid(who.player,DROID_CONSTRUCT,me);
			if (droids.length > 0) {
				for (const i in droids) {
					if (distBetweenTwoPoints_p(who.x,who.y,droids[i].x,droids[i].y) <= 3) {
//						debugMsg("Атакующая меня башня подвергается ремонту!",5);
						return droids[i];
					}
				}
			}
			return false;

		}
		default: {
//			debugMsg("Нууу, не знаю, нас атакует Ктулху! ["+who.player+"]",5);
			return false;

		}
	}

}



//Функция сортирует массив по дистанции от заданного массива
//передаются параметры:
//arr - сортируемый массив
//obj - игровой объект в отношении чего сортируем массив
//num - кол-во возвращённых ближайших объектов
//reach - если задано, и если obj не может добраться на своём propulsion до arr[n], то из массива arr будут изъяты эти результаты
//если num не передан, возвращает полный сортированный массив
//если num равен 1, то всё равно возвращается массив но с единственным объектом
function sortByDistance(arr, obj, num, reach){
	if (typeof reach === "undefined") reach = false;
	if (typeof num === "undefined" || num === null || num === false) num = 0;
	else if (arr.length === 1) num = 1;

	if (num === 1) {

		var b = Infinity;
		var c = [];
		for (const i in arr) {
			if (reach)if (!droidCanReach_p(obj, arr[i].x, arr[i].y))continue;
//			if (reach)if (!droidCanReach_p(arr[i], obj.x, obj.y))continue;
			var a = distBetweenTwoPoints_p(obj.x, obj.y, arr[i].x, arr[i].y);
			if (a < b) {
				b = a;
				c[0] = arr[i];
			}
		}
		return c;
	}

	if (num !== 1) {
		arr.sort((a, b) => (
			distBetweenTwoPoints_p(obj.x, obj.y, a.x, a.y) - distBetweenTwoPoints_p(obj.x, obj.y, b.x, b.y)
		));
	}

	if (reach) {arr = arr.filter((e) => {
		if (droidCanReach_p(obj,e.x,e.y)) return true;
//		if (droidCanReach_p(e,obj.x,obj.y)) return true;
		return false;
	});}

	if (num === 0) return arr;

	if (num >= arr.length) num = (arr.length-1);

	return arr.slice(0,num);

}

//Сортировка объектов по проценту жизней
function sortByHealth(arr){
	if (arr.length > 1) arr.sort((a, b) => (a.health - b.health));
	return arr;
}

function checkProcess(){
	if (!running)return;
	if (Math.floor(gameTime / 1000) < 300) return;

	if (playerLoose(me)) {
		gameStop("loose");
	}

	for (const plally in bc_ally) {
		if (bc_ally[plally] === me) continue;

		if (getInfoNear(base.x,base.y,'safe',base_range).value) {

			if (playerPower(bc_ally[plally]) < 100 && playerPower(me) > 100) {
				var _pow = Math.floor(playerPower(me)/2);
				debugMsg("Send "+_pow+" power to "+bc_ally[plally], 'ally');
				donatePower(_pow, bc_ally[plally]);
			}

			if (enumDroid(bc_ally[plally], DROID_CONSTRUCT).length < 3) {
				if (groupSize(buildersMain) >= 2) {
					var truck = enumGroup(buildersMain);
					if (truck.length > 0) {
						debugMsg("Send builder["+truck[0].id+"] from "+me+" to "+bc_ally[plally], 'ally');
						donateObject(truck[0], bc_ally[plally]);
						break;
					} else { debugMsg("No more builders from "+me+" to "+bc_ally[plally], 'ally');}
				}

				if (groupSize(buildersHunters) > 1) {
					var truck = enumGroup(buildersHunters);
					if (truck.length > 0) {
						donateObject(truck[0], bc_ally[plally]);
						debugMsg("Send hunter["+truck[0].id+"] from "+me+" to "+bc_ally[plally], 'ally');
						break;
					} else { debugMsg("No more hunters from "+me+" to "+bc_ally[plally], 'ally');}
				}
			}
		}
	}
}

function gameStop(condition){


	if (condition === 'loose') {
		debugMsg("I guess, i'm loose.. Give up", 'end');
		if (running) {
			playerData.forEach((data, player) => {
				if (!asPlayer)chat(player, ' from '+debugName+': '+chatting('loose'));
			});
		}

	} else if (condition === 'kick') {
		debugMsg("KICKED", 'end');
		var _ally=false;
		playerData.forEach((data, player) => {
			if (player === me) return;
			if (playerLoose(player)) return;
			if (playerSpectator(player)) return;
			if (!allianceExistsBetween(me,player)) return;
			if (allianceExistsBetween(me,player)) {
				if (playerPower(me) > 100) donatePower(playerPower(me), player);
				_ally = player;
			}
		});
/*
	Not working, due WZ Engine
		var myDroids = enumDroid(me);
		if (myDroids.length > 0 && _ally !== false) {
			myDroids.forEach((o) => {
				donateObject(o, _ally);
			});
		}
*/
		if (running) {
			playerData.forEach((data, player) => {
				if (!asPlayer)chat(player, ' from '+debugName+': '+chatting('kick'));
			});
		}
	}
		running = false;
}

function playerLoose(player){
	var loose = false;
	if (enumStruct(player,"A0LightFactory").length === 0
		&& enumDroid(player, DROID_CONSTRUCT).length === 0
		&& enumStruct(player,"A0CyborgFactory").length === 0
		&& enumDroid(player, 10).length === 0) loose = true;
	return loose;
}

function playerSpectator(player){
	var loose = false;
	if ((enumStruct(player, "A0Sat-linkCentre").length === 1 || enumStruct(player, "A0CommandCentre").length === 1)
		&& enumStruct(player,"A0LightFactory").length === 0
		&& enumStruct(player,"A0CyborgFactory").length === 0
		&& enumDroid(player, 10).length === 0) loose = true;
	return loose;
}

//функция отфильтровывает объекты, которые находяться близко
//к "живым" союзникам, полезно для отказа от захвата ресурсов союзника
function filterNearAlly(obj){
	for (let p = 0; p < maxPlayers; ++p) {
		if (p === me) continue; //Выкидываем себя
		if (!allianceExistsBetween(me,p)) continue; //Выкидываем вражеские
//		if (playerLoose(p)) continue; //Пропускаем проигравших
		if (playerSpectator(p)) continue;

		//Если союзнику доступно 40 вышек, не уступаем ему свободную.
		if (policy['build'] === 'rich' && rage > 1) {
			var allyRes = enumFeature(ALL_PLAYERS, "OilResource");
			allyRes = allyRes.filter((e) => (distBetweenTwoPoints_p(startPositions[p].x,startPositions[p].y,e.x,e.y) < base_range));
			allyRes = allyRes.concat(enumStruct(p, "A0ResourceExtractor").filter((e) => (distBetweenTwoPoints_p(startPositions[p].x,startPositions[p].y,e.x,e.y) < base_range)));
			if (allyRes.length >= 40) continue;
		}

		if (distBetweenTwoPoints_p(base.x,base.y,startPositions[p].x,startPositions[p].y) < base_range) continue; //Если союзник внутри радиуса нашей базы, вышки забираем
		obj = obj.filter((e) => (distBetweenTwoPoints_p(e.x,e.y,startPositions[p].x,startPositions[p].y) >= base_range));
	}
	return obj;
}

//Функция отфильтровывает объекты внутри радиуса базы
function filterNearBase(obj){
	return obj.filter((e) => (distBetweenTwoPoints_p(base.x,base.y, e.x, e.y) > base_range));
}

//Функция отфильтровывает объекты за пределами радиуса базы
function filterOutBase(obj){
	return obj.filter((e) => (distBetweenTwoPoints_p(base.x,base.y, e.x, e.y) < base_range));
}

//функция отфильтровывает недостежимые объекты
function filterInaccessible(obj){
	return obj.filter((e) => (
		//Если попыток постройки больше 3, отфильтровываем их
		getInfoNear(e.x,e.y,'actionTry',0,300000,false,false,false).value <= 3
	));
}

function getEnemyNearAlly(){
//	return []; // <-- disable this funtion
	var targ = [];
	var enemy = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		targ = targ.concat(enumDroid(e, DROID_WEAPON, me));
	}
	if (scavengers !== NO_SCAVENGERS) {
		targ = targ.concat(enumDroid(scavengerPlayer, DROID_WEAPON, me));
	}

	for (let p = 0; p < maxPlayers; ++p) {
		if (p === me) continue;
		if (!allianceExistsBetween(me,p)) continue;
//		if (playerLoose(p)) continue; //Пропускаем проигравших
		if (playerSpectator(p)) continue;
//		enemy = enemy.concat(targ.filter((e) => {if (distBetweenTwoPoints_p(e.x,e.y,startPositions[p].x,startPositions[p].y) < (base_range/2)) {
		enemy = enemy.concat(targ.filter((e) => {if (distBetweenTwoPoints_p(e.x,e.y,startPositions[p].x,startPositions[p].y) < base_range) {
//			debugMsg("TRUE name="+e.name+"; id="+e.id+"; dist="+distBetweenTwoPoints_p(e.x,e.y,startPositions[p].x,startPositions[p].y)+"<"+(base_range/2)+"; player="+p,'temp');
			return true;
		}
//		debugMsg("FALSE name="+e.name+"; id="+e.id+"; dist="+distBetweenTwoPoints_p(e.x,e.y,startPositions[p].x,startPositions[p].y)+"<"+(base_range/2)+"; player="+p, 'temp');
		return false;}));
	}

	return enemy;
}

function getEnemyNearPos(x,y,r){
	if (typeof r === "undefined") r = 7;
	var targ = [];
	var enemy = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		if (playerSpectator(e)) continue;
		if (e === me) continue;
		targ = targ.concat(enumDroid(e, DROID_WEAPON, me));
	}
	if (scavengers !== NO_SCAVENGERS) {
		targ = targ.concat(enumDroid(scavengerPlayer, DROID_WEAPON, me));
	}

	enemy = enemy.concat(targ.filter((e) => (distBetweenTwoPoints_p(e.x,e.y,x,y) < r)));

	return enemy;

}

function getAllyArmy(){
	var army = [];

	for (let p = 0; p < maxPlayers; ++p) {
		if (p === me) continue;
		if (!allianceExistsBetween(me,p)) continue;
		if (playerSpectator(p)) continue;

		army = army.concat(enumDroid(p, DROID_WEAPON));

	}

	return army;
}

//Функция возвращает все свободные нефтеточки на карте.
function getFreeResources(){
	return enumFeature(ALL_PLAYERS, "OilResource");
}

//Функция возвращает все свободные и занятые нефтеточки на карте.
function getAllResources(){
	var resources = getFreeResources();
	for (let e = 0; e < maxPlayers; ++e) resources = resources.concat(enumStruct(e,RESOURCE_EXTRACTOR));
	if (scavengers !== NO_SCAVENGERS) {
		resources = resources.concat(enumStruct(scavengerPlayer, "A0ResourceExtractor"));
	}
	return resources;
}




//Функция возвращяет вышки, о которых в данный момент не известно ничего
//Просто сравниваем два массива объектов и фильтруем в третий
function getUnknownResources(){
	var iSee = getSeeResources();
	if (iSee.length === 0) return allResources;
	var notSee = allResources.filter((value) => {
		for (const i in iSee) {
			if (value.x === iSee[i].x && value.y === iSee[i].y) return false;
		}
		return true;
	});
	return notSee;
}


//Функция возвращает все видимые ресурсы, свободные, свои и занятые кем либо
function getSeeResources(){
	var iSee = [];
	iSee = iSee.concat(enumFeature(me, "OilResource"));
	for (let e = 0; e < maxPlayers; ++e) {
//		if (!allianceExistsBetween(me,e)) continue; //Выкидываем вражеские
		iSee = iSee.concat(enumStruct(e, RESOURCE_EXTRACTOR, me));
	}

	return iSee;

}

function getProblemBuildings(){
	var targ=[];
	for (let p = 0; p < maxPlayers; ++p) {
		if (!allianceExistsBetween(me,p)) continue; //Выкидываем вражеские
		if (playerSpectator(p)) continue; //Пропускаем неиграющих
		targ = targ.concat(enumStruct(p).filter((e) => (e.status === BEING_BUILT || e.health < 99)));
	}
	return targ;
}

function getEnemyFactories(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		targ = targ.concat(enumStruct(e, FACTORY, me));
		targ = targ.concat(enumStruct(e, CYBORG_FACTORY, me));
	}
	if (scavengers !== NO_SCAVENGERS) {
		targ = targ.concat(enumStruct(scavengerPlayer, FACTORY, me));
		targ = targ.concat(enumStruct(scavengerPlayer, CYBORG_FACTORY, me));
	}
	return targ;
}

function getEnemyFactoriesVTOL(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		targ = targ.concat(enumStruct(e, VTOL_FACTORY, me));
	}
	return targ;
}

function getEnemyPads(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		targ = targ.concat(enumStruct(e, REARM_PAD, me));
	}
	return targ;
}


function getEnemyNearBase(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		targ = targ.concat(enumDroid(e, DROID_ANY, me));
		targ = targ.concat(enumStruct(e, DEFENSE, me));
	}
	if (scavengers !== NO_SCAVENGERS) {
		targ = targ.concat(enumStruct(scavengerPlayer, DROID_ANY, me));
		targ = targ.concat(enumStruct(scavengerPlayer, DEFENSE, me));
	}
	return targ.filter((e) => (distBetweenTwoPoints_p(e.x,e.y,base.x,base.y) < base_range && !isFixVTOL(e)));
}
function getEnemyCloseBase(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		targ = targ.concat(enumDroid(e, DROID_ANY, me));
	}
	if (scavengers !== NO_SCAVENGERS) {
		targ = targ.concat(enumStruct(scavengerPlayer, DROID_ANY, me));
	}
	return targ.filter((e) => (distBetweenTwoPoints_p(e.x,e.y,base.x,base.y) < (base_range/2) && !isFixVTOL(e)));
}

function getOurDefences(){
	var targ = [];
	for (const a in ally) {
		targ = targ.concat(enumStruct(a, DEFENSE).filter((e) => (e.status === BUILT)));
	}
	targ = targ.concat(enumStruct(me, DEFENSE).filter((e) => (e.status === BUILT)));
	return targ;
}

function getNearFreeResources(pos){
	return enumFeature(ALL_PLAYERS, "OilResource").filter((e) => (distBetweenTwoPoints_p(pos.x,pos.y,e.x,e.y) < base_range));
}

function getNumEnemies(){
	var enemies = 0;
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		if (playerSpectator(e)) continue;
		if (playerLoose(e)) continue;
		if (e === me) continue;
		enemies++;
	}
	return enemies;
}

function isHumanOverride(){
	if (playerData[me].isHuman) return true;
	return false;
}


function isHumanAlly(){
	for (let e = 0; e < maxPlayers; ++e) {
		if (playerData[e].isHuman && allianceExistsBetween(me, e)) return true;
	}
	return false;
}

//Возвращает булево, если база игрока имеет:
//10 Генераторов с модулями
// 5 Лаб с модулями
// 5 Заводов с двумя модулями
// 5 Киборг заводов
// остальные знадние не важны, так же как и не учитывается 5 авиазаводов
// Для учёта авиа заводов, использовать isCompleteBase(player);
function isFullBase(player){
	if (fullBaseTrigger > gameTime) {
//		debugMsg('fast return: '+fullBase, 'base');
		return fullBase;
	}
	fullBaseTrigger = gameTime + fullBaseTimer;
//	debugMsg("checkFullBase", 'base');
	var obj = enumStruct(player, POWER_GEN, me);
	if (obj.length < 10) {fullBase = false; return false;}
//	debugMsg("powergen check", 'base');
	obj = obj.filter((o) => (o.status === BUILT && o.modules === 1));
	if (obj.length < 10) {fullBase = false; return false;}
//	debugMsg("powergen double check", 'base');
	obj = enumStruct(player, RESEARCH_LAB, me);
	if (obj.length < 5) {fullBase = false; return false;}
//	debugMsg("labs check", 'base');
	obj = obj.filter((o) => (o.status === BUILT && o.modules === 1));
	if (obj.length < 5) {fullBase = false; return false;}
//	debugMsg("labs double check", 'base');
	obj = enumStruct(player, CYBORG_FACTORY, me);
	if (obj.length < 5) {fullBase = false; return false;}
//	debugMsg("cyb check", 'base');
	obj = obj.filter((o) => (o.status === BUILT));
	if (obj.length < 5) {fullBase = false; return false;}
//	debugMsg("cyb double check", 'base');
	obj = enumStruct(player, FACTORY, me);
	if (obj.length < 5) {fullBase = false; return false;}
//	debugMsg("fact check", 'base');
	obj = obj.filter((o) => (o.status === BUILT && o.modules === 2));
	if (obj.length < 5) {fullBase = false; return false;}
//	debugMsg("FULLBASE check", 'base');
	fullBase = true;
	return true;
}

function mark(x,y){
	removeBeacon(0);
	addBeacon(x, y, 0);
	hackMarkTiles();
	debugMsg(x+'x'+y, 'mark');
}

function getEnemyStartPos(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		if (playerSpectator(e)) continue;
		if (playerLoose(e)) continue;
		targ = targ.concat(startPositions[e]);
	}
	return targ;
}

function getEnemyBuilders(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		targ = targ.concat(enumDroid(e, DROID_CONSTRUCT, me));
//		targ = targ.concat(enumDroid(e, 10, me)); // Киборг-строитель
	}
//	return targ.filter((e) => (distBetweenTwoPoints_p(e.x,e.y,base.x,base.y) < base_range));
	return targ;
}

function getEnemyWarriors(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		targ = targ.concat(enumDroid(e, DROID_WEAPON, me));
		targ = targ.concat(enumDroid(e, DROID_CYBORG, me));
	}
	return targ;
}

function getEnemyDefences(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		targ = targ.concat(enumStruct(e, DEFENSE, me));
	}
	if (scavengers !== NO_SCAVENGERS) {
		targ = targ.concat(enumStruct(scavengerPlayer, DEFENSE, me));
		targ = targ.concat(enumStruct(scavengerPlayer, WALL, me));
	}
	return targ;
}

function getEnemyStructures(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (e === me) continue;
		if (allianceExistsBetween(me,e)) continue;
		if (playerSpectator(e)) continue;
		targ = targ.concat(enumStruct(e, DEFENSE, me));
		targ = targ.concat(enumStruct(e, RESOURCE_EXTRACTOR, me));
		targ = targ.concat(enumStruct(e, FACTORY, me));
		targ = targ.concat(enumStruct(e, CYBORG_FACTORY, me));
		targ = targ.concat(enumStruct(e, HQ, me));
		targ = targ.concat(enumStruct(e, LASSAT, me));
		targ = targ.concat(enumStruct(e, POWER_GEN, me));
		targ = targ.concat(enumStruct(e, REARM_PAD, me));
		targ = targ.concat(enumStruct(e, REPAIR_FACILITY, me));
		targ = targ.concat(enumStruct(e, RESEARCH_LAB, me));
		targ = targ.concat(enumStruct(e, SAT_UPLINK, me));
		targ = targ.concat(enumStruct(e, VTOL_FACTORY, me));
	}
	if (scavengers !== NO_SCAVENGERS) {
		targ = targ.concat(enumStruct(scavengerPlayer, DEFENSE, me));
		targ = targ.concat(enumStruct(scavengerPlayer, WALL, me));
	}
	return targ;
}

function getEnemyWalls(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		targ = targ.concat(enumStruct(e, WALL, me));
	}
	if (scavengers !== NO_SCAVENGERS) {
		targ = targ.concat(enumStruct(scavengerPlayer, WALL, me));
	}
	return targ;
}

//Функция возвращает все видимые вражеские ресурсы
function getEnemyResources(){
	var enemyRigs = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		var tmp = enumStruct(e, RESOURCE_EXTRACTOR, me);
		enemyRigs = enemyRigs.concat(tmp);
	}
	if (scavengers !== NO_SCAVENGERS) enemyRigs = enemyRigs.concat(enumStruct(scavengerPlayer, RESOURCE_EXTRACTOR, me));

	return enemyRigs;
}

//Возвращает строителей, инженеров, заводы и киборг-заводы
function getEnemyProduction(){
	var targ = [];
	for (let e = 0; e < maxPlayers; ++e) {
		if (allianceExistsBetween(me,e)) continue;
		targ = targ.concat(enumStruct(e, RESOURCE_EXTRACTOR, me));
		targ = targ.concat(enumDroid(e, DROID_CONSTRUCT, me));
		targ = targ.concat(enumDroid(e, 10, me)); // Киборг-строитель
		targ = targ.concat(enumStruct(e, FACTORY, me));
		targ = targ.concat(enumStruct(e, CYBORG_FACTORY, me));
	}
	if (scavengers !== NO_SCAVENGERS) {
		targ = targ.concat(enumStruct(scavengerPlayer, RESOURCE_EXTRACTOR, me));
		targ = targ.concat(enumDroid(scavengerPlayer, DROID_CONSTRUCT, me));
		targ = targ.concat(enumDroid(scavengerPlayer, 10, me)); // Киборг-строитель
		targ = targ.concat(enumStruct(scavengerPlayer, FACTORY, me));
		targ = targ.concat(enumStruct(scavengerPlayer, CYBORG_FACTORY, me));
	}
	return targ;
}

function removeDuplicates(originalArray, objKey) {
	var trimmedArray = [];
	var values = [];
	var value;

	for (let i = 0; i < originalArray.length; ++i) {
		value = originalArray[i][objKey];

		if (values.indexOf(value) === -1) {
			trimmedArray.push(originalArray[i]);
			values.push(value);
		}
	}

	return trimmedArray;

}

//Возвращает кол-во производящихся на данный момент типов в заводах.
function inProduce(type){
	if (produceTrigger.length === 0) return 0;
	var _prod = 0;

	for (const p in produceTrigger) {
		if (produceTrigger[p] === type) _prod++;
	}

	return _prod;
}


function unitIdle(obj){
//	debugMsg(obj.name+" "+obj.order+" "+obj.action, 'temp');
	if (obj.order === DORDER_NONE) {
//		debugMsg(obj.name+" idle", 'temp');
		return true;
	}
	return false;
}

//Функция равномерного распределения войск на несколько целей
//targets - цель
//warriors - атакующая группа
//num - количество целей для распределения от 1 до 10
function attackObjects(targets, warriors, num, scouting){
	if (targets.length === 0 || warriors.length === 0) return false;
	if (typeof num === "undefined" || num === null || num === 0) num = 3;
	if (num > 10) num = 10;
	if (warriors.length < num) num = warriors.length;

	targets = targets.slice(0,num);

	for (const i in targets) {
		var target = isBeingRepaired(targets[i]);
		if (target != false) {
			targets[i] = target;
		}
	}

	if (targets.length >= warriors.length) {
		for (let i = 0, len = warriors.length; i<len; ++i) {
			if (scouting) orderDroidLoc_p(warriors[i], DORDER_SCOUT, targets[i].x, targets[i].y);
			else orderDroidObj_p(warriors[i], DORDER_ATTACK, targets[i]);
		}
		return true;
	} else {
		var a = Math.floor(warriors.length/targets.length);
		var i=0;
		var t=0;
		for (const n in warriors) {
			t++;
			if (i === targets.length) return true;
			var busy = false;
			for (const j in targets) {
				if (distBetweenTwoPoints_p(targets[j].x,targets[j].y,warriors[n].x,warriors[n].y) < 7) {
					if (scouting) orderDroidLoc_p(warriors[n], DORDER_SCOUT, targets[j].x, targets[j].y);
					else orderDroidObj_p(warriors[n], DORDER_ATTACK, targets[j]);
					busy = true;
				}
			}
			if (busy) continue;
			if (scouting) orderDroidLoc_p(warriors[n], DORDER_SCOUT, targets[i].x, targets[i].y);
			else orderDroidObj_p(warriors[n], DORDER_ATTACK, targets[i]);
			if (t >= a) {
//				debugMsg("getTarget: Атака на "+targets.length+" цели по "+a+" юнита ("+targets[i].x+","+targets[i].y+")",4);
				t=0;
				i++;
			}
		}
		return true;
	}


}

//Исключает из двумерных масивов tech, массив excludes
function excludeTech(tech, excludes){
	var ex = [];
	tech = tech.filter((o) => (o.isArray || excludes.indexOf(o) === -1));
	tech.forEach((e) => {
		if (!(e instanceof Array)) {
			ex.push(e);
			return;
		}
		var check = e.filter((o) => (excludes.indexOf(o) === -1));
		if (check.length) ex.push(check);
	});
	return ex;
}

//Неоптимизированно, нужно доделать.
function checkEventIdle(){
	if (!running)return;
	targetPartisan();
	queue('targetCyborgs', 200);
	queue('targetRegular', 400);
}


//для 3.2

function recycleBuilders(){
	/*
	var factory = enumStruct(me, FACTORY);
	factory = factory.concat(enumStruct(me, REPAIR_FACILITY));
	var factory_ready = factory.filter((e) => (e.status === BUILT));
	if (factory_ready.length > 0) {
	*/
	var _builders = enumDroid(me,DROID_CONSTRUCT);
	_builders.forEach((e) => {
//		orderDroid(e, DORDER_RECYCLE);
		recycleDroid(e);
	});
//	}
}


//from: https://warzone.atlassian.net/wiki/pages/viewpage.action?pageId=360669
// More reliable way to identify VTOLs
var isFixVTOL = function(obj) {
	try {
		return ( (obj.type === DROID) && (("isVTOL" in obj && obj.isVTOL) || isVTOL(obj)) );
	} catch(e) {
//		debugMsg("isFixVTOL(): "+e.message, 'error');
	}
}

//from: http://stackoverflow.com/questions/6274339/how-can-i-shuffle-an-array
function shuffle(a) {
	var j, x, i;
	for (i = a.length; i; i--) {
		j = Math.floor(Math.random() * i);
		x = a[i - 1];
		a[i - 1] = a[j];
		a[j] = x;
	}
}

function posRnd(pos, axis){
	var p=pos+Math.round(Math.random()*2-1);
	if (p<1 || (axis === 'x' && p >= mapWidth)) return pos;
	if (p<1 || (axis === 'y' && p >= mapHeight)) return pos;
	return p;
}

function secondTick(){
	if (!running) return;

	if (earlyGame && gameTime/1000 > 130) {
		earlyGame = false;
		if (policy['build'] === 'rich') {
			maxPartisans = 1;
			scannersTimer = 300000;
			removeTimer("buildersOrder");
		}
	}

	//CHEAT
	if (berserk) {
		var qp=queuedPower(me);
//		debugMsg('qp: '+qp, 'berserk');
		if (qp > 0) {
			setPower(qp+1, me);
			credit += (qp+1);
			debugMsg('+'+(qp+1), 'berserk');
			debugMsg('credit: '+credit, 'berserk');
		}
	}

	debugMsg('power: '+playerPower(me), 'power');
}

function intersect_arrays(a, b) {
	var sorted_a = a.concat().sort();
	var sorted_b = b.concat().sort();
	var common = [];
	var a_i = 0;
	var b_i = 0;

	while (a_i < a.length && b_i < b.length)
	{
		if (sorted_a[a_i] === sorted_b[b_i]) {
			common.push(sorted_a[a_i]);
			a_i++;
			b_i++;
		}
		else if (sorted_a[a_i] < sorted_b[b_i]) {
			a_i++;
		}
		else {
			b_i++;
		}
	}
	return common;
}

//Всякоразно, исправление недочётов.
//Каждые 2 минуты
function longCycle(){
//	debugMsg("-debug-", 'debug');
	if (!running) return;

	//Повторно отправляем дроидов на продажу
	var recycle = enumGroup(droidsRecycle);
	if (recycle.length > 0) {
		debugMsg("recycle: "+recycle.length, 'debug');
		recycle.forEach((o) => {
			if (o.droidType === DROID_CONSTRUCT && (o.order === DORDER_BUILD || o.order === DORDER_HELPBUILD)) return;
			orderDroid(o, DORDER_RECYCLE);
		});
	}

	if (nf['policy'] === 'land') {
		var access = 'land';
		playerData.forEach((data, player) => {
			if (!access) return;
			if (player === me) return;
			if (playerLoose(player)) return;
			if (playerSpectator(player)) return;
			if (allianceExistsBetween(me,player)) return;
			if (propulsionCanReach('wheeled01', base.x, base.y, startPositions[player].x, startPositions[player].y)) { access = false; debugMsg("LongCycle: "+player+', land - exit', 'debug'); return; }
			else {access = 'island'; debugMsg("LongCycle: "+player+', island', 'debug');}
		});
		if (access === 'island') {
			nf['policy'] = 'island';
			switchToIsland();
			debugMsg("Switch to Island", 'debug');
		}
	}

	//Повторно отправляем дроидов, которые неуспешно продались на островной карте
	if (nf['policy'] === 'island' && getResearch("R-Vehicle-Prop-Hover").done) {
		debugMsg("-island-", 'debug');

		var droids = enumDroid(me, DROID_WEAPON);
		debugMsg("warriors: "+droids.length, 'debug');
		droids = droids.concat(enumGroup(buildersHunters));
		debugMsg("hunters: "+droids.length, 'debug');
		droids = droids.concat(enumGroup(buildersMain));
		debugMsg("main: "+droids.length, 'debug');
		droids = droids.concat(enumGroup(armyScanners));
		debugMsg("scanners: "+droids.length, 'debug');
		droids = droids.concat(enumGroup(armyFixers));
		debugMsg("fixers: "+droids.length, 'debug');
		debugMsg("droids to recycle: "+droids.length, 'debug');
		if (droids.length > 0)droids = droids.filter((o) => (o.propulsion === "wheeled01" || o.propulsion === "HalfTrack" || o.propulsion === "tracked01"));
		droids = droids.concat(enumDroid(me, DROID_CYBORG));
		debugMsg("filtered droids="+droids.length, 'debug');
		if (droids.length > 0)droids.forEach((o) => {recycleDroid(o);});
	}

	var points = getFixPoints();
	var broken = enumGroup(droidsBroken);
	if (broken.length > 0) {
		broken.forEach((o) => {
			if (o.health > 80) {groupArmy(o); return;}
			if (!points) {recycleDroid(o); return;}
			var p=0;
			if (points.length > 1)p=Math.floor(Math.random()*points.length);
			orderDroidLoc_p(o, DORDER_MOVE, points[p].x, points[p].y); return;
		});
	}

	fleetsReturn();

	if (playerPower(me) > 10000) {
		playerData.forEach((data, player) => {
			if (player === me) return;
			if (playerLoose(player)) return;
			if (playerSpectator(player)) return;
			if (!allianceExistsBetween(me,player)) return;
			if (allianceExistsBetween(me,player)) {
				debugMsg('pp:'+player+' '+playerPower(me)+' '+playerPower(player), 'temp');
				if (playerPower(me) > 3000 && playerPower(player) < 1000) donatePower(2000, player);
			}
		});
	}


}

function switchToIsland(){

	if (policy['build'] !== 'rich') policy['build'] = 'island';
	maxCyborgs = 0;

	minBuilders = 3;
	buildersTimer = 35000;
	minPartisans = 20;

	research_path.unshift(
		["R-Vehicle-Prop-Hover"],
		["R-Struc-PowerModuleMk1"],
		["R-Struc-Research-Module"],
		["R-Struc-Factory-Module"]
	);

	research_path = excludeTech(research_path, tech['cyborgs']);
	research_path = excludeTech(research_path, tech['tracks']);
}




function recycleDroid(droid){
/*
	debugMsg('droidID: '+droid.id+' health: '+droid.health, 'group');
	debugMsg(JSON.stringify(droid), 'group');
	debugMsg(typeof droid.group, 'group');
*/


	if (!(typeof droid.group === "undefined") && droid.group != droidsRecycle) groupAdd(droidsRecycle, droid);

	debugMsg(droid.health+": "+droid.x+"x"+droid.y, 'droids');

	var factory = enumStruct(me, FACTORY);
	factory = factory.concat(enumStruct(me, REPAIR_FACILITY));

	var factory_ready = factory.filter((e) => (e.status === BUILT));

	if (droid.droidType === DROID_CONSTRUCT && (droid.order === DORDER_BUILD || droid.order === DORDER_HELPBUILD)) return false;

	if (factory_ready.length > 0) {
		orderDroid(droid, DORDER_RECYCLE);
		return true;
	} else {
		orderDroidLoc_p(droid, DORDER_MOVE, base.x, base.y);
		return true;
	}
}

function getFixPoints(droid){

	if (typeof droid === "undefined") droid = false;

	var points = enumStruct(me, REPAIR_FACILITY);
	points = points.concat(enumGroup(armyFixers));

	if (points.length > 0) {

		if (droid !== false) points = sortByDistance(points, droid, 0, true);

		return points;
	}

	return false;

}

function fixDroid(droid){

	if (!(typeof droid.group === "undefined") && droid.group != droidsBroken) groupAdd(droidsBroken, droid);

	var points = getFixPoints(droid);
	debugMsg('fdp:'+points.length, 'temp');
	if (!points) return recycleDroid(droid);

	if (points.length > 1) {
		var _points=[];
		points.forEach((p) => {
			if (distBetweenTwoPoints_p(droid.x, droid.y, p.x, p.y) > 7) _points.push(p);
		});
		points = sortByDistance(_points, droid);
	}

	if (points.length > 0) { orderDroidLoc_p(droid, DORDER_MOVE, points[0].x, points[0].y); return true; }

	return false;
}

//TODO как ни будь доработать в будущем
//Отходить должны к ближайшим войскам, после определённого радиуса
function getFleetPoint(droid){

	if (typeof droid === "undefined") return false;

	var droidsNear = enumRange(droid.x, droid.y, 10, ALLIES);
//	debugMsg('dl:'+droidsNear.length, 'temp');
//	debugMsg('dn - '+JSON.stringify(droidsNear), 'temp');
	droidsNear = sortByDistance(droidsNear, base);
//	debugMsg('dl:'+droidsNear.length, 'temp');
//	debugMsg('dr:'+droidCanReach(droid, base.x, base.y), 'temp');
	if (droidsNear.length === 0 || droidsNear[0].id === droid.id) return base
	return droidsNear[0];

}


function fleetDroid(droid){

	if (!(typeof droid.group === "undefined") && droid.group != droidsFleet) groupAdd(droidsFleet, droid);
//	debugMsg(JSON.stringify(droid), 'temp');
	var point = getFleetPoint(droid);
//	debugMsg('point:'+JSON.stringify(point), 'temp');
//	debugMsg(JSON.stringify(droid), 'temp');
//	debugMsg('fleet from '+droid.x+'x'+droid.y+' to '+point.x+'x'+point.y, 'temp');
	orderDroidLoc_p(droid, DORDER_MOVE, point.x, point.y);
	return true;
}

function fleetsReturn(){
	var fleets = enumGroup(droidsFleet);
	if (fleets.length === 0) fleets = enumGroup(droidsBroken).filter((o) => (o.health > 80));
	if (fleets.length > 0) {
		fleets.forEach((o) => {
			if (o.health < 40) {fixDroid(o); return;}
			if (o.health < 10) {recycleDroid(o); return;}
			groupMixedDroids(o);
		});
	}
}

function groupMixedDroids(droid){
	switch (droid.droidType) {
		case DROID_WEAPON:
			if (isVTOL(droid))groupAdd(VTOLAttacker,droid);
			groupArmy(droid);
			break;
		case DROID_CONSTRUCT:
		case 10:

			if (groupSize(buildersMain) >= 2) groupAdd(buildersHunters, droid);
			else { groupBuilders(droid); }

			if (!getInfoNear(base.x,base.y,'safe',base_range).value) {
				base.x = droid.x;
				base.y = droid.y;
			}
//			func_buildersOrder_trigger = 0;
			buildersOrder();

			if (!running) {
				base.x = droid.x;
				base.y = droid.y;
				queue("initBase", 500);
				queue("letsRockThisFxxxingWorld", 1000);
			}

			break;
		case DROID_SENSOR:
			groupAdd(armyScanners, droid);
			targetSensors();
			break;
		case DROID_CYBORG:
			groupArmy(droid);
			break;
		case DROID_REPAIR:
			groupArmy(droid);
			break;
	}
}
