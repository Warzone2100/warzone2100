// Scavenger control script

var scavengerFile;

if (scavengers === SCAVENGERS)
{
	scavengerFile = "originalScavengers.js";
}
else
{
	scavengerFile = "ultimateScavengers.js";
}

include("/multiplay/script/scavengers/" + scavengerFile);
