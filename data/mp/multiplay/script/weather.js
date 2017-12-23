//Controls weather conditions like rain/snow. Fog stuff goes here.

function weatherCycle()
{
     if (syncRandom(100) > 33)
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
