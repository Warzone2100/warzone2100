include("script/campaign/libcampaign.js");

const mis_Labels = {
	startPos: {x: 88, y: 101},
	lz: {x: 86, y: 99, x2: 88, y2: 101},
	lz2: {x: 49, y: 83, x2: 51, y2: 85},
	trPlace: {x: 87, y: 100},
	trExit: {x: 32, y: 1}
};

function eventStartLevel()
{
	camSetupTransporter(mis_Labels.trPlace.x, mis_Labels.trPlace.y, mis_Labels.trExit.x, mis_Labels.trExit.y);
	centreView(mis_Labels.startPos.x, mis_Labels.startPos.y);
	setNoGoArea(mis_Labels.lz.x, mis_Labels.lz.y, mis_Labels.lz.x2, mis_Labels.lz.y2, CAM_HUMAN_PLAYER);
	setNoGoArea(mis_Labels.lz2.x, mis_Labels.lz2.y, mis_Labels.lz2.x2, mis_Labels.lz2.y2, CAM_THE_COLLECTIVE);
	setMissionTime(camChangeOnDiff(camHoursToSeconds(1)));
	camPlayVideos([{video: "MB2_5_MSG", type: CAMP_MSG}, {video: "MB2_5_MSG2", type: MISS_MSG}]);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_5");
}
