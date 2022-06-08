debugMsg('Module: builders.js','init');

function groupBuilders(droid){
	if (typeof order === "undefined") order = false;
	var buildersMainLen = groupSize(buildersMain);
	var buildersHuntersLen = groupSize(buildersHunters);

	//распределяем строителей по группам
	if (droid) {

		if (!earlyGame && getNearFreeResources.length > 0 && buildersHuntersLen === 0) {groupAdd(buildersHunters, droid); debugMsg("+....buildersHunters +1",'group'); }
		else if (policy['build'] !== 'rich' && earlyGame && distBetweenTwoPoints_p(base.x, base.y, droid.x, droid.y) < base_range && buildersMainLen < 2) { groupAdd(buildersMain, droid); debugMsg("buildersMain +1",'group'); }
		else if (policy['build'] !== 'rich' && earlyGame && distBetweenTwoPoints_p(base.x, base.y, droid.x, droid.y) > base_range) { groupAdd(buildersHunters, droid); debugMsg("...buildersHunters +1",'group'); }
		else if ((distBetweenTwoPoints_p(base.x, base.y, droid.x, droid.y) > base_range || (earlyGame && buildersHuntersLen < 2)) && buildersMainLen >= 2 && policy['build'] !== 'rich') { groupAdd(buildersHunters, droid); debugMsg("....buildersHunters +1",'group'); }
		//Если основных строителей меньше минимальных, то добавляем новичка в группу основных строителей
		else if (buildersMainLen < minBuilders || factory_ready.length === 0) { groupAdd(buildersMain, droid); debugMsg("buildersMain +1", 'group'); }
		//Если нет строителей-охотников, то добавляем к ним новичка
		else if (buildersHuntersLen < 1) { groupAdd(buildersHunters, droid);  debugMsg(".buildersHunters +1",'group');}
		//Держим такой баланс в группе: строителей на 1 больше чем строителей-охотников
		else if (buildersMainLen < (buildersHuntersLen+1) && buildersMainLen < 5) { groupAdd(buildersMain, droid); debugMsg("buildersMain +1",'group'); }
		else { groupAdd(buildersHunters, droid); debugMsg("..buildersHunters +1",'group'); }
	}
}


//Подсчитываем постройки на базе
var factory, power_gen, resource_extractor, research_lab, hq, cyborg_factory, vtol_factory, rearm_pad, uplink_center, lassat, ccontrol, repfac;
var factory_ready, power_gen_ready, resource_extractor_ready, research_lab_ready, hq_ready, cyborg_factory_ready, vtol_factory_ready, rearm_pad_ready, uplink_center_ready, lassat_ready, ccontrol_ready, repfac_ready;
var se_r=0; //Resource extractor length


function checkBase(){
	factory = enumStruct(me, FACTORY);
	power_gen = enumStruct(me, POWER_GEN);
	resource_extractor = enumStruct(me, RESOURCE_EXTRACTOR);
	research_lab = enumStruct(me, RESEARCH_LAB);
	hq = enumStruct(me, HQ);
	cyborg_factory = enumStruct(me, CYBORG_FACTORY);
	vtol_factory = enumStruct(me, VTOL_FACTORY);
	rearm_pad = enumStruct(me, REARM_PAD);
	uplink_center = enumStruct(me, SAT_UPLINK);
	lassat = enumStruct(me, "A0LasSatCommand"); // LASSAT don't work
	ccontrol = enumStruct(me, COMMAND_CONTROL);
	repfac = enumStruct(me, REPAIR_FACILITY);

	factory_ready = factory.filter((e) => (e.status === BUILT));
	power_gen_ready = power_gen.filter((e) => (e.status === BUILT));
	resource_extractor_ready = resource_extractor.filter((e) => (e.status === BUILT));
	se_r = resource_extractor_ready.length;
	research_lab_ready = research_lab.filter((e) => (e.status === BUILT));
	hq_ready = hq.filter((e) => (e.status === BUILT));
	cyborg_factory_ready = cyborg_factory.filter((e) => (e.status === BUILT));
	vtol_factory_ready = vtol_factory.filter((e) => (e.status === BUILT));
	rearm_pad_ready = rearm_pad.filter((e) => (e.status === BUILT));
	uplink_center_ready = uplink_center.filter((e) => (e.status === BUILT));
	lassat_ready = lassat.filter((e) => (e.status === BUILT));
	ccontrol_ready = ccontrol.filter((e) => (e.status === BUILT));
	repfac_ready = repfac.filter((e) => (e.status === BUILT));


	/*
	debugMsg("checkBase(): factory="+factory_ready.length+"/"+factory.length
	+"; power_gen="+power_gen_ready.length+"/"+power_gen.length
	+"; resource_extractor="+resource_extractor_ready.length+"/"+resource_extractor.length
	+"; research_lab="+research_lab_ready.length+"/"+research_lab.length
	+"; cyborg_factory="+cyborg_factory_ready.length+"/"+cyborg_factory.length
	+"; hq="+hq_ready.length+"/"+hq.length
	+"; vtol_factory="+vtol_factory_ready.length+"/"+vtol_factory.length
	);
	*/
/*
	research_lab.forEach((e, i) => {
		debugMsg("checkBase(): lab["+i+"] status:"+e.status );
	});
*/
}

//Строим базу
function builderBuild(droid, structure, rotation, position){
	if (typeof position === "undefined") position = false;
	var struct;
	switch (structure) {
		case "A0LightFactory":if (enumStruct(me,FACTORY).length >= maxFactories)return false;struct = factory; break;
		case "A0ResearchFacility":if (enumStruct(me,RESEARCH_LAB).length >= maxLabs)return false;struct = research_lab; break;
		case "A0PowerGenerator":if (enumStruct(me,POWER_GEN).length >= maxGenerators)return false;struct = power_gen; break;
		case "A0CommandCentre":struct = hq; break;
		case "A0Sat-linkCentre":struct = uplink_center; break;
		case "A0LasSatCommand":struct = lassat; break;
		case "A0ComDroidControl":struct = ccontrol; break;
		case "A0CyborgFactory":if (enumStruct(me,CYBORG_FACTORY).length >= maxFactoriesCyb)return false;struct = cyborg_factory; break;
		case "A0VTolFactory1":if (enumStruct(me,VTOL_FACTORY).length >= maxFactoriesVTOL)return false;struct = vtol_factory; break;
		case "A0VtolPad":if (enumStruct(me,REARM_PAD).length >= maxPads)return false;struct = rearm_pad; break;
		case "A0RepairCentre3":struct = repfac; break;
//		case "A0ResourceExtractor":struct = resource_extractor; break;
//		default: return false;
	}
	var stop=false;
	//Проверяем, если заданное здание уже кем-либо заложено и строится, просто едем помочь достроить
	if (struct.length > 0) {struct.forEach((obj) => {
//		debugMsg("builderBuild(): name="+obj.name+"; status="+obj.status);
		if (obj.status === BEING_BUILT) { orderDroidObj_p(droid, DORDER_HELPBUILD, obj); stop=true; return true;}
	});}
	if (stop) return true;

	//Строим новое здание
	if (isStructureAvailable(structure, me)) {

		debugMsg('try '+structure, 'builders');

		if (position) var _pos = position;
		else var _pos = base;

		if (structure === "A0RepairCentre3" && !position) {
			if (repfac.length === 0) {
				var target = getUnknownResources();
				target = target.concat(getSeeResources());
				target = sortByDistance(target, base).filter((e) => (
					distBetweenTwoPoints_p(e.x,e.y,base.x,base.y) < base_range && droidCanReach(droid, e.x,e.y)
				));
				_pos = target[0];
			} else {
				_pos = repfac[0];
			}
		}

		debugMsg("droid:"+droid.id+", structure:"+structure+", pos:"+_pos.x+'x'+_pos.y+", try", "builders");
		var pos = pickStructLocation(droid,structure,_pos.x+1,_pos.y+1);
		if (pos && !(typeof pos === "undefined") && (policy['build'] === 'rich' || base_range < 15 || distBetweenTwoPoints_p(pos.x,pos.y,base.x,base.y) < (base_range+10))) {

			var result = orderDroidBuild_p(droid, DORDER_BUILD, structure, pos.x, pos.y, rotation);
			debugMsg("droid:"+droid.id+", structure:"+structure+", pos:"+pos.x+'x'+pos.y+", "+result, "builders");
			if (result) {
				debugMsg("droid:"+droid.id+", structure:"+structure+", pos:"+pos.x+'x'+pos.y+", ok", "builders");
				return true;
			}
			else {
				debugMsg("droid:"+droid.id+", structure:"+structure+", pos:"+pos.x+'x'+pos.y+", fail", "builders");
				return false;
			}
		} else {
			//Перевод базы на хрен знает что..
			var _base = sortByDistance(getSeeResources(), base).filter((e) => (
				distBetweenTwoPoints_p(e.x,e.y,base.x,base.y) > base_range && droidCanReach(droid, e.x,e.y)
			));
			if (_base.length > 0) {
//				debugMsg("WARNING: Не найдено подходящей площадки для постройки "+pos.x+"x"+pos.y+" "+structure+", меняем позицию базы с "+base.x+"x"+base.y+" на "+_base[0].x+"x"+_base[0].y+", dist="
//				+distBetweenTwoPoints_p(base.x,base.y,_base[0].x,_base[0].y), 'builders');
//				if (!release) mark(_base[0].x,_base[0].y);
				base = {x:_base[0].x,y:_base[0].y};
			}/*else {
				debugMsg("WARNING: Не найдено подходящей площадки для постройки "+pos.x+"x"+pos.y+" "+structure+", останов.", 'builders');
			}*/
			return false;
		}
	} else {
//			debugMsg("WARNING: ["+structure+"] - строение не доступно",1);
			return false;
	}
}

/*
// Базовые постройки
const b_factory			= "A0LightFactory";
const b_power			= "A0PowerGenerator";
const b_cc				= "A0CommandCentre";
const b_lab 			= "A0ResearchFacility";
const b_rig				= "A0ResourceExtractor";
const b_cyborg			= "A0CyborgFactory";
const b_oil				= "OilResource";
const b_vtol			= "A0VTolFactory1";
const b_light_defence	= "GuardTower1";

const m_power 			= "A0PowMod1";
const m_factory 		= "A0FacMod1";
const m_lab				= "A0ResearchModule1";

const mr_power 			= "R-Struc-PowerModuleMk1";
const mr_factory 		= "R-Struc-Factory-Module";
const mr_lab			= "R-Struc-Research-Module";
*/

//Главная функция строителей
var builder_targets;
function buildersOrder(order,target) {
	if (!running)return false;
	if (!func_buildersOrder)return false;

	//function controller
	if (func_buildersOrder_trigger <= gameTime) func_buildersOrder_trigger = gameTime + func_buildersOrder_timer;
	else return false;
	queue("buildersOrder", func_buildersOrder_timer);
	debugMsg("func_buildersOrder_timer: "+func_buildersOrder_timer, 'controller');

	allResources = getAllResources();
//	debugMsg('buildersOrder()', 'builders_advanced');

//	if (typeof order === "undefined") order = false;
//	if (typeof target === "undefined") target = false;

	var buildersMainLen = groupSize(buildersMain);
	var buildersHuntersLen = groupSize(buildersHunters);

	if (buildersMainLen === 0 && buildersHuntersLen === 0) return false;

	/*
	if (order === "AA" && AA_defence.length > 0 && target !== false) {
		var _def = AA_defence[Math.floor(Math.random()*Math.min(AA_defence.length, 3))]; //Случайная из 3 последних
		debugMsg("Срочно строим ПВО "+_def+" "+target.x+"x"+target.y, 'builders');
		var _build = 0;
		var pos = pickStructLocation(obj,_def,target.x,target.y);
		if (pos) {
			enumGroup(buildersMain).forEach((obj, iter) => {
				if (builderBusy(obj)) return;
				orderDroidBuild_p(obj, DORDER_BUILD, _def, pos.x, pos.y, 0);
				_build++;
			});
			if (_build > 0) {
				debugMsg(_build+" строителя едут строить ПВО", 'builders');
				return;
			} else {
				AA_queue.push({x:pos.x,y:pos.y});
			}
		}
	}
	*/

	var rnd = Math.floor(Math.random()*4);
	var rotation = 0;
	switch (rnd) {
		case 0:rotation = 0;break;
		case 1:rotation = 90;break;
		case 2:rotation = 180;break;
		case 3:rotation = 270;break;
	}
	checkBase(); // <-- подсчитываем количество строений на базе

//	debugMsg("buildersOrder(): buildersMainLen="+buildersMainLen+"; buildersHuntersLen="+buildersHuntersLen+"; rnd="+rnd+"; rotation="+rotation+"; order="+order, 'builders');
//	if (buildersHuntersLen < 2) need_builder = true;

	builder_targets = [];
	if (resource_extractor.length < maxExtractors || ally.length === 0) {
		builder_targets = builder_targets.concat(enumFeature(me, "OilResource"));
		if (ally.length > 0) builder_targets = filterNearAlly(builder_targets);
//		if (rage === HARD || rage === INSANE) builder_targets = sortByDistance(builder_targets, base);
	}
//	debugMsg("to capture: "+builder_targets.length, 'builders');
//	var oil_free = builder_targets; //для дебага
	var oil_unknown = getUnknownResources();
	builder_targets = builder_targets.concat(oil_unknown);
	if (earlyGame && enemyDist > 100 && policy['build'] !== 'rich' && nf['policy'] !== 'island') builder_targets = filterNearBase(builder_targets);

	builder_targets = builder_targets.concat(enumFeature(me, "OilDrum"));
	builder_targets = builder_targets.concat(enumFeature(me, "Crate"));

	builder_targets = filterInaccessible(builder_targets);

	var oil_enemy = getEnemyResources();

	if (defence.length > 0) builder_targets = builder_targets.concat(oil_enemy);



	//	builder_targets.forEach((e, i) => { debugMsg("#"+i+" "+e.id+" "+e.name+" "+e.type+" "+e.player+" "+e.x+"x"+e.y); } );

	//назначаем задания основным строителям/строим базу
	if (buildersMainLen > 0) {mainBuilders(rotation);} else {
//		debugMsg("Нет строителей в группе buildersMain", 'builders');

		//Если нет основных строителей -И- база под атакой -И- заводы уничтожены или не достроены
		if (!getInfoNear(base.x,base.y,'safe',base_range).value && factory_ready.length === 0) {
			enumDroid(me, DROID_CONSTRUCT).forEach((e) => {groupBuilders(e);}); //Изыскиваем резервы
			if (groupSize(buildersHunters) === 0) {
//				debugMsg("Нет строителей вообще! Каюк!", 'builders');
				//TODO доработать, найти завод киборгов, построить киборга-строителя или попросить помощи у союзника
			} else {
				var _builders = enumGroup(buildersHunters);
				base = {x:_builders[0].x,y:_builders[0].y}; //Меняем место базы на первого строителя (Возможно повезёт)
//				debugMsg("Дислокация базы! "+base.x+"x"+base.y, 'builders');
				queue("buildersOrder", 1000);
			}
		} else {
			var _hunters = enumGroup(buildersHunters);
			if (_hunters.length > 2) {
				_hunters = sortByDistance(_hunters, base, 1);
				groupAdd(buildersMain, _hunters[0]);
//				debugMsg('Hunter --> Builder +1', 'group');
			}
		}
	}

	//Назначаем работу строителям-охотникам
	if (buildersHuntersLen > 0) {
		/*
		if (builder_targets*.length === 0) { // Если нет целей для разведки или захвата ресурсов
			for (const h in hunters) {if (!builderBusy(hunters[h])) {
				if (rigDefence(hunters[h])) continue;
				orderDroidLoc_p(hunters[h],DORDER_MOVE,base.x,base.y);
			}}
			return;
		}
		*/
		var hunters = enumGroup(buildersHunters);
		var problemBuildings = sortByDistance(getProblemBuildings(), base);
		for (const h in hunters) {
			var huntOnDuty = oilHunt(hunters[h]);
			if (huntOnDuty) {
//				debugMsg(hunters[h].id+' oilHunt', 'hunters');
				continue;
			}
//			debugMsg("buildersOrder: Строитель-охотник №"+hunters[h].id+" на службе? "+huntOnDuty, 'builders');
			if (huntOnDuty === false && !builderBusy(hunters[h])) huntOnDuty = rigDefence(hunters[h]);
			if (huntOnDuty) {
//				debugMsg(hunters[h].id+' rigDefence', 'hunters');
				continue;

			}
//			if (huntOnDuty === false) if (distBetweenTwoPoints_p(hunters[h].x,hunters[h].y,base.x,base.y) > 10 && !builderBusy(hunters[h])) {
			if (huntOnDuty === false && !builderBusy(hunters[h])) {
				if (problemBuildings.length > 0) {
//					debugMsg(hunters[h].id+"Help with "+problemBuildings[0].name, 'hunters');
					if (problemBuildings[0].status === BEING_BUILT) {orderDroidObj_p(hunters[h], DORDER_HELPBUILD, problemBuildings[0]);continue;}
					if (problemBuildings[0].health < 99) {orderDroidObj_p(hunters[h], DORDER_REPAIR, problemBuildings[0]);continue;}
					if (problemBuildings.length !== 1)problemBuildings.shift();
					continue;
				}
				if (distBetweenTwoPoints_p(hunters[h].x,hunters[h].y,base.x,base.y) > 10) orderDroidLoc_p(hunters[h],DORDER_MOVE,base.x,base.y);
				continue;
			}
//			debugMsg(hunters[h].id+' WARNING: IDLE HUNTERS!!!', 'hunters');
		}
	}
}


//Функция постройка защиты у ресурса
function rigDefence(obj, nearbase){
	if (typeof nearbase === "undefined") nearbase = false;

//	if (((playerPower(me) < 700 && nf['policy'] !== 'island') || playerPower(me) < 200) && !berserk) {

	if ((playerPower(me) < 700 && nf['policy'] !== 'island') && !berserk) {
		debugMsg('exit low power not rich', 'defence');
		return false;
	}

//	debugMsg("rigDefence(): "+defQueue.length);

	if (!nearbase) defQueue = filterNearBase(defQueue);

	if (defQueue.length === 0) {
//		debugMsg('exit defQueue empty', 'defence');
		return false; //Очередь для постройки защиты
	}

	if (defence.length === 0) {
//		debugMsg('exit no defence avail', 'defence');
		return false; //Количество возможных защитных башен исследовано
	}
	var toBuild = defence[Math.floor(Math.random()*defence.length)];


	defQueue = sortByDistance(defQueue,obj,0);
	if (policy['build'] !== 'rich' && !getInfoNear(defQueue[0].x,defQueue[0].y,'safe').value) {
//		debugMsg('exit not rich and not safe', 'defence');
		defQueue.shift();
//		debugMsg("rigDefence(): Danger, cancel "+defQueue.length);
		return false;
	}

	if (nearbase && distBetweenTwoPoints_p(base.x,base.y,defQueue[0].x,defQueue[0].y) > base_range/2) return false;

	if (!getInfoNear(defQueue[0].x,defQueue[0].y,'safe').value) return false;

//	debugMsg("rigDefence(): Строителем №"+obj.id+" строим "+toBuild+" "+defQueue[0].x+"x"+defQueue[0].y);
	var _def = getInfoNear(defQueue[0].x,defQueue[0].y,'buildDef',5,30000,false);
	if (_def.value && !builderBusy(obj)) {
		debugMsg('try-build: '+defQueue[0].x+'x'+defQueue[0].y+", build: "+_def.x+'x'+_def.y+', '+nearbase, 'defence');
		orderDroidBuild_p(obj, DORDER_BUILD, toBuild, _def.x, _def.y, 0);
		defQueue.shift();
		return true;
	}
/*	var pos = pickStructLocation(obj,toBuild,posRnd(defQueue[0].x, 'x'), posRnd(defQueue[0].y, 'y'));
	if (pos && !builderBusy(obj)) {
		debugMsg('try-build: '+defQueue[0].x+'x'+defQueue[0].y+", build: "+pos.x+'x'+pos.y+', '+nearbase, 'defence');
		orderDroidBuild_p(obj, DORDER_BUILD, toBuild, pos.x, pos.y, 0);
		defQueue.shift();
		return true;
	}
*/
//	debugMsg("rigDefence(): Отмена");
	debugMsg('exit no reason', 'defence');
	return false;
}

//Составляем очередь на постройку защитный сооружений
var defQueue = [];
function defenceQueue(){
	if (!running)return;
	if (defence.length === 0) return;
	var myDefence = enumStruct(me,DEFENSE);
	var onBase = myDefence.filter((e) => (distBetweenTwoPoints_p(base.x,base.y,e.x,e.y) < base_range));
	var myRigs = [];

	if (policy['build'] === 'rich') {

		if (rage === INSANE) {
			myRigs = myRigs.concat(enumGroup(armyRegular).filter((e) => (distBetweenTwoPoints_p(base.x,base.y,e.x,e.y) > (base_range/2))));
		} else {

			if (pointRegular) {
				myRigs = myRigs.concat(pointRegular);
			}
			if (targRegular) {
				myRigs = myRigs.concat(targRegular);
			}
			if (lastImpact) {
				myRigs = myRigs.concat(lastImpact);
			}
		}
	}
	else
	{
		myRigs = myRigs.concat(allResources.filter((e) => (!(distBetweenTwoPoints_p(base.x,base.y,e.x,e.y) < (base_range/2) && onBase.length > 20))));
	//	var myRigs = enumStruct(me,RESOURCE_EXTRACTOR).filter((e) => (!(distBetweenTwoPoints_p(base.x,base.y,e.x,e.y) < (base_range/2) && onBase.length > 20)));
	//	myRigs = myRigs.concat(enumFeature(me, "OilResource")); //Добавляем незанятые


	}

	if (playerPower(me) < 1500) {
		myRigs = myRigs.concat(enumStruct(me, FACTORY));
		myRigs = myRigs.concat(enumStruct(me, CYBORG_FACTORY));
	}
//	var myRigs = enumStruct(me,RESOURCE_EXTRACTOR);
//	var enemyRigs = getEnemyResources();
//	var enQueue = [];
	//Добавляем в очередь все координаты, где наши качалки без защитных сооружений
	if (myRigs.length >= 3 || policy['build'] === 'rich') {
		defQueue = myRigs.filter(
			(e) => {
				if (myDefence.length === 0) return true; //Если защитных сооружений вообще нет, добавляем все координаты всех наших качалок
				if (!getInfoNear(e.x,e.y,'buildDef',5,30000,false).value) return false; //Если не получается построить рядом защиту - запоминаем это на 5 минут и пропускаем
				var defNum = 0;
				for (const i in myDefence) {
					if (distBetweenTwoPoints_p(e.x,e.y,myDefence[i].x,myDefence[i].y) < 7) defNum++; //Если к качалке есть близко на 7 тайлов защита, считаем
					if (rage === EASY && defNum > 0) return false;
					if ((rage === MEDIUM || rage === HARD || berserk) && defNum >= 1) return false; //если к качалке близко есть башня, пропускаем
					if (rage === INSANE && defNum >= 2) return false;
				}
				return true; //Добавляем координаты к очереди
			}
		);
		/*
		var enQueue = enemyRigs.filter(
			(e) => {
				if (myDefence.length === 0) return true; //Если защитных сооружений вообще нет, добавляем все координаты всех наших качалок
				for (const i in myDefence) {
					if (distBetweenTwoPoints_p(e.x,e.y,myDefence[i].x,myDefence[i].y) < 7) return false; //Если к качалке есть близко защита, пропускаем
				}
				return true; //Добавляем координаты к очереди
			}
		);
		*/
		//Тут можно ещё накидать
		//if (enQueue.length > 0) Object.assign(defQueue,enQueue); //Похоже Object.assign не работает тут.
	}
//	debugMsg("defenceQueue(): Защитных башен="+myDefence.length+", отдалённых качалок="+myRigs.length+", типы башен="+defence.length+", к постройке="+defQueue.length, 'builders');
//	defQueue=defQueue.concat(enQueue);
//	debugMsg("defenceQueue(): вражеских="+enQueue.length+", итого="+defQueue.length);
}


//Функция строит защиту ПВО
function AA_build(obj, nearbase){
//	if (typeof nearbase === "undefined") nearbase = false;
	if (AA_defence.length > 0 && AA_queue.length > 0) {
		var _def = AA_defence[Math.floor(Math.random()*Math.min(AA_defence.length, 3))]; //Случайная из 3 последних
		var target = AA_queue.shift();
//		debugMsg("Строим ПВО "+_def+" "+target.x+"x"+target.y, 'builders');
		var pos = pickStructLocation(obj,_def,target.x,target.y);

		if (pos) {
			if (builderBusy(obj)) return;
			orderDroidBuild_p(obj, DORDER_BUILD, _def, pos.x, pos.y, 0);
			return true;
		}
	}
	return false;
}


function oilHunt(obj, nearbase){
	if (typeof nearbase === "undefined") nearbase = false;
	if (builder_targets.length === 0) return false;
	if (builderBusy(obj)) return false;
	var myDefence = enumStruct(me,DEFENSE);
	builder_targets = builder_targets.concat(myDefence.filter((e) => (e.status === BEING_BUILT || e.health < 100))); //Добавляем к целям недостроенные защитные сооружения


	//Если строитель рядом с вражеским ресурсом
	/*
	if (defQueue.length > 0 && defence.length > 0) {
		for (const i in oil_enemy) {if (distBetweenTwoPoints_p(oil_enemy[i].x,oil_enemy[i].y,obj.x,obj.y) <= 15 && !getInfoNear(oil_enemy[i].x,oil_enemy[i].y,'defended').value && getInfoNear(oil_enemy[i].x,oil_enemy[i].y,'safe').value) {
		var toBuild = defence[Math.floor(Math.random()*defence.length)];
		var pos = pickStructLocation(obj,toBuild,oil_enemy[i].x+Math.round(Math.random()*2-1), oil_enemy[i].y+Math.round(Math.random()*2-1));
		if (pos && !builderBusy(obj)) {
			orderDroidBuild_p(obj, DORDER_BUILD, toBuild, pos.x, pos.y, 0);
			debugMsg("oilHunt(): Строим вышку у вражеского ресурса");
			return true;
		}
	}}}
	*/

	if ((rage === HARD || rage === INSANE) && policy['build'] === 'rich') builder_targets = sortByDistance(builder_targets, obj);

	//Если строитель рядом с целью

	var target_range = 7;
	var feature_try = 3;
	if (policy['build'] === 'rich') {
		target_range = 20;
		feature_try = 10;
	}

	for (const i in builder_targets) {
		if (getInfoNear(builder_targets[i].x,builder_targets[i].y,'safe').value) {
			if (distBetweenTwoPoints_p(builder_targets[i].x,builder_targets[i].y,obj.x,obj.y) <= target_range) {
				if (typeof builder_targets[i] === "undefined") { debugMsg("ERROR in oilHunt(): Выход за пределы массива, исправить!", 'error'); break;}
//				debugMsg(getInfoNear(builder_targets[i].x,builder_targets[i].y,'buildRig',0,300000,obj,false,true).value, 'temp');
				if (builder_targets[i].type === FEATURE && getInfoNear(builder_targets[i].x,builder_targets[i].y,'actionTry',0,300000,false,false,true).value < feature_try) {

					if (builder_targets[i].stattype === OIL_DRUM || builder_targets[i].stattype === ARTIFACT) {
						orderDroidObj_p(obj, DORDER_RECOVER, builder_targets[i]);
						builder_targets.splice(i,1);
						return true;
					}

					orderDroidBuild_p(obj,DORDER_BUILD,"A0ResourceExtractor",builder_targets[i].x,builder_targets[i].y);
//					debugMsg("oilHunt(): Захват ресурса строителем №"+obj.id);
					builder_targets.splice(i,1);
					return true;
				} else if (builder_targets[i].type === STRUCTURE && builder_targets[i].stattype === DEFENSE && builder_targets[i].player === me) {
					if (builder_targets[i].status === BEING_BUILT) orderDroidObj_p(obj, DORDER_HELPBUILD, builder_targets[i]);
					else orderDroidObj_p(obj, DORDER_REPAIR, builder_targets[i]);
					builder_targets.splice(i,1);
					return true;
				} else if (defence.length > 0 && builder_targets[i].type === STRUCTURE && builder_targets[i].stattype === RESOURCE_EXTRACTOR
					&& builder_targets[i].player !== me) {

					//Проверяем, вдруг рядом другой строитель уже строит защитную башню

					var def = enumRange(builder_targets[i].x, builder_targets[i].y, 7, ALLIES)
					.filter((e) => (e.type === STRUCTURE && e.stattype === DEFENSE && e.status === BEING_BUILT));
					if (def.length > 0) {
						orderDroidObj_p(obj, DORDER_HELPBUILD, def[0]);
						return true;
					}


					var toBuild = defence[Math.floor(Math.random()*Math.min(defence.length, 3))];
					var pos = pickStructLocation(obj,toBuild,builder_targets[i].x+Math.round(Math.random()*3-1), builder_targets[i].y+Math.round(Math.random()*3-1));
//                    debugMsg(JSON.stringify(pos), 'pos');
					if (pos && !builderBusy(obj) && (!getInfoNear(builder_targets[i].x,builder_targets[i].y,'defended').value || (playerPower(me) > 500 || berserk))) {
						orderDroidBuild_p(obj, DORDER_BUILD, toBuild, pos.x, pos.y, 0);
//						debugMsg("oilHunt(): Строим вышку у вражеского ресурса");
						return true;
					} else {
						builder_targets.splice(i,1);
						return false;
					}
				} else {
//					debugMsg("oilHunt(): Разведка строителем #"+obj.id+" на ближайшую неизвестную "+builder_targets[i].x+"x"+builder_targets[i].y+" "+i+"/"+builder_targets.length+" "+builder_targets[i].name+","+builder_targets[i].type+","+builder_targets[i].player);
					orderDroidLoc_p(obj,DORDER_MOVE,builder_targets[i].x,builder_targets[i].y);
					builder_targets.splice(i,1);
					return true;
				}
			}
		}
	}

	builder_targets = sortByDistance(builder_targets,obj,0,true);


	if (typeof builder_targets === "undefined" || builder_targets.length === 0 || typeof builder_targets[0] === "undefined") return false;


	if (nearbase) if (distBetweenTwoPoints_p(base.x,base.y,builder_targets[0].x,builder_targets[0].y) > (base_range/2)) {
		return false; //Запрещаем основным строителям далеко отходить от базы
	}

	if (policy['build'] === 'rich') if (distBetweenTwoPoints_p(base.x,base.y,builder_targets[0].x,builder_targets[0].y) > base_range) {
		groupAdd(buildersMain, obj);
//		debugMsg('Hunter --> Builder +1', 'group');
		return false;
	}

	if (getInfoNear(builder_targets[0].x,builder_targets[0].y,'safe').value) {
		orderDroidLoc_p(obj,DORDER_MOVE,builder_targets[0].x,builder_targets[0].y); //"A0ResourceExtractor"
//		debugMsg("oilHunt() двигаем строителем #"+obj.id+" к "+builder_targets[0].name+","+builder_targets[0].type+","+builder_targets[0].player+", поз.: "+builder_targets[0].x+"x"+builder_targets[0].y+" "+builder_targets.length);
		builder_targets.shift();
		return true;
	}
	//if (builder_targets.length > 1)
	builder_targets.shift();
	return false;
}

// Проверяем занятость строителей
function builderBusy(builder) {
	//v2.7 test MOVE=BUSY
//	if (builder.order === DORDER_MOVE)
//		return true
	if (builder.order === DORDER_BUILD)
		return true;
	if (builder.order === DORDER_HELPBUILD)
		return true;
	if (builder.order === DORDER_LINEBUILD)
		return true;
	if (builder.order === DORDER_DEMOLISH)
		return true;
	if (builder.order === 21) // DORDER_RECYCLE for 3.1.5 compatible
		return true;
	//	if (builder.order === DORDER_MOVE && getDistance(builder) > 3) return true;
	return false;

}


function builderRecycleWalls(builder){
//	if (builderBusy(obj)) return false; //А зачем? В общем цикле на "занятость" уже есть проверка.
	var myWalls = enumStruct(me, GATE);
	myWalls = myWalls.concat(enumStruct(me, COMMAND_CONTROL));
	if (playerPower(me) < 700) {
		myWalls = myWalls.concat(enumStruct(me, WALL));
//		myWalls = myWalls.concat(enumStruct(me, WALL));
	}
	if (playerPower(me) < 50) myWalls = myWalls.concat(filterOutBase(enumStruct(me, DEFENSE)));
	if (myWalls.length === 0) return false;
	myWalls = sortByDistance(myWalls, builder, 0, true);
//	debugMsg(myWalls[0].id+": "+myWalls[0].x+"x"+myWalls[0].y, 'builders');
	if (myWalls.length === 0) return false;
	if (orderDroidObj_p(builder, DORDER_DEMOLISH, myWalls[0])) return true;
}





/*

function findRig() {
	var myBuilders = [];
	var myMainBuilders = [];
	var myHelpBuilders = [];

	myMainBuilders = enumDroid(me, DROID_CONSTRUCT);
	myHelpBuilders = enumDroid(me, 10);

	var seeResources = getSeeResources();
//	var unkResources = getUnknownResources();

	seeResources = sortByDistance(seeResources,base,null);


	for (let m = 0, len = myMainBuilders.length; m < len; ++m) {
		if (m >= seeResources.length) break;
		if (!builderBusy(myMainBuilders[m])) {
//			if ()
		}
	}
}
*/
