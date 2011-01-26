function tick()
{
	debug("Tick, tock");
}

function eventGameInit()
{
	var p = scavengerPlayer();
	debug("TEST scav is " + p);
	setGlobalTimer("tick", 100);
}
