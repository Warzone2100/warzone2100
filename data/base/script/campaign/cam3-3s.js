include("script/campaign/libcampaign.js");

const mis_Labels = {
	startPos: {x: 57, y: 119},
	lz: {x: 55, y: 119, x2: 57, y2: 121},
	trPlace: {x: 56, y: 120},
	trExit: {x: 25, y: 87}
};

function giveExtraPower()
{
	const EXTRA_POWER = 10000; // So player doesn't have to wait.
	setPower(playerPower(CAM_HUMAN_PLAYER) + EXTRA_POWER);
	playSound(cam_sounds.powerTransferred);
}

function eventStartLevel()
{
	camSetupTransporter(mis_Labels.trPlace.x, mis_Labels.trPlace.y, mis_Labels.trExit.x, mis_Labels.trExit.y);
	centreView(mis_Labels.startPos.x, mis_Labels.startPos.y);
	setNoGoArea(mis_Labels.lz.x, mis_Labels.lz.y, mis_Labels.lz.x2, mis_Labels.lz.y2, CAM_HUMAN_PLAYER);
	camSetMissionTimer(camChangeOnDiff(camMinutesToSeconds(120)));
	camPlayVideos([{video: "MB3_3_MSG", type: CAMP_MSG}]);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, cam_levels.gammaBonus.offWorld);
	queue("giveExtraPower", camSecondsToMilliseconds(2));
}
