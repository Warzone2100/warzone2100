debugMsg('Module: targeting.js','init');

function targetVTOL(){
	if (!running)return;
//	debugMsg("targetVTOL():", 'targeting');

	var target = [];
	var scout = [];

	if (!(policy['build'] === 'rich')) {
		target = target.concat(sortByDistance(getEnemyResources(), base));
	}

	if (target.length === 0) target = target.concat(getEnemyFactoriesVTOL());
	if (target.length === 0) target = sortByDistance(getEnemyFactories(), base);
//	if (target.length === 0) target = sortByDistance(getEnemyPads(), base);
	if (target.length === 0) {
		scout = scout.concat(getEnemyResources());
		scout = scout.concat(getEnemyDefences());
		scout = scout.concat(getEnemyPads());
		scout = sortByDistance(scout, base, 1);
	}

	if (target.length === 0) {target = getEnemyWarriors();}
	if (target.length === 0) {target = getEnemyStructures();}

	var ready = enumGroup(VTOLAttacker).filter((e) => {
//		debugMsg(e.id+"-"+e.action, 'vtol');
		return !(e.action === 32 || e.action === 33 || e.action === 34 || e.action === 35 || e.action === 36 || e.action === 37 || e.action === 41 || e.action === 1);
	});

	var group = enumGroup(VTOLAttacker).filter((e) => (e.action === 41 || e.action === 1 || distBetweenTwoPoints_p(e.x,e.y,base.x,base.y) > base_range));
//	debugMsg("VTOLs: "+groupSize(VTOLAttacker)+"; patrol: "+ready.length+"; ready: "+group.length+"; targets: "+target.length, "vtol");
	if (group.length >= 3 && (target.length > 0 || scout.length > 0)) {
//		debugMsg("Attack!", "vtol");
		if (target.length > 0) {

			//lassat fire
			if (lassat_charged) {
				debugMsg('lassat init','lassat');
				var laser_sat = enumStruct(me, "A0LasSatCommand");
				debugMsg('lassat buildings: '+laser_sat.length,'lassat');
				laser_sat = laser_sat.filter((e) => (structureIdle(e)));
				debugMsg('lassat ready: '+laser_sat.length,'lassat');
				if (laser_sat.length > 0) {
					activateStructure(laser_sat[0], target[0]);
					debugMsg('lassat fire on '+target[0].x+'x'+target[0].y,'lassat');
					lassat_charged = false;
					playerData.forEach((data, player) => {
						if (!asPlayer)chat(player, ' from '+debugName+': '+chatting('lassat_fire'));
					});
				}
			}


			if (group.length <= 8) {

				if (!(policy['build'] === 'rich')) {
					attackObjects(target, group, target.length, false);
				}/* else {
					group.forEach((e) => {
						var attack = orderDroidObj_p(e, DORDER_ATTACK, target[0]);
	//					debugMsg("Attacking: "+target[0].name+"-"+attack, 'vtol');
					});
				}*/

			} else if (group.length <= 10) {
				attackObjects(target, group, 2, false);
			} else if (group.length <= 15) {
				attackObjects(target, group, 3, false);
			} else {
				attackObjects(target, group, 5, false);
			}
		} else if (scout.length > 0) {
			attackObjects(target, group, 5, true);
		}

	}
	ready.forEach((e) => {orderDroidLoc_p(e, 40, base.x, base.y);});
}

function targetJammers(){
	if (!running)return;
//	debugMsg("targetJammers():", 'targeting');


	var jammers = enumGroup(armyJammers);

	if (se_r >= army_rich) var partisans = enumGroup(armyRegular);
	else var partisans = enumGroup(armyPartisans);


	if (jammers.length === 0 || partisans.length === 0) return;

	//Армия дохнет? Спасаем задницу бегством!
	if (partisans.length <= 3) {
		var def = enumStruct(me,DEFENSE);
		if (def.length === 0) {
			if (distBetweenTwoPoints_p(base.x, base.y, jammers[0].x, jammers[0].y) > 3) jammers.forEach((e) => {orderDroidLoc_p(e, DORDER_MOVE, base.x, base.y);});
		} else {
			def = sortByDistance(def,jammers[0], 1);
			if (distBetweenTwoPoints_p(def[0].x, def[0].y, jammers[0].x, jammers[0].y) > 2) jammers.forEach((e) => {orderDroidLoc_p(e, DORDER_MOVE, def[0].x, def[0].y);});
		}
		return;
	}

	partisans = sortByDistance(partisans, base);
	jammers.reverse();
	jammers.forEach((f) => {
//		var target = partisans;
		orderDroidLoc_p(f, DORDER_MOVE, partisans[Math.round(partisans.length/2)].x, partisans[Math.round(partisans.length/2)].y);
	});
}

function targetSensors(){
	var sensors = enumGroup(armyScanners);
//	debugMsg("sensors="+sensors.length, 'debug');
	var target = getUnknownResources();
	if (target.length === 0) target = getEnemyStartPos();
//	else target = sortByDistance(target, base);
	target = sortByDistance(target, base, 1);
//	debug(target.length);

	if (target.length > 0) {
		sensors.forEach((e) => {
//			debug(target[0].x+'x'+target[0].y);
			orderDroidLoc_p(e, DORDER_SCOUT, target[0].x, target[0].y);
		});
	}

}

function targetFixers(){
//	debugMsg("targetFixers():", 'targeting');

	var fixers = enumGroup(armyFixers);

	var partisans = enumGroup(armyPartisans);

	if (fixers.length === 0 || partisans.length === 0) return;

	//Армия дохнет? Спасаем задницу бегством!
	if (partisans.length < 2) {
		var def = enumStruct(me,DEFENSE);
		if (def.length === 0) {
			if (distBetweenTwoPoints_p(base.x, base.y, fixers[0].x, fixers[0].y) > 3) fixers.forEach((e) => {orderDroidLoc_p(e, DORDER_MOVE, base.x, base.y);});
		} else {
			def = sortByDistance(def,fixers[0], 1);
			if (distBetweenTwoPoints_p(def[0].x, def[0].y, fixers[0].x, fixers[0].y) > 2) fixers.forEach((e) => {orderDroidLoc_p(e, DORDER_MOVE, def[0].x, def[0].y);});
		}
		return;
	}

	var target = enumGroup(droidsBroken);
	if (target.length === 0) target = enumGroup(droidsFleet).filter((o) => (o.health < 95));
	if (target.length === 0) target = partisans.filter((o) => (o.health < 95));

//	partisans = sortByDistance(partisans, base);
	fixers.reverse();

	fixers.forEach((f) => {
//		var target = partisans.filter((p) => (p.health < 100 && distBetweenTwoPoints_p(p.x,p.y,f.x,f.y) < 7));
		if (target.length > 0) {
			target = sortByDistance(target, f, 1);
//			if (distBetweenTwoPoints_p(f.x,f.y,target[0].x, target[0].y) < 2) orderDroidLoc_p(f, 41, f.x, f.y); // 41 - DORDER_HOLD
			if (distBetweenTwoPoints_p(f.x,f.y,target[0].x, target[0].y) < 2) {
//				orderDroidObj_p(f, 26, f); // 26 - DORDER_DROIDREPAIR
				return;
			}
			else {
				orderDroidLoc_p(f, DORDER_MOVE, target[0].x, target[0].y);
				orderDroidLoc_p(target[0], DORDER_MOVE, f.x, f.y);
				return;
			}
		}
		orderDroidLoc_p(f, DORDER_MOVE, partisans[Math.round(partisans.length/2)].x, partisans[Math.round(partisans.length/2)].y);
	});
}

function targetPartisan(){

	if (!running)return;

	if (partisanTrigger > gameTime) return;

//	debugMsg("targetPartisan():", 'targeting');

	var partisans = enumGroup(armyPartisans);
	if (partisans.length === 0) return false;

	var target=[];

	//Отключаем лишние функции в NTW картах
	if (!(se_r >= army_rich)) {
		target = target.concat(sortByDistance(getEnemyWalls().filter((e) => (distBetweenTwoPoints_p(e.x,e.y,base.x,base.y) < base_range)), base, 1));

	//	if (target.length > 0) debugMsg("partisans TARGET walls", 'targeting');

		if (target.length === 0) {
			target = target.concat(getEnemyNearBase());
	//		debugMsg("partisans TARGET near base", 'targeting');
		}

		if (target.length === 0 && ally.length > 0) {
			target = target.concat(getEnemyNearAlly());
	//		debugMsg("partisans TARGET near ally", 'targeting');
		}
	} else {
		target = sortByDistance(getEnemyBuilders(), partisans[0], 1, true);
	}
	//Если слишком мало партизан -И- нет срочной нужны в подмоге, то кучкуем армию у ближайших ресурсов за пределами базы
	if (partisans.length < minPartisans && target.length === 0 && !(gameTime/1000 < 280 && enemyDist > 80)) {
//		if (fixers.length === 0) {
			target = target.concat(getUnknownResources());
			target = target.concat(getSeeResources());
			target = sortByDistance(target, base).filter((e) => (
				distBetweenTwoPoints_p(e.x,e.y,base.x,base.y) < base_range && droidCanReach(partisans[0], e.x,e.y)
			));
//		} else {
//			target = fixers;
//		}

		if (target.length > 0) {
//			if (target.length > 4) target = target.slice(4);
//			target = target[Math.floor(Math.random()*target.length)];
			target = target[0];
//			debugMsg("Мало партизан "+partisans.length+", собираю всех у "+target.name+" недалеко от базы "+distBetweenTwoPoints_p(base.x,base.y,target.x,target.y), 'targeting');
			partisans.forEach((e) => {orderDroidLoc_p(e, DORDER_SCOUT, target.x, target.y);});
		}
		return false;
	}


	//Атакую партизанами ближайшую нефтевышку
	if (target.length === 0) {
		target = getEnemyResources();
		target = sortByDistance(target, partisans[0], 1, true);
//		debugMsg("partisans TARGET extractor", 'targeting');
	}

	if (target.length === 0) {
		target = sortByDistance(getUnknownResources(), partisans[0], 1, true);
//		debugMsg("partisans GO unknown resources", 'targeting');
	}

	if (target.length === 0 && partisans.length >= minPartisans) {
		target = sortByDistance(getEnemyFactories(), partisans[0], 1, true);
//		debugMsg("partisans TARGET factories", 'targeting');
	}

	if (target.length === 0 && partisans.length >= minPartisans) {
		target = sortByDistance(getEnemyProduction(), partisans[0], 1, true);
//		debugMsg("partisans TARGET builders", 'targeting');
	}

	if (target.length > 0) {
//		debugMsg("Партизан="+partisans.length+", атакую "+target[0].name+" расстояние от партизан="+distBetweenTwoPoints_p(partisans[0].x,partisans[0].y,target[0].x,target[0].y)+", от базы="+distBetweenTwoPoints_p(base.x,base.y,target[0].x,target[0].y), 'targeting');
		partisans.forEach((e) => {

			if ((target[0].type === STRUCTURE && target[0].stattype === WALL) || (target[0].type === DROID && target[0].droidType === DROID_CONSTRUCT)) {
				orderDroidObj_p(e, DORDER_ATTACK, target[0]);
//				debugMsg("ATTACK "+target[0].name, 'targeting');
			}
			else if (se_r >= army_rich && target[0].type === FEATURE && target[0].stattype === OIL_RESOURCE) {
				orderDroidLoc_p(e, DORDER_MOVE, target[0].x, target[0].y);
			}
			else {
				orderDroidLoc_p(e, DORDER_SCOUT, target[0].x, target[0].y);
//				debugMsg("SCOUT to "+target[0].x+"x"+target[0].y, 'targeting');
			}
		});
		return true;
	}
}


function targetCyborgs(){
//	debugMsg("targetCyborgs():", 'targeting');

	var _cyborgs = enumGroup(armyCyborgs).filter((e) => (e.order !== DORDER_ATTACK));
	if (_cyborgs.length === 0) return;

	var target = [];
	target = target.concat(getEnemyResources());
	if (groupSize(armyCyborgs) >= 10) {
		target = target.concat(getEnemyDefences());
	}
	if (groupSize(armyCyborgs) >= 15) {
		target = target.concat(getEnemyFactories());
	}
	var enemy = getEnemyNearBase();
	if (enemy.length === 0 && ally.length > 0)enemy = getEnemyNearAlly();

	if (enemy.length > 0) target = enemy; //Заменяем

	if (target.length > 0) {
		target = sortByDistance(target, _cyborgs[0], 1);
		_cyborgs.forEach((e) => {
//			debugMsg("Cyborgs attack "+target[0].name+" at "+target[0].x+"x"+target[0].y, 'targeting');
			orderDroidLoc_p(e, DORDER_SCOUT, target[0].x, target[0].y);
		});
		return;
	}

	//Если киборгам нечем заняться, добавляем их к общей армии
	_cyborgs.forEach((e) => {
		groupAdd(armyRegular, e);
//		debugMsg("Cyborg --> Regular +1", 'group');
	});

}


var brave = 3;

//Направляем армию
function pointRegularArmy(army){


	if (earlyGame) {
		var _target = [];
		var _enemy=0;
		_target = sortByDistance(getEnemyStructures(),base,1);
		if (_target.length > 0) _enemy = getEnemyNearPos(_target[0].x, _target[0].y).length;
		if (_target.length > 0 && _enemy <= 3) return _target[0];
	}

	//Если заварушка на базе, ничего не делаем, пусть воюют
	if (distBetweenTwoPoints_p(army[0].x, army[0].y, base.x, base.y) < base_range) return false;

	//Если есть точка сбора для армии
	if (pointRegular) {
		//Ближайшие войска к точке сбора
		var near = enumRange(pointRegular.x, pointRegular.y, 20, ALLIES).filter((obj) => ((obj.type === DROID && (obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG)) || (obj.type === STRUCTURE && obj.stattype === DEFENSE)));
//		var near = enumRange(pointRegular.x, pointRegular.y, 20, ALLIES).filter((obj) => (obj.group === army[0].group));

		//Вражеские войска на нашей точке сбора
		var enemy = enumRange(pointRegular.x, pointRegular.y, 40, ENEMIES, true).filter((obj) => ((obj.type === DROID && !obj.isVTOL) || (obj.type === STRUCTURE && obj.stattype === DEFENSE)));

		//Если точка сбора уже оккупирована, сбрасываем её
		if (enemy.length > 0) pointRegular = false;

//		var pointDist = distBetweenTwoPoints_p(pointRegular.x, pointRegular.y, startPos.x, startPos.y);
	}

	//Если нет точки сбора
	if (!pointRegular) {
		//Ближайшие войска к самому дальному от стартовой позиции (передовая армия)
		var near = enumRange(army[0].x, army[0].y, 20, ALLIES).filter((obj) => ((obj.type === DROID && (obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG)) || (obj.type === STRUCTURE && obj.stattype === DEFENSE)));

		//Ближайшие враги к передовой армии
		var enemy = enumRange(army[0].x, army[0].y, 40, ENEMIES, true).filter((obj) => ((obj.type === DROID && !obj.isVTOL) || (obj.type === STRUCTURE && obj.stattype === DEFENSE)));
		if (enemy.length > 0) lastEnemiesSeen = enemy.length;

//		var pointDist = false;
	}

	//Процент передовой армии от всей
	var percent = Math.round(near.length*100/army.length);

//	debugMsg('army='+near.length+'/'+army.length+', '+percent+'%, point='+pointDist, 'army');
//	debugMsg('enemies='+enemy.length+', lastSeen='+lastEnemiesSeen, 'army');

	if (pointRegular && percent < 80) {
//		debugMsg('Сбор у точки', 'army');
		return pointRegular;
	} else {
		pointRegular = false;
	}

//	//Все враги, которых видно на карте
//	var allenemy = getEnemyWarriors();



	//Если врагов больше чем кусок передовой армии - отступаем
	if (enemy.length > near.length+brave) {
//		debugMsg('Бежать!', 'army');
		var armyDist = distBetweenTwoPoints_p(army[0].x, army[0].y, startPos.x, startPos.y);

		var def = [];
		if (rage === MEDIUM) {
			var def = sortByDistance(getOurDefences().filter((e) => (distBetweenTwoPoints_p(e.x, e.y, startPos.x, startPos.y) < armyDist)), startPos);
			def.reverse();
		}

		if (def.length > 0) {
			//Отходим к ближайшей защите
			pointRegular = def[0];
		} else {
			//Отходим к началу тянущегося хвоста армии, указываем точку для необходимого сбора армии
			pointRegular = army[army.length-Math.ceil((army.length-near.length)/2+(army.length-near.length)/(near.length/2))];
		}
		return pointRegular;
	}

//	var target = sortByDistance(getEnemyStructures(),base,1);
//	var enemy = getEnemyNearPos(target[0].x, target[0].y);
	//Если процент передовой армии меньше 65% или видимые враги привышают передовую армию и вся армия не достигла предела, то ждём хвосты
//	if ((percent < 65 || lastEnemiesSeen > near.length) && army.length < maxRegular) {
//	if (percent < 65 && army.length < maxRegular && army.length < enemy.length) {
		if (percent < 65 && army.length < maxRegular) {
//		debugMsg('Ждать хвосты', 'army');
//		return army[Math.floor(army.length/4)];
		return army[0];
	}
//	lastEnemiesSeen = 0;
//	debugMsg('go', 'army');
	return false;
}

function targetRegularRich(target, victim){
//	debugMsg("targetRegularRich():", 'targeting');
	if (typeof target === "undefined") target = false;
	if (typeof victim === "undefined") victim = false;


	var regular = enumGroup(armyRegular);
	//Если армии нет - выход из функции
	if (regular.length === 0) return false;

	//Если из армии атакован огненный боец, берём всех огненных рядом и в партизаны/камикадзе
	if (victim != false && victim.droidType === DROID_WEAPON) {
//		debugMsg("Check weapon for "+victim.name, "army");
		if (Stats.Weapon[victim.weapons[0].fullname].ImpactType === "HEAT") {
			enumRange(victim.x, victim.y, 7, ALLIES).filter((obj) => (obj.type === DROID && obj.droidType == DROID_WEAPON && Stats.Weapon[obj.weapons[0].fullname].ImpactType === "HEAT"))
			.forEach((d) => {groupAdd(armyPartisans, d);});
		}
	}

	//Сортируем армию от стартовой позиции
	regular = sortByDistance(regular, startPos);

	//Сначало самые дальние юниты
	regular.reverse();

	//Если указана цель, но отсутствует точка сбора
	if (target && !pointRegular) {
		//Если до цели непроходимая местность, сбрасываем цель
		if (!droidCanReach(regular[0],target.x,target.y)) {
//			debugMsg("regular: event не достежим", 'targeting');
			target = false;
		} else if (victim) {
//			debugMsg("regular: event от армии "+distBetweenTwoPoints_p(target.x,target.y,regular[0].x,regular[0].y), 'targeting');

			//Если атакованный дроид дальше от базы, чем армия, идём туда.
			if (distBetweenTwoPoints_p(base.x, base.y, victim.x, victim.y) > distBetweenTwoPoints_p(base.x, base.y, regular[0].x,regular[0].y)) targRegular = target;

			if (reactWarriorsTrigger < gameTime && victim.type === DROID && victim.group === regular[0].group) {
				reactWarriorsTrigger = gameTime + reactWarriorsTimer;
				var near = enumRange(victim.x, victim.y, 20, ALLIES).filter((obj) => (obj.group === regular[0].group));
//				debugMsg('Ответный огонь '+near.length, 'targeting');
				near.forEach((e) => {orderDroidLoc_p(e, DORDER_SCOUT, target.x, target.y);});
			} else {
				if ((victim.type === DROID && victim.droidType === DROID_CONSTRUCT) || victim.type === STRUCTURE) targRegular = target;
			}
		} else {
			targRegular = target;
		}
	}

	if (reactRegularArmyTrigger > gameTime) {
//		debugMsg('targetRegularRich(force exit)', 'targeting');
		return false;
	}
	reactRegularArmyTrigger = gameTime + reactRegularArmyTimer;

	var help = [];
	var stopPoint;
	if (ally.length > 0) help = getEnemyNearAlly();
	if (help.length === 0) {
		help = getEnemyCloseBase();
	}
	if (help.length > 0) {
		targRegular = {x:help[0].x,y:help[0].y};
		pointRegular = false;
	} else {
		stopPoint = pointRegularArmy(regular);
	}

	if (stopPoint) {
		var endPoint = {x:stopPoint.x,y:stopPoint.y};
	}
	else {
		var endPoint = {x:targRegular.x,y:targRegular.y};
	}

	if (targRegular.x === 0 && targRegular.y === 0 && !target) {
//		debugMsg("regular: Стандартный сбор у базы", 'targeting');
		targRegular = base;
	}

	//Армия у места назначения
	if (distBetweenTwoPoints_p(endPoint.x,endPoint.y,regular[0].x,regular[0].y) <= 7) {
//		debugMsg('Армия на месте бездельничает', 'targeting');
		target = sortByDistance(getEnemyFactories(), regular[0], 1);
		if (target.length === 0) target = sortByDistance(getEnemyProduction(), regular[0], 1);
//		if (target.length === 0) target = sortByDistance(getEnemyStartPos(), regular[0], 1);
		if (target.length > 0) {
//			debugMsg('Дополнительные цели: '+target.length, 'targeting');
			targRegular = {x:target[0].x,y:target[0].y};
		}
	}


//	if (regular.length > minRegular || regular.length > _enemy) {
	if (regular.length > minRegular || earlyGame) {
		if (stopPoint) {
//			debugMsg("regular: Регрупировка "+distBetweenTwoPoints_p(endPoint.x,endPoint.y,base.x,base.y), 'targeting');
			regular.forEach((e) => {orderDroidLoc_p(e, DORDER_MOVE, endPoint.x, endPoint.y);});
		}
		else {
//			debugMsg("regular: Атака/Разведка "+distBetweenTwoPoints_p(endPoint.x,endPoint.y,base.x,base.y), 'targeting');
			if (!release)mark(endPoint.x,endPoint.y);
			regular.forEach((e) => {orderDroidLoc_p(e, DORDER_SCOUT, endPoint.x, endPoint.y);});
		}
		return true;
	}

//	debugMsg('targetRegularRich(exit)', 'targeting');
	return false;
}

function targetRegular(target, victim){
	if (!running)return;

	if (typeof victim === "undefined") victim = false;

	//	Перенаправляем функцию
	if (se_r >= army_rich && (rage === MEDIUM || rage === HARD || rage === INSANE)) {
		if (targetRegularRich(target, victim)) return true;
		return false;
	}

//	debugMsg("targetRegular():", 'targeting');

	var regular = enumGroup(armyRegular);
	if (regular.length === 0) return false;


	var help = [];
	if (ally.length > 0) help = getEnemyNearAlly();
//	debugMsg("Enemy near ally "+help.length, 'targeting');
	if (help.length === 0) {
		help = getEnemyCloseBase();
//		debugMsg("Enemy near base "+help.length, 'targeting');
	}
	if (help.length > 0) {
//		debugMsg("Helping with our mighty army, targets="+help.length, 'targeting');
		regular.forEach((e) => {orderDroidLoc_p(e, DORDER_SCOUT, help[0].x, help[0].y);});
		return;
	}

	if (typeof target === "undefined") target = false;
//	if (target) debugMsg("regular: event от армии "+distBetweenTwoPoints_p(target.x,target.y,regular[0].x,regular[0].y), 'targeting');
	if (target && !droidCanReach(regular[0],target.x,target.y)) {
//		debugMsg("regular: event не достежим", 'targeting');
		target = false;
	}
	if (targRegular.x === 0 && targRegular.y === 0 && !target) {
//		debugMsg("regular: Стандартный сбор у базы", 'targeting');
		targRegular = base;
	}

	var _reach = regular.filter((e) => (distBetweenTwoPoints_p(e.x,e.y,targRegular.x,targRegular.y) <= 7));

	//Если армии более 40% на точке
	if ((_reach.length * 100 / regular.length) > 40) {
//		debugMsg("regular: Армия готова: сбор "+Math.floor(_reach.length * 100 / regular.length)+"%", 'targeting');
		if (target) {
//			debugMsg("regular: меняем цель на event "+distBetweenTwoPoints_p(target.x,target.y,regular[0].x,regular[0].y), 'targeting');
			targRegular = target;
//			targRegular.x = target.x;
//			targRegular.y = target.y;
		}
		else {
			target = getEnemyFactories();
//			if (target.length === 0 && regular.length >= maxRegular/2) {	//Если армии больше половины собрано, а целей всё ещё нет
			if (target.length === 0) {	//Если целей всё ещё нет
				target = sortByDistance(getEnemyProduction(), regular[0], 1);
			}
			if (target.length > 0) {
//				debugMsg("regular: меняем цель на вражеский завод "+distBetweenTwoPoints_p(target[0].x,target[0].y,regular[0].x,regular[0].y), 'targeting');
				targRegular = target[0];
//				targRegular.x = target[0].x;
//				targRegular.y = target[0].y;
			}
		}
	}
	else {
//		debugMsg("regular: Армия далеко от цели, собрано: "+Math.floor(_reach.length * 100 / regular.length)+"%", 'targeting');
		if (regular.length > minRegular) {
//			debugMsg("regular: Атака "+distBetweenTwoPoints_p(targRegular.x,targRegular.y,regular[0].x,regular[0].y), 'targeting');
			regular.forEach((e) => {orderDroidLoc_p(e, DORDER_SCOUT, targRegular.x, targRegular.y);});
			return;
		} else {
//			debugMsg("regular: Сбор "+distBetweenTwoPoints_p(targRegular.x,targRegular.y,regular[0].x,regular[0].y), 'targeting');
			regular.forEach((e) => {orderDroidLoc_p(e, DORDER_SCOUT, regular[0].x,regular[0].y);});
			return;
		}
	}



}
