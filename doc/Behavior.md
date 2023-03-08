# Micro AI behavior
This file describes what are primary and secondary states before/after a test.
It doesn't describe what one *wants* to happen, only facts.

If what you are currently experiencing in the game does not correspond to
description in this file, then you should either update this file,
or to file a bug report.

Please do explicitly specify which version has been tested on: unintended changes
in behaviors between releases makes it impossible to leave the versioning implicit.

Do explicitly specify what is secondary state before/after your test, don't rely on
your assumptions: de-select then re-select the unit once again, so that UI has no chance
of being stuck/stale.

Don't forget to use cheat-code "showorders" to see DORDER_/DACTION_.

A Regular Fighter unit's secondary state can be described as four letters:
- preferred fire distance (optimum, short, long) O S L
- retreat level (doordie, heavy, medium) D H M
- aggresiveness level (atwill, return, hold) A R H
- mobility level (pursue, guard, hold) P G H

Commander's / VTOL secondary state can be as three letters:
- retreat level (doordie, heavy, medium) D H M
- aggresiveness level (atwill, return, hold) A R H
- mobility level (pursue, guard, hold) P G H

Builder Truck's / MRT (mobile repair turret) secondary state has only 2 parameters:
- retreat level (doordie, heavy, medium) D H M
- mobility level (pursue, guard, hold) P G H


## Behavior Cases


When moving any ground unit, version 4.2.7:
* Unit's primary is DORDER_NONE, secondary is LMHH or LMHP
   - moves to the new position, then becomes DORDER_NONE
   - secondary preserved

* Unit's primary is DORDER_GUARD, secondary is LMHG
   - moves to the new position, then becomes DORDER_GUARD
   - secondary preserved


When moving VTOL, version 4.2.7:
* VTOL has DORDER_GUARD, MHG or MHH or MHP:
   - moves to the new position, then DORDER_NONE.
   - secondary preserved


When artillery is attached to Sensor Tower, version 4.2.7:
* artillery has FIRESUPPORT, LMRH, gets damaged > threshold:
   - receives DORDER_RTR, moves to repair station
   - when gets repaired, moves to rally point, DORDER_NONE
   - secondary preserved


When a droid next to a repair station, version 4.2.7:
* Unit DORDER_NONE, LMHH or LHHH, gets damaged > threshold:
   - receives DORDER_RTR, moves to repair station
   - when gets repaired, moves to rally point, DORDER_NONE
   - secondary preserved

* Unit DORDER_NONE, LDHH or LDHG or LDHP, gets damaged > threshold:
   - keeps DORDER_NONE, doesnt move
   - when gets repaired, doesnt moves to rally point, DORDER_NONE
   - secondary preserved


When a VTOL is damaged, version 4.2.7:
- doesnt get auto-repaired.
- when "Return to repairs" is pressed in droid menu,
  and there are rearm pads,
  VTOL goes to a rearm pad and gets repaired.
- when "Return to repairs" is pressed in droid menu,
  and there are NO rearm pads,
  receives RTB, lands next to QG,
  becomes DORDER_NONE, DACTION_NONE


When VTOL in FIRESUPPORT mode, attached to VTOL Strike Tower, version 4.2.7:
* VTOL DORDER_FIRESUPPORT LAG, gets damaged > threshold:
   - does not get detached. 
   - when there are NO rearm pads, receives RTB order, even if repair stations
   are available. 
   when reaches QG, lands on the ground with DORDER_NONE, DACTION_NONE
   - when there are rearm pads, but VTOL is full ammo,
   goes next to a rearm pad, but lands next to it (no rearming/no reparing. Likely a bug).
   
   - when there are rearm pads, and VTOL has depleted ammo,
   goes to a rearm pad, and lands on it for rearm (no repairs! Likely a bug. Rearm ok). 
   Stays DORDER_FIRESUPPORT, DACTION_FIRESUPPORT


When VTOL in FIRESUPPORT mode, attached to commander, version 4.2.7:
* VTOL DORDER_FIRESUPPORT LAG, gets damaged > threshold:
   - does not get detached. 
   - same as when VTOL is attached to VTOL Strike Tower

* Commander attacks a target
   - VTOL acquires the same target, moves to it, attacks.
   - when depleted, 
   and there are rearm pads, goes to rearm pad for rearming.
   when rearmed, stays in DORDER_FIRESUPPORT, DACTION_FIRESUPPORT. 
   doesnt move out of rearm pad.
   - when depleted, 
   and there is NO rearm pads, receives RTB order.
   when reaches QG, lands on the ground, becomes DORDER_NONE, DACTION_NONE.
   

When artillery in FIRESUPPORT mode, attached to commander, version 4.2.7:
* Unit DORDER_FIRESUPPORT LMHH, gets damaged > threshold:
   - gets detached from commander, receives RTR, moves to repair station
   - when gets repaired, moves to rally point, DORDER_NONE
   - secondary preserved

* Unit DORDER_FIRESUPPORT LMHG, gets damaged > threshold:
   - gets detached from commander, receives RTR, moves to repair station
   - when gets repaired, moves to rally point, then DORDER_GUARD
   - secondary preserved

* Unit DORDER_FIRESUPPORT LMHP, gets damaged > threshold:
   - receives RTR, moves to repair station
   - when gets repaired, moves to rally point, then DORDER_NONE
   - secondary preserved


When simply attaching unit to a commander, and then checking its secondary state, version 4.2.7:
- preferred distance must be same as before
- retreat, aggressiveness, mobility must be equal to commander


When a unit is attached to commander, and secondary state of
the commander changes, then secondary state of the attached unit changes too,
and stays synchronized. This doesn't include artillery attached to a commander, neither VTOLs
which continues to keep its own secondary state. version 4.2.7


When "Return to repairs" is given to a commander (droid menu), version 4.2.7:
- the whole team receives RTR, and moves to repairs, even if some units have full HP.
- when reached repair station, order is changed to MOVE to rally point (nothing to repair)
- secondary preserved


When a fighter unit is attached to commander, version 4.2.7:
* Commander GUARD, MRG or MRH, unit gets damaged beyond threshold:
   - unit receives RTR, moves to a repair station
   - when gets repaired, moves back to commander.
   however, doesnt stop when reachees commander (DACTION_MOVE). Likely a bug.
   Secondary order is synchronized to commander, primary is DORDER_GUARD

* attached unit gets damaged beyond threshold:
  - unit goes for repairs, but player orders the squad to move elsewhere
  - unit cancels its movement toward repairs, and moves with the whole squad. 
    (This is likely a bug, we want damaged units to continue moving toward repairs, 
    despite player ordering to move the squad)

* attached unit gets attacked and aggressiveness level of the squad is not HOLD
  - the squad reacts to being attacked

When builder truck is idling next a structure which is being built, or which is damaged, version 4.2.7:
* Truck has secondary MG, and the structure is within REPAIR_RADIUS
   - truck keeps DORDER_GUARD, but turns its head to the structure, and helps construction, DACTION_BUILD
   - secondary preserved

* Truck has secondary MH or MP, and the structure is within REPAIR_RADIUS
   - truck keeps DORDER_NONE, but turns its head to the structure, and helps construction, DACTION_BUILD
   - secondary preserved

* Truck has secondary MP or MG, and the structure is within REPAIR_MAXDIST
   - truck keeps DORDER_NONE (when secondary was MP) or DORDER_GUARD (when secondary was MG)
   - moves to the damaged/unfinshed structure, becomes DACTION_BUILD


When builder truck is building a structure, version 4.2.7:
* Truck has secondary MG or MH, DORDER_BUILD, damaged > threshold:
   - goes to repair station
   - when gets repaired, moves to rally point, DORDER_GUARD
   - secondary preserved

* Player has not enough oil to fully fund the building
   - truck still has DORDER_BUILD DACTION_BUILD
   - blue triangle appears visually indicating "in-process"
   - the building's readiness indicator doesn't advance: yellow line indicates building progress, green line indicates current HP / Max HP.
   - if player orders the truck to move elsewhere, while the building still has not been funded, the building's blueprint will disappear.
   - when sufficient funds are collected, both lines start growing


When builder truck is ordered to help an unfinished structure, version 4.2.7:
* Truck has secondary MG or MH, DORDER_HELPBUILD, damaged > threshold:
   - goes to repair station
   - when gets repaired, moves to rally point, DORDER_GUARD
   - secondary preserved


When unit receives order to attack, version 4.2.7:
* Unit has LMAH, DORDER_NONE
   - user gives target which is outside of the range
   - unit doesnt move. Gets DORDER_NONE.
   - secondary is preserved

* Unit has LMHG, DORDER_GUARD
   - target moves outside of the range
   - unit follows the target, keeps attacking. Gets DORDER_ATTACK, DACTION_ATTACK
   - secondary is preserved


When MRT is next to a damaged unit, version 4.2.7:
* MRT has DH or DP, a damaged unit is outside of REPAIR_RANGE
   - MRT doesnt move.

* MRT has DG, a damaged unit is outside of REPAIR_RANGE, but inside REPAIR_MAXDIST
   - MRT goes to the damaged unit with DACTION_MOVETODROIDREPAIR
   - when unit reached, action becomes DACTION_DROIDRREPAIR
   - when unit is healed, MRT has DORDER_NONE, DACTION_NONE
   - secondary is preserved for both


When artillery is attached to Wide Sensor Tower (WST), version 4.2.7:
* Enemy is within range of WST and is visible, artillery has primary FIRESUPPORT
  - artillery keeps primary, but secondary becomes DACTION_ATTACK
  - enemy unit is kept visible as long as it is within the range of WST
  - artillery keeps shooting at enemy as long as it is within the range of WST, 
    even if enemy moves to an invisible location (behind a mountain).
    Enemy keeps being visible.
  - When unit moves out of WST range, it becomes invisible, and artillery looses its target.
    Artillery keeps FIRESUPPORT, secondary is FIRESUPPORT


When a droid is being attacked, version 4.2.7:
* Droid has primary DORDER_NONE, secondary LDAP, DACTION_NONE:
  - when enemy approaches within fire range, droid becomes DORDER_ATTACK, DACTION_ATTACK.
    When enemy moves out of fire range, droid follows him, and keeps DORDER_ATTACK.

*  Droid has primary DORDER_NONE, secondary LDRP, DACTION_NONE:
   - droid doesn't react to enemy traversing fire range.
   - when enemy attacks, droid starts attacking, and follows the enemy, even when enemy
     moves out of range.

* Droid has primary primary GUARD, secondary LDRG, DACTION_NONE
  - droid doesn't react enemy traversing fire range.
  - when enemy attacks, droid follows it, but only no further than DEFEND_BASEDIST.
    When droid moves at >= DEFEND_BASEDIST, it moves back to where it has been first attacked.
    Droid doesnt shoot while moving back to that location (likely a bug).
    Consequently, if ANY enemy only approaches within fire range, droid will
    start shooting/following it again (again, within DEFEND_BASEDIST) (likely a bug,
    we want to start shooting only when we are being attacked)

* Droid has primary primary GUARD/NONE, secondary LDHG or LDHP or LDHH, DACTION_NONE
  - doesnt react to being shot at, doesnt move, primary is preserved, secondary preserved
  
