
const MIN_ATTACKERS = 12;
const MIN_NEXUS = 4;
const MAX_DEFENDERS = 8;
const MAX_GLOBAL_DEFENDERS = 25;
const MAX_SENSORS = 5;
const MAX_UNITS = 300;
const MAX_HELICOPTERS = 40;
const MAX_CRANES = 10;

const CRANE_BODY = "B2crane";
const CRANE_WEAP = "scavCrane";


const derrick = "A0ResourceExtractor";
const factory = "A0BaBaFactory";
const vtolfac = "A0BaBaVtolFactory";
const gen = "A0BaBaPowerGenerator";
const oilres = "OilResource";
const repair = "ScavRepairCentre";
const vtolpad = "A0BaBaVtolPad";

const defenses = [
    "A0BaBaBunker",
    "A0BaBaBunker",
    "A0BaBaBunker",
    "A0CannonTower",
    "A0CannonTower",
    "A0CannonTower",
    "A0BaBaFlameTower",
    "A0BaBaFlameTower",
    "A0BaBaRocketPit",
    "A0BaBaRocketPit",
    "A0BaBaRocketPitAT",
    "A0BaBaMortarPit",
    "bbaatow",
];

const templates = [
    ["B4body-sml-trike01-Ultimate","bTrikeMG"],
    ["B4body-sml-trike01-Ultimate","bTrikeMG"],
    ["B4body-sml-trike01-Ultimate","bTrikeMG"],
    ["B4body-sml-trike01-Ultimate","bTrikeMG"],
    ["B3body-sml-buggy01-Ultimate","BuggyMG"],
    ["B3body-sml-buggy01-Ultimate","BuggyMG"],
    ["B2JeepBody-Ultimate","BJeepMG"],
    ["B2JeepBody-Ultimate","BJeepMG"],
    ["B2JeepBody-Ultimate","BJeepMG"],
    ["B3bodyRKbuggy01-Ultimate","BabaRocket"],
    ["B3bodyRKbuggy01-Ultimate","BabaRocket"],
    ["B2RKJeepBody-Ultimate","BabaRocket"],
    ["B2RKJeepBody-Ultimate","BabaRocket"],
    ["BusBody","BusCannon"],
    ["BusBody","BusCannon"],
    ["BusBody","BabaPitRocketAT"],
    ["B2tractor","BabaFlame"],
    ["B2tractor","BabaFlame"],
    ["B2tractor","BabaFlame"],
    ["FireBody","BabaFlame"],
    ["FireBody","BabaFlame"],
    ["FireBody","BusCannon"],
    ["FireBody","BabaPitRocket"],
    ["FireBody","BabaPitRocketAT"],
    ["ScavCamperBody","BabaPitRocket"],
    ["ScavCamperBody","BusCannon"],
    ["ScavTruckBody","BabaFlame","BabaRocket","BabaPitRocketAT"],
    ["ScavTruckBody","BusCannon","BabaPitRocket","BabaRocket"],
    ["ScavIcevanBody","BabaFlame"],
    ["ScavIcevanBody","Mortar1Mk1"],
    ["ScavNEXUStrack","ScavNEXUSlink"],
    ["ScavNEXUStrack","ScavNEXUSlink"],
    ["ScavNEXUStrack","ScavNEXUSlink"],
];

const vtolTemplates = [
    ["ScavengerChopper", "MG1-VTOL-SCAVS"],
    ["HeavyChopper", "Rocket-VTOL-Pod-SCAVS"],
];

// scav groups
var globalDefendGroup; // tanks that defend all bases
var needToPickGroup; // a group
var baseInfo = [];

// unit limit constant
function atLimits()
{
    return countDroid(DROID_ANY, me) >= MAX_UNITS;
}

// random integer between 0 and max-1 (for convenience)
function random(max)
{
    return (max <= 0) ? 0 : Math.floor(Math.random() * max);
}

// Returns true if something is defined
function isDefined(data)
{
    return typeof(data) !== "undefined";
}

function isCopterPropulsion(droidProp)
{
    var helicopterPropulsions = [
        "Helicopter",
    ];

    for (var i = 0, len = helicopterPropulsions.length; i < len; ++i)
    {
        var propulsion = helicopterPropulsions[i];

        if (propulsion === droidProp)
        {
            return true;
        }
    }

    return false;
}

// Make sure a unit does not try to go off map
function mapLimits(x, y, num1, num2, xOffset, yOffset)
{
    var xPos = x + xOffset + random(num1) - num2;
    var yPos = y + yOffset + random(num1) - num2;

    if (xPos < 2)
    {
        xPos = 2;
    }
    if (yPos < 2)
    {
        yPos = 2;
    }
    if (xPos >= mapWidth - 2)
    {
        xPos = mapWidth - 3;
    }
    if (yPos >= mapHeight - 2)
    {
        yPos = mapHeight - 3;
    }

    return {x: xPos, y: yPos};
}

//Return a closeby enemy object. Will be undefined if none.
function rangeStep(obj, visibility)
{
    const MAX_TILE_LIMIT = 260;
    const STEP = 5;
    var target;

    for (var i = 0; i <= MAX_TILE_LIMIT; i += STEP)
    {
        var temp = enumRange(obj.x, obj.y, i, ENEMIES, visibility);
        if (temp.length > 0)
        {
            target = findNearest(temp, obj.x, obj.y, true);
            break;
        }
    }

    return target;
}

function constructBaseInfo(factory)
{
    var info = {
        x: factory.x,
        y: factory.y,
        id: factory.id,
        defendGroup: newGroup(),
        nexusGroup: newGroup(),
        builderGroup: newGroup(),
        attackGroup: newGroup(),
        helicopterAttackers: newGroup(),
    };

    return info;
}

function findNearest(list, x, y, flag)
{
    var minDist = Infinity, minIdx;
    for (var i = 0, len = list.length; i < len; ++i)
    {
        var d = distBetweenTwoPoints(list[i].x, list[i].y, x, y);
        if (d < minDist)
        {
            minDist = d;
            minIdx = i;
        }
    }

    if (!isDefined(minIdx))
    {
        return undefined;
    }

    return (flag === true) ? list[minIdx] : minIdx;
}

function reviseGroups()
{
    var list = enumGroup(needToPickGroup);
    for (var i = 0, len = list.length; i < len; ++i)
    {
        var droid = list[i];
        if (addDroidToSomeGroup(droid))
        {
            var coords = mapLimits(droid.x, droid.y, 15, 7, 0, 0);
            orderDroidLoc(droid, DORDER_SCOUT, coords.x, coords.y);
        }
    }
}

function addDroidToSomeGroup(droid)
{
    var base = findNearest(baseInfo, droid.x, droid.y, true);
    if (!base)
    {
        return false;
    }

    switch (droid.droidType)
    {
        case DROID_CONSTRUCT:
        {
            groupAddDroid(base.builderGroup, droid);
            break;
        }
        case DROID_WEAPON:
        {
            if (droid.name.indexOf("Nexus") > -1)
            {
                if (groupSize(base.nexusGroup) < MIN_NEXUS)
                {
                    groupAddDroid(base.nexusGroup, droid);
                }
                break;
            }

            if (isCopterPropulsion(droid.propulsion))
            {
                groupAddDroid(base.helicopterAttackers, droid);
                break;
            }

            if (groupSize(base.defendGroup) < MAX_DEFENDERS)
            {
                groupAddDroid(base.defendGroup, droid);
                break;
            }

            if (groupSize(base.attackGroup) < MIN_ATTACKERS)
            {
                groupAddDroid(base.attackGroup, droid);
                break;
            }

            if (groupSize(globalDefendGroup) < MAX_GLOBAL_DEFENDERS)
            {
                groupAddDroid(globalDefendGroup, droid);
                break;
            }
            else
            {
                groupAddDroid(base.attackGroup, droid);
            }
        }
        break;
        case DROID_SENSOR:
        {
            groupAddDroid(base.attackGroup, droid);
        }
        break;
    }

    return true;
}

function groupOfTank(droid)
{
    for (var i = 0, b = baseInfo.length; i < b; ++i)
    {
        if (droid.group === baseInfo[i].attackGroup)
        {
            return baseInfo[i].attackGroup;
        }

        if (droid.group === baseInfo[i].nexusGroup)
        {
            return baseInfo[i].nexusGroup;
        }
    }
}

function buildStructure(droid, stat)
{
    if (droid.order !== DORDER_BUILD && isStructureAvailable(stat, me))
    {
        const MAX_BLOCK_TILES = 0;
        var loc = pickStructLocation(droid, stat, droid.x, droid.y, MAX_BLOCK_TILES);

        if (loc && orderDroidBuild(droid, DORDER_BUILD, stat, loc.x, loc.y))
        {
            return true;
        }
    }

    return false;
}

function buildTower(droid)
{
    return buildStructure(droid, defenses[random(defenses.length)]);
}

function establishBase(droid)
{
    if (buildStructure(droid, factory))
    {
        baseInfo.push(constructBaseInfo(factory));
        groupAddDroid(baseInfo[baseInfo.length - 1].builderGroup, droid);
        return true;
    }

    return false;
}

function buildThingsWithDroid(droid)
{
    const MAX_FACTORY_COUNT = 30;

    switch (random(7))
    {
        case 0:
            if ((countStruct(factory) < MAX_FACTORY_COUNT) && ((5 * countStruct(factory)) < countStruct(derrick)) || (playerPower(me) > 500))
            {
                establishBase(droid);
            }
        break;
        case 1:
            if ((countStruct(derrick) - (countStruct(gen) * 4)) > 0)
            {
                buildStructure(droid, gen);
            }
        break;
        case 2:
            if ((4*countStruct(vtolfac)) < countStruct(factory) && (gameTime > 150000))
            {
                buildStructure(droid, vtolfac);
            }
        break;
        case 3:
            var result = findNearest(enumFeature(-1, oilres), droid.x, droid.y, true);
            if (result)
            {
                orderDroidBuild(droid, DORDER_BUILD, derrick, result.x, result.y);
            }
        break;
        case 4:
            if ((playerPower(me) > 60) && (countStruct(repair) < 5) && (gameTime > 200000))
            {
                buildStructure(droid, repair);
            }
        break;
        case 5:
            if (countHelicopters() > 2 * countStruct(vtolpad))
            {
                buildStructure(droid, vtolpad);
            }
        break;
        default:
            if (playerPower(me) > 150)
            {
                buildTower(droid);
            }
        break;
    }
}

function buildThings()
{
    var list = enumDroid(me, DROID_CONSTRUCT);

    for (var i = 0, len = list.length; i < len; ++i)
    {
        var droid = list[i];
        if (droid.order !== DORDER_RTR && droid.order !== DORDER_BUILD)
        {
            //Build a defense at an enemy derrick should we happen to be idle near one
            for (var j = 0; j < maxPlayers; ++j)
            {
                var dlist = enumStruct(j, derrick);
                for (var x = 0, len2 = dlist.length; x < len2; ++x)
                {
                    var enemy_derrick = dlist[x];
                    if (distBetweenTwoPoints(droid.x, droid.y, enemy_derrick.x, enemy_derrick.y) < 3)
                    {
                        buildTower(droid);
                    }
                }
            }
            buildThingsWithDroid(droid);
        }
    }
}

function scavBuildDroid(fac, name, body, prop, weapon)
{
    var success = false;

    if (weapon.length === 3)
    {
        success = buildDroid(fac, name, body, prop, "", "", weapon[0], weapon[1], weapon[2]);
    }
    else if (weapon.length === 2)
    {
        success = buildDroid(fac, name, body, prop, "", "", weapon[0], weapon[1]);
    }
    else
    {
        success = buildDroid(fac, name, body, prop, "", "", weapon[0]);
    }

    return success;
}

function produceCrane(fac)
{
    if (countDroid(DROID_CONSTRUCT, me) >= MAX_CRANES)
    {
        return false;
    }

    var num = random(2) + 1; // Choose crane 1 or 2.

    return buildDroid(fac, "Crane", CRANE_BODY + num, "BaBaProp", "", "", CRANE_WEAP + num);
}

function produceDroid(fac)
{
    const MIN_CRANES = 4;
    var craneCount = countDroid(DROID_CONSTRUCT, me);

    if ((craneCount < MIN_CRANES) || ((craneCount < MAX_CRANES) && !random(10)))
    {
        produceCrane(fac);
        return;
    }

    var weapons = [];
    if (!random(10))
    {
        if (countDroid(DROID_SENSOR, me) < MAX_SENSORS)
        {
            weapons.push("ScavSensor");
            scavBuildDroid(fac, "Sensor", "BusBody", "BaBaProp", weapons);
        }
    }
    else
    {
        var j = random(templates.length);
        var name = (templates[j][1].indexOf("NEXUS") > -1) ? "Nexus Tank" : "Scavenger unit";

        for (var x = 1; x < templates[j].length; ++x)
        {
            var weapon = templates[j][x];
            weapons.push(weapon);
        }

        scavBuildDroid(fac, name, templates[j][0], "BaBaProp", weapons);
    }
}

function produceHelicopter(fac)
{
    var j = random(vtolTemplates.length);
    var weapons = [];

    for (var x = 1; x < vtolTemplates[j].length; ++x)
    {
        var weapon = vtolTemplates[j][x];
        weapons.push(weapon);
    }

    scavBuildDroid(fac, "ScavengerHelicopter", vtolTemplates[j][0], "Helicopter", weapons);
}

function produceThings()
{
    if (atLimits())
    {
        return;
    }

    var list = enumStruct(me, factory).concat(enumStruct(me, vtolfac));
    for (var i = 0, len = list.length; i < len; ++i)
    {
        var fac = list[i];

        if (structureIdle(fac) && fac.status === BUILT)
        {
            if (fac.stattype === FACTORY)
            {
                produceDroid(fac);
            }
            else if (fac.stattype === VTOL_FACTORY)
            {
                produceHelicopter(fac);
            }
        }
    }
}

function attackWithDroid(droid, target, force)
{
    if (isCopterPropulsion(droid.propulsion) || droid.order === DORDER_RTR)
    {
        return;
    }

    if (droid.droidType === DROID_WEAPON)
    {
        if ((droid.order !== DORDER_ATTACK) || force)
        {
            orderDroidObj(droid, DORDER_ATTACK, target);
        }
    }
    else if (droid.droidType === DROID_SENSOR)
    {
        if ((droid.order !== DORDER_OBSERVE) || force)
        {
            orderDroidObj(droid, DORDER_OBSERVE, target);
        }
    }
}

function helicopterArmed(obj)
{
    for (var i = 0, len = obj.weapons.length; i < len; ++i)
    {
        var weapon = obj.weapons[i];
        if (weapon.armed > 0)
        {
            return true;
        }
    }

    return false;
}

function helicopterReady(droid)
{
    if (droid.order === DORDER_REARM)
    {
        return false;
    }
    if (helicopterArmed(droid) && droid.health > 50)
    {
        return true;
    }
    if (droid.order !== DORDER_REARM)
    {
        orderDroid(droid, DORDER_REARM);
    }

    return false;
}

//Helicopters can only attack things that the scavengers have seen
function helicopterAttack()
{
    for (var i = 0, len = baseInfo.length; i < len; ++i)
    {
        var base = baseInfo[i];
        var copters = enumGroup(base.helicopterAttackers);
        var target = rangeStep(base, false);

        for (var j = 0, len2 = copters.length; j < len2; ++j)
        {
            var coords = [];
            var droid = copters[j];

            if (!helicopterReady(droid))
            {
                continue;
            }

            if (target)
            {
                coords = mapLimits(target.x, target.y, 5, 2, 0, 0);
            }
            else
            {
                var xOff = random(2);
                var yOff = random(2);
                xOff = (!xOff) ? -random(10) : random(10);
                yOff = (!yOff) ? -random(10) : random(10);
                coords = mapLimits(droid.x, droid.y, 5, 2, xOff, yOff);
            }

            orderDroidLoc(droid, DORDER_SCOUT, coords.x, coords.y);
        }
    }
}

//Ignores lifts
function countHelicopters()
{
    var count = 0;

    enumDroid(me).forEach(function(droid) {
        if (isCopterPropulsion(droid.propulsion))
        {
            ++count;
        }
    });

    return count;
}

function groundAttackStuff()
{
    for (var i = 0, len = baseInfo.length; i < len; ++i)
    {
        var base = baseInfo[i];
        var target = rangeStep(base, false);
        if (target)
        {
            var attackDroids = enumGroup(base.attackGroup);
            var nexusDroids = enumGroup(base.nexusGroup);
            if (groupSize(base.attackGroup) > MIN_ATTACKERS)
            {
                for (var i = 0, len2 = attackDroids.length; i < len2; ++i)
                {
                    attackWithDroid(attackDroids[i], target, false);
                }
            }

            if (groupSize(base.nexusGroup) > MIN_NEXUS)
            {
                for (var i = 0, len2 = nexusDroids.length; i < len2; ++i)
                {
                    attackWithDroid(nexusDroids[i], target, false);
                }
            }
        }
    }
}

function eventAttacked(victim, attacker)
{
    // don't quarrel because of friendly splash damage
    if (attacker === null || victim.player !== me || attacker.player === me)
    {
        return;
    }

    var droids = enumGroup(globalDefendGroup);
    for (var i = 0, len = droids.length; i < len; ++i)
    {
        var droid = droids[i];
        if (droid.order !== DORDER_ATTACK)
        {
            attackWithDroid(droid, attacker, true);
        }
    }

    if (victim.type === STRUCTURE)
    {
        var base = findNearest(baseInfo, victim.x, victim.y, true);
        if (!base)
        {
            return;
        }

        var list = enumGroup(base.defendGroup);

        //Let this base build more defense units then
        if (list.length < Math.floor(MAX_DEFENDERS / 2))
        {
            list = enumGroup(base.attackDroids);
        }

        for (var i = 0, len = list.length; i < len; ++i)
        {
            attackWithDroid(list[i], attacker, true);
        }
    }
    else if (victim.type === DROID)
    {
        if (isCopterPropulsion(victim.propulsion))
        {
            return;
        }

        retreat(victim);
    }
}


function eventDroidBuilt(droid, fac)
{
    groupAddDroid(needToPickGroup, droid);
    reviseGroups();
}

function eventStructureBuilt(structure, droid)
{
    if (structure.stattype === FACTORY)
    {
        if (!produceCrane(structure))
        {
            produceDroid(structure);
        }
    }
    else if (structure.stattype === VTOL_FACTORY)
    {
        produceHelicopter(structure);
    }
}

// respond correctly on unit transfers
function eventObjectTransfer(object, from)
{
    if (object.player !== me)
    {
        return; //not mine
    }

    if (object.type === DROID)
    {
        eventDroidBuilt(object, null);
    }
    else
    {
        eventStructureBuilt(object, null);
    }
}

function retreat(obj)
{
    const REPAIR_PERCENT = 85;

    if (obj.type === DROID && obj.order !== DORDER_RTR)
    {
        if (!isCopterPropulsion(obj.propulsion) && obj.health < REPAIR_PERCENT)
        {
            orderDroid(obj, DORDER_RTR);
        }
        return;
    }
}

//Check to see if a base factory still exists, and, if not, then free its groups
//and put them into another base.
function cleanupBaseInfo()
{
    var units = [];

    for (var i = 0, len = baseInfo.length; i < len; ++i)
    {
        var base = baseInfo[i];
        var factory = getObject(STRUCTURE, me, base.id);

        if (factory === null)
        {
            var atk = enumGroup(base.attackGroup);
            var nex = enumGroup(base.nexusGroup);
            var def = enumGroup(base.defendGroup);
            var con = enumGroup(base.builderGroup);
            var cop = enumGroup(base.helicopterAttackers);
            units.concat(atk).concat(nex).concat(def).concat(con).concat(cop);
            baseInfo.splice(i, 1);
            break;
        }
    }

    for (var i = 0, len = units.length; i < len; ++i)
    {
        var droid = units[i];
        eventDroidBuilt(droid, null);
    }
}

function eventStartLevel()
{
    var factories = enumStruct(me, factory);
    for (var i = 0, len = factories.length; i < len; ++i)
    {
        var fac = factories[i];
        baseInfo.push(constructBaseInfo(fac));
    }

    var droids = enumDroid(me);
    for (var i = 0, len = droids.length; i < len; ++i)
    {
        addDroidToSomeGroup(droids[i]);
    }

    globalDefendGroup = newGroup();
    needToPickGroup = newGroup();

    produceThings();
    setTimer("produceThings", 300);
    setTimer("buildThings", 900);
    setTimer("groundAttackStuff", 1200);
    setTimer("helicopterAttack", 2900);
    //setTimer("cleanupBaseInfo", 4000);
}
