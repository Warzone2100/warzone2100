debugMsg('Module: events.js','init');

function bc_eventStructureReady(structure) {
	if (structure.player === me) {
		switch (structure.stattype) {
			case LASSAT:
				lassat_charged = true;
				debugMsg('lassat charged','lassat');
				playerData.forEach((data, player) => {
					chat(player, ' from '+debugName+': '+chatting('lassat_charged'));
				});
			break;
		}
	}
}
function bc_eventResearched(research, structure, player) {

	if (!running) return;

	debugMsg("Новая технология \""+research_name[research.name]+"\" ["+research.name+"]", 'research');
	prepeareProduce();
	queue("doResearch", 1000);


	if (research.name === 'R-Vehicle-Prop-Hover') {
		minBuilders = 7;
		buildersTimer = 5000;
		if (!nf['oilfinite'])recycleBuilders();
	}

	//Remove chaingun and flamer cyborgs if better available
	if (research.name === 'R-Wpn-MG4') {cyborgs = cyborgs.filter((e) => (e[2] !== 'CyborgChaingun'));}
	if (research.name === 'R-Wpn-Flame2') {cyborgs = cyborgs.filter((e) => (e[2] !== 'CyborgFlamer01'));}
	if (research.name === 'R-Struc-PowerModuleMk1') {func_buildersOrder_trigger = 0;buildersOrder();}
	if (research.name === 'R-Struc-Factory-Module') {func_buildersOrder_trigger = 0;buildersOrder();}
	if (research.name === 'R-Struc-Research-Module') {func_buildersOrder_trigger = 0;buildersOrder();}

}

//3.2+
function bc_eventPlayerLeft(player) {
	bc_ally = bc_ally.filter((p) => (p !== player));
	ally = ally.filter((p) => (p !== player));
	enemy = enemy.filter((p) => (p !== player));
	if (player === me) gameStop('kick');
}

// Обязательно использовать
function bc_eventDroidIdle(droid) {

	debugMsg('idle '+droidTypes[droid.droidType], 'events');

	switch (droid.droidType) {
		case DROID_CYBORG:
			if (gameTime > eventsRun['targetCyborgs']) {
				debugMsg("targetCyborgs", 'events');
				eventsRun['targetCyborgs'] = gameTime + 5000;
				targetCyborgs();
//				queue("targetPartisan", 1500);
			}
		break;
		case DROID_WEAPON:

			if (gameTime > eventsRun['targetArmy']) {
				debugMsg("targetArmy", 'events');
				eventsRun['targetArmy'] = gameTime + 9000;
				targetPartisan();
				queue("targetRegular", 1000);
//				queue("targetCyborgs", 2000);
			}

		break;
		case DROID_CONSTRUCT:
			if (gameTime > eventsRun['buildersOrder']) {
				debugMsg("buildersOrder", 'events');
				eventsRun['buildersOrder'] = gameTime + 2000;
				buildersOrder();
			}
		break;
		case DROID_SENSOR:
			// Ищем чего бы подсветить/разведать
			if (gameTime > eventsRun['targetSensors']) {
				debugMsg("targetSensors", 'events');
				eventsRun['targetSensors'] = gameTime + 5000;
				targetSensors();
			}
		break;
		case DROID_ECM:
			if (gameTime > eventsRun['targetJammers']) {
				debugMsg("targetJammers", 'events');
				eventsRun['targetJammers'] = gameTime + 5000;
				targetJammers();
			}
		break;
		case DROID_REPAIR:
			if (gameTime > eventsRun['targetFixers']) {
				debugMsg("targetFixers", 'events');
				eventsRun['targetFixers'] = gameTime + 6000;
				targetFixers();
			}
		break;
	}
}

//Я так понял, данный триггер больше не работает так как был задуман.
function bc_eventObjectSeen(sensor, gameObject) {

	debug(sensor.name+': '.gameObject.name, 'eventSeen');
/*
	if (sensor.type === DROID && sensor.droidType === DROID_SENSOR && !allianceExistsBetween(me, gameObject.player)) {
		var sensors = enumGroup(armyScanners);
		pointRegular = armyScanners[0];
		debug(pointRegular.x+'x'+pointRegular.y);
	}
*/

	switch (gameObject.type) {
		case STRUCTURE:
		if (!allianceExistsBetween(me,gameObject.player)) {
//			debugMsg("eventObjectSeen: "+ sensor.name+" обнаружил вражеское строение: "+gameObject.name, 'events');
//			getTarget();
		}
		break;
		case DROID:
		if (!allianceExistsBetween(me,gameObject.player)) {
//			debugMsg("eventObjectSeen: "+ sensor.name+" обнаружил вражескую еденицу: "+gameObject.name, 'events');
//			getTarget();
			if (gameObject.droidType === DROID_WEAPON
				&& isFixVTOL(gameObject)
				&& distBetweenTwoPoints_p(gameObject.x,gameObject.y,base.x,base.y) < base_range)
				AA_queue.push({x:gameObject.x,y:gameObject.y});
		}
		break;
		case FEATURE:
//			debugMsg("eventObjectSeen: "+ sensor.name+" обнаружил FEATURE: "+gameObject.name, 'events');
//			getTarget();
			buildersOrder();
		break;
	}
}

//Если произошла передача от игрока к игроку
function bc_eventObjectTransfer(gameObject, from) {
	debugMsg("I="+me+"; object '"+gameObject.name+"' getting to "+gameObject.player+" from "+from, 'transfer');

	if (gameObject.player === me) { // Что то получили
		groupMixedDroids(gameObject);
	}
	/*
	else { // От нас что-то ушло
		if (allianceExistsBetween(gameObject.player,from)) { // Союзнику

		} else { // Похоже наших вербуют!!!

		}
	}
	*/
}

//Срабатывает при завершении строительства здания
function bc_eventStructureBuilt(structure, droid){

	/*
	if (policy['build'] === 'rich') {
		var _b = enumGroup(buildersMain)[0];
		if (typeof _b !== "undefined") {
			base.x = _b.x;
			base.y = _b.y;
		}
	}
	*/
	func_buildersOrder_trigger = 0;
	buildersOrder();

	switch (structure.stattype) {
		case RESEARCH_LAB:
			queue("doResearch", 1000);
			if (rage !== EASY && gameTime < 300000) {
				//Ротация строителей в начале игры, для более быстрого захвата ресурсов на карте
				//Отключено в лёгком режиме
				factory = enumStruct(me, FACTORY);
				research_lab = enumStruct(me, RESEARCH_LAB);
//				power_gen = enumStruct(me, POWER_GEN);
//				power_gen_ready = power_gen.filter((e) => (e.status === BUILT));
				factory_ready = factory.filter((e) => (e.status === BUILT));
				research_lab_ready = research_lab.filter((e) => (e.status === BUILT));
//				if ((factory_ready.length === 1 && research_lab_ready.length === 1) || power_gen_ready.length === 1)
				if (factory_ready.length === 2 && research_lab_ready.length === 1) {
					enumGroup(buildersMain).forEach((e, i) => {if (i > 0) {groupAdd(buildersHunters, e);debugMsg("FORCE "+i+" Builder --> Hunter +1", 'group');}});
				}
				else if (((factory_ready.length === 1 && research_lab_ready.length === 3) || research_lab_ready.length === 4) && policy['build'] === 'rich') {
					var e = enumGroup(buildersMain)[0];
					groupAdd(buildersHunters, e);
				}
			}

		break;
		case FACTORY:
			factory = enumStruct(me, FACTORY);
			factory_ready = factory.filter((e) => (e.status === BUILT));

			if (groupSize(buildersMain) > 0) {
				if (( (factory_ready.length === 1 && research_lab_ready.length === 1) || factory_ready.length === 2 || factory_ready.length === 3) && policy['build'] === 'rich') {
					var e = enumGroup(buildersMain)[0];
					groupAdd(buildersHunters, e);
				}

	//			if (policy['build'] !== 'rich') {
					var _b = enumGroup(buildersMain)[0];
					if (typeof _b !== "undefined") {
						base.x = _b.x;
						base.y = _b.y;
					}
	//			}
			}
			produceDroids();
		break;
		case CYBORG_FACTORY:
			produceCyborgs();
		break;
		case VTOL_FACTORY:
			produceVTOL();
		break;
		case HQ:
			prepeareProduce();
		break;
	}

}

//этот триггер срабатывает при выходе из завода нового свежего юнита.
function bc_eventDroidBuilt(droid, structure) {

//	debugMsg("eventDroidBuilt: droidType="+droid.droidType+", name="+droid.name, 'eventDroidBuilt');

	if (produceTrigger[structure.id]) {
		var rem = produceTrigger.splice(structure.id, 1);
		debugMsg('BUILT: removed from '+structure.id+' '+rem, 'triggers');
		groupArmy(droid, rem);
		return;
	}

	switch (droid.droidType) {
		case DROID_WEAPON:
			break;
		case DROID_CONSTRUCT:
			groupBuilders(droid);
			func_buildersOrder_trigger = 0;
			queue("buildersOrder", 1000);
			break;
		case 10:
			groupAdd(buildersMain, droid);
			func_buildersOrder_trigger = 0;
			queue("buildersOrder", 1000);
			break;
	}

	switch (structure.stattype) {
		case FACTORY:
			if (droid.droidType === DROID_WEAPON) {
				if (checkDonate(droid)) {return;}
				else groupArmy(droid);
				if (se_r >= army_rich) targetRegular();
			}
			if (droid.droidType === DROID_REPAIR) {
				groupArmy(droid);
			}
			if (droid.droidType === DROID_SENSOR) {
				groupAdd(armyScanners, droid);
				targetSensors();
			}
//			if (droid.droidType === DROID_ECM) groupArmy(droid);
			produceDroids();
//			targetRegular();
			break;
		case CYBORG_FACTORY:
			if (droid.droidType === DROID_CYBORG) {
				if (checkDonate(droid)) {return;}
				else groupArmy(droid);
			}
			produceCyborgs();
//			targetCyborgs();
			break;
		case VTOL_FACTORY:
			if (vtolToPlayer !== false) {donateObject(droid, vtolToPlayer);}
			else {
				orderDroidLoc_p(droid, 40, base.x, base.y);
				groupAdd(VTOLAttacker, droid);
			}
			produceVTOL();
//			targetVTOL();
			break;
	}

}
/*			if (gameTime > eventsRun['buildersOrder']) {
				debugMsg("buildersOrder",'events');
				eventsRun['buildersOrder'] = gameTime + 10000;
				buildersOrder();
			}
*/


function bc_eventAttacked(victim, attacker) {
//	debugMsg(JSON.stringify(victim), 'temp');
	if (allianceExistsBetween(me, attacker.player)) return;

	if (scavengers !== NO_SCAVENGERS && attacker.player === scavengerPlayer && victim.type === DROID && !isFixVTOL(victim)) {

		if (victim.health < 10) {
			if (recycleDroid(victim)) return;
		}
		if (victim.health < 40) {
			if (fixDroid(victim)) return;
		}

		var myDroids = enumRange(victim.x, victim.y, 12, ALLIES).filter((o) => (
			o.player === me && o.type === DROID
		));
		myDroids = myDroids.filter((o) => (!(o.droidType === DROID_CONSTRUCT && builderBusy(o))));

		var enemyObjects = enumRange(attacker.x, attacker.y, 7, ENEMIES, me);

		debugMsg('myDroids: '+myDroids.length+', enemyObjects: '+enemyObjects.length, 'temp');

		var myHP = 0;
		var enemyHP = 0;

		myDroids.forEach((o) => {myHP+=o.health;});
		enemyObjects.forEach((o) => {enemyHP+=o.health;});

		debugMsg('scav: myHP: '+myHP+', enemyHP: '+enemyHP, 'temp');

		if (myHP < (enemyHP/2)) {
			myDroids.forEach((o) => {
				if (o.droidType !== DROID_REPAIR) fleetDroid(o);
//				groupAdd(droidsFleet, o);
//				orderDroidLoc_p(o, DORDER_MOVE, base.x, base.y);
			});
//			partisanTrigger = gameTime + reactPartisanTimer;
		} else fleetsReturn();

		return;
	}

	//Если атака с самолёта рядом с базой, строим ПВО
	if (isFixVTOL(attacker) && distBetweenTwoPoints_p(victim.x,victim.y,base.x,base.y) < base_range) AA_queue.push({x:victim.x,y:victim.y});

	var lastImpact;

	//Если атака по стратегическим точкам, направляем основную армию
	if (((victim.type === DROID && victim.droidType === DROID_CONSTRUCT) || (victim.type === STRUCTURE)) && gameTime > eventsRun['targetRegular']) {
		eventsRun['targetRegular'] = gameTime + 5000;
		lastImpact = {x:victim.x,y:victim.y};
		if (distBetweenTwoPoints_p(lastImpact.x, lastImpact.y, base.x, base.y) < base_range) {
			var warriors = enumRange(victim.x, victim.y, 20, ALLIES).filter((e) => (e.player === me && e.type === DROID && e.droidType === DROID_WEAPON));
			warriors.forEach((e) => {orderDroidLoc_p(e, DORDER_SCOUT, lastImpact.x, lastImpact.y);});
		}
		targetRegular(attacker, victim);
	}

	if (policy['build'] === 'rich' && victim.type === DROID && victim.droidType === DROID_CONSTRUCT) {
		lastImpact = {x:victim.x,y:victim.y};
		orderDroidLoc_p(victim, DORDER_MOVE, base.x, base.y);
	}

	if (victim.type === DROID && victim.droidType === DROID_SENSOR) {
		var point = enumDroid(me, DROID_WEAPON);

		//Отходим к союзникам, если союзная армия ближе
		point = point.concat(getAllyArmy());

		if (point.length > 0) {
			point = sortByDistance(point, victim, 1);
		} else point = base;

		orderDroidLoc_p(victim, DORDER_MOVE, point.x, point.y);

	}

	//Если атака по армии, отводим атакованного
	if (victim.type === DROID && victim.droidType === DROID_WEAPON && !isFixVTOL(victim)) {
		lastImpact = {x:victim.x,y:victim.y};

		var myDroids = enumRange(victim.x, victim.y, 10, ALLIES).filter((o) => (
			o.player === me && o.type === DROID && o.droidType === DROID_WEAPON
		));
		var enemyObjects = enumRange(attacker.x, attacker.y, 5, ENEMIES, me);
		var myHP = 0;
		var enemyHP = 0;
		myDroids.forEach((o) => {myHP+=o.health;});
		enemyObjects.forEach((o) => {enemyHP+=o.health;});
		debugMsg('enemy: myHP: '+myHP+', enemyHP: '+enemyHP, 'temp');

		if (myHP < (enemyHP/2)) {
			myDroids.forEach((o) => {
				if (Stats.Weapon[o.weapons[0].fullname].ImpactType !== "HEAT") fleetDroid(o);
//				groupAdd(droidsFleet, o);
//				orderDroidLoc_p(o, DORDER_MOVE, base.x, base.y);
			});
//			partisanTrigger = gameTime + reactPartisanTimer;
		} else fleetsReturn();

		if (victim.health < 10) {
			if (recycleDroid(victim)) return;
		}
		if (victim.health < 40) {
			if (fixDroid(victim)) return;
		}
		//т.к. в богатых картах кол-во партизан всего 2, направляем всю армию к атакованным
		if (se_r >= army_rich) { targetRegular(attacker, victim);return;}

		//Если атакуют огнемётные войска, атакуем ими ближайшего врага
		if (Stats.Weapon[victim.weapons[0].fullname].ImpactType === "HEAT") {
			var enemies = enumRange(victim.x, victim.y, 3, ENEMIES).filter((e) => (e.type === DROID));
			if (enemies.length > 0) {
				enemies = sortByDistance(enemies, victim, 1);
				orderDroidObj_p(victim, DORDER_ATTACK, enemies[0]);
				return;
			}
			return;
		}




		if (fleetTrigger < gameTime) {
			fleetsReturn();
			fleetTrigger = gameTime + 1000;
			return;
		}
		fleetDroid(victim);
//		orderDroidLoc_p(victim, DORDER_MOVE, base.x, base.y);
	}

	//Если атака по киборгам, ответный огонь ближайшими киборгами
	if (victim.type === DROID && victim.droidType === DROID_CYBORG && !isFixVTOL(victim) && gameTime > eventsRun['victimCyborgs']) {
		if (victim.health < 30) {
			fixDroid(victim);
		}
		eventsRun['victimCyborgs'] = gameTime + 10000;
		var cyborgs = enumGroup(armyCyborgs);
		cyborgs.forEach((e) => {
			if (distBetweenTwoPoints_p(e.x,e.y,attacker.x,attacker.y) < 10)
//			orderDroidLoc_p(e, DORDER_SCOUT, {x:attacker.x,y:attacker.y});
			orderDroidObj_p(e, DORDER_ATTACK, attacker);
		});
	}

}

function bc_eventDestroyed(obj){
	if (obj.type === STRUCTURE && obj.stattype === FACTORY) {
		if (produceTrigger[obj.id]) {
			var rem = produceTrigger.splice(obj.id, 1);
			debugMsg('DESTROYED: removed from '+obj.id+' '+rem, 'triggers');
		}
	}

	if (obj.type === STRUCTURE && (obj.player === me || obj.stattype === RESOURCE_EXTRACTOR) && isFullBase(me)) {

		//Возвращаем частоту функции
		func_buildersOrder_timer = 5000+me*100;
		func_buildersOrder_trigger = 0;

		buildersOrder();
	}

}

function bc_eventChat(sender, to, message) {
	if (asPlayer) return;
//	debugMsg('from: '+sender+', to: '+to+', msg: '+message, 'chat')
	if (!release)
	switch (message) {
		case "disable buildersOrder":
			func_buildersOrder = false;
			chat(sender, "buildersOrder() disabled");
			break;
		case "enable buildersOrder":
			func_buildersOrder = true;
			chat(sender, "buildersOrder() enabled");
			break;
	}

	if (sender !== me)
	if (allianceExistsBetween(me, sender))
	if (message.substr(0,3) === "bc ")
	switch (message.substr(3)) {
		case "wgr":
			setResearchWay("Green");
			chat(sender, "Goind to Green way");
			break;
		case "wor":
			setResearchWay("Orange");
			chat(sender, "Goind to Orange way");
			break;
		case "wyl":
			setResearchWay("Yellow");
			chat(sender, "Goind to Yellow way");
			break;
		case "wrd":
			setResearchWay("Red");
			chat(sender, "Goind to Red way");
			break;
		case "btm":
			chat(sender, "My position base to you at "+startPositions[sender].x+"x"+startPositions[sender].y);
			base.x = startPositions[sender].x;
			base.y = startPositions[sender].y;
			break;
		case "gmp":
			chat(sender, "I have "+playerPower(me));
			if (playerPower(me)>500) {
				donatePower(Math.floor(playerPower(me)/2), sender);
				chat(sender, "Remained "+playerPower(me)+" power");
			}
			else
				chat(sender, "My power is to low");
			break;
		case "gtn":
			if (groupSize(buildersMain) === 0)
				chat(sender, "I do not have free Trucks");
			else {
				chat(sender, "Here my new shiny Truck");
				var truck = enumGroup(buildersMain)[0];
				donateObject(truck, sender);
			}
			break;
		case "gaa":
			armyToPlayer = sender;
			chat(sender, "All my new shiny army for you.");
			chat(sender, "Say \"bc caa\" to cancel this");
			break;
		case "caa":
			armyToPlayer = false;
			chat(sender, "Army is mine now.");
			break;
		case "gav":
			vtolToPlayer = sender;
			chat(sender, "All my new shiny VTOLs for you.");
			break;
		case "cav":
			vtolToPlayer = false;
			chat(sender, "VTOLs is mine now.");
			break;
		case "dbg":
			debugMsg("DEBUG: ", 'dbg');
/*			for (const i in avail_research) {
				debugMsg(avail_research[i].name, 'dbg');
			}
			debugMsg("<==-==>", 'dbg');
			debugMsg("DEBUG: research_way", 'dbg');
			for (const i in research_way) {
				debugMsg(research_way[i], 'dbg');
			}
*/
			debug(JSON.stringify(_globalInfoNear));
			debug(JSON.stringify(_globalOrders));
			debug(JSON.stringify(defQueue));
			debug(JSON.stringify(nf));
			debug(JSON.stringify(policy));

			break;
		case "ver":
			chat(sender, "Version: "+vernum+" ("+verdate+")");
			break;
	}

	if (message.substr(0,8) === "cheat me") {
		if (!isMultiplayer && (!isHumanAlly() || !release)) {
			berserk = true;
			debugMsg('Berserk activated', 'init');
			chat(sender, ' from '+debugName+': '+chatting('berserk'));
		} else {
			chat(sender, ' from '+debugName+': '+chatting('no'));
		}
	}

	if (message.substr(0,13) === "cheat me hard") {
		if (!isMultiplayer && (!isHumanAlly() || !release)) {
			debugMsg('Big army activated', 'init');
			minPartisans = 20;
			maxPartisans = 25;
			minRegular = 30;
			maxRegular = 70;
			minCyborgs = 40;
			maxCyborgs = 50;
			seer = true;
			debugMsg('Seer activated', 'init');
			chat(sender, ' from '+debugName+': '+chatting('seer'));
		} else {
			chat(sender, ' from '+debugName+': '+chatting('no'));
		}
	}
}
