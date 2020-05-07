/*
SUB_2_1S campaign script
Authors: Cristian Odorico (Alpha93) / KJeff01
 */
include("script/campaign/libcampaign.js");

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
	setNoGoArea(49, 83, 51, 85, THE_COLLECTIVE);
	//Set Mission Time
	setMissionTime(camChangeOnDiff(camMinutesToSeconds(30)));
	//Give player briefings
	camPlayVideos(["MB2_1_MSG", "MB2_1_MSG2"]);
}
