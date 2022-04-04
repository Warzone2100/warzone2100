
////////////////////////////////////////////////////////////////////////////////
// Research related functions.
////////////////////////////////////////////////////////////////////////////////

//;; ## camEnableRes(list, player)
//;;
//;; Grants research from the given list to player
//;;
function camEnableRes(list, player)
{
	for (const research of list)
	{
		enableResearch(research, player);
		completeResearch(research, player);
	}
}

//;; ## camCompleteRequiredResearch(items, player)
//;;
//;; Grants research from the given list to player and also researches
//;; the required research for that item.
//;;
function camCompleteRequiredResearch(items, player)
{
	dump("\n*Player " + player + " requesting accelerated research.");

	for (const research of items)
	{
		dump("Searching for required research of item: " + research);
		var reqRes = findResearch(research, player).reverse();

		if (reqRes.length === 0)
		{
			//HACK: autorepair like upgrades don't work after mission transition.
			if (research === "R-Sys-NEXUSrepair")
			{
				completeResearch(research, player, true);
			}
			continue;
		}

		reqRes = camRemoveDuplicates(reqRes);
		for (const reqRe of reqRes)
		{
			var researchReq = reqRe.name;
			dump("	Found: " + researchReq);
			enableResearch(researchReq, player);
			completeResearch(researchReq, player);
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
