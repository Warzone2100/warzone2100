// Scavenger control script

// Various constants, declared here for convenience only
const maxDroids = 25;		// max guys to handle.
const factory = "A0BaBaFactory";
const trike = "BarbarianTrike";
const buggy = "BarbarianBuggy";
const bloke = "BaBaPeople";
const jeep = "BabaJeep";
const firetruck	= "BabaFireTruck";
const cannonbus = "BabaBusCan";
const firebus = "BabaFireCan";

// scav group
var attackGroup;
var lastAttack = 0;

function activateProduction(fac)
{
	// Remind factory to produce
	if (structureIdle(fac))
	{
		buildDroid(trike, fac);
	}
}

// Regularly check back on our scavs
function scavtick()
{
	// enum functions now return a list of results
	var factorylist = enumStruct(me, factory);

	// one way of dealing with lists is running a function on each member of the list
	if (factorylist)
	{
		factorylist.forEach(activateProduction);
	}

	if ((gameTime - lastAttack) > 9000)
	{
		lastAttack = gameTime;

		// Return to nearest factory (ie base)
		var droidlist = enumGroup(attackGroup);

		if (droidlist && factorylist)
		{
			// another way of dealing with lists is to iterate over them
			// note, you must NOT use the for (... in ...) construct to iterate over an array of objects with properties!
			for (var i = 0; i < droidlist.length; i++)
			{
				var droid = droidlist[i];
				var current = 0;
				var closest = 9999;
				var clfac;		// starts undefined

				// Find closest factory; notice that we still have the factory list from earlier, which
				// saves us a few expensive scripting calls
				for (var j = 0; j < factorylist.length; j++)
				{
					var fact = factorylist[j];
					current = distBetweenTwoPoints(fact.x, fact.y, droid.x, droid.y);
					if (current < closest)
					{
						closest = current;
						clfac = fact;
					}
				}

				// If we found a factory, return to it. If clfac remains undefined, it evaluates false.
				if (clfac)
				{
					orderDroidLoc(droid, DORDER_MOVE, clfac.x, clfac.y);
				}
			}
		}
	}
}

function eventGameInit()
{
	attackGroup = newGroup();	// allocate a new group
	groupAddArea(attackGroup, 0, 0, (mapWidth * 128), (mapHeight * 128));
	scavtick();
	setGlobalTimer("scavtick", 15000);	// start a constant timer function
}

// deal with a droid being built by us
function eventDroidBuilt(droid, fac1)
{
	groupAddDroid(attackGroup, droid);

	// Build another
	if (fac1 && structureIdle(fac1) && groupSize(attackGroup) < maxDroids)
	{
		// We now have switch statements! And we can use the built-in Math library
		switch (Math.random() * 10)
		{
		case 0:	buildDroid(trike, fac1); break;
		case 1: buildDroid(buggy, fac1); break;
		case 2: buildDroid(jeep, fac1); break;
		case 3: buildDroid(cannonbus, fac1); break;
		case 4: buildDroid(firebus, fac1); break;
		default: buildDroid(bloke, fac1); break;
		}
	}
}

// watch for structures being attacked. Send the cavalry as required.
function eventStructureAttacked(structure, attacker)
{
	if ((gameTime - lastAttack) > 3000)
	{
		lastAttack = gameTime;
		var droidlist = enumGroup(attackGroup);
		for (var i = 0; i < droidlist.length; i++)
		{
			var droid = droidlist[i];
			if (distBetweenTwoPoints(structure.x, structure.y, attacker.x, attacker.y) < (24 * 128))
			{
				orderDroidLoc(droid, DORDER_MOVE, attacker.x, attacker.y);
			}
		}
	}
}
