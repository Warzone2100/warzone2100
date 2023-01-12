
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
	return seconds * MILLISECONDS_IN_SECOND;
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
	return minutes * camSecondsToMilliseconds(SECONDS_IN_MINUTE);
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
	return minutes * SECONDS_IN_MINUTE;
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
	return hours * camMinutesToSeconds(MINUTES_IN_HOUR);
}

//;; ## camSetMissionTime(time)
//;;
//;; Set mission countdown in seconds.
//;;
//;; @param {number} seconds
//;; @returns {boolean}
//;;
function camSetMissionTime(time)
{
        // revert modifiers on easy and super easy difficulties
        var modifier = 0;

        switch (difficulty)
	{
		case SUPEREASY:
			modifier = 2;
			break;
		case EASY:
			modifier = 1.5;
			break;
                default:
			modifier = 1;
			break;
        }

        return setMissionTime(time / modifier);
}
