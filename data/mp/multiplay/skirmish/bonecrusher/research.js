debugMsg('Module: research.js','init');


//Rewrited for 3.3+ game version
function doResearch(){
	if (!running)return false;

	//old dependency
	if (!getInfoNear(base.x,base.y,'safe',base_range).value && !(playerPower(me) > 300 || berserk) && avail_guns.length > 0) return false;



	//Get labs where build and ready
	var labs = enumStruct(me,RESEARCH_LAB);
	var labs_len = labs.length;
	labs = labs.filter((e) => (e.status === BUILT && structureIdle(e)));

	//If no ready labs
	if (labs.length === 0) return false;

	//old dependency
	if (policy['build'] !== 'rich') {
			if (countStruct('A0ResourceExtractor', me) < 8 && !(playerPower(me) > 700 || berserk) && (labs_len-labs.len) >= 3) return false;
			if (countStruct('A0ResourceExtractor', me) < 5 && !(playerPower(me) > 500 || berserk) && (labs_len-labs.len) >= 2) return false;
			if (countStruct('A0ResourceExtractor', me) < 3 && !(playerPower(me) > 300 || berserk) && (labs_len-labs.len) >= 1) return false;
	}


	//Get all available researches, filterout started by ally
	var avail_research = enumResearch().filter((o) => (!o.started));

	if (avail_research.length === 0) return false;

	debugMsg('Labs: '+labs_len+', ready: '+labs.length+', avail_research: '+avail_research.length, 'research');

//	debugMsg('research_path.length:'+research_path.length, 'research');

	//Clear research path from completed researches
	research_path = research_path.filter((o) => {
		var r = getResearch(o);
		if (!r) {debugMsg('Research "'+o+'" not found', 'error');return false;}
		return !r.done;
	});

	var prepare_research = [];

	if (research_path.length > 0) {
		//Filter out started researches by me or ally
		prepare_research = research_path.filter((o) => (!getResearch(o).started));

		if (prepare_research.length === 0) return false;

		debugMsg('Path length: '+prepare_research.length+'; follow to: '+prepare_research[0], 'research');

	}

	//Get clean array without objects
	var researches = [];
	for (const r in avail_research) {researches.push(avail_research[r].id);}

	var to_research = [];

	//Check if we are at a dead end of the technology branch
	for (const t in prepare_research) {
		var research = findResearch(prepare_research[t]);
		for (const r in research) {
			to_research.push(research[r].id);
		}
//		to_research = to_research.filter((o) => (researches.indexOf(o) !== -1));
		to_research = intersect_arrays(to_research, researches);

		if (to_research.length > 0) break;

		debugMsg('Cannot follow to "'+prepare_research[t]+'" for now', 'research');

	}

	// if (findResearch(to_research[0]).filter((o) => (!getResearch(o).started)).length === 0)

	//No more research at this moment
	if (to_research.length === 0) {
		var rnd = Math.floor(Math.random()*researches.length);
		debugMsg('researches.length: '+researches.length+', rnd: '+rnd, 'research');
		var rnd_research = researches[rnd];
		debugMsg('Nothing research, start random research: "'+rnd_research+'"', 'research');
		to_research.push(rnd_research);
	}

	//Finish research line
	if (research_path.length === 0 && to_research.length === 0) {debugMsg('No more research in research_path', 'error'); return false;}

	debugMsg('Start research: '+to_research[0], 'research');

	//Start pursue research to given technology
	if (!pursueResearch(labs[0], to_research)) {
		debugMsg('Something wrong in doResearch() function', 'error');
		return false;
	}
	//debug(JSON.stringify(pursueResearch(labs[0], to_research)));

	//If there more technology to research and more ready labs - repeat function
	if (labs.length > 1 && prepare_research.length > 1) queue("doResearch", 700);

	return true;
}


//Функция предерживается приоритетов исследований
//и ровномерно распределяет по свободным лабораториям
//и должна вызыватся в 3-х случаях (не в цикле)
//1. При старте игры
//2. При постройке лабаротории
//3. При завершении исследования
function doResearch_old(){
	if (!running)return;
//	debugMsg("doResearch()", 'research_advance');
	//	debugMsg(getInfoNear(base.x,base.y,'safe',base_range).value+" && "+playerPower(me)+"<300 && "+avail_guns.length+"!=0", 'research_advance');
	if (!getInfoNear(base.x,base.y,'safe',base_range).value && !(playerPower(me) > 300 || berserk) && avail_guns.length > 0) return;


	var avail_research = enumResearch().filter((e) => {
		//		debugMsg(e.name+' - '+e.started+' - '+e.done, 'research_advance');
		return !e.started
	});

	if (research_way.length === 0 || avail_research.length === 0) {
//		debugMsg("doResearch: Исследовательские пути завершены!!! Останов.", 'research_advance');
		return;
	}

	if (research_way.length < 5) {
		var rnd = Math.floor(Math.random()*avail_research.length);
		var _research = avail_research[rnd].name;
//		debugMsg(_research, 'temp');
		research_way.push([_research]);
//		debugMsg("doResearch: Исследовательские пути ("+research_way.length+") подходят к концу! Добавляем рандом. \""+research_name[_research]+"\" ["+_research+"]", 'research_advance');
	}


	var labs = enumStruct(me,RESEARCH_LAB);
	if (typeof _r === "undefined") _r = 0;
	var _busy = 0;

	var _last_r = research_way[_r][research_way[_r].length-1];
	var _way = getResearch(_last_r);
	if (!_way) return;

	if (_way.done) {
		//		debugMsg("doResearch: Путей "+research_way.length+", путь "+_r+" завершён", 'research_advance');
		research_way.splice(_r,1);
		//		debugMsg("doResearch: Осталось путей "+research_way.length, 'research_advance');
		_r=0;
		if (research_way.length === 0) {
//			debugMsg("doResearch: Исследовательские пути завершены! Останов.", 'research_advance');
			return;
		}
	}

	//Если меньше 8 нефтевышек, и меньше 1000 денег, и уже запущенны 3 лабы - выход
// 	if (countStruct('A0ResourceExtractor', me) < 8 && playerPower(me) < 1000 && enumStruct(me, RESEARCH_LAB).filter((e) => (!structureIdle(e)&&e.status === BUILT)).length >= 3) return;
// 	if (countStruct('A0ResourceExtractor', me) < 5 && playerPower(me) < 500 && enumStruct(me, RESEARCH_LAB).filter((e) => (!structureIdle(e)&&e.status === BUILT)).length >= 2) return;
// 	if (countStruct('A0ResourceExtractor', me) <= 3 && playerPower(me) < 300 && enumStruct(me, RESEARCH_LAB).filter((e) => (!structureIdle(e)&&e.status === BUILT)).length >= 1) return;

	for (const l in labs) {

		if (policy['build'] !== 'rich') {
			if (countStruct('A0ResourceExtractor', me) < 8 && !(playerPower(me) > 700 || berserk) && _busy >= 3) break;
			if (countStruct('A0ResourceExtractor', me) < 5 && !(playerPower(me) > 500 || berserk) && _busy >= 2) break;
			if (countStruct('A0ResourceExtractor', me) < 3 && !(playerPower(me) > 300 || berserk) && _busy >= 1) break;
		}
		if ((labs[l].status === BUILT) && structureIdle(labs[l])) {
//			debugMsg("Лаборатория("+labs[l].id+")["+l+"] исследует путь "+_r, 'research_advance');
			pursueResearch(labs[l], research_way[_r]);
		} else {
//			debugMsg("Лаборатория("+labs[l].id+")["+l+"] занята", 'research_advance');
			_busy++;
		}
	}

	if (_r == research_way.length-1) {
		_r = 0;
//		debugMsg("doResearch: Все исследования запущены, останов.", 'research_advance');
	} else if (_busy === labs.length) {
//		debugMsg("doResearch: Все все лаборатории заняты, останов.", 'research_advance');
		_r = 0;
	} else {
		_r++;
//		debugMsg("doResearch: Планировка проверки занятости лабораторий...", 'research_advance');
		queue("doResearch", 1000);
	}
}


function fixResearchWay(way){
	if (typeof way === "undefined") return false;
	if (!(way instanceof Array)) return false;
//	debugMsg('Check tech '+way.length, 'research');
	var _out = [];

	for (const i in way) {
//		debugMsg('Check: '+way[i], 'research');
		var _res = getResearch(way[i]);
		if (_res == null) {
			debugMsg('Unknown research "'+way[i]+'" - ignored', 'error');
			continue;
		}
		_out.push(way[i]);
	}

	debugMsg('Checked research way length='+way.length+', returned='+_out.length, 'init');
	return _out;
}

function addPrimaryWay(){
	if (typeof research_primary === "undefined") return false;
	if (!(research_primary instanceof Array)) return false;
	if (researchStrategy === "Smudged") {
		research_primary.reverse();
		for (const i in research_primary) {
			research_way.unshift([research_primary[i]]);
		}
		debugMsg("research_primary smudged", 'research');
		return true;
	}
	if (researchStrategy === "Strict") {
		var _out=[];
		for (const i in research_primary) {
			_out.push(research_primary[i]);
		}
		research_way.unshift(_out);
		debugMsg("research_primary strict", 'research');
		return true;
	}
	if (researchStrategy === "Random") {
		shuffle(research_primary);
		for (const i in research_primary) {
			research_way.unshift([research_primary[i]]);
		}
		debugMsg("research_primary random", 'research');
		return true;
	}
	return false;
}
