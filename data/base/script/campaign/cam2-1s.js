include("script/campaign/libcampaign.js");

const mis_Labels = {
	startPos: {x: 88, y: 101},
	lz: {x: 86, y: 99, x2: 88, y2: 101},
	trPlace: {x: 87, y: 100},
	trExit: {x: 70, y: 126}
};

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, cam_levels.beta2.offWorld);
	camSetupTransporter(mis_Labels.trPlace.x, mis_Labels.trPlace.y, mis_Labels.trExit.x, mis_Labels.trExit.y);
	centreView(mis_Labels.startPos.x, mis_Labels.startPos.y);
	setNoGoArea(mis_Labels.lz.x, mis_Labels.lz.y, mis_Labels.lz.x2, mis_Labels.lz.y2, CAM_HUMAN_PLAYER);
	camSetMissionTimer(camChangeOnDiff(camMinutesToSeconds(30)));
	camPlayVideos([{video: "MB2_1_MSG", type: CAMP_MSG}, {video: "MB2_1_MSG2", type: MISS_MSG}]);
}
