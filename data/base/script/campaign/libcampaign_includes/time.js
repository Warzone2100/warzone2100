
////////////////////////////////////////////////////////////////////////////////
// Time and Timer related functions.
// Useful for setting the time values for queue(), setTimer(), or setMissionTime().
////////////////////////////////////////////////////////////////////////////////

function camSecondsToMilliseconds(sec)
{
	return sec * MILLISECONDS_IN_SECOND;
}

function camMinutesToMilliseconds(min)
{
	return min * camSecondsToMilliseconds(SECONDS_IN_MINUTE);
}

function camMinutesToSeconds(min)
{
	return min * SECONDS_IN_MINUTE;
}

function camHoursToSeconds(hour)
{
	return hour * camMinutesToSeconds(MINUTES_IN_HOUR);
}
