
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var initialStructures;
const landingZoneList = [ "NPLZ1", "NPLZ2", "NPLZ3", "NPLZ4", "NPLZ5" ];
const landingZoneMessages = [ "C1CA_LZ1", "C1CA_LZ2", "C1CA_LZ3", "C1CA_LZ4", "C1CA_LZ5" ];
var baseEstablished;
var lastLZ, lastHeavy;

// mimics wzscript's numStructsButNotWallsInArea(),
// not sure if worth including in libcampaign.
function countStructuresInBuildArea()
{
	var list = enumArea("buildArea", 0, false);
	var ret = 0;
	for (var i = 0; i < list.length; ++i)
		if (list[i].type === STRUCTURE && list[i].stattype !== WALL
		                               && list[i].status === BUILT)
		{
			++ret;
		}
	return ret;
}

// a simple extra victory condition callback
function extraVictoryCondition()
{
	// at least one enemy transporter should have landed
	if (camDef(lastLZ))
	{
		if (baseEstablished) // if base is destroyed later, we don't care
			return true;
		if (countStructuresInBuildArea() >= initialStructures + 4)
		{
			baseEstablished = true;
			hackRemoveMessage("C1CA_OBJ1", PROX_MSG, 0);
			return true;
		}
	}
	// otherwise returns 'undefined', which suppresses victory;
	// returning 'false' would have triggered an instant defeat
}

function eventStructureBuild()
{
	// update `baseEstablished' variable instantly
	// rather than by timer
	extraVictoryCondition();
}

function sendTransport()
{
	// start with light forces
	if (!camDef(lastHeavy))
		lastHeavy = true;
	// find an LZ that is not compromised
	var list = [];
	for (var i = 0; i < landingZoneList.length; ++i)
	{
		var lz = landingZoneList[i];
		if (enumArea(lz, 0, false).length === 0)
			list[list.length] = { idx: i, label: lz };
	}
	if (list.length === 0)
	{
		queue('sendTransport', 10000);
		return;
	}
	var picked = list[camRand(list.length)];
	lastLZ = picked.idx;
	var pos = camMakePos(picked.label);

	// (2 or 3 or 4) pairs of each droid template.
	// This emulates wzcam's droid count distribution.
	var count = [ 2, 3, 4, 4, 4, 4, 4, 4, 4 ][camRand(9)];

	var templates;
	if (lastHeavy)
	{
		lastHeavy = false;
		queue('sendTransport', 90000);
		with (camTemplates) templates = [ nppod, nphmg, npmrl, npsmc ];
	}
	else
	{
		lastHeavy = true;
		queue('sendTransport', 180000);
		with (camTemplates) templates = [ npsmct, npmor, npsmc, npmmct,
		                                  npmrl, nphmg, npsbb ];
	}

	var droids = [];
	for (var i = 0; i < count; ++i)
	{
		var t = templates[camRand(templates.length)];
		// two droids of each template
		droids[droids.length] = t;
		droids[droids.length] = t;
	}

	camSendReinforcement(1, pos, droids, CAM_REINFORCE_TRANSPORT, {
		entry: { x: 126, y: 36 },
		exit: { x: 126, y: 76 },
		message: landingZoneMessages[lastLZ],
		order: CAM_ORDER_ATTACK,
		data: { regroup: true, count: -1, pos: "buildArea" }
	});
}

function eventStartLevel()
{
	initialStructures = countStructuresInBuildArea();
	baseEstablished = false;

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_1_4AS", {
		callback: "extraVictoryCondition"
	});
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, 0);

	// make sure player doesn't build on enemy LZs
	for (var i = 1; i <= 5; ++i)
	{
		var ph = getObject("PhantomLZ" + i);
		// HACK: set LZs of bad players, namely 2...6,
		// note: player 1 is NP
		setNoGoArea(ph.x, ph.y, ph.x2, ph.y2, i + 1);
	}

	setMissionTime(1800);
	hackAddMessage("MB1CA_MSG", MISS_MSG, 0, true);
	hackAddMessage("C1CA_OBJ1", PROX_MSG, 0, false);

	// first transport after 10 seconds; will re-queue itself
	queue('sendTransport', 10000);
}
