function setupGame()
{
	//Use light fog for multiplayer
	setRevealStatus(true);

	if (tilesetType == "ARIZONA")
	{
		setCampaignNumber(1);
	}
	else if (tilesetType == "URBAN")
	{
		setCampaignNumber(2);
		replaceTexture("page-8-player-buildings-bases.png", "page-8-player-buildings-bases-urban.png");
		replaceTexture("page-9-player-buildings-bases.png", "page-9-player-buildings-bases-urban.png");
		replaceTexture("page-7-barbarians-arizona.png", "page-7-barbarians-urban.png");
	}
	else if (tilesetType == "ROCKIES")
	{
		setCampaignNumber(3);
		replaceTexture("page-8-player-buildings-bases.png", "page-8-player-buildings-bases-rockies.png");
		replaceTexture("page-9-player-buildings-bases.png", "page-9-player-buildings-bases-rockies.png");
		replaceTexture("page-7-barbarians-arizona.png", "page-7-barbarians-kevlar.png");
		// for some reason rockies will use arizona babas
	}
	if (tilesetType != "ARIZONA")
	{
		setSky("texpages/page-25-sky-urban.png", 0.5, 10000.0);
	}
	if (!isSpectator(-1))
	{
		// Disabled by default
		setMiniMap(false);
	}
	// Enable all templates
	setDesign(true);

	showInterface(); // init buttons. This MUST come before setting the reticule button data
	setMainReticule();
	mainReticule = true;
}
