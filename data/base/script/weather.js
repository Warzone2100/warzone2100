//Controls weather conditions like rain/snow. Fog stuff goes here.
var loaded; //don't execute stuff in eventGameInit more than once

function wDefined(x)
{
     return typeof(x) !== "undefined";
}

//Allows up to max - 1 to be generated.
function wRandom(max)
{
     return Math.floor(Math.random() * max);
}

function eventGameInit()
{
     if (wDefined(loaded) && loaded)
     {
          return;
     }

     loaded = true;
     if (tilesetType === "URBAN" || tilesetType === "ROCKIES")
     {
          weatherCycle();
          setTimer("weatherCycle", 45000);
     }
     else
     {
          setWeather(WEATHER_CLEAR);
     }
}

function weatherCycle()
{
     if (wRandom(100) > 33)
     {
          if (tilesetType === "URBAN")
          {
               setWeather(WEATHER_RAIN);
          }
          else if (tilesetType === "ROCKIES")
          {
               setWeather(WEATHER_SNOW);
          }
     }
     else
     {
          setWeather(WEATHER_CLEAR); //stop weather effect
     }
}
