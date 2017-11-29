// Hack to keep the old wzscript happy
showInterface();

function eventGameInit()
{
	replaceTexture("page-8-player-buildings-bases.png", "page-8-player-buildings-bases-rockies.png");
	replaceTexture("page-9-player-buildings-bases.png", "page-9-player-buildings-bases-rockies.png");
	setSky("texpages/page-25-sky-urban.png", 0.5, 10000.0);
	setReticuleButton(0, _("Close"), "image_cancel_up.png", "image_cancel_down.png");
}

function eventStartLevel()
{
	setMiniMap(true);
	setDesign(false);
	removeTemplate("ConstructionDroid");
	removeTemplate("ViperLtMGWheels");
}
