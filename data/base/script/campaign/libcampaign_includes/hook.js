
////////////////////////////////////////////////////////////////////////////////
// Event hooks magic. This makes all the library catch all the necessary events
// without distracting the script authors from handling them on their own.
// It assumes that script authors do not implement another similar mechanism,
// or something BAD would happen.
////////////////////////////////////////////////////////////////////////////////

//;; ## camAreaEvent(label, callback(droid))
//;;
//;; Implement `eventArea${label}()` in a debugging-friendly way.
//;; The function marks the area until the event is triggered,
//;; and traces entering the area in the TRACE log.
//;;
//;; @param {string} label
//;; @param {Function} callback
//;; @returns {void}
//;;
function camAreaEvent(label, callback)
{
	const __EVENT_NAME = "eventArea" + label;
	camMarkTiles(label);
	__camPreHookEvent(__EVENT_NAME, function(droid)
	{
		if (camDef(droid))
		{
			camTrace("Player", droid.player, "enters", label);
		}
		camUnmarkTiles(label);
		callback(droid);
	});
}

//////////// privates

function __camPreHookEvent(eventname, hookcode)
{
	// store the original event handler
	if (camDef(__camGlobalContext()[eventname]))
	{
		__camOriginalEvents[eventname] = __camGlobalContext()[eventname];
	}

	__camGlobalContext()[eventname] = function()
	{
		// Don't trigger hooks after level end.
		// No, this should't say "if (__camLevelEnded) return;".
		// Because we are not hooking all possible events,
		// and we are not documenting which exactly events are hooked.
		// We should not intervene with level events in unobvious ways.
		if (!__camLevelEnded)
		{
			hookcode.apply(__camGlobalContext(), arguments);
		}
		// otherwise, call the original event if any
		if (camDef(__camOriginalEvents[eventname]))
		{
			__camOriginalEvents[eventname].apply(__camGlobalContext(), arguments);
		}
	};
}
