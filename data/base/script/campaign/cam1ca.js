
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const landingZoneList = [ "NPLZ1", "NPLZ2", "NPLZ3", "NPLZ4", "NPLZ5" ];
const landingZoneMessages = [ "C1CA_LZ1", "C1CA_LZ2", "C1CA_LZ3", "C1CA_LZ4", "C1CA_LZ5" ];
var blipActive;
var lastLZ, lastHeavy;
var totalTransportLoads;

//See if we have enough structures on the plateau area and toggle
//the green objective blip on or off accordingly.
function baseEstablished()
{
	//Now we check if there is stuff built here already from cam1-C.
	if (camCountStructuresInArea("buildArea") >= 7)
	{
		if (blipActive)
		{
			blipActive = false;
			hackRemoveMessage("C1CA_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
		}
		return true;
	}
	else
	{
		if (!blipActive)
		{
			blipActive = true;
			hackAddMessage("C1CA_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);
		}
		return false;
	}
}

// a simple extra victory condition callback
function extraVictoryCondition()
{
	const MIN_TRANSPORT_RUNS = 10;
	var enemies = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false);
	// No enemies on map and at least eleven New Paradigm transport runs.
	if (baseEstablished() && (totalTransportLoads > MIN_TRANSPORT_RUNS) && !enemies.length)
	{
		return true;
	}
	// otherwise returns 'undefined', which suppresses victory;
	// returning 'false' would have triggered an instant defeat
}

function sendTransport()
{
	// start with light forces
	if (!camDef(lastHeavy))
	{
		lastHeavy = true;
	}
	// find an LZ that is not compromised
	var list = [];
	var i = 0;
	for (i = 0; i < landingZoneList.length; ++i)
	{
		var lz = landingZoneList[i];
		if (enumArea(lz, CAM_HUMAN_PLAYER, false).length === 0)
		{
			list.push({ idx: i, label: lz });
		}
	}
	//If all are compromised then choose the LZ randomly
	if (list.length === 0)
	{
		for (i = 0; i < 2; ++i)
		{
			var rnd = camRand(landingZoneList.length);
			list.push({ idx: rnd, label: landingZoneList[rnd] });
		}
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
		queue("sendTransport", camChangeOnDiff(90000));
		templates = [ cTempl.nppod, cTempl.nphmg, cTempl.npmrl, cTempl.npsmc ];
	}
	else
	{
		lastHeavy = true;
		queue("sendTransport", camChangeOnDiff(180000));
		templates = [ cTempl.npsmct, cTempl.npmor, cTempl.npsmc, cTempl.npmmct, cTempl.npmrl, cTempl.nphmg, cTempl.npsbb ];
	}

	var droids = [];
	for (i = 0; i < count; ++i)
	{
		var t = templates[camRand(templates.length)];
		// two droids of each template
		droids[droids.length] = t;
		droids[droids.length] = t;
	}

	camSendReinforcement(NEW_PARADIGM, pos, droids, CAM_REINFORCE_TRANSPORT, {
		entry: { x: 126, y: 36 },
		exit: { x: 126, y: 76 },
		message: landingZoneMessages[lastLZ],
		order: CAM_ORDER_ATTACK,
		data: { regroup: true, count: -1, pos: "buildArea" }
	});

	totalTransportLoads = totalTransportLoads + 1;
}

function eventStartLevel()
{
	totalTransportLoads = 0;
	blipActive = false;

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_1_4AS", {
		callback: "extraVictoryCondition"
	});
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	// make sure player doesn't build on enemy LZs
	for (var i = 1; i <= 5; ++i)
	{
		var ph = getObject("PhantomLZ" + i);
		// HACK: set LZs of bad players, namely 2...6,
		// note: player 1 is NP
		setNoGoArea(ph.x, ph.y, ph.x2, ph.y2, i + 1);
	}

	setMissionTime(camChangeOnDiff(1800));
	camPlayVideos("MB1CA_MSG");

	// first transport after 10 seconds; will re-queue itself
	queue('sendTransport', 10000);
}
