include("script/campaign/libcampaign.js");

const mis_Labels = {
	startPos: {x: 57, y: 119},
	lz: {x: 55, y: 119, x2: 57, y2: 121},
	trPlace: {x: 56, y: 120},
	trExit: {x: 25, y: 87}
};

function eventStartLevel()
{
	camSetupTransporter(mis_Labels.trPlace.x, mis_Labels.trPlace.y, mis_Labels.trExit.x, mis_Labels.trExit.y);
	centreView(mis_Labels.startPos.x, mis_Labels.startPos.y);
	setNoGoArea(mis_Labels.lz.x, mis_Labels.lz.y, mis_Labels.lz.x2, mis_Labels.lz.y2, CAM_HUMAN_PLAYER);
	camSetMissionTimer(camChangeOnDiff(camMinutesToSeconds((tweakOptions.classicTimers) ? 70 : 75)));
	camPlayVideos([{video: "MB3_1A_MSG", type: CAMP_MSG}, {video: "MB3_1A_MSG2", type: MISS_MSG}]);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, cam_levels.gamma2.offWorld);
}
