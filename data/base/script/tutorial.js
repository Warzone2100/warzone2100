// Hack to keep the old wzscript happy
showInterface();

function eventGameInit()
{
	replaceTexture("page-8-player-buildings-bases.png", "page-8-player-buildings-bases-rockies.png");
	replaceTexture("page-9-player-buildings-bases.png", "page-9-player-buildings-bases-rockies.png");
	setSky("texpages/page-25-sky-urban.png", 0.5, 10000.0);
	setReticuleButton(0, _("Close"), "image_cancel_up.png", "image_cancel_down.png");
	setReticuleButton(1, _("Manufacture (F1)"), "image_manufacture_up.png", "image_manufacture_down.png");
	setReticuleButton(2, _("Research (F2)"), "image_research_up.png", "image_research_down.png");
	setReticuleButton(3, _("Build (F3)"), "image_build_up.png", "image_build_down.png");
	setReticuleButton(4, _("Design (F4)"), "image_design_up.png", "image_design_down.png");
	setReticuleButton(5, _("Intelligence Display (F5)"), "image_intelmap_up.png", "image_intelmap_down.png");
	setReticuleButton(6, _("Commanders (F6)"), "image_commanddroid_up.png", "image_commanddroid_down.png");
}

function eventStartLevel()
{
	setMiniMap(true);
	setDesign(false);
	removeTemplate("ConstructionDroid");
	removeTemplate("ViperLtMGWheels");
}
