
////////////////////////////////////////////////////////////////////////////////
// Research related functions.
////////////////////////////////////////////////////////////////////////////////

//;; ## camEnableRes(researchIds, playerId)
//;;
//;; Grants research from the given list to player
//;;
//;; @param {string[]} researchIds
//;; @param {number} playerId
//;; @returns {void}
//;;
function camEnableRes(researchIds, playerId)
{
	for (let i = 0, l = researchIds.length; i < l; ++i)
	{
		var researchId = researchIds[i];
		enableResearch(researchId, playerId);
		completeResearch(researchId, playerId);
	}
}

//;; ## camCompleteRequiredResearch(researchIds, playerId)
//;;
//;; Grants research from the given list to player and also researches the required research for that item.
//;;
//;; @param {string[]} researchIds
//;; @param {number} playerId
//;; @returns {void}
//;;
function camCompleteRequiredResearch(researchIds, playerId)
{
	dump("\n*Player " + playerId + " requesting accelerated research.");

	for (let i = 0, l = researchIds.length; i < l; ++i)
	{
		var researchId = researchIds[i];
		dump("Searching for required research of item: " + researchId);
		var reqRes = findResearch(researchId, playerId).reverse();

		if (reqRes.length === 0)
		{
			//HACK: autorepair like upgrades don't work after mission transition.
			if (researchId === "R-Sys-NEXUSrepair")
			{
				completeResearch(researchId, playerId, true);
			}
			continue;
		}

		reqRes = camRemoveDuplicates(reqRes);
		for (let s = 0, r = reqRes.length; s < r; ++s)
		{
			var researchReq = reqRes[s].name;
			dump("	Found: " + researchReq);
			enableResearch(researchReq, playerId);
			completeResearch(researchReq, playerId);
		}
	}
}

//////////// privates

//granted shortly after mission start to give enemy players instant droid production.
function __camGrantSpecialResearch()
{
	for (let i = 1; i < CAM_MAX_PLAYERS; ++i)
	{
		if (!allianceExistsBetween(CAM_HUMAN_PLAYER, i) && (countDroid(DROID_ANY, i) > 0 || enumStruct(i).length > 0))
		{
			//Boost AI production to produce all droids within a factory throttle
			completeResearch("R-Struc-Factory-Upgrade-AI", i);
		}
	}
}
