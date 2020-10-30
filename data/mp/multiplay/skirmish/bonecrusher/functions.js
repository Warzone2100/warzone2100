
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
	if(command == 'buildRig'){
		debugMsg('DEBUG: '+x+','+y+','+command+','+time+','+inc, 'temp');
		if(typeof _globalInfoNear[x+'_'+y+'_'+command] !== 'undefined'){ 
			debugMsg((gameTime-_globalInfoNear[x+'_'+y+'_'+command].setTime)+' left', 'temp');
		}
	}*/
	
	if ( typeof _globalInfoNear[x+'_'+y+'_'+command] !== 'undefined'
	&& gameTime < (_globalInfoNear[x+'_'+y+'_'+command].setTime + _globalInfoNear[x+'_'+y+'_'+command].updateIn) ) {
		if(inc){
			_globalInfoNear[x+'_'+y+'_'+command].value++;
		}
		return _globalInfoNear[x+'_'+y+'_'+command];
	}else{
		if(typeof time === 'undefined') time = 30000;
		if(typeof range === 'undefined') range = 7;
		if(typeof cheat === 'undefined') var view = me;
		else if(cheat == true) var view = -1;
//		_globalInfoNear[x+'_'+y+'_'+command] = new Array();
		_globalInfoNear[x+'_'+y+'_'+command] = { setTime: gameTime, updateIn: time };
		
		if(command == 'safe'){
			var danger = new Array();
			for ( var e = 0; e < maxPlayers; ++e ) {
				if ( allianceExistsBetween(me,e) ) continue;
				danger = danger.concat(enumDroid(e, DROID_WEAPON, view));
				danger = danger.concat(enumDroid(e, DROID_CYBORG, view));
				danger = danger.concat(enumStruct(e, DEFENSE, view));
			}
			if ( scavengerPlayer != -1 ) {
				danger = danger.concat(enumDroid(scavengerPlayer, DROID_WEAPON, view));
				danger = danger.concat(enumDroid(scavengerPlayer, DROID_CYBORG, view));
				danger = danger.concat(enumStruct(scavengerPlayer, DEFENSE, view));
			}
			
			for ( var d in danger ) {
				if ( distBetweenTwoPoints_p(x,y,danger[d].x,danger[d].y) < range ) { 
					_globalInfoNear[x+'_'+y+'_'+command].value = false;
					return _globalInfoNear[x+'_'+y+'_'+command]; 
				}
			}
			_globalInfoNear[x+'_'+y+'_'+command].value = true;
			return _globalInfoNear[x+'_'+y+'_'+command];
		}else if(command == 'defended'){
			
			var defenses = enumStruct(me, DEFENSE).filter(function(e){if(e.status == BUILT) return true; return false;});
			for ( var d in defenses ) {
				if ( distBetweenTwoPoints_p(x,y,defenses[d].x,defenses[d].y) < range ) { 
					_globalInfoNear[x+'_'+y+'_'+command].value = true;
					return _globalInfoNear[x+'_'+y+'_'+command];
				}
			}
			_globalInfoNear[x+'_'+y+'_'+command].value = false;
			return _globalInfoNear[x+'_'+y+'_'+command];
			
		}else if(command == 'buildDef'){

			var _builder = enumGroup(buildersHunters);
			if(_builder.length == 0) _builder = enumDroid(me,DROID_CONSTRUCT);
			if(_builder.length == 0){ //Невозможно в данный момент проверить, запоминаем на 10 секунд
				_globalInfoNear[x+'_'+y+'_'+command].updateIn = 10000;
				_globalInfoNear[x+'_'+y+'_'+command].value = false;
				return _globalInfoNear[x+'_'+y+'_'+command];
			}
			var toBuild = defence[Math.floor(Math.random()*defence.length)];
			var pos = pickStructLocation(_builder[0],toBuild,x,y);
			if(!!pos && distBetweenTwoPoints_p(x,y,pos.x,pos.y) < range){
				_globalInfoNear[x+'_'+y+'_'+command].value = true;
			}else{
				_globalInfoNear[x+'_'+y+'_'+command].value = false;
			}
			return _globalInfoNear[x+'_'+y+'_'+command];
		}else if(command == 'buildRig'){
			if(typeof _globalInfoNear[x+'_'+y+'_'+command].value === 'undefined'){
				_globalInfoNear[x+'_'+y+'_'+command].value = 0;
//				debugMsg('getInfoNear set 0','temp');
				
			}
			return _globalInfoNear[x+'_'+y+'_'+command];
		}
	}
}

function _getInfoNear(x,y,command,range,time,obj,cheat,inc){
	
	if(command == 'buildRig'){
		debugMsg('DEBUG: '+x+','+y+','+command+','+time+','+inc, 'temp');
		if(typeof _globalInfoNear[x] !== 'undefined'){ debugMsg('x', 'temp');
			if(typeof _globalInfoNear[x][y] !== 'undefined'){debugMsg('y', 'temp');
				if(typeof _globalInfoNear[x][y][command] !== 'undefined'){
					debugMsg('command', 'temp');
				}
			}
		}
	}

	if ( typeof _globalInfoNear[x] !== 'undefined'
		&& typeof _globalInfoNear[x][y] !== 'undefined' // <--
		&& typeof _globalInfoNear[x][y][command] !== 'undefined'
		&& gameTime < (_globalInfoNear[x][y][command].setTime + _globalInfoNear[x][y][command].updateIn) ) {
//		debugMsg("x="+x+"; y="+y+"; command="+command+"; fast return value="+_globalInfoNear[x][y][command].value+"; timeout="+(_globalInfoNear[x][y][command].setTime+_globalInfoNear[x][y][command].updateIn-gameTime), "getInfoNear");
		if(inc){
//			if(typeof _globalInfoNear[x][y][command].value === 'undefined'){
//				_globalInfoNear[x][y][command].value = 0;
//			}
			_globalInfoNear[x][y][command].value++;
			debugMsg('getInfoNear inc '+_globalInfoNear[x][y][command].value, 'temp');
			
		}
		debugMsg('getInfoNear fast '+command,'temp');
		return _globalInfoNear[x][y][command];
	}else{
		if(typeof time === 'undefined') time = 30000;
		if(typeof range === 'undefined') range = 7;
		if(typeof cheat === 'undefined') var view = me;
		else if(cheat == true) var view = -1;
		_globalInfoNear[x] = new Array();
		_globalInfoNear[x][y] = new Array();
		_globalInfoNear[x][y][command] = new Array();
		_globalInfoNear[x][y][command] = { setTime: gameTime, updateIn: time };

		if(command == 'safe'){
			var danger = new Array();
			for ( var e = 0; e < maxPlayers; ++e ) {
				if ( allianceExistsBetween(me,e) ) continue;
				danger = danger.concat(enumDroid(e, DROID_WEAPON, view));
				danger = danger.concat(enumDroid(e, DROID_CYBORG, view));
				danger = danger.concat(enumStruct(e, DEFENSE, view));
			}
			if ( scavengerPlayer != -1 ) {
				danger = danger.concat(enumDroid(scavengerPlayer, DROID_WEAPON, view));
				danger = danger.concat(enumDroid(scavengerPlayer, DROID_CYBORG, view));
				danger = danger.concat(enumStruct(scavengerPlayer, DEFENSE, view));
			}
			
			for ( var d in danger ) {
				if ( distBetweenTwoPoints_p(x,y,danger[d].x,danger[d].y) < range ) { 
					_globalInfoNear[x][y][command].value = false;
//					debugMsg("x="+x+"; y="+y+"; command="+command+"; setTime="+_globalInfoNear[x][y][command].setTime+"; updateIn="+_globalInfoNear[x][y][command].updateIn+"; value="+_globalInfoNear[x][y][command].value, "getInfoNear");
					return _globalInfoNear[x][y][command]; 
				}
			}
			_globalInfoNear[x][y][command].value = true;
//			debugMsg("x="+x+"; y="+y+"; command="+command+"; setTime="+_globalInfoNear[x][y][command].setTime+"; updateIn="+_globalInfoNear[x][y][command].updateIn+"; value="+_globalInfoNear[x][y][command].value, "getInfoNear");
			return _globalInfoNear[x][y][command];
		}else if(command == 'defended'){
			
			var defenses = enumStruct(me, DEFENSE).filter(function(e){if(e.status == BUILT) return true; return false;});
			for ( var d in defenses ) {
				if ( distBetweenTwoPoints_p(x,y,defenses[d].x,defenses[d].y) < range ) { 
					_globalInfoNear[x][y][command].value = true;
//					debugMsg("x="+x+"; y="+y+"; command="+command+"; setTime="+_globalInfoNear[x][y][command].setTime+"; updateIn="+_globalInfoNear[x][y][command].updateIn+"; value="+_globalInfoNear[x][y][command].value, "getInfoNear");
					return _globalInfoNear[x][y][command];
				}
			}
			_globalInfoNear[x][y][command].value = false;
//			debugMsg("x="+x+"; y="+y+"; command="+command+"; setTime="+_globalInfoNear[x][y][command].setTime+"; updateIn="+_globalInfoNear[x][y][command].updateIn+"; value="+_globalInfoNear[x][y][command].value, "getInfoNear");
			return _globalInfoNear[x][y][command];
			
		}else if(command == 'buildDef'){
			
			var _builder = enumGroup(buildersHunters);
			if(_builder.length == 0) _builder = enumDroid(me,DROID_CONSTRUCT);
			if(_builder.length == 0){ //Невозможно в данный момент проверить, запоминаем на 10 секунд
				_globalInfoNear[x][y][command].updateIn = 10000;
				_globalInfoNear[x][y][command].value = false;
//				debugMsg("x="+x+"; y="+y+"; command="+command+"; setTime="+_globalInfoNear[x][y][command].setTime+"; updateIn="+_globalInfoNear[x][y][command].updateIn+"; value="+_globalInfoNear[x][y][command].value, "getInfoNear");
				return _globalInfoNear[x][y][command];
			}
			var toBuild = defence[Math.floor(Math.random()*defence.length)];
			var pos = pickStructLocation(_builder[0],toBuild,x,y);
			if(!!pos && distBetweenTwoPoints_p(x,y,pos.x,pos.y) < range){
				_globalInfoNear[x][y][command].value = true;
			}else{
				_globalInfoNear[x][y][command].value = false;
			}
//			debugMsg("x="+x+"; y="+y+"; command="+command+"; setTime="+_globalInfoNear[x][y][command].setTime+"; updateIn="+_globalInfoNear[x][y][command].updateIn+"; value="+_globalInfoNear[x][y][command].value, "getInfoNear");
			return _globalInfoNear[x][y][command];
		}else if(command == 'buildRig'){
			if(typeof _globalInfoNear[x][y][command].value === 'undefined'){_globalInfoNear[x][y][command].value = 0;debugMsg('getInfoNear set 0','temp');}
			debugMsg('setTime '+_globalInfoNear[x][y][command].setTime, 'temp');
			debugMsg('updateIn '+_globalInfoNear[x][y][command].updateIn, 'temp');
			return _globalInfoNear[x][y][command];
		}
	}
}



//Проверяем есть ли союзники, не спектаторы ли они
//Выбираем одного для поддержки 3.2+
function checkAlly(){
//	return;
	debugMsg('me='+me, 'ally');
	playerData.forEach( function(data, player) {
		var dist = distBetweenTwoPoints_p(base.x,base.y,startPositions[player].x,startPositions[player].y);
		var ally = allianceExistsBetween(me, player);
		
		if(playerSpectator(player)){
			debugMsg("#"+player+" name="+data.name+", human="+data.isHuman+", ai="+data.isAI
			+", ally=spectator, dist="+dist, "ally");
			return;
		}
		if(playerLoose(player)){
			debugMsg("#"+player+" name=\"Empty Slot\""
			+", ally=empty, dist="+dist, "ally");
			return;
		}
		if(player == me){
			debugMsg("#"+player+" name="+vername+", ai=ofcource"
			+", ally=me, dist=0", "ally");
			return;
		}
		debugMsg("#"+player+" name="+data.name+", human="+data.isHuman+", ai="+data.isAI
		+", ally="+ally+", dist="+dist, "ally");
		
		//Если враг
		if(!ally){
			if(Math.floor(dist/2) < base_range){
				debugMsg('base_range снижено: '+base_range+'->'+Math.floor(dist/2), 'init');
				base_range = Math.floor(dist/2);
			}
			if(!enemyDist || enemyDist > dist) enemyDist = dist;
		}
		
	});
//	playerSpectator();
}

function checkDonate(obj){
	if(!getInfoNear(base.x,base.y,'safe',base_range).value) return false;
	if(groupSize(armyPartisans) < (minPartisans/2)) return false;
//	if()
//	debugMsg(obj.name+" to "+armyToPlayer, 'donate');
	if(obj.droidType == DROID_WEAPON && armyToPlayer !== false){
//		debugMsg(obj.name+" -> "+armyToPlayer, 'donate');
		donateObject(obj, armyToPlayer);
		return true;
	}
	return false;
}

function groupArmy(droid, type){
	
	if ( typeof type === "undefined" ) type = false;
	
	if(type == 'jammer'){
//		debugMsg("armyJammers +1", 'group');
		groupAddDroid(armyJammers, droid);
		return;
	}
	
	if(droid.droidType == DROID_REPAIR){
//		debugMsg("armyFixers +1", 'group');
		groupAddDroid(armyFixers, droid);
		return;
	}
	
	//Забираем киборгов под общее коммандование
	if(droid.droidType == DROID_CYBORG && policy['build'] == 'rich' && (difficulty == HARD || difficulty == INSANE)){
//		debugMsg("armyRegular +1", 'group');
		groupAddDroid(armyRegular, droid);
		return;
	}
	
	//Если армия партизан меньше 7 -ИЛИ- нет среднего Body -ИЛИ- основная армия достигла лимитов
//	if(groupSize(armyPartisans) < 7 || !getResearch("R-Vehicle-Body05").done || groupSize(armyRegular) >= maxRegular ){
	if(groupSize(armyPartisans) <= minPartisans || groupSize(armyRegular) >= maxRegular){
//		debugMsg("armyPartisans +1", 'group');
		groupAddDroid(armyPartisans, droid);
	}else{
		
		if(droid.droidType == DROID_CYBORG || groupSize(armyCyborgs) == 0){
//			debugMsg("armyCyborgs +1", 'group');
			groupAddDroid(armyCyborgs, droid);
			return;
		}
		
//		debugMsg("armyRegular +1", 'group');
		groupAddDroid(armyRegular, droid);
	}
	
	//Перегрупировка
	if(groupSize(armyPartisans) < minPartisans && groupSize(armyRegular) > 1 && !(policy['build'] == 'rich' && (difficulty == HARD || difficulty == INSANE))){
		var regroup = enumGroup(armyRegular);
		regroup.forEach(function(e){
//			debugMsg("armyRegular --> armyPartisans +1", 'group');
			groupAddDroid(armyPartisans, e);
		});
	}
	
}


function stats(){
	if(!running)return;
//	if(release) return;
	
	var _rigs = enumStruct(me, "A0ResourceExtractor").filter(function(e){if(e.status==BUILT)return true;return false;}).length;
	var _gens = enumStruct(me, "A0PowerGenerator").filter(function(e){if(e.status==BUILT)return true;return false;}).length*4;
	if(_gens > _rigs) _gens = _rigs;
	
	debugMsg("---===---", 'stats');
	debugMsg("Power: "+playerPower(me)+"; rigs="+_gens+"/"+_rigs+"; free="+enumFeature(me, "OilResource").length+"/"+allResources.length+"; unknown="+getUnknownResources().length+"; enemy="+getEnemyResources().length, 'stats');
	debugMsg("Hold: me="+Math.round(_rigs*100/allResources.length)+"%; enemy~="+Math.round((getEnemyResources().length+getUnknownResources().length)*100/allResources.length)+"%", 'stats');
	debugMsg("Army: "+(groupSize(armyPartisans)+groupSize(armyRegular)+groupSize(armyCyborgs)+groupSize(VTOLAttacker))+"; Partisans="+groupSize(armyPartisans)+"; Regular="+groupSize(armyRegular)+"; Borgs="+groupSize(armyCyborgs)+"; VTOL="+groupSize(VTOLAttacker), 'stats');
//	debugMsg("Units: Builders="+groupSize(buildersMain)+"; Hunters="+groupSize(buildersHunters)+"; Repair="+groupSize(armyFixers)+"; Jammers="+groupSize(armyJammers)+"; targets="+builder_targets.length, 'stats');
	debugMsg("Research: avail="+avail_research.length+"; Ways="+research_way.length+"; way="+rWay, 'stats');
	debugMsg("Weapons: "+guns.length+"; known="+avail_guns.length+"; cyborgs="+avail_cyborgs.length+"; vtol="+avail_vtols.length, 'stats');
	debugMsg("Base: "+base.x+"x"+base.y+", r="+base_range+", safe="+getInfoNear(base.x,base.y,'safe',base_range).value+"; defense="+enumStruct(me, DEFENSE).length+"; labs="+enumStruct(me, RESEARCH_LAB).length+"; factory="+enumStruct(me, FACTORY).length+"; cyb_factory="+enumStruct(me, CYBORG_FACTORY).length+"; vtol="+enumStruct(me, VTOL_FACTORY).length+", full="+isFullBase(me), 'stats');
	debugMsg("Bodies: light="+light_bodies.length+"; medium="+medium_bodies.length+"; heavy="+heavy_bodies.length, 'stats');
	debugMsg("Misc: enemyDist="+enemyDist+"; barrels="+enumFeature(ALL_PLAYERS, "").filter(function(e){if(e.player == 99)return true;return false;}).length
		+"; known defence="+defence.length+"; known AA="+AA_defence.length+"; AA_queue="+AA_queue.length, 'stats');
	debugMsg("Ally="+ally.length+"; enemy="+enemy.length, 'stats');
	debugMsg("Produce: "+produceTrigger.length, 'stats');
	debugMsg('defQueue: '+defQueue.length, 'stats');

	debugMsg("_globalInfoNear level1 = "+Object.keys(_globalInfoNear).length, 'stats');

// 	var level1=_globalInfoNear.length;
// 	var level2=0;
// 	var level3=0;
// 	var level4=0;
// 	for (var x in _globalInfoNear){
// 	debugMsg("x = "+Object.keys(_globalInfoNear), 'stats');
// 		level2+=_globalInfoNear[x].length;
// 		debugMsg("y = "+Object.keys(_globalInfoNear[x]), 'stats');
// 		for (var y in _globalInfoNear[x]){
// 			level3+=1;
// 			debugMsg("z = "+Object.keys(_globalInfoNear[x][y]), 'stats');
// 			for (var z in _globalInfoNear[x][y]){
// 				level4+=3;
// 				debugMsg("keys = "+Object.keys(_globalInfoNear[x][y][z]), 'stats');
// 				
// 			}
// 		}
// 	}
// 	debugMsg("_globalInfoNear level1 = "+level1, 'stats');
// 	debugMsg("_globalInfoNear level2 = "+level2, 'stats');
// 	debugMsg("_globalInfoNear level3 = "+level3, 'stats');
// 	debugMsg("_globalInfoNear level4 = "+level4, 'stats');
	
//	enumFeature(ALL_PLAYERS, "").filter(function(e){if(e.armour == 5 && e.thermal == 5 && !e.damageable && e.health == 100 && e.player==12)return true;return false;})
//	.forEach(function(e){
//		debugMsg(e.name+' '+e.x+'x'+e.y+'; type='+e.type+'; id='+e.id+'; player='+e.player+'; selected='+e.selected+'; health='+e.health+'; armour='+e.armour+'; thermal='+e.thermal+'; damageable='+e.damageable, 'stats');
//	});
}

//Функция определяет подвергается ли ремонту наша цель
//Если да, возвращяем объект, кто ремонтирует нашу цель
function isBeingRepaired(who){
	if ( typeof who === "undefined" ) {
//		debugMsg("Атакующий неизвестен",5);
		return false;
	}

	if ( allianceExistsBetween(me, who.player) ) {
//		debugMsg("Атакующий мой союзник, прощаю",5);
		return false;
	}

	switch ( who.type ) {
		case DROID: {
//			debugMsg("Нас атаковал вражеский дроид ["+who.player+"]",5);
//TODO
//			Тут нужно доработать
			return false;

		}
		case STRUCTURE: {
//			debugMsg("Нас атакует вражесая башня ["+who.player+"]",5);
			var droids = enumDroid(who.player,DROID_CONSTRUCT,me);
			if ( droids.length != 0 ) {
				for ( var i in droids ) {
					if ( distBetweenTwoPoints_p(who.x,who.y,droids[i].x,droids[i].y) <= 3 ) {
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

//Функция предерживается приоритетов исследований
//и ровномерно распределяет по свободным лабораториям
//и должна вызыватся в 3-х случаях (не в цикле)
//1. При старте игры
//2. При постройке лабаротории
//3. При завершении исследования
function doResearch(){
	if(!running)return;
//	debugMsg("doResearch()", 'research_advance');
	//	debugMsg(getInfoNear(base.x,base.y,'safe',base_range).value+" && "+playerPower(me)+"<300 && "+avail_guns.length+"!=0", 'research_advance');
	if(!getInfoNear(base.x,base.y,'safe',base_range).value && !(playerPower(me) > 300 || berserk) && avail_guns.length != 0) return;

	
	var avail_research = enumResearch().filter(function(e){
		//		debugMsg(e.name+' - '+e.started+' - '+e.done, 'research_advance');
		if(e.started)return false;return true;
	});
		
	if ( research_way.length == 0 || avail_research.length == 0 ) {
//		debugMsg("doResearch: Исследовательские пути завершены!!! Останов.", 'research_advance');
		return;
	}

	if ( research_way.length < 5 ){
		var rnd = Math.floor(Math.random()*avail_research.length);
		var _research = avail_research[rnd].name;
//		debugMsg(_research, 'temp');
		research_way.push([_research]);
//		debugMsg("doResearch: Исследовательские пути ("+research_way.length+") подходят к концу! Добавляем рандом. \""+research_name[_research]+"\" ["+_research+"]", 'research_advance');
	}


	var labs = enumStruct(me,RESEARCH_LAB);
	if ( typeof _r === "undefined" ) _r = 0;
	var _busy = 0;

	var _last_r = research_way[_r][research_way[_r].length-1];
	var _way = getResearch(_last_r);
	if(!_way) return;
		
	if (_way.done == true ) {
		//		debugMsg("doResearch: Путей "+research_way.length+", путь "+_r+" завершён", 'research_advance');
		research_way.splice(_r,1);
		//		debugMsg("doResearch: Осталось путей "+research_way.length, 'research_advance');
		_r=0;
		if ( research_way.length == 0 ) {
//			debugMsg("doResearch: Исследовательские пути завершены! Останов.", 'research_advance');
			return;
		}
	}

	//Если меньше 8 нефтевышек, и меньше 1000 денег, и уже запущенны 3 лабы - выход
// 	if(countStruct('A0ResourceExtractor', me) < 8 && playerPower(me) < 1000 && enumStruct(me, RESEARCH_LAB).filter(function(e){if(!structureIdle(e)&&e.status==BUILT)return true;return false;}).length >= 3) return;
// 	if(countStruct('A0ResourceExtractor', me) < 5 && playerPower(me) < 500 && enumStruct(me, RESEARCH_LAB).filter(function(e){if(!structureIdle(e)&&e.status==BUILT)return true;return false;}).length >= 2) return;
// 	if(countStruct('A0ResourceExtractor', me) <= 3 && playerPower(me) < 300 && enumStruct(me, RESEARCH_LAB).filter(function(e){if(!structureIdle(e)&&e.status==BUILT)return true;return false;}).length >= 1) return;
	
	for ( var l in labs ){
		
		if(policy['build'] != 'rich'){
			if(countStruct('A0ResourceExtractor', me) < 8 && !(playerPower(me) > 700 || berserk) && _busy >= 3) break;
			if(countStruct('A0ResourceExtractor', me) < 5 && !(playerPower(me) > 500 || berserk) && _busy >= 2) break;
			if(countStruct('A0ResourceExtractor', me) < 3 && !(playerPower(me) > 300 || berserk) && _busy >= 1) break;
		}
		if( (labs[l].status == BUILT) && structureIdle(labs[l]) ){
//			debugMsg("Лаборатория("+labs[l].id+")["+l+"] исследует путь "+_r, 'research_advance');
			pursueResearch(labs[l], research_way[_r]);
		}else{
//			debugMsg("Лаборатория("+labs[l].id+")["+l+"] занята", 'research_advance');
			_busy++;
		}
	}

	if ( _r == research_way.length-1 ) {
		_r = 0;
//		debugMsg("doResearch: Все исследования запущены, останов.", 'research_advance');
	} else if (_busy == labs.length ) {
//		debugMsg("doResearch: Все все лаборатории заняты, останов.", 'research_advance');
		_r = 0;
	} else {
		_r++;
//		debugMsg("doResearch: Планировка проверки занятости лабораторий...", 'research_advance');
		queue("doResearch", 1000);
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
	if ( typeof reach === "undefined" ) reach = false;
	if ( typeof num === "undefined" || num == null || num == false) num = 0;
	else if ( arr.length == 1 ) num = 1;

	if ( num == 1 ) {

		var b = Infinity;
		var c = new Array();
		for ( var i in arr ) {
			if(reach)if(!droidCanReach(obj, arr[i].x, arr[i].y))continue;
			var a = distBetweenTwoPoints_p( obj.x, obj.y, arr[i].x, arr[i].y );
			if ( a < b ) {
				b = a;
				c[0] = arr[i];
			}
		}
		return c;
	}

	if ( num != 1 ) {
		arr.sort( function(a,b){
			if( distBetweenTwoPoints_p( obj.x, obj.y, a.x, a.y ) < distBetweenTwoPoints_p( obj.x, obj.y, b.x, b.y ) ) return -1;
			if( distBetweenTwoPoints_p( obj.x, obj.y, a.x, a.y ) > distBetweenTwoPoints_p( obj.x, obj.y, b.x, b.y ) ) return 1;
			return 0;
		});
	}

	if(reach){arr = arr.filter( function(e){
		if(!droidCanReach(obj,e.x,e.y)) return false;
		return true;
	});}
	
	if ( num == 0 ) return arr;

	if ( num >= arr.length ) num = (arr.length-1);
	
	return arr.slice(0,num);

}

//Сортировка объектов по проценту жизней
function sortByHealth(arr){
	if ( arr.length > 1 ) arr.sort( function (a,b) {
		if(a.health < b.health) return -1;
		if(a.health > b.health) return 1;
		return 0;
	});
	return arr;
}

function checkProcess(){
	if(!running)return;
	if(Math.floor(gameTime / 1000) < 300) return;
	
	if(playerLoose(me)){
		gameStop("loose");
	}
	
	for ( var plally in bc_ally ){
		if(bc_ally[plally] == me) continue;

		if(getInfoNear(base.x,base.y,'safe',base_range).value){
			
			if(playerPower(bc_ally[plally]) < 100 && playerPower(me) > 100){
				var _pow = Math.floor(playerPower(me)/2);
				donatePower(_pow, bc_ally[plally]);
				debugMsg("Send "+_pow+" power to "+bc_ally[plally], 'ally');
			}

			if(enumDroid(bc_ally[plally], DROID_CONSTRUCT).length < 3){
				if(groupSize(buildersMain) >= 2){
					var truck = enumGroup(buildersMain);
					if(truck.length != 0){
						donateObject(truck[0], bc_ally[plally]);
						debugMsg("Send builder["+truck[0].id+"] from "+me+" to "+bc_ally[plally], 'ally');
						break;
					} else { debugMsg("No more builders from "+me+" to "+bc_ally[plally], 'ally');}
				}

				if(groupSize(buildersHunters) > 1){
					var truck = enumGroup(buildersHunters);
					if(truck.length != 0){
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
	
	
	if(condition == 'loose'){
		debugMsg("I guess, i'm loose.. Give up", 'end');
		if(running){
			playerData.forEach( function(data, player) {
				chat(player, ' from '+debugName+': '+chatting('loose'));
			});
		}
		
	}else if(condition == 'kick'){
		debugMsg("KICKED", 'end');
		if(running){
			playerData.forEach( function(data, player) {
				chat(player, ' from '+debugName+': '+chatting('kick'));
			});
		}
	}
		running = false;
}

function playerLoose(player){
	var loose = false;
	if(enumStruct(player,"A0LightFactory").length == 0
		&& enumDroid(player, DROID_CONSTRUCT).length == 0
		&& enumStruct(player,"A0CyborgFactory").length == 0
		&& enumDroid(player, 10).length == 0) loose = true;
	return loose;
}

function playerSpectator(player){
	var loose = false;
	if( ( enumStruct(player, "A0Sat-linkCentre").length == 1 || enumStruct(player, "A0CommandCentre").length == 1 )
		&& enumStruct(player,"A0LightFactory").length == 0
		&& enumStruct(player,"A0CyborgFactory").length == 0
		&& enumDroid(player, 10).length == 0) loose = true;
	return loose;
}

//функция отфильтровывает объекты, которые находяться близко
//к "живым" союзникам, полезно для отказа от захвата ресурсов союзника
function filterNearAlly(obj){
	for ( var p = 0; p < maxPlayers; ++p ){
		if ( p == me ) continue; //Выкидываем себя
		if ( !allianceExistsBetween(me,p) ) continue; //Выкидываем вражеские
//		if ( playerLoose(p) ) continue; //Пропускаем проигравших
		if ( playerSpectator(p) ) continue;
		
		//Если союзнику доступно 40 вышек, не уступаем ему свободную.
		if(policy['build'] == 'rich' && difficulty > 1){
			var allyRes = enumFeature(ALL_PLAYERS, "OilResource");
			allyRes = allyRes.filter(function(e){if(distBetweenTwoPoints_p(startPositions[p].x,startPositions[p].y,e.x,e.y) < base_range) return true; return false;});
			allyRes = allyRes.concat(enumStruct(p, "A0ResourceExtractor").filter(function(e){if(distBetweenTwoPoints_p(startPositions[p].x,startPositions[p].y,e.x,e.y) < base_range) return true; return false;}));
			if(allyRes.length >= 40) continue;
		}
		
		if ( distBetweenTwoPoints_p(base.x,base.y,startPositions[p].x,startPositions[p].y) < base_range ) continue; //Если союзник внутри радиуса нашей базы, вышки забираем
		obj = obj.filter(function(e){if(distBetweenTwoPoints_p(e.x,e.y,startPositions[p].x,startPositions[p].y) < base_range )return false; return true;})
	}
	return obj;
}

//Функция отфильтровывает объекты внутри радиуса базы
function filterNearBase(obj){
	return obj.filter(function(e){
		if( distBetweenTwoPoints_p(base.x,base.y, e.x, e.y) > base_range) return true; return false;
	});
}

//функция отфильтровывает недостежимые объекты
function filterInaccessible(obj){
	return obj.filter(function(e){
		//Если попыток постройки больше 3, отфильтровываем их
		if(getInfoNear(e.x,e.y,'buildRig',0,300000,false,false,false).value > 3)return false;return true;
	});
}

//TODO need to fix logic, sometimes it's buggy
//it's triggered in middle of battlefield,
//and send help to random location.. don't known why..
function getEnemyNearAlly(){
//	return []; // <-- disable this funtion
	var targ = [];
	var enemy = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		targ = targ.concat(enumDroid(e, DROID_WEAPON, me));
	}
	if(scavengers == true) {
		targ = targ.concat(enumDroid(scavengerPlayer, DROID_WEAPON, me));
	}
	
	for ( var p = 0; p < maxPlayers; ++p ) {
		if ( p == me ) continue;
		if ( !allianceExistsBetween(me,p) ) continue;
//		if ( playerLoose(p) ) continue; //Пропускаем проигравших
		if ( playerSpectator(p) ) continue;
//		enemy = enemy.concat(targ.filter(function(e){if(distBetweenTwoPoints_p(e.x,e.y,startPositions[p].x,startPositions[p].y) < (base_range/2) ){
		enemy = enemy.concat(targ.filter(function(e){if(distBetweenTwoPoints_p(e.x,e.y,startPositions[p].x,startPositions[p].y) < base_range ){
//			debugMsg("TRUE name="+e.name+"; id="+e.id+"; dist="+distBetweenTwoPoints_p(e.x,e.y,startPositions[p].x,startPositions[p].y)+"<"+(base_range/2)+"; player="+p,'temp');
			return true;
		}
//		debugMsg("FALSE name="+e.name+"; id="+e.id+"; dist="+distBetweenTwoPoints_p(e.x,e.y,startPositions[p].x,startPositions[p].y)+"<"+(base_range/2)+"; player="+p, 'temp');
		return false;}));
	}
	
	return enemy;
}

function getEnemyNearPos(x,y,r){
	if(typeof r === 'undefined') r = 7;
	var targ = [];
	var enemy = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		if ( playerSpectator(e) ) continue;
		if ( e == me ) continue;
		targ = targ.concat(enumDroid(e, DROID_WEAPON, me));
	}
	if(scavengers == true) {
		targ = targ.concat(enumDroid(scavengerPlayer, DROID_WEAPON, me));
	}
	
	enemy = enemy.concat(targ.filter(function(e){if(distBetweenTwoPoints_p(e.x,e.y,x,y) < r )return true;return false;}));
	
	return enemy;
	
}

function getAllyArmy(){
	var army = [];
	
	for ( var p = 0; p < maxPlayers; ++p ) {
		if ( p == me ) continue;
		if ( !allianceExistsBetween(me,p) ) continue;
		if ( playerSpectator(p) ) continue;
		
		army = army.concat(enumDroid(p, DROID_WEAPON));
		
	}
	
	return army;
}


//Функция возвращяет вышки, о которых в данный момент не известно ничего
//Просто сравниваем два массива объектов и фильтруем в третий
function getUnknownResources(){
	var iSee = getSeeResources();
	if ( iSee.length == 0 ) return allResources;
	var notSee = allResources.filter(function (value) {
		for ( var i in iSee ) {
			if ( value.x == iSee[i].x && value.y == iSee[i].y ) return false;
		}
		return true;
	});
	return notSee;
}


//Функция возвращает все видимые ресурсы, свободные, свои и занятые кем либо
function getSeeResources(){
	var iSee = new Array();
	iSee = iSee.concat(enumFeature(me, "OilResource"));
	for ( var e = 0; e < maxPlayers; ++e ){
//		if ( !allianceExistsBetween(me,e) ) continue; //Выкидываем вражеские
		iSee = iSee.concat(enumStruct(e, RESOURCE_EXTRACTOR, me));
	}

	return iSee;
	
}

function getProblemBuildings(){
	var targ=[];
	for ( var p = 0; p < maxPlayers; ++p ){
		if ( !allianceExistsBetween(me,p) ) continue; //Выкидываем вражеские
		if ( playerSpectator(p) ) continue; //Пропускаем неиграющих
		targ = targ.concat(enumStruct(p).filter(function(e){if(e.status == BEING_BUILT || e.health < 99)return true;return false;}));
	}
	return targ;
}

function getEnemyFactories(){
	var targ = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		targ = targ.concat(enumStruct(e, FACTORY, me));
		targ = targ.concat(enumStruct(e, CYBORG_FACTORY, me));
	}
	if(scavengers == true) {
		targ = targ.concat(enumStruct(scavengerPlayer, FACTORY, me));
		targ = targ.concat(enumStruct(scavengerPlayer, CYBORG_FACTORY, me));
	}
	return targ;
}

function getEnemyFactoriesVTOL(){
	var targ = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		targ = targ.concat(enumStruct(e, VTOL_FACTORY, me));
	}
	return targ;
}

function getEnemyPads(){
	var targ = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		targ = targ.concat(enumStruct(e, REARM_PAD, me));
	}
	return targ;
}


function getEnemyNearBase(){
	var targ = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		targ = targ.concat(enumDroid(e, DROID_ANY, me));
		targ = targ.concat(enumStruct(e, DEFENSE, me));
	}
	if(scavengers == true) {
		targ = targ.concat(enumStruct(scavengerPlayer, DROID_ANY, me));
		targ = targ.concat(enumStruct(scavengerPlayer, DEFENSE, me));
	}
	return targ.filter(function(e){if(distBetweenTwoPoints_p(e.x,e.y,base.x,base.y) < base_range && !isFixVTOL(e))return true; return false;});
}
function getEnemyCloseBase(){
	var targ = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		targ = targ.concat(enumDroid(e, DROID_ANY, me));
	}
	if(scavengers == true) {
		targ = targ.concat(enumStruct(scavengerPlayer, DROID_ANY, me));
	}
	return targ.filter(function(e){if(distBetweenTwoPoints_p(e.x,e.y,base.x,base.y) < (base_range/2) && !isFixVTOL(e))return true; return false;});
}

function getOurDefences(){
	var targ = [];
	for ( var a in ally ){
		targ = targ.concat(enumStruct(a, DEFENSE).filter(function(e){if(e.status == 1)return true; return false;}));
	}
	targ = targ.concat(enumStruct(me, DEFENSE).filter(function(e){if(e.status == 1)return true; return false;}));
	return targ;
}

function getNearFreeResources(pos){
	return enumFeature(ALL_PLAYERS, "OilResource").filter(function(e){if(distBetweenTwoPoints_p(pos.x,pos.y,e.x,e.y) < base_range) return true; return false;});
}

function getNumEnemies(){
	var enemies = 0;
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		if ( playerSpectator(e) ) continue;
		if ( playerLoose(e) ) continue;
		if ( e == me ) continue;
		enemies++;
	}
	return enemies;
}

function getIsMultiplayer(){
	var humans = 0;
	for ( var e = 0; e < maxPlayers; ++e ) {
		if(playerData[e].isHuman) humans++;
	}
	if(humans > 1) return true;
	return false;
}

function isHumanAlly(){
	for ( var e = 0; e < maxPlayers; ++e ) {
		if(playerData[e].isHuman && allianceExistsBetween(me, e)) return true;
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
	if(fullBaseTrigger > gameTime){
//		debugMsg('fast return: '+fullBase, 'base');
		return fullBase;
	}
	fullBaseTrigger = gameTime + fullBaseTimer;
//	debugMsg("checkFullBase", 'base');
	var obj = enumStruct(player, POWER_GEN, me);
	if(obj.length < 10) {fullBase = false; return false;}
//	debugMsg("powergen check", 'base');
	obj = obj.filter(function(o){if(o.status == BUILT && o.modules == 1)return true;return false;});
	if(obj.length < 10) {fullBase = false; return false;}
//	debugMsg("powergen double check", 'base');
	obj = enumStruct(player, RESEARCH_LAB, me);
	if(obj.length < 5) {fullBase = false; return false;}
//	debugMsg("labs check", 'base');
	obj = obj.filter(function(o){if(o.status == BUILT && o.modules == 1)return true;return false;});
	if(obj.length < 5) {fullBase = false; return false;}
//	debugMsg("labs double check", 'base');
	obj = enumStruct(player, CYBORG_FACTORY, me);
	if(obj.length < 5) {fullBase = false; return false;}
//	debugMsg("cyb check", 'base');
	obj = obj.filter(function(o){if(o.status == BUILT)return true;return false;});
	if(obj.length < 5) {fullBase = false; return false;}
//	debugMsg("cyb double check", 'base');
	obj = enumStruct(player, FACTORY, me);
	if(obj.length < 5) {fullBase = false; return false;}
//	debugMsg("fact check", 'base');
	obj = obj.filter(function(o){if(o.status == BUILT && o.modules == 2)return true;return false;});
	if(obj.length < 5) {fullBase = false; return false;}
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
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		if ( playerSpectator(e) ) continue;
		if ( playerLoose(e) ) continue;
		targ = targ.concat(startPositions[e]);
	}
	return targ;
}

function getEnemyBuilders(){
	var targ = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		targ = targ.concat(enumDroid(e, DROID_CONSTRUCT, me));
//		targ = targ.concat(enumDroid(e, 10, me)); // Киборг-строитель
	}
//	return targ.filter(function(e){if(distBetweenTwoPoints_p(e.x,e.y,base.x,base.y) < base_range)return true; return false;});
	return targ;
}

function getEnemyWarriors(){
	var targ = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		targ = targ.concat(enumDroid(e, DROID_WEAPON, me));
		targ = targ.concat(enumDroid(e, DROID_CYBORG, me));
	}
	return targ;
}

function getEnemyDefences(){
	var targ = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		targ = targ.concat(enumStruct(e, DEFENSE, me));
	}
	if(scavengers == true) {
		targ = targ.concat(enumStruct(scavengerPlayer, DEFENSE, me));
		targ = targ.concat(enumStruct(scavengerPlayer, WALL, me));
	}
	return targ;
}

function getEnemyStructures(){
	var targ = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( e == me ) continue;
		if ( allianceExistsBetween(me,e) ) continue;
		if ( playerSpectator(e) ) continue;
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
	if(scavengers == true) {
		targ = targ.concat(enumStruct(scavengerPlayer, DEFENSE, me));
		targ = targ.concat(enumStruct(scavengerPlayer, WALL, me));
	}
	return targ;
}

function getEnemyWalls(){
	var targ = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		targ = targ.concat(enumStruct(e, WALL, me));
	}
	if(scavengers == true) {
		targ = targ.concat(enumStruct(scavengerPlayer, WALL, me));
	}
	return targ;
}

//Функция возвращает все видимые вражеские ресурсы
function getEnemyResources(){
	var enemyRigs = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		var tmp = enumStruct(e, RESOURCE_EXTRACTOR, me);
		enemyRigs = enemyRigs.concat(tmp);
	}
	if(scavengers == true) enemyRigs = enemyRigs.concat(enumStruct(scavengerPlayer, RESOURCE_EXTRACTOR, me));
/*
	playerData.forEach( function(player) {
		if ( !allianceExistsBetween(me, player) ) { // enemy player
			enemyRigs = enemyStructs.concat(enumStruct(player, RESOURCE_EXTRACTOR, me));
		}
	} );
*/
	return enemyRigs;
}

//Возвращает строителей, инженеров, заводы и киборг-заводы
function getEnemyProduction(){
	var targ = [];
	for ( var e = 0; e < maxPlayers; ++e ) {
		if ( allianceExistsBetween(me,e) ) continue;
		targ = targ.concat(enumStruct(e, RESOURCE_EXTRACTOR, me));
		targ = targ.concat(enumDroid(e, DROID_CONSTRUCT, me));
		targ = targ.concat(enumDroid(e, 10, me)); // Киборг-строитель
		targ = targ.concat(enumStruct(e, FACTORY, me));
		targ = targ.concat(enumStruct(e, CYBORG_FACTORY, me));
	}
	if(scavengers == true) {
		targ = targ.concat(enumStruct(scavengerPlayer, RESOURCE_EXTRACTOR, me));
		targ = targ.concat(enumDroid(scavengerPlayer, DROID_CONSTRUCT, me));
		targ = targ.concat(enumDroid(scavengerPlayer, 10, me)); // Киборг-строитель
		targ = targ.concat(enumStruct(scavengerPlayer, FACTORY, me));
		targ = targ.concat(enumStruct(scavengerPlayer, CYBORG_FACTORY, me));
	}
	return targ;
}

function fixResearchWay(way){
	if ( typeof way === "undefined" ) return false;
	if(!(way instanceof Array)) return false;
//	debugMsg('Check tech '+way.length, 'research');
	var _out = [];
	
	for(var i in way){
//		debugMsg('Check: '+way[i], 'research');
		var _res = getResearch(way[i]);
		if(_res == null){
			debugMsg('Unknown research "'+way[i]+'" - ignored', 'error');
			continue;
		}
		_out.push(way[i]);
	}
	
	debugMsg('Checked research way length='+way.length+', returned='+_out.length, 'research');
	return _out;
}

function addPrimaryWay(){
	if ( typeof research_primary === "undefined" ) return false;
	if(!(research_primary instanceof Array)) return false;
	if(researchStrategy == "Smudged"){
		research_primary.reverse();
		for(var i in research_primary){
			research_way.unshift([research_primary[i]]);
		}
		debugMsg("research_primary smudged", 'research');
		return true;
	}
	if(researchStrategy == "Strict"){
		var _out=[];
		for(i in research_primary){
			_out.push(research_primary[i]);
		}
		research_way.unshift(_out);
		debugMsg("research_primary strict", 'research');
		return true;
	}
	//Oh shi... desync ahead.
	/*
	if(researchStrategy == "Random"){
		shuffle(research_primary);
		for(i in research_primary){
			research_way.unshift([research_primary[i]]);
		}
		debugMsg("research_primary random", 'research');
		return true;
	}
	*/
	return false;
}

function removeDuplicates(originalArray, objKey) {
	var trimmedArray = [];
	var values = [];
	var value;
	
	for(var i = 0; i < originalArray.length; i++) {
		value = originalArray[i][objKey];
		
		if(values.indexOf(value) === -1) {
			trimmedArray.push(originalArray[i]);
			values.push(value);
		}
	}
	
	return trimmedArray;
	
}

//Возвращает кол-во производящихся на данный момент типов в заводах.
function inProduce(type){
	if(produceTrigger.length == 0) return 0;
	var _prod = 0;
	
	for ( var p in produceTrigger ){
		if (produceTrigger[p] == type) _prod++;
	}
	
	return _prod;
}


function unitIdle(obj){
//	debugMsg(obj.name+" "+obj.order+" "+obj.action, 'temp');
	if(obj.order == DORDER_NONE){
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
	if ( targets.length == 0 || warriors.length == 0 ) return false;
	if ( typeof num === "undefined" || num == null || num == 0 ) num = 3;
	if ( num > 10 ) num = 10;
	if ( warriors.length < num ) num = warriors.length;

	targets = targets.slice(0,num);

	for ( var i in targets ) {
		var target = isBeingRepaired(targets[i]);
		if ( target != false) {
			targets[i] = target;
		}
	}

	if ( targets.length >= warriors.length ) {
		for ( var i = 0, len = warriors.length; i<len; ++i ) {
			if(scouting) orderDroidLoc_p(warriors[i], DORDER_SCOUT, targets[i].x, targets[i].y);
			else orderDroidObj_p( warriors[i], DORDER_ATTACK, targets[i] );
		}
		return true;
	}else{
		var a = Math.floor(warriors.length/targets.length);
		var i=0;
		var t=0;
		for ( var n in warriors ) { 
			t++;
			if ( i == targets.length ) return true;
			var busy = false;
			for ( var j in targets ) {
				if ( distBetweenTwoPoints_p ( targets[j].x,targets[j].y,warriors[n].x,warriors[n].y ) < 7 ) {
					if(scouting) orderDroidLoc_p(warriors[n], DORDER_SCOUT, targets[j].x, targets[j].y);
					else orderDroidObj_p( warriors[n], DORDER_ATTACK, targets[j] );
					busy = true;
				}
			}
			if ( busy ) continue;
			if(scouting) orderDroidLoc_p(warriors[n], DORDER_SCOUT, targets[i].x, targets[i].y);
			else orderDroidObj_p( warriors[n], DORDER_ATTACK, targets[i] );
			if ( t >= a ){
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
	tech = tech.filter(function(o){
		if(o.isArray) return true;
		if(excludes.indexOf(o) != -1)return false;
		return true;
	});
	tech.forEach(function(e){
		if (!(e instanceof Array)){
			ex.push(e);
			return;
		}
		var check = e.filter(function(o){
			if(excludes.indexOf(o) != -1)return false;
			return true;
		});
		if (check.length) ex.push(check);
	});
	return ex;
}

//Неоптимизированно, нужно доделать.
function checkEventIdle(){
	if(!running)return;
	targetPartisan();
	queue('targetCyborgs', 200);
	queue('targetRegular', 400);
}

//для 3.2
function recycleBuilders(){
	var factory = enumStruct(me, FACTORY);
	var factory_ready = factory.filter(function(e){if(e.status == 1)return true; return false;});
	if(factory_ready.length != 0){
		var _builders = enumDroid(me,DROID_CONSTRUCT);
		_builders.forEach(function(e){
			orderDroid(e, DORDER_RECYCLE);
		});
	}
}


//from: https://warzone.atlassian.net/wiki/pages/viewpage.action?pageId=360669
// More reliable way to identify VTOLs
var isFixVTOL = function(obj) {
	try {
		return ( (obj.type == DROID) && (("isVTOL" in obj && obj.isVTOL) || isVTOL(obj)) );
	} catch(e) {
//		debugMsg("isFixVTOL(): "+e.message, 'error');
	}
}

//from: http://stackoverflow.com/questions/6274339/how-can-i-shuffle-an-array
//Oh shi... desync ahead.
/*
function shuffle(a) {
	var j, x, i;
	for (i = a.length; i; i--) {
		j = Math.floor(Math.random() * i);
		x = a[i - 1];
		a[i - 1] = a[j];
		a[j] = x;
	}
}
*/

function posRnd(pos, axis){
	var p=pos+Math.round(Math.random()*2-1);
	if(p<1 || (axis=='x' && p >= mapWidth)) return pos;
	if(p<1 || (axis=='y' && p >= mapHeight)) return pos;
	return p;
}

function secondTick(){
	if(earlyGame && gameTime/1000 > 130){
		earlyGame = false;
		if(policy['build'] == 'rich'){
			maxPartisans = 1;
			scannersTimer = 300000;
			removeTimer("buildersOrder");
		}
	}
	
	//CHEAT
	if(berserk){
		var qp=queuedPower(me);
//		debugMsg('qp: '+qp, 'berserk');
		if(qp > 0){
			setPower(qp+1, me);
			credit += (qp+1);
			debugMsg('+'+(qp+1), 'berserk');
			debugMsg('credit: '+credit, 'berserk');
		}
	}
	
	debugMsg('power: '+playerPower(me), 'power');
}
