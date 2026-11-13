// Shared by the skirmish AIs: make them ignore everything outside the map's scroll limits.
//
// Maps can restrict the playable area with scroll limits, while the terrain (and any oil
// resources, scavenger bases, etc.) outside of it still exists. Ground units can't path
// there and players can't see it, so the AIs must not try to use or target anything there.
//
// Including this file wraps the object enumeration API so that objects outside the scroll
// limits are never returned (the AI's own objects are always kept), keeps the results of
// pickStructLocation() inside the playable area, and provides helpers for code that
// computes map positions on its own.
//
// Include it before any other AI code.

(function(_global) {

// The pathfinder blocks the outermost tile row/column inside the scroll limits for ground units.
const BLOCKED_BORDER_TILES = 1;

// The current scroll limits, clipped to the map. Read every time rather than cached:
// they are cheap to get, and a cache would be state that a saved game doesn't keep.
function getLimits()
{
	const l = getScrollLimits(); // {x, y, x2, y2}
	return {
		x1: Math.max(0, l.x),
		y1: Math.max(0, l.y),
		x2: Math.min(mapWidth, l.x2),
		y2: Math.min(mapHeight, l.y2),
	};
}

function isWholeMap(limits)
{
	return (limits.x1 <= 0 && limits.y1 <= 0 && limits.x2 >= mapWidth && limits.y2 >= mapHeight);
}

function inside(limits, x, y, margin)
{
	return (x >= limits.x1 + margin && y >= limits.y1 + margin &&
		x < limits.x2 - margin && y < limits.y2 - margin);
}

function clampAxis(v, lo, hi)
{
	if (hi < lo)
	{
		return Math.floor((lo + hi) / 2);
	}
	return Math.min(Math.max(v, lo), hi);
}

// The AI's own objects are always kept.
function keepObject(limits, obj)
{
	return (!obj || obj.player === me || inside(limits, obj.x, obj.y, 0));
}

function filterObjects(list)
{
	const limits = getLimits();
	if (isWholeMap(limits) || !Array.isArray(list))
	{
		return list;
	}
	return list.filter((obj) => keepObject(limits, obj));
}

// Returns the current scroll limits as {x1, y1, x2, y2} (tile coordinates, x2/y2 exclusive).
_global.getAIScrollLimits = function()
{
	return getLimits();
};

// withinScrollLimits(obj[, margin]) or withinScrollLimits(x, y[, margin])
// True if the given object or tile position is inside the scroll limits, at least
// `margin` tiles away from their edge.
_global.withinScrollLimits = function(a, b, c)
{
	let x, y, margin;
	if (typeof a === "object" && a !== null)
	{
		x = a.x;
		y = a.y;
		margin = b;
	}
	else
	{
		x = a;
		y = b;
		margin = c;
	}
	return inside(getLimits(), x, y, (typeof margin === "number") ? margin : 0);
};

// Returns {x, y} moved inside the scroll limits, at least `margin` tiles away from their edge.
_global.clampToScrollLimits = function(x, y, margin)
{
	const limits = getLimits();
	const m = (typeof margin === "number") ? margin : 0;
	return {
		x: clampAxis(x, limits.x1 + m, limits.x2 - 1 - m),
		y: clampAxis(y, limits.y1 + m, limits.y2 - 1 - m),
	};
};

// Wrap the enumeration API.
for (const name of ["enumStruct", "enumDroid", "enumFeature", "enumRange", "enumArea", "enumBlips"])
{
	const original = _global[name];
	if (typeof original !== "function")
	{
		continue;
	}
	_global[name] = function(...args)
	{
		return filterObjects(original(...args));
	};
}

const originalGetObject = _global.getObject;
if (typeof originalGetObject === "function")
{
	_global.getObject = function(...args)
	{
		const obj = originalGetObject(...args);
		const limits = getLimits();
		if (isWholeMap(limits) || keepObject(limits, obj))
		{
			return obj;
		}
		return null;
	};
}

// pickStructLocation() searches outwards from the given position and knows nothing about the
// scroll limits, so near the edge it can pick a spot outside of them. Start the search inside,
// and if the result still ends up too close to (or past) the edge, start again further inwards.
const originalPickStructLocation = _global.pickStructLocation;
if (typeof originalPickStructLocation === "function")
{
	// Keep the centre of the structure far enough from the edge that a 3x3 footprint stays
	// out of the pathfinder-blocked border.
	const PICK_MARGIN = BLOCKED_BORDER_TILES + 1;
	const MAX_ATTEMPTS = 4;

	_global.pickStructLocation = function(droid, structureName, x, y, ...rest)
	{
		const limits = getLimits();
		if (isWholeMap(limits))
		{
			return originalPickStructLocation(droid, structureName, x, y, ...rest);
		}

		const minX = limits.x1 + PICK_MARGIN;
		const minY = limits.y1 + PICK_MARGIN;
		const maxX = limits.x2 - 1 - PICK_MARGIN;
		const maxY = limits.y2 - 1 - PICK_MARGIN;
		let startX = clampAxis(Math.floor(x), minX, maxX);
		let startY = clampAxis(Math.floor(y), minY, maxY);

		for (let attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
		{
			const pos = originalPickStructLocation(droid, structureName, startX, startY, ...rest);
			if (!pos)
			{
				return pos;
			}
			if (pos.x >= minX && pos.y >= minY && pos.x <= maxX && pos.y <= maxY)
			{
				return pos;
			}
			// Move the search start away from whichever edge the result went past.
			if (pos.x < minX)
			{
				startX += minX - pos.x + 1;
			}
			else if (pos.x > maxX)
			{
				startX -= pos.x - maxX + 1;
			}
			if (pos.y < minY)
			{
				startY += minY - pos.y + 1;
			}
			else if (pos.y > maxY)
			{
				startY -= pos.y - maxY + 1;
			}
			startX = clampAxis(startX, minX, maxX);
			startY = clampAxis(startY, minY, maxY);
		}

		return undefined;
	};
}

})(this);
