debugMsg('Module: weapons.js','init');

/*

	Данный модуль отслеживает произведённые улучшения на оружия и принимает решения, какие оружия устанавливать на новый юнит

*/

function _weaponsGetGuns(num){

	var _weapons = [];
	var _weapon, _points, _dbg;

	//Цикл по всем типам стволов
	for ( var t in guns_type ) {

		_weapon = false;
		_points = 0;
		_dbg = '';

//		debugMsg("Check typ: "+t, 'weap');

		//Цикл по стволам одного типа
		for ( var g in guns_type[t] ) {
//			debugMsg("Check wpn: "+guns_type[t][g][0]+" - "+getResearch(guns_type[t][g][0]), 'weap');
			if (getResearch(guns_type[t][g][0]).done){ _weapon = guns_type[t][g][1]; _dbg = research_name[guns_type[t][g][0]]; break; }
		}

		//Цикл по исследованиям одного типа
		for ( var r in guns_pts[t] ) {
//			debugMsg("Check pts: "+guns_pts[t][r]+" - "+getResearch(guns_pts[t][r]), 'weap');
			if(getResearch(guns_pts[t][r]).done) _points++;
		}
		if(_weapon){
			var precent = Math.round((_points+1)*100/(guns_pts[t].length+1));
			_weapons.push([_weapon, _points+1]);
			debugMsg(_dbg+" - "+precent+"%", 'weap');
		}
	}

	if(_weapons.length != 0){
		_weapons.sort(function (a,b){
			if(a[1] < b[1]) return -1;
			if(a[1] > b[1]) return 1;
			return 0;
		});
		_weapons.reverse();
		_weapons=_weapons.slice(0, num);

		var _out = [];

		for ( var w in _weapons ){
			debugMsg(_weapons[w][0]+", "+_weapons[w][1], 'weap');
			for(var i=0;i<_weapons[w][1];i++) _out.push(_weapons[w][0]);
		}
		return _out;
	}else return [];
}
