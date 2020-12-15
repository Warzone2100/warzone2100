
////////////////////////////////////////////////////////////////////////////////
// Debugging helpers.
////////////////////////////////////////////////////////////////////////////////

//;; ## camMarkTiles(label | array of labels)
//;;
//;; Mark area on the map by label(s), but only if debug mode is enabled.
//;; Otherwise, remember what to mark in case it is going to be.
//;;
function camMarkTiles(label)
{
	if (camIsString(label))
	{
		__camMarkedTiles[label] = true;
	}
	else
	{
		for (var i = 0, l = label.length; i < l; ++i)
		{
			__camMarkedTiles[label[i]] = true;
		}
	}
	// apply instantly
	__camUpdateMarkedTiles();
}

//;; ## camUnmarkTiles(label | array of labels)
//;;
//;; No longer mark area(s) with given label(s) in debug mode.
//;;
function camUnmarkTiles(label)
{
	if (camIsString(label))
	{
		delete __camMarkedTiles[label];
	}
	else
	{
		for (var i = 0, l = label.length; i < l; ++i)
		{
			delete __camMarkedTiles[label[i]];
		}
	}
	// apply instantly
	__camUpdateMarkedTiles();
}

//;; ## camDebug(string...)
//;;
//;; Pretty debug prints - a wrapper around ```debug()```.
//;; Prints a function call stack and the argument message,
//;; prefixed with "DEBUG". Only use this function to indicate
//;; actual bugs in the scenario script, because game shouldn't
//;; print things when nothing is broken. If you want to keep
//;; some prints around to make debugging easier without distracting
//;; the user, use ```camTrace()```.
//;;
function camDebug()
{
	__camGenericDebug("DEBUG",
	                  debugGetCallerFuncName(),
	                  arguments,
	                  true,
	                  __camBacktrace());
}

//;; ## camDebugOnce(string...)
//;;
//;; Same as ```camDebug()```, but prints each message only once
//;; during script lifetime.
//;;
function camDebugOnce()
{
	var str = debugGetCallerFuncName() + ": " + Array.prototype.join.call(arguments, " ");
	if (camDef(__camDebuggedOnce[str]))
	{
		return;
	}
	__camDebuggedOnce[str] = true;
	__camGenericDebug("DEBUG",
	                  debugGetCallerFuncName(),
	                  arguments,
	                  true,
	                  __camBacktrace());
}

//;; ## camTrace(string...)
//;;
//;; Same as ```camDebug()```, but only warns in cheat mode.
//;; Prefixed with "TRACE". It's safe and natural to keep ```camTrace()```
//;; calls in your code for easier debugging.
//;;
function camTrace()
{
	if (!camIsCheating())
	{
		return;
	}
	__camGenericDebug("TRACE",
	                  debugGetCallerFuncName(),
	                  arguments);
}

//;; ## camTraceOnce(string...)
//;;
//;; Same as ```camTrace()```, but prints each message only once
//;; during script lifetime.
//;;
function camTraceOnce()
{
	if (!camIsCheating())
	{
		return;
	}
	var str = debugGetCallerFuncName() + ": " + Array.prototype.join.call(arguments, " ");
	if (camDef(__camTracedOnce[str]))
	{
		return;
	}
	__camTracedOnce[str] = true;
	__camGenericDebug("TRACE",
	                  debugGetCallerFuncName(),
	                  arguments);
}

//;; ## camIsCheating()
//;;
//;; Check if the player is in cheat mode.
//;;
function camIsCheating()
{
	return __camCheatMode;
}

//////////// privates

function __camUpdateMarkedTiles()
{
	hackMarkTiles();
	if (camIsCheating() && camDef(__camMarkedTiles))
	{
		for (var label in __camMarkedTiles)
		{
			hackMarkTiles(label);
		}
	}
}

function __camLetMeWin()
{
	__camLetMeWinArtifacts();
	hackMarkTiles(); // clear marked tiles, as they may actually remain
	__camGameWon();
}

function __camGenericDebug(flag, func, args, err, bt)
{
	if (camDef(bt) && bt)
	{
		for (var i = bt.length - 1; i >= 0; --i)
		{
			debug("STACK: from", [bt[i]]);
		}
	}
	if (!func)
	{
		func = "<anonymous>";
	}
	var str = flag + ": " + func + ": " + Array.prototype.join.call(args, " ");
	debug(str);
	if (camDef(err) && err)
	{
		dump(str);
	}
}

function __camBacktrace()
{
	return debugGetBacktrace();
}
