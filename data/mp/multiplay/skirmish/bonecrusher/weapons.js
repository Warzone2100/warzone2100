debugMsg('Module: weapons.js','init');

/*

	Данный модуль отслеживает произведённые улучшения на оружия и принимает решения, какие оружия устанавливать на новый юнит

*/

function _weaponsGetGuns(num){

	var _weapons = [];
	var _weapon, _points, _dbg;

	//Цикл по всем типам стволов
	for (const t in guns_type) {

		_weapon = false;
		_points = 0;
		_dbg = '';

//		debugMsg("Check typ: "+t, 'weap');

		//Цикл по стволам одного типа
		for (const g of guns_type[t]) {
//			debugMsg("Check wpn: "+g[0]+" - "+getResearch(g[0]), 'weap');
			if (getResearch(g[0]).done){ _weapon = g[1]; _dbg = research_name[g[0]]; break; }
		}

		//Цикл по исследованиям одного типа
		for (const r of guns_pts[t]) {
//			debugMsg("Check pts: "+r+" - "+getResearch(r), 'weap');
			if(getResearch(r).done) _points++;
		}
		if(_weapon){
			var precent = Math.round((_points+1)*100/(guns_pts[t].length+1));
			_weapons.push([_weapon, _points+1]);
			debugMsg(_dbg+" - "+precent+"%", 'weap');
		}
	}

	if(_weapons.length > 0){
		_weapons.sort((a, b) => (a[1] < b[1]));
		_weapons.reverse();
		_weapons=_weapons.slice(0, num);

		var _out = [];

		for (const weapon of _weapons) {
			debugMsg(weapon[0]+", "+weapon[1], 'weap');
			for (let i = 0; i < weapon[1]; i++) _out.push(weapon[0]);
		}
		return _out;
	}else return [];
}
