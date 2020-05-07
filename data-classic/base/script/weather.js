//Controls weather conditions like rain/snow. Fog stuff goes here.

//Allows up to max - 1 to be generated.
function wRandom(max)
{
     return Math.floor(Math.random() * max);
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
