function isPowerResearch(research) {
    const POWERS = [
        "R-Struc-Power-Upgrade01",
        "R-Struc-Power-Upgrade01b",
        "R-Struc-Power-Upgrade01c",
        "R-Struc-Power-Upgrade02",
        "R-Struc-Power-Upgrade03",
        "R-Struc-Power-Upgrade03a",
    ];

    for (let i = 0, len = POWERS.length; i < len; ++i) {
        if (research === POWERS[i]) {
            return true;
        }
    }

    return false;
}

function evalResearch(lab, list) {
    const sufficientPower = getRealPower() > 15000;

    for (let i = 0; i < list.length; ++i) {
        if (sufficientPower && isPowerResearch(list[i])) {
            // Skip power research if we have enough power
            continue;
        }

        if (pursueResearch(lab, list[i])) {
            return true; // Research found and pursued
        }
    }

    return false; // No research pursued
}


function research() {
    // Don't research if:
    // We don't have builders
    // We finished all the researches
    if (!countDroid(DROID_CONSTRUCT) || researchDone) {
        return;
    }

    let labList = enumStruct(me, RES_LAB_STAT).filter((lab) => (
        lab.status === BUILT && structureIdle(lab)
    ));

    if (labList.length === 0) {
        return;
    }

    // Always allow at least 1 research lab to start researching even when power is low,
    // and require MIN_RESEARCH_POWER only when running multiple labs simultaneously
    const totalLabs = enumStruct(me, RES_LAB_STAT).filter(lab => lab.status === BUILT).length;
    const busyLabs = totalLabs - labList.length;
    if (busyLabs >= 1 && getRealPower() < MIN_RESEARCH_POWER) {
        return;
    }

    const researchPriorities = [
        isSeaMap ? ["R-Vehicle-Prop-Hover"] : null,
        START_COMPONENTS,

        // T1
        FUNDAMENTALS_ROCKET_WEAPON_T1,
        FUNDAMENTALS_MACHINEGUN_WEAPON_T1,
        FUNDAMENTALS_BODY_T1,
        FUNDAMENTALS_PROPULSION_T1,
        FUNDAMENTALS_UPGRADES_T1,
        FUNDAMENTALS_DEFENCE_T1,
		
		enemiesHaveVTOLs ? AA_RESEARCH : null,

        // T2
        FUNDAMENTALS_ROCKET_WEAPON_T2,
        FUNDAMENTALS_MACHINEGUN_WEAPON_T2,
        FUNDAMENTALS_BODY_T2,
        FUNDAMENTALS_PROPULSION_T2,
        ["R-Vehicle-Prop-VTOL"],
        FUNDAMENTALS_UPGRADES_T2,
        FUNDAMENTALS_VTOL_BOMBS_T2,
        FUNDAMENTALS_DEFENCE_T2,     

        // T3
        FUNDAMENTALS_ROCKET_WEAPON_T3,
        FUNDAMENTALS_MACHINEGUN_WEAPON_T3,
        FUNDAMENTALS_BODY_T3,
        FUNDAMENTALS_VTOL_BOMBS_T3,
        FUNDAMENTALS_PROPULSION_T3,
        FUNDAMENTALS_UPGRADES_T3,
        FUNDAMENTALS_DEFENCE_T3,

        // T4
        FUNDAMENTALS_ROCKET_WEAPON_T4,
        FUNDAMENTALS_MACHINEGUN_WEAPON_T4,
        FUNDAMENTALS_BODY_T4,
        FUNDAMENTALS_DEFENCE_T4,
        FUNDAMENTALS_PROPULSION_T4,
        FUNDAMENTALS_UPGRADES_T4
    ].filter(Boolean); // Removes any null values

    // List of all currently available research items
    const allResearchList = enumResearch();
    if (allResearchList.length === 0) {
        // IMPORTANT: In a "No Research" start, there are only a few root tech nodes at the beginning.
        // If busyLabs > 0, other labs are currently researching prerequisites that will unlock the rest of the tree!
        // Only mark researchDone if ALL labs are idle (busyLabs === 0) AND end-game Dragon body ("Body14SUP") is already unlocked.
        if (busyLabs === 0 && totalLabs > 0 && componentAvailable("Body14SUP")) {
            researchDone = true;
        }
        return;
    }

    for (let lab of labList) {
        let found = false;

        // Try each research priority
        for (let researchCategory of researchPriorities) {
            if (found) break;
            found = evalResearch(lab, researchCategory);
        }

        // Last step: if all priority branches are currently locked by another lab's in-progress prerequisite,
        // pick an available non-cannon/non-mortar/non-howitzer research item first
        if (!found) {
            const currentAvailable = enumResearch();
            const filteredAvailable = currentAvailable.filter(r =>
                !r.name.includes("Cannon") &&
                !r.name.includes("Mortar") &&
                !r.name.includes("Howitzer") &&
                !r.name.includes("Flame")
            );
            const pool = filteredAvailable.length > 0 ? filteredAvailable : currentAvailable;
            if (pool.length > 0) {
                const idx = Math.floor(Math.random() * pool.length);
                pursueResearch(lab, pool[idx].name);
            }
        }
    }
}
