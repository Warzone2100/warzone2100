include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");
var consoleVar;
var tutState;
var didTheyHelpBuildGen;

//Alias for button
const CLOSE_BUTTON = 0;
const PRODUCTION_BUTTON = 1;
const RESEARCH_BUTTON = 2;
const BUILD_BUTTON = 3;
const DESIGN_BUTTON = 4;
const INTEL_BUTTON = 5;
const COMMAND_BUTTON = 6;

//consoleVar is an array of objects used to hold information about the following:
//1. audio: What sound file to play.
//2. clear: Should the console be cleared now.
//3. message: What to display in the console.
//4. state: What phase in the tutorial we need to be in to use this object
//5. wait: how long to wait, in seconds, before using this object. This is volatile and changes to zero later.
//6. func: a function to execute.
function setUpConsoleAndAudioVar()
{
	consoleVar = [
		{"audio": "tut1.ogg", "clear": false, "message": _("In this training session you'll learn how to build an effective base"), "state": 0, "wait": 1},
		{"audio": "tut2.ogg", "clear": false, "message": _("Oil resources are key to your success"), "state": 0, "wait": 3},
		{"audio": "tut3.ogg", "clear": false, "message": _("To build an oil derrick, select one of your trucks"), "state": 0, "wait": 3},
		{"audio": "tut4.ogg", "clear": false, "message": _("Left click the oil pool indicated by the radar pulse to order your truck to build"), "state": 1, "wait": 1},
		{"audio": "tut5.ogg", "clear": false, "message": _("The oil derrick is currently dormant. Build a power generator to convert the oil into power"), "state": 2, "wait": 2},
		{"audio": "tut6.ogg", "clear": false, "message": undefined, "state": 2, "wait": 2},
		{"audio": "tut7.ogg", "clear": false, "message": _("Left click the flashing build icon"), "state": 2, "wait": 4, "func": "enableBuild"},
		{"audio": "tut8.ogg", "clear": false, "message": _("Now left click the power generator in the build menu on the left"), "state": 3, "wait": 0},
		{"audio": "tut9.ogg", "clear": false, "message": _("Position the square at your base and left click to start the build process"), "state": 4, "wait": 0},
		{"audio": "tut10.ogg", "clear": false, "message": _("To increase your build rate, select your second truck"), "state": 5, "wait": 0},
		{"audio": "tut11.ogg", "clear": false, "message": _("Now left click the power generator site"), "state": 6, "wait": 2},
		{"audio": "tut12.ogg", "clear": false, "message": _("The other truck will now help to build the power generator"), "state": 7, "wait": 2},
		{"audio": "tut13.ogg", "clear": true, "message": _("During missions you need to locate and recover technologies from before the Collapse"), "state": 7, "wait": 3},
		//==PART TWO== Power Station has been built, by whatever means
		{"audio": "tut15.ogg", "clear": false, "message": _("Use a truck to search for the artifact indicated by the radar pulse"), "state": 8, "wait": 3, "func": "addTheArtifact"},
		{"audio": "tut16.ogg", "clear": false, "message": _("Move the pointer over the artifact and left click to recover it"), "state": 8, "wait": 4},
		{"audio": "tut17.ogg", "clear": false, "message": undefined, "state": 8, "wait": 3},
		{"audio": "tut18.ogg", "clear": false, "message": _("A research facility is required to research artifacts"), "state": 9, "wait": 1},
		{"audio": "tut19.ogg", "clear": false, "message": _("Left click the flashing BUILD icon"), "state": 9, "wait": 3},
		{"audio": "tut20.ogg", "clear": false, "message": _("Now left click the research facility and position it at your base"), "state": 10, "wait": 1},
		{"audio": "tut21.ogg", "clear": false, "message": _("Use your other truck to help build the research facility"), "state": 11, "wait": 0},
		//==PART THREE== they've built a research centre!!! Whahey!
		{"audio": "tut22.ogg", "clear": false, "message": _("To research the artifact, left click on the flashing RESEARCH icon"), "state": 12, "wait": 0, "func": "enableRes"},
		{"audio": "tut23.ogg", "clear": false, "message": _("Now left click the machinegun artifact"), "state": 13, "wait": 0},
		{"audio": "tut24.ogg", "clear": false, "message": _("The artifact is now being researched by the facility"), "state": 14, "wait": 0},
		{"audio": "tut28.ogg", "clear": false, "message": _("The researched machinegun can now be used to design a new vehicle"), "state": 15, "wait": 1},
		{"audio": "tut29.ogg", "clear": false, "message": _("Left click the flashing DESIGN icon"), "state": 15, "wait": 3},
		{"audio": "tut30.ogg", "clear": false, "message": _("To start your design, left click the NEW DESIGN icon"), "state": 16, "wait": 2},
		{"audio": "tut31.ogg", "clear": true, "message": _("Now left click the Vehicle Body icon"), "state": 16, "wait": 3},
		{"audio": "tut32.ogg", "clear": false, "message": _("Then left click the Viper body"), "state": 16, "wait": 3},
		{"audio": "tut33.ogg", "clear": false, "message": _("Left click the Wheels icon"), "state": 17, "wait": 1},
		{"audio": "tut34.ogg", "clear": false, "message": _("Then left click the machinegun to complete your design"), "state": 18, "wait": 1},
		{"audio": "tut35.ogg", "clear": false, "message": undefined, "state": 18, "wait": 2},
		{"audio": "tut36.ogg", "clear": false, "message": undefined, "state": 18, "wait": 3},
		{"audio": "tut37.ogg", "clear": false, "message": _("To finalize your design, left click the CLOSE icon"), "state": 19, "wait": 3},
		//==PART FOUR== they've designed their first droid!!!
		{"audio": "tut38.ogg", "clear": true, "message": _("A factory is now required to manufacture your new vehicle"), "state": 20, "wait": 2},
		{"audio": "tut39.ogg", "clear": false, "message": _("Left click the flashing BUILD icon"), "state": 20, "wait": 3},
		{"audio": "tut40.ogg", "clear": false, "message": _("Now left click the factory icon and position it at your base"), "state": 21, "wait": 3},
		{"audio": "tut41.ogg", "clear": false, "message": _("Select a location for your factory"), "state": 21, "wait": 2},
		{"audio": "tut42.ogg", "clear": true, "message": _("The delivery point is indicated by the number 1"), "state": 23, "wait": 1},
		{"audio": "tut43.ogg", "clear": false, "message": _("All new units will drive to this location once built"), "state": 23, "wait": 2},
		{"audio": "tut44.ogg", "clear": false, "message": _("You can move the delivery point at any time by left clicking it"), "state": 23, "wait": 4},
		{"audio": "tut45.ogg", "clear": false, "message": _("Now click where you want to position the delivery point"), "state": 23, "wait": 2},
		{"audio": "tut47.ogg", "clear": false, "message": _("The factory can now be ordered to build a new unit"), "state": 24, "wait": 2},
		{"audio": "tut48.ogg", "clear": false, "message": _("Left click the flashing MANUFACTURE icon"), "state": 24, "wait": 2},
		{"audio": "tut49.ogg", "clear": false, "message": undefined, "state": 25, "wait": 2},
		{"audio": "tut50.ogg", "clear": false, "message": _("Now left click the units you want to build from the menu at the left"), "state": 25, "wait": 2},
		{"audio": "tut51.ogg", "clear": false, "message": _("Please wait whilst the factory manufactures the new unit(s)"), "state": 25, "wait": 2},
		//==PART FIVE== They've built their first droid.
		{"audio": "tut81.ogg", "clear": false, "message": _("Congratulations commander - you are ready for your first mission"), "state": 26, "wait": 2},
	];
}

function increaseTutorialState()
{
	tutState = tutState + 1;
}

function grantStartTech()
{
	const TECH = [
		"R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels"
	];
	const STRUCTS = [
		"A0CommandCentre", "A0PowerGenerator", "A0ResourceExtractor",
		"A0ResearchFacility", "A0LightFactory"
	];

	camCompleteRequiredResearch(TECH, CAM_HUMAN_PLAYER);
}

function eventDroidBuilt()
{
	if (tutState === 25)
	{
		increaseTutorialState();
		//removeReticuleButton(PRODUCTION_BUTTON);
	}
}

function eventDeliveryPointMoved()
{
	if (tutState === 23)
	{
		increaseTutorialState();
		setReticuleButton(1, _("Manufacture (F1)"), "image_manufacture_up.png", "image_manufacture_down.png");
		setReticuleFlash(PRODUCTION_BUTTON, true);
	}
}

function eventMenuManufacture()
{
	if (tutState === 24)
	{
		increaseTutorialState();
		setReticuleFlash(PRODUCTION_BUTTON, false);
	}
}

//show the build button
function enableBuild()
{
	setReticuleButton(BUILD_BUTTON, _("Build (F3)"), "image_build_up.png", "image_build_down.png");
	setReticuleFlash(BUILD_BUTTON, true);
	enableStructure("A0PowerGenerator", CAM_HUMAN_PLAYER);
}

//show the research button
function enableRes()
{
	setReticuleButton(2, _("Research (F2)"), "image_research_up.png", "image_research_down.png");
	setReticuleFlash(RESEARCH_BUTTON, true);
	enableStructure("A0ResearchFacility", CAM_HUMAN_PLAYER);
}

//They clicked the button so turn off flash
function eventMenuBuild()
{
	if (tutState === 2) //power generator
	{
		increaseTutorialState();
		setReticuleFlash(BUILD_BUTTON, false);
	}
	else if (tutState === 9) // research facility
	{
		increaseTutorialState();
		setReticuleFlash(BUILD_BUTTON, false);
	}
	else if (tutState === 20)
	{
		increaseTutorialState();
		setReticuleFlash(BUILD_BUTTON, false);
	}
}

//They clicked the button so turn off flash
function eventMenuResearch()
{
	if (tutState === 12)
	{
		increaseTutorialState();
		setReticuleFlash(RESEARCH_BUTTON, false);
	}
}

function eventMenuBuildSelected()
{
	if (tutState === 3)
	{
		increaseTutorialState();
		checkForPowGen();
	}
	else if (tutState === 10)
	{
		increaseTutorialState();
		checkResFac();
	}
	else if (tutState === 21)
	{
		increaseTutorialState();
	}
}

function eventResearched(research)
{
	if (tutState === 14)
	{
		increaseTutorialState();
		setReticuleButton(DESIGN_BUTTON, _("Design (F4)"), "image_design_up.png", "image_design_down.png");
		setReticuleFlash(DESIGN_BUTTON, true);
	}
}

function eventMenuResearchSelected()
{
	if (tutState === 13)
	{
		increaseTutorialState();
		//removeReticuleButton(RESEARCH_BUTTON);
	}
}

function eventMenuDesign()
{
	if (tutState === 15)
	{
		increaseTutorialState();
		setReticuleFlash(DESIGN_BUTTON, false);
	}
}

function eventDesignBody()
{
	if (tutState === 16)
	{
		increaseTutorialState();
	}
}

function eventDesignPropulsion()
{
	if (tutState === 17)
	{
		increaseTutorialState();
	}
}

function eventDesignWeapon()
{
	if (tutState === 18)
	{
		increaseTutorialState();
		setReticuleFlash(CLOSE_BUTTON, true);
	}
}

function eventDesignQuit()
{
	if (tutState === 19)
	{
		increaseTutorialState();
		//removeReticuleButton(DESIGN_BUTTON);
		enableStructure("A0LightFactory", CAM_HUMAN_PLAYER);
		setReticuleFlash(CLOSE_BUTTON, false);
		setReticuleButton(3, _("Build (F3)"), "image_build_up.png", "image_build_down.png");
		setReticuleFlash(BUILD_BUTTON, true);
		enableTemplate("ConstructionDroid");
	}
}

//They built the power gen at tutState 4.
function checkForPowGen()
{
	if (tutState === 4)
	{
		if (countStruct("A0PowerGenerator", CAM_HUMAN_PLAYER) > 0)
		{
			//removeReticuleButton(BUILD_BUTTON);
			increaseTutorialState();
		}
		else
		{
			queue("checkForPowGen", 150);
		}
	}
}

function checkResFac()
{
	if (tutState === 11)
	{
		if (countStruct("A0ResearchFacility", CAM_HUMAN_PLAYER) > 0)
		{
			//removeReticuleButton(BUILD_BUTTON);
		}
		else
		{
			queue("checkResFac", 150);
		}
	}
}

//They need to get the second truck to help build the generator
function checkHelpBuild()
{
	if (tutState === 6)
	{
		var objects = enumDroid(CAM_HUMAN_PLAYER);
		for (var i = 0, l = objects.length; i < l; ++i)
		{
			var obj = objects[i];
			if (obj.type === DROID && obj.droidType === DROID_CONSTRUCT && obj.order === DORDER_HELPBUILD)
			{
				increaseTutorialState();
				didTheyHelpBuildGen = true;
				break;
			}
		}

		queue("checkHelpBuild", 150);
	}
}

//See local eventPickup next.
function addTheArtifact()
{
	camSetArtifacts({ "mgArtifactPos": { tech: "R-Wpn-MG1Mk1" } });
}

function eventPickup(feature, droid)
{
	if (tutState === 8)
	{
		increaseTutorialState();
		setReticuleFlash(BUILD_BUTTON, true);
		enableStructure("A0ResearchFacility", CAM_HUMAN_PLAYER);
	}
}

//automatically plays audio and displays console text when the correct state is achieved
//by requeueing itself.
function addToConsole()
{
	if (consoleVar.length)
	{
		var tutPhase = consoleVar[0];
		if (tutPhase.state <= tutState)
		{
			//Check if we need to wait
			if (camDef(tutPhase.wait) && tutPhase.wait > 0)
			{
				queue("addToConsole", tutPhase.wait * 1000); //wait is in seconds
				consoleVar[0].wait = 0;
				return;
			}

			if (camDef(tutPhase.func))
			{
				queue(tutPhase.func, 200);
			}

			if (camDef(tutPhase.audio))
			{
				playSound(tutPhase.audio, CAM_HUMAN_PLAYER);
			}

			if (camDef(tutPhase.clear) && tutPhase.clear)
			{
				clearConsole();
			}

			if (camDef(tutPhase.message))
			{
				console(tutPhase.message);
			}

			camTrace(tutState);
			consoleVar.shift();
		}

		queue("addToConsole", 600);
	}
}

function eventSelectionChanged(objects)
{
	var tut0 = tutState === 0 && consoleVar.length;
	var tut5 = tutState === 5 && consoleVar.length;

	if (tut0 || tut5)
	{
		//Check if they selected a truck.
		for (var i = 0, l = objects.length; i < l; ++i)
		{
			var obj = objects[i];
			if (obj.type === DROID && obj.droidType === DROID_CONSTRUCT)
			{
				if (tut0)
				{
					enableStructure("A0ResourceExtractor", CAM_HUMAN_PLAYER);
					increaseTutorialState();
				}
				else if (tut5 && obj.order !== DORDER_BUILD)
				{
					increaseTutorialState();
					checkHelpBuild();
				}
				break;
			}
		}
	}
}

function eventStructureBuilt(structure, droid)
{
	if (tutState === 1 && structure.stattype === RESOURCE_EXTRACTOR)
	{
		increaseTutorialState();
	}
	else if (tutState === 7 && structure.stattype === POWER_GEN)
	{
		//Maybe they did not understand instructions. Whatever the case, move on.
		if (!didTheyHelpBuildGen)
		{
			for (var i = 0; i < 3; ++i)
			{
				consoleVar.shift(); //skip ahead
			}
			tutState = 8;
		}
		else
		{
			increaseTutorialState();
		}
	}
	else if (tutState === 11 && structure.stattype === RESEARCH_LAB)
	{
		increaseTutorialState(); //If they did not make the other truck help build, move on.
	}
	else if (tutState === 22 && structure.stattype === FACTORY)
	{
		increaseTutorialState();
		//removeReticuleButton(BUILD_BUTTON);
	}
}

function eventGameInit()
{
	setTutorialMode(true);
}

//No way of winning if the extra victory callback does nothing.
function noWinningForYou() {}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, undefined, {
		callback: "noWinningForYou"
	});
	var startpos = getObject("startPosition");
	tutState = 0;
	didTheyHelpBuildGen = false;
	setUpConsoleAndAudioVar();

	centreView(startpos.x, startpos.y);
	setMissionTime(-1);
	setPower(500, CAM_HUMAN_PLAYER);
	hackStopIngameAudio();

	grantStartTech();
	setStructureLimits("A0LightFactory", 1, CAM_HUMAN_PLAYER);
	setStructureLimits("A0ResourceExtractor", 1, CAM_HUMAN_PLAYER);
	setStructureLimits("A0PowerGenerator", 1, CAM_HUMAN_PLAYER);
	setStructureLimits("A0ResearchFacility", 1, CAM_HUMAN_PLAYER);

	camSafeRemoveObject("mgArtifact", false);
	addFeature("OilResource", 24, 26);

	setMiniMap(true);
	setDesign(false);
	removeTemplate("ConstructionDroid");
	removeTemplate("ViperLtMGWheels")

	setReticuleButton(CLOSE_BUTTON, _("Close"), "image_cancel_up.png", "image_cancel_down.png");
	setReticuleButton(PRODUCTION_BUTTON, _("Manufacture - build factory first"), "", "");
	setReticuleButton(RESEARCH_BUTTON, _("Research - build research facility first"), "", "");
	setReticuleButton(BUILD_BUTTON, _("Build - manufacture constructor droids first"), "", "");
	setReticuleButton(DESIGN_BUTTON, _("Design (F4)"), "image_design_up.png", "image_design_down.png");
	setReticuleButton(INTEL_BUTTON, _("Intelligence Display (F5)"), "image_intelmap_up.png", "image_intelmap_down.png");
	setReticuleButton(COMMAND_BUTTON, _("Commanders - manufacture commanders first"), "", "");
	showInterface();

	queue("addToConsole", 2000);
}
