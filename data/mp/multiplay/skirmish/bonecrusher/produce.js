//Подготавливаем технологии для производства
//на самом деле лишняя функция, однако необходимая
//для того, что бы не читерить. Мы не можем производить
//войска по исследованным технологиям без HQ, так этой функцией запретим
//это делать и ИИ
function prepeareProduce(){
//	debugMsg('prepeareProduce()', 'production')
	//Если есть HQ
	var hq = enumStruct(me, HQ).filter(function (e){if(e.status == BUILT)return true;return false;});
	if (hq.length != 0){
		
		
		//Составляем корпуса
		light_bodies=[];
		medium_bodies=[];
		heavy_bodies=[];
		bodies.forEach(function(e){
			//			debugMsg("Body: "+e[1]+" "+getResearch(e[0]).done, 'production');
			switch (e[1]){
				case "Body1REC":
				case "Body4ABT":
				case "Body2SUP":
				case "Body3MBT":
					if(getResearch(e[0]).done) light_bodies.unshift(e[1]);
					   break;
				case "Body5REC":
				case "Body8MBT":
				case "Body6SUPP":
				case "Body7ABT":
					if(getResearch(e[0]).done) medium_bodies.unshift(e[1]);
					   break;
				case "Body11ABT":
				case "Body12SUP":
				case "Body9REC":
				case "Body10MBT":
				case "Body13SUP":
				case "Body14SUP":
					if(getResearch(e[0]).done) heavy_bodies.unshift(e[1]);
					   break;
					
			}
		});
		
		
		/*
		//Сортируем пушки по "крутизне", базируясь на research.points
		var _guns=guns.filter(function(e){
			debugMsg(e[0]+' - '+getResearch(e[0]).done, 'weap');
			if(getResearch(e[0]).done)return true;return false;
		}).sort(function (a,b){
			if(getResearch(a[0]).points < getResearch(b[0]).points ) return -1;
			if(getResearch(a[0]).points > getResearch(b[0]).points ) return 1;
			return 0;
		});
		
		avail_guns=[];
		for (var i in _guns){
			avail_guns.push(_guns[i][1]);
//			debugMsg(getResearch(_guns[i][0]).points+" "+_guns[i][0]+"->"+_guns[i][1], 'weap');
			debugMsg(getResearch(_guns[i][0]).points+" - "+research_name[_guns[i][0]], 'weap');
		}
		if(avail_guns.length > 2) avail_guns.shift(); //Выкидываем первый пулемётик
		if(avail_guns.length > 2) avail_guns.shift(); //Выкидываем спаренный пулемётик
		avail_guns.reverse();
		*/

		//Дай мне три типа лучших пушек на данный момент
		avail_guns = _weaponsGetGuns(3);
		
//		for(i in avail_guns) debugMsg(avail_guns[i], 'weap');
		
		//Сайборги заполонили!
		avail_cyborgs=[];
		var _cyb=cyborgs.filter(function(e){if(getResearch(e[0]).done)return true;return false;}).sort(function (a,b){
			if(getResearch(a[0]).points < getResearch(b[0]).points ) return -1;
			if(getResearch(a[0]).points > getResearch(b[0]).points ) return 1;
			return 0;
		});
		for (var i in _cyb){
			avail_cyborgs.push([_cyb[i][1],_cyb[i][2]]);
		}
		avail_cyborgs.reverse();
		
		//В.В.иП.
		avail_vtols=[];
		var _vtols=vtols.filter(function(e){if(getResearch(e[0]).done)return true;return false;}).sort(function (a,b){
			if(getResearch(a[0]).points < getResearch(b[0]).points ) return -1;
			if(getResearch(a[0]).points > getResearch(b[0]).points ) return 1;
			return 0;
		});
		for (var i in _vtols){
			avail_vtols.push(_vtols[i][1]);
		}
		avail_vtols.reverse();
		avail_vtols.unshift("Bomb3-VTOL-LtINC");	// <-- *facepalm*
		avail_vtols.unshift("Bomb4-VTOL-HvyINC");	// <-- *facepalm*
	}
	
	defence=[];
	towers.forEach(function(e){if(getResearch(e[0]).done)defence.unshift(e[1]);});
	
	AA_defence=[];
	AA_towers.forEach(function(e){if(getResearch(e[0]).done)AA_defence.unshift(e[1]);});
	
	//	if(AA_defence.length != 0) defence.unshift(AA_defence[0]); //add best AA to normal defence
	
}

function produceDroids(){
	if(!running)return;
	debugMsg('produceDroids()', 'production');
	var droid_factories = enumStruct(me,FACTORY).filter(function(e){if(e.status == BUILT && structureIdle(e))return true;return false;});
	
	//	var builders_limit = getDroidLimit(me, DROID_CONSTRUCT);
	var builders = enumDroid(me, DROID_CONSTRUCT);
	//	debugMsg("Have builders: "+builders.length+"; limits: "+builders_limit, 'production');
	//	debugMsg("Have warriors="+groupSize(armyRegular)+" partisan="+groupSize(armyPartisans), 'production');
	if(droid_factories.length != 0){
		
		var _body=light_bodies;
		if(droid_factories[0].modules >= 1 && playerPower(me)>500 && medium_bodies.length != 0) _body = medium_bodies;
		if(droid_factories[0].modules == 2 && playerPower(me)>1500 && heavy_bodies.length != 0) _body = heavy_bodies;
		
		var _prop = ['tracked01','HalfTrack','wheeled01'];
		if(nf['policy']=='island') _prop = ['hover01'];
		else if(nf['policy'] == 'both') _prop = ['hover01', 'tracked01', 'HalfTrack', 'wheeled01'];
		
		
		if(earlyGame && getResearch("R-Sys-Sensor-Turret01").done){
			
			//SensorTurret1Mk1
			var todo;
		}
		
		//Строители
		//Если строители не в лимите -И- база не подвергается нападению
		//Если целей для охотников более 7 -И- денег более 750 -ИЛИ- (строитель всего один или ноль охотников), а денег более 150 -ИЛИ- вообще нет строителей
		//ТО заказуаэм!
//		debugMsg("buildersTrigger="+buildersTrigger+"; fixersTrigger="+fixersTrigger+"; gameTime="+gameTime, 'production');
		
		debugMsg('('+builders.length+'<'+(maxConstructors-3)+' && '+getInfoNear(base.x,base.y,'safe',base_range).value+') && ( ('+playerPower(me)+'>'+builderPts+' && '+builder_targets.length+'>7) || ( ('+groupSize(buildersMain)+'==1 || '+groupSize(buildersHunters)+'==0) && '+playerPower(me)+'>150) || '+builders.length+'==0) && '+buildersTrigger+'<'+gameTime, 'production');
		
		if( (builders.length < (maxConstructors-3) && (getInfoNear(base.x,base.y,'safe',base_range).value || policy['build'] == 'rich'))
			&& ( (playerPower(me) > builderPts && builder_targets.length > 7) || ( (groupSize(buildersMain) == 1 || groupSize(buildersHunters) == 0) && playerPower(me) > 150) || builders.length == 0 )
			&& buildersTrigger < gameTime && (policy['build'] != 'rich' || !isFullBase(me))){
			buildersTrigger = gameTime + buildersTimer;
			debugMsg("buildersTrigger="+buildersTrigger+", gameTime="+gameTime+", buildersTimer="+buildersTimer, 'production');
			buildDroid(droid_factories[0], "Truck", ['Body2SUP','Body4ABT','Body1REC'], ['hover01','wheeled01'], "", DROID_CONSTRUCT, "Spade1Mk1");
			return;
		}

		if (policy['build'] != 'rich' && getInfoNear(base.x,base.y,'safe',base_range).value && groupSize(armyFixers) < maxFixers && groupSize(armyPartisans) > 5 && fixersTrigger < gameTime 
			&& ( getResearch("R-Sys-MobileRepairTurret01").done || getResearch("R-Sys-MobileRepairTurretHvy").done) && (playerPower(me) > 300 || groupSize(armyFixers) == 0)){
			fixersTrigger = gameTime + fixersTimer;
			var _repair = "LightRepair1";
			if(getResearch("R-Sys-MobileRepairTurretHvy").done) _repair = "HeavyRepair";
			buildDroid(droid_factories[0], "Fixer", _body, _prop, "", DROID_REPAIR, _repair);
			return;
		}
/*
		if (version.substr(0,3) == '3.2' && getResearch('R-Sys-ECM-Upgrade01').done && getInfoNear(base.x,base.y,'safe',base_range).value && (groupSize(armyJammers) == 0 || groupSize(armyJammers) < maxJammers) && inProduce('jammer') == 0){
			var _jammer = "ECM1TurretMk1";
			produceTrigger[droid_factories[0].id] = 'jammer';
			debugMsg("ADD jammer "+droid_factories[0].id, 'triggers');
			buildDroid(droid_factories[0], "Jammer", _body, _prop, "", DROID_ECM, _jammer);
		}
*/
		//Армия
		if(light_bodies.length != 0 && avail_guns.length != 0){
//			if( ( (groupSize(armyPartisans) < 7 || playerPower(me) > 250) && groupSize(armyPartisans) < maxPartisans) || !getInfoNear(base.x,base.y,'safe',base_range).value){
			if( (groupSize(armyPartisans) < minPartisans || playerPower(me) > (groupSize(armyPartisans)*50) ) || !getInfoNear(base.x,base.y,'safe',base_range).value){

//				var _weapon = avail_guns[Math.floor(Math.random()*Math.min(avail_guns.length, 5))]; //Случайная из 5 последних крутых пушек
				var _weapon = avail_guns[Math.floor(Math.random()*avail_guns.length)];
				debugMsg(_body+" "+_prop+" "+_weapon, 'template');
				buildDroid(droid_factories[0], "Army", _body, _prop, "", DROID_WEAPON, _weapon);
			}
		}
	}
}
function produceCyborgs(){
	if(!running || nf['policy'] == 'island')return;
	if(groupSize(armyCyborgs) >= maxCyborgs) return;
	if(playerPower(me) < 200 && groupSize(armyCyborgs) > 2) return;
	var cyborg_factories = enumStruct(me,CYBORG_FACTORY).filter(function(e){if(e.status == BUILT && structureIdle(e))return true;return false;});
	debugMsg("Cyborg: fact="+cyborg_factories.length+"; cyb="+avail_cyborgs.length, 'production');
	if( cyborg_factories.length != 0 && avail_cyborgs.length != 0 && (groupSize(armyCyborgs) < maxCyborgs || !getInfoNear(base.x,base.y,'safe',base_range).value) ){
		var _cyb = avail_cyborgs[Math.floor(Math.random()*Math.min(avail_cyborgs.length, 3))]; //Случайный киборг из 3 полседних крутых
		var _body = _cyb[0];
		_body = 'CyborgLightBody';
		var _weapon = _cyb[1];
		debugMsg("Cyborg: body="+_body+"; weapon="+_weapon ,'production');
		buildDroid(cyborg_factories[0], "Terminator", _body, "CyborgLegs", "", DROID_CYBORG, _weapon);
	}
}

function produceVTOL(){
	if(!running)return;
	if(groupSize(VTOLAttacker) >= maxVTOL) return;
	if(playerPower(me) < 300 && groupSize(VTOLAttacker) > 3) return;
	/*
	 * Missile-VTOL-AT			_("VTOL Scourge Missile")
	 * Rocket-VTOL-BB			_("VTOL Bunker Buster")
	 * Rocket-VTOL-Pod			_("VTOL Mini-Rocket")
	 * Rocket-VTOL-LtA-T			_("VTOL Lancer")
	 * Rocket-VTOL-HvyA-T		_("VTOL Tank Killer")
	 * AAGun2Mk1-VTOL				_("VTOL Flak Cannon")
	 * Cannon1-VTOL				_("VTOL Cannon")
	 * Cannon4AUTO-VTOL				_("VTOL Hyper Velocity Cannon")
	 * Cannon5Vulcan-VTOL			_("VTOL Assault Cannon")
	 * Laser2PULSE-VTOL				_("VTOL Pulse Laser")
	 * MG1-VTOL					_("VTOL Machinegun")
	 * MG2-VTOL					_("VTOL Twin Machinegun")
	 * MG3-VTOL					_("VTOL Heavy Machinegun")
	 * MG4ROTARY-VTOL				_("VTOL Assault Gun")
	 * RailGun1-VTOL				_("VTOL Needle Gun")
	 * RailGun2-VTOL				_("VTOL Rail Gun")
	 * Bomb1-VTOL-LtHE				_("VTOL Cluster Bomb Bay")
	 * Bomb2-VTOL-HvHE				_("VTOL Heap Bomb Bay")
	 * Bomb3-VTOL-LtINC				_("VTOL Phosphor Bomb Bay")
	 * Bomb4-VTOL-HvyINC				_("VTOL Thermite Bomb Bay")
	 * 
	 * Cannon1-VTOL                            _("VTOL Cannon")
	 * Cannon4AUTO-VTOL                                _("VTOL Hyper Velocity Cannon")
	 * Cannon5Vulcan-VTOL                      _("VTOL Assault Cannon")
	 * Laser2PULSE-VTOL                                _("VTOL Pulse Laser")
	 * 
	 * MG1-VTOL                                        _("VTOL Machinegun")
	 * MG2-VTOL                                        _("VTOL Twin Machinegun")
	 * MG3-VTOL                                        _("VTOL Heavy Machinegun")
	 * MG4ROTARY-VTOL                          _("VTOL Assault Gun")
	 * RailGun1-VTOL                           _("VTOL Needle Gun")
	 * RailGun2-VTOL                           _("VTOL Rail Gun")
	 * 
	 * PBomb                                           _("Proximity Bomb Turret")
	 * SPBomb                                  _("Proximity Superbomb Turret")
	 * 
	 * Bomb1-VTOL-LtHE                         _("VTOL Cluster Bomb Bay")
	 * Bomb2-VTOL-HvHE                         _("VTOL Heap Bomb Bay")
	 * Bomb3-VTOL-LtINC                                _("VTOL Phosphor Bomb Bay")
	 * Bomb4-VTOL-HvyINC                               _("VTOL Thermite Bomb Bay")
	 * Bomb5-VTOL-Plasmite			_("VTOL Plasmite Bomb Bay")
	 * 
	 * 
	 * */
	
	if(groupSize(VTOLAttacker) > 20) return;
	vtol_factory = enumStruct(me, VTOL_FACTORY);
	var vtol_factories = vtol_factory.filter(function(e){if(e.status == BUILT && structureIdle(e))return true;return false;});
	if(vtol_factories.length != 0 ){
		var _body=light_bodies;
		if(playerPower(me)>300 && playerPower(me)<500 && medium_bodies.length != 0) _body = medium_bodies;
		if(playerPower(me)>800 && heavy_bodies.length != 0) _body = heavy_bodies;
		var _weapon = avail_vtols[Math.floor(Math.random()*Math.min(avail_vtols.length, 5))]; //Случайная из 5 последних крутых пушек
		buildDroid(vtol_factories[0], "Bomber", _body, "V-Tol", "", DROID_WEAPON, _weapon);
	}
}


