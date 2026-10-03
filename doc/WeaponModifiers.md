# Weapon Modifiers

## Introduction

Warzone 2100 supports a variety of weapon modifiers to fine tune damage dealt to a specific propulsion type.
All propulsion damage modifiers for the weapon modifier classes are initialized to 100% by default if not defined.
To use one, modify the `weapons.json` values for the weapon's `weaponEffect` you wish to change.

## Nomenclature

Sometimes you may see `SLOW` as a prefix in a modifier. This usually means a weapon that does great impact, damage, or has slow moving projectiles. Examples
would be `Lancer` being a `SLOW ROCKET` while the `Mini Rocket Pod` is a standard `ROCKET`.

## Defaults

These are the modifiers that have been around for a long time. Please note that `ALL ROUNDER` is an alias for the original `ANTI AIRCRAFT`
modifier.

```txt
"ANTI PERSONNEL",
"ANTI TANK",
"BUNKER BUSTER",
"ARTILLERY ROUND",
"FLAMER",
"ANTI AIRCRAFT" / "ALL ROUNDER"
```

## New Additions

These ones will cover the entire scope of all existing weapons the game has to offer at the current time of this writing.

```txt
"BOMB",
"FIRE BOMB",
"EMP BOMB",
"MACHINEGUN",
"ASSAULT MACHINEGUN",
"CANNON",
"HYPER CANNON",
"SLOW CANNON",
"ASSAULT CANNON",
"PLASMA CANNON",
"ROCKET",
"SLOW ROCKET",
"GAUSS",
"SLOW GAUSS",
"MISSILE",
"SLOW MISSILE",
"LASER",
"SLOW LASER",
"PARTICLE LASER",
"MORTAR",
"FIRE MORTAR",
"HOWITZER",
"FIRE HOWITZER",
"ROCKET ARTILLERY",
"SLOW ROCKET ARTILLERY",
"MISSILE ARTILLERY",
"SLOW MISSILE ARTILLERY",
"PLASMA",
"PLASMA FLAMER",
"PLASMA ARTILLERY",
"SLOW PLASMA ARTILLERY",
"ELECTRONIC",
"NEXUS LINK",
"EMP",
"EMP ARTILLERY",
"SLOW EMP ARTILLERY",
"LASSAT",
"ANTI AIR GENERIC"
```

## Making Use of the New Modifier

Changing the `weaponEffect` of a weapon is the first part, now you must create or edit sections for `weaponModifiers.json`.
Modifier classes can be copied and the name replaced with one of the above modifier names to add new ones. Each modifier holds key-value pairs
for Propulsion Types to determine percentage of damage output. Below is a copy of the Multiplayer Flamer modifier as an example:

```txt
"FLAMER": {
    "Half-Tracked": 100,
    "Hover": 130,
    "Legged": 130,
    "Legged-Super": 130,
    "Lift": 25,
    "Propellor": 90,
    "Tracked": 90,
    "Wheeled": 110
},
```

## Example

Say you wanted to change `Ripple Rockets` in Multiplayer/Skirmish to use a different modifier over `Mini-Rocket Array`. To do this find the `Ripple Rocket` in `weapons.json` within
the `data/[base|mp]/stats` (base = Campaign, mp = Skirmish/Multiplayer) folder and change the `weaponEffect` value from `"ARTILLERY ROUND"` to `"SLOW ROCKET ARTILLERY"`, then do similar for the `Mini-Rocket Array` but with instead `"ROCKET ARTILLERY"`.
