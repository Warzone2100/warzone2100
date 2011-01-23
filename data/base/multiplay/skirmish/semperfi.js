function tick()
{
	debug("Tick, tock");
}

function eventGameInit()
{
	var p = scavengerPlayer();
	console("hey, i'm ze console! scav is " + p);
	debug("TEST scav is " + p);
	setGlobalTimer(tick, 100);
}
