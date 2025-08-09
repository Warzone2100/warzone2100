debugMsg('Module: performance.js','init');


//function getOrder(obj, order, loc){




//}

var perfFunc=[];
function distBetweenTwoPoints_p(x1,y1,x2,y2){
	if (!perfFunc["distBetweenTwoPoints"])perfFunc["distBetweenTwoPoints"]=1;
	else perfFunc["distBetweenTwoPoints"]++;
	return distBetweenTwoPoints(x1,y1,x2,y2);
}

function droidCanReach_p(obj,x,y){
	debugMsg('droidCanReach:'+x+'x'+y, 'performance');
	debugMsg('droidCanReach:'+JSON.stringify(obj), 'performance');
	return droidCanReach(obj,x,y);
}

var perfOrder=[];

/*
function orderDroidObj_p(who, order, obj){
	var result = orderDroidObj(who, order, obj);
	if (typeof perfOrder[droidTypes[who.droidType]+'_'+droidOrders[order]] !== "number") {
		debugMsg(perfOrder[droidTypes[who.droidType]+'->'+droidOrders[order]], 'performance');
		perfOrder[droidTypes[who.droidType]+'->'+droidOrders[order]]=1;
		debugMsg(droidTypes[who.droidType]+'->'+droidOrders[order]+'->'+obj.type+'('+obj.x+'x'+obj.y+')', 'performance');
	}
	else {
		debugMsg('Obj-else', 'performance');
		perfOrder[droidTypes[who.droidType]+'->'+droidOrders[order]]++;
	}
	return result;
}
*/



function orderDroidObj_p(who, order, obj){

	if (weakCPU && perfFunc["orderDroidObj"] > ordersLimit) return false;

	if (!perfFunc["orderDroidObj"])perfFunc["orderDroidObj"]=1;
	else perfFunc["orderDroidObj"]++;


	var type_order = droidTypes[who.droidType]+'_'+droidOrders[order];
	var orders = perfOrder[type_order];

	if (typeof orders !== "number") {
		orders = 1;
	}
	else {
//		debugMsg('Obj-else', 'performance');
		orders++;
	}

	if (!release)perfOrder[type_order] = orders;
	var result = orderDroidObj(who, order, obj);
	return result;
}

function orderDroidLoc_p(who, order, x, y){

	if (weakCPU && perfFunc["orderDroidLoc"] > ordersLimit) return false;

	if (!perfFunc["orderDroidLoc"])perfFunc["orderDroidLoc"]=1;
	else perfFunc["orderDroidLoc"]++;

	var type_order = droidTypes[who.droidType]+'_'+droidOrders[order];
	var orders = perfOrder[type_order];

	if (typeof orders !== "number") {
		orders = 1;
	}
	else {
//		debugMsg('Loc-else', 'performance');
		orders++;
	}

	if (!release)perfOrder[type_order] = orders;
	var result = orderDroidLoc(who, order, x, y);
//	debugMsg('orderDroidLoc: '+droidTypes[who.droidType]+'; '+droidOrders[order]+'; '+x+'x'+y+'; '+result, 'performance');
	return result;
}

function orderDroidBuild_p(who, order, building, x, y, rotation){
	if (!perfFunc["orderDroidBuild"])perfFunc["orderDroidBuild"]=1;
	else perfFunc["orderDroidBuild"]++;
	var type_order = droidTypes[who.droidType]+'_'+droidOrders[order];
	var orders = perfOrder[type_order];

	if (typeof orders !== "number") {
		orders = 1;
	}
	else {
		orders++;
	}

	if (!release)perfOrder[type_order] = orders;
	var result = orderDroidBuild(who, order, building, x, y, rotation);
	return result;

}
/*
function perfMonitor(){
	if (!running)return;
	if (Object.keys(perfOrder).length > 0) {
		var pout=[];
		Object.keys(perfOrder).map((k, i) => {
			pout += "\n"+i+": "+k+"="+perfOrder[k];
		});
		debugMsg(pout, 'performance');
		perfOrder=[];
	}

	for (const i in perfFunc) {if (perfFunc[i]) {
		debugMsg(i+'='+perfFunc[i], 'performance');
		perfFunc[i]=0;

	}}
}
*/
