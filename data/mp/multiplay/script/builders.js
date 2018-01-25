function buildersAssist(structure, demolish)
{
	//Help build
	if(!demolish)
	{
		builders = buildersIdleNearby(structure);
		builders.forEach(function(droid){
			orderDroidObj(droid, DORDER_HELPBUILD, structure);
		});
		delete builders;
	}
	//We don't need to help demolish the ally buildings
	else if(structure.player == me)
	{
		builders = buildersIdleNearby(structure);
		builders.forEach(function(droid){
			orderDroidObj(droid, DORDER_DEMOLISH, structure);
		});
		delete builders;
	}
}

//Get nearby idle buiders and cyborgs engeeners
function buildersIdleNearby(structure)
{
	return enumRange(structure.x, structure.y, buildersAssistRadius, ALLIES).filter(function(obj)
	{
		if(obj.type == DROID && obj.droidType == DROID_CONSTRUCT && obj.action == DACTION_NONE) return true;
		return false;
	});
}