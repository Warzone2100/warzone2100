include("script/campaign/libcampaign.js");

var cheat;

function eventChat(from, to, message)
{
	if(isCheating() && message === "let me win")
	{
		cheat = true;
	}
}

//Video if player does not yet have power module built
function resPowModVideo()
{
	hackAddMessage("MB1_B2_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);
}

//Sector clear commander!
function secondVideo()
{
	hackAddMessage("SB1_1_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);
}

//Has player built the power module?
function powerModuleBuilt()
{
	var gens = enumStruct(CAM_HUMAN_PLAYER, "A0PowerGenerator", false);
	for(var x = 0; x < gens.length; ++x)
	{
		if((gens[x].modules > 0) && (gens[x].status === BUILT))
		{
			return true;
		}
	}
	return false;
}

//Only way to pass this mission is to build a power module or be in cheat mode.
function checkForPowerModule()
{
	if(cheat || powerModuleBuilt())
	{
		camSetupTransporter(11, 52, 1, 32);
		setMissionTime(900); // 15 min
		secondVideo();
	}
	else
	{
		queue("checkForPowerModule", 3000);
	}
}

function eventStartLevel()
{
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(-1);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_1");
	cheat = false;

	if(!powerModuleBuilt())
	{
		resPowModVideo();
	}

	checkForPowerModule();
}
