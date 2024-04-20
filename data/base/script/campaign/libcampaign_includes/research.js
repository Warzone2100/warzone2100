
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
		const __RESEARCH_ID = researchIds[i];
		enableResearch(__RESEARCH_ID, playerId);
		completeResearch(__RESEARCH_ID, playerId);
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
		const __RESEARCH_ID = researchIds[i];
		dump("Searching for required research of item: " + __RESEARCH_ID);
		let reqRes = findResearch(__RESEARCH_ID, playerId).reverse();

		if (reqRes.length === 0)
		{
			//HACK: autorepair like upgrades don't work after mission transition.
			if (__cam_nexusTech.indexOf(__RESEARCH_ID) !== -1)
			{
				completeResearch(__RESEARCH_ID, playerId, true);
			}
			continue;
		}

		reqRes = camRemoveDuplicates(reqRes);
		for (let s = 0, r = reqRes.length; s < r; ++s)
		{
			const __RESEARCH_REQ = reqRes[s].name;
			dump("	Found: " + __RESEARCH_REQ);
			enableResearch(__RESEARCH_REQ, playerId);
			completeResearch(__RESEARCH_REQ, playerId);
		}
	}
}

//;; ## camClassicResearch(researchIds, playerId)
//;;
//;; Grants research from the given list to player based on the "classic balance" variant.
//;;
//;; @param {string[]} researchIds
//;; @param {number} playerId
//;; @returns {void}
//;;
function camClassicResearch(researchIds, playerId)
{
	if (!camClassicMode())
	{
		return;
	}
	if (tweakOptions.camClassic_balance32)
	{
		camEnableRes(researchIds, playerId);
	}
	else
	{
		camCompleteRequiredResearch(researchIds, playerId);
	}
}


//////////// privates

//granted shortly after mission start to give enemy players instant droid production.
function __camGrantSpecialResearch()
{
	for (let i = 1; i < __CAM_MAX_PLAYERS; ++i)
	{
		if (!allianceExistsBetween(CAM_HUMAN_PLAYER, i) && (countDroid(DROID_ANY, i) > 0 || enumStruct(i).length > 0))
		{
			//Boost AI production to produce all droids within a factory throttle
			completeResearch(__CAM_AI_INSTANT_PRODUCTION_RESEARCH, i);
		}
	}
}
