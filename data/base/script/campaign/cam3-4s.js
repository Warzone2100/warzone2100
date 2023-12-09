include("script/campaign/libcampaign.js");

const mis_Labels = {
	startPos: {x: 50, y: 245},
	lz: {x: 49, y: 244, x2: 51, y2: 246},
	lz2: {x: 7, y: 52, x2: 9, y2: 54},
	trPlace: {x: 50, y: 245},
	trExit: {x: 63, y: 118},
	limits: {x: 0, y: 137, x2: 64, y2: 256}
};

function eventStartLevel()
{
	camSetupTransporter(mis_Labels.trPlace.x, mis_Labels.trPlace.y, mis_Labels.trExit.x, mis_Labels.trExit.y);
	centreView(mis_Labels.startPos.x, mis_Labels.startPos.y);
	setNoGoArea(mis_Labels.lz.x, mis_Labels.lz.y, mis_Labels.lz.x2, mis_Labels.lz.y2, CAM_HUMAN_PLAYER);
	setNoGoArea(mis_Labels.lz2.x, mis_Labels.lz2.y, mis_Labels.lz2.x2, mis_Labels.lz2.y2, CAM_NEXUS);
	setScrollLimits(mis_Labels.limits.x, mis_Labels.limits.y, mis_Labels.limits.x2, mis_Labels.limits.y2);
	setMissionTime(camMinutesToSeconds(30));
	setPower(playerPower(CAM_HUMAN_PLAYER) + 50000, CAM_HUMAN_PLAYER);
	camPlayVideos([{video: "MB3_4_MSG", type: CAMP_MSG}, {video: "MB3_4_MSG2", type: MISS_MSG}]);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "CAM_3_4");
}
