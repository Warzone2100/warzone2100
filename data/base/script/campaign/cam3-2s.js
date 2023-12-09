include("script/campaign/libcampaign.js");

const mis_Labels = {
	startPos: {x: 57, y: 119},
	lz: {x: 55, y: 119, x2: 57, y2: 121},
	lz2: {x: 7, y: 52, x2: 9, y2: 54},
	trPlace: {x: 56, y: 120},
	trExit: {x: 25, y: 87}
};

function eventStartLevel()
{
	camSetupTransporter(mis_Labels.trPlace.x, mis_Labels.trPlace.y, mis_Labels.trExit.x, mis_Labels.trExit.y);
	centreView(mis_Labels.startPos.x, mis_Labels.startPos.y);
	setNoGoArea(mis_Labels.lz.x, mis_Labels.lz.y, mis_Labels.lz.x2, mis_Labels.lz.y2, CAM_HUMAN_PLAYER);
	setNoGoArea(mis_Labels.lz2.x, mis_Labels.lz2.y, mis_Labels.lz2.x2, mis_Labels.lz2.y2, CAM_NEXUS);
	setMissionTime(camChangeOnDiff(camHoursToSeconds(1)));
	camPlayVideos([{video: "MB3_2_MSG", type: CAMP_MSG}, {video: "MB3_2_MSG2", type: MISS_MSG}]);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_3_2");
}
