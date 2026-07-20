
////////////////////////////////////////////////////////////////////////////////
// Time and Timer related functions.
// Useful for setting the time values for queue(), setTimer(), or setMissionTime().
////////////////////////////////////////////////////////////////////////////////

//;; ## camSecondsToMilliseconds(seconds)
//;;
//;; Returns the amount of milliseconds in `seconds`.
//;;
//;; @param {number} seconds
//;; @returns {number}
//;;
function camSecondsToMilliseconds(seconds)
{
	return seconds * CAM_MILLISECONDS_IN_SECOND;
}

//;; ## camMinutesToMilliseconds(minutes)
//;;
//;; Returns the amount of milliseconds in `minutes`.
//;;
//;; @param {number} minutes
//;; @returns {number}
//;;
function camMinutesToMilliseconds(minutes)
{
	return minutes * camSecondsToMilliseconds(CAM_SECONDS_IN_MINUTE);
}

//;; ## camMinutesToSeconds(minutes)
//;;
//;; Returns the amount of seconds in `minutes`.
//;;
//;; @param {number} minutes
//;; @returns {number}
//;;
function camMinutesToSeconds(minutes)
{
	return minutes * CAM_SECONDS_IN_MINUTE;
}

//;; ## camHoursToSeconds(hours)
//;;
//;; Returns the amount of seconds in `hours`.
//;;
//;; @param {number} hours
//;; @returns {number}
//;;
function camHoursToSeconds(hours)
{
	return hours * camMinutesToSeconds(CAM_MINUTES_IN_HOUR);
}

//;; ## camSetMissionTimer(seconds)
//;;
//;; Sets the mission time for a level. This acts as a wrapper for `setMissionTime()`
//;; and is mostly used to either set the time value passed (as seconds) or infinite
//;; time if the `infiniteTime` tweak option is active.
//;;
//;; @param {number} time value in seconds
//;; @returns {void}
//;;
function camSetMissionTimer(seconds)
{
	let timer = -1;
	if (camDef(seconds) &&
		camDef(tweakOptions.infiniteTime) &&
		!tweakOptions.infiniteTime &&
		(seconds >= 0))
	{
		timer = seconds;
	}
	setMissionTime(timer);
}
