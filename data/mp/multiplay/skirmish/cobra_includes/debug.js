//Hold of the debug related stuff here.

//Dump some text.
function log(message)
{
	dump(gameTime + " : " + message);
}

//Dump information about an object and some text.
function logObj(obj, message)
{
	dump(gameTime + " : [" + obj.name + " id=" + obj.id + "] > " + message);
}

function debugLogAtEnd()
{
     if (!DEBUG_LOG_ON)
     {
          return;
     }

     var wonDebug = countStruct("A0LightFactory", me) + countStruct("A0CyborgFactory", me) + countDroid(DROID_ANY);
     if (freeForAll() || ((gameTime > 90000) && !wonDebug))
     {
          const TAB = "     ";
          log("**DEBUG_SESSION**");

          log("--PERSONALITY--");
          log(TAB + personality);

          log("--MAP OIL LEVEL--");
          log(TAB + mapOilLevel());

          log("--MAP OIL AVERAGE PER PLAYER--");
          log(TAB + averageOilPerPlayer());

          log("--SCAVENGERS--");
          log(isDefined(scavengerPlayer) ? TAB + "YES" : TAB + "NO");

          log("--WON--");
          log(wonDebug ? TAB + "YES" : TAB + "NO");

          log("--RESEARCH PATH--");
          log(TAB + subPersonalities[personality].resPath);

          log("--RESEARCH HISTORY--");
          for (var i = 0, l = resHistory.length; i < l; ++i)
          {
               log(TAB + resHistory[i]);
          }
          resHistory = [];

          log("**DEBUG_END**");
          removeThisTimer("debugLogAtEnd");
     }
}
