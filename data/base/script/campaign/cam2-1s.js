/*
SUB_2_1S campaign script
Authors: Cristian Odorico (Alpha93) / KJeff01
 */
include("script/campaign/libcampaign.js");
var index;

function eventVideoDone()
{
	const VIDEOS = ["MB2_1_MSG", "MB2_1_MSG2"];
	if(!camDef(index))
	{
		index = 0;
	}

	if(index < VIDEOS.length)
	{
		hackAddMessage(VIDEOS[index], MISS_MSG, CAM_HUMAN_PLAYER, true);
		index = index + 1;
	}
}

function eventStartLevel()
{
	//Set Victory Conditions
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_1");
	//Setup Transport
	camSetupTransporter(87, 100, 70, 126);
	//Centre View on Area
	centreView(88, 101);
	//Setup Landing Zone
	setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
	//Set Mission Time
	setMissionTime(camChangeOnDiff(1800));
	//Give player briefings
	eventVideoDone();
}
