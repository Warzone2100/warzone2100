
////////////////////////////////////////////////////////////////////////////////
// Debugging helpers.
////////////////////////////////////////////////////////////////////////////////

//;; ## camMarkTiles(label|labels)
//;;
//;; Mark area on the map by label(s), but only if debug mode is enabled.
//;; Otherwise, remember what to mark in case it is going to be.
//;;
//;; @param {string|string[]} label
//;; @returns {void}
//;;
function camMarkTiles(label)
{
	if (camIsString(label))
	{
		__camMarkedTiles[label] = true;
	}
	else
	{
		for (let i = 0, l = label.length; i < l; ++i)
		{
			__camMarkedTiles[label[i]] = true;
		}
	}
	// apply instantly
	__camUpdateMarkedTiles();
}

//;; ## camUnmarkTiles(label|labels)
//;;
//;; No longer mark area(s) with given label(s) in debug mode.
//;;
//;; @param {string|string[]} label
//;; @returns {void}
//;;
function camUnmarkTiles(label)
{
	if (camIsString(label))
	{
		delete __camMarkedTiles[label];
	}
	else
	{
		for (let i = 0, l = label.length; i < l; ++i)
		{
			delete __camMarkedTiles[label[i]];
		}
	}
	// apply instantly
	__camUpdateMarkedTiles();
}

//;; ## camDebug(...args)
//;;
//;; Pretty debug prints - a wrapper around `debug()`.
//;; Prints a function call stack and the argument message, prefixed with `DEBUG`.
//;; Only use this function to indicate actual bugs in the scenario script,
//;; because game shouldn't print things when nothing is broken.
//;; If you want to keep some prints around to make debugging easier
//;; without distracting the user, use `camTrace()`.
//;;
//;; @param {...string} args
//;; @returns {void}
//;;
function camDebug(...args)
{
	__camGenericDebug("DEBUG", debugGetCallerFuncName(), args, true, __camBacktrace());
}

//;; ## camDebugOnce(...args)
//;;
//;; Same as `camDebug()`, but prints each message only once during script lifetime.
//;;
//;; @param {...string} args
//;; @returns {void}
//;;
function camDebugOnce(...args)
{
	const __STR = debugGetCallerFuncName() + ": " + args.join(" ");
	if (camDef(__camDebuggedOnce[__STR]))
	{
		return;
	}
	__camDebuggedOnce[__STR] = true;
	__camGenericDebug("DEBUG", debugGetCallerFuncName(), args, true, __camBacktrace());
}

//;; ## camTrace(...args)
//;;
//;; Same as `camDebug()`, but only warns in cheat mode.
//;; Prefixed with `TRACE`. It's safe and natural to keep `camTrace()` calls in your code for easier debugging.
//;;
//;; @param {...string} args
//;; @returns {void}
//;;
function camTrace(...args)
{
	if (!camIsCheating())
	{
		return;
	}
	__camGenericDebug("TRACE", debugGetCallerFuncName(), args);
}

//;; ## camTraceOnce(...args)
//;;
//;; Same as `camTrace()`, but prints each message only once during script lifetime.
//;;
//;; @param {...string} args
//;; @returns {void}
//;;
function camTraceOnce(...args)
{
	if (!camIsCheating())
	{
		return;
	}
	const __STR = debugGetCallerFuncName() + ": " + args.join(" ");
	if (camDef(__camTracedOnce[__STR]))
	{
		return;
	}
	__camTracedOnce[__STR] = true;
	__camGenericDebug("TRACE", debugGetCallerFuncName(), args);
}

//;; ## camIsCheating()
//;;
//;; Check if the player is in cheat mode.
//;;
//;; @returns {boolean}
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
		for (const label in __camMarkedTiles)
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

function __camGenericDebug(flag, functionName, args, err, backtrace)
{
	if (camDef(backtrace) && backtrace)
	{
		for (let i = backtrace.length - 1; i >= 0; --i)
		{
			debug("STACK: from", [backtrace[i]]);
		}
	}
	if (!functionName)
	{
		functionName = "<anonymous>";
	}
	const __STR = flag + ": " + functionName + ": " + Array.prototype.join.call(args, " ");
	debug(__STR);
	if (camDef(err) && err)
	{
		dump(__STR);
	}
}

function __camBacktrace()
{
	return debugGetBacktrace();
}
