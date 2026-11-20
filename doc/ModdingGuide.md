# Making Mods for Warzone 2100

**Start here before you change any C++.**

If you want new units, a new faction, new mechanics, a new economy, or a whole new campaign, you almost certainly do **not** need to fork the engine. Warzone 2100 is built to be extended with **data files** (stats, models, textures, sounds) plus **JavaScript** that runs against the game's scripting API, *WZAPI*. Packaged as a `.wz` file, your mod:

- works with official releases, so players don't need a custom build,
- keeps working when the engine updates, because you aren't carrying a patch set,
- is **saved and loaded with the game automatically** (script globals go into savegames),
- in multiplayer, is **sent to joining players by the host**, the same way maps are.

Sections:

- [Proof: what has already been built this way](#proof-what-has-already-been-built-this-way)
- [What a mod is made of](#what-a-mod-is-made-of)
- [Kinds of mods and where they go](#kinds-of-mods-and-where-they-go)
- [Where your scripts plug in](#where-your-scripts-plug-in)
- [The rules of the road: determinism and saving](#the-rules-of-the-road-determinism-and-saving)
- [Where to look](#where-to-look)
- ["I want to…" cookbook](#i-want-to-cookbook)
- [Your first mod: salvage drums](#your-first-mod-salvage-drums)
- [When there's no direct API: patterns that work](#when-theres-no-direct-api-patterns-that-work)
- [When an engine change *is* the right answer](#when-an-engine-change-is-the-right-answer)
- [For AI coding assistants](#for-ai-coding-assistants)

---

## Proof: what has already been built this way

Everything below ships with the game and is written as data plus JavaScript:

| What | Where | Scale |
|---|---|---|
| **The entire original campaign**: every mission's objectives, enemy bases, attack waves, reinforcements, transporters, cutscene triggers, victory/defeat | `data/base/script/campaign/` and the `libcampaign` helper library | ~19,000 lines of JS |
| **Reclamation**, a full add-on campaign with its own story, factions, stats, models, audio and texture pages | `data/mods/campaign/reclamation` (bundled submodule) | ~11,000 lines of JS, zero engine changes |
| **Fractured Kingdom**, a full add-on campaign set in its own universe, with its own sequences, guide topics and achievements | `data/mods/campaign/fractured-kingdom` (bundled submodule) | ~19,500 lines of JS, zero engine changes |
| **Every skirmish AI**: Nexus, BoneCrusher, Cobra, NullBot variants, SemperFi | `data/mp/multiplay/skirmish/` | ~27,000 lines of JS |
| **Skirmish/multiplayer rules**: starting bases, tech levels, power, limits, win/loss, oil-drum spawning, weather | `data/mp/multiplay/script/rules/`, `functions/` | |
| **Two scavenger factions**, including *Ultimate Scavengers* with helicopters and cranes | `data/mp/multiplay/script/scavengers/` | |
| **The tutorial** | `data/base/script/tutorial.js` | |
| **Campaign balance mods** ("Classic", "Pumpkin") | `data/mods/campaign/wz2100_camclassic`, `wz2100_campumpkin` | stats only |

**Community mods turn the game into entirely different games**, with no engine changes:

| Mod | What it changes |
|---|---|
| [**Tower Defense**](https://github.com/aco4/WZmodTowerDefense) | Protect your command center from scavenger waves spawning at the map edges. Power is earned only by kills. Works on any map, with any number of players. ~1,800 lines of JS. |
| [**Wave Defense**](https://github.com/vaut/WZmodWave) | You start on a thin strip of map. With each wave, the playable area grows (`setScrollLimits`), enemies arrive by dropship with designs that change over time, and structure limits rise. Works in multiplayer, and only the host needs the mod. Built on the `multiplay/script/mods/` hook. |
| [**NRS: Next Research System**](https://github.com/Iluvalar/NRS) | A complete remake of the tech tree and economy: price classes, regional weapon families, new power-generation buildings (housing, solar, nuclear, parks), an interest-rate economy, extra bodies and weapons, and its own AI. |

**From mod to built-in feature:** *Ultimate Scavengers* started as a third-party mod that replaced the scavenger script and added new units. Because it was a clean, self-contained mod, it was upstreamed as a built-in game option. Today it's a scavenger setting in the lobby (`ULTIMATE_SCAVENGERS`), and that setting is the only engine change. A well-made mod is the best way to get an idea into the base game.

If the original campaign, two add-on campaigns, and every AI can be done this way, your idea very likely can too.

---

## What a mod is made of

A mod is a zip file renamed to `.wz`. Inside the zip, files use the same paths the game's own files have *within* `data/base/` (campaign) or `data/mp/` (skirmish/multiplayer), starting at the zip's root. The `data/base/` or `data/mp/` prefix is **not** included:

| The game's file (in the source repository) | The same file in your mod |
|---|---|
| `data/mp/stats/weapons.json` | `stats/weapons.json` |
| `data/mp/multiplay/script/mods/init.js` | `multiplay/script/mods/init.js` |
| `data/base/script/campaign/cam1a.js` | `script/campaign/cam1a.js` |

A file at the same path **replaces** the game's file, and new files simply add content. (Elsewhere in this guide, paths that start with `data/` point to examples in the source repository.)

| Layer | What it controls | Formats & docs |
|---|---|---|
| **Stats** | What exists and how strong it is: bodies, propulsions, weapons, sensors, structures, features, research, templates | JSON in `stats/` (see `data/mp/stats/*.json` and `data/base/stats/*.json` for every field in use) |
| **Models & animation** | How things look and move: connectors for turrets, texture animation, model animation on move/fire/die/activate | PIE: [PIE.md](PIE.md), [Animation.md](Animation.md), Blender add-on in `tools/blender/pie-addon` |
| **Textures, images, audio** | Texture pages, UI images, sounds (`audio/audio.json`), music | [Music.md](Music.md), [EffectLights.md](EffectLights.md) |
| **Maps** | Terrain, start positions, pre-placed objects, labels/areas for scripts | Map editor (FlaME), `tools/maptools-cli` |
| **Scripts** | What happens: rules, events, AI, missions, economy, custom mechanics | [Scripting.md](Scripting.md) and the `js-*.md` reference |
| **Campaign packaging** | Campaign selector entry, tweak options, banner | [CampaignMods.md](CampaignMods.md) |
| **Text & guide** | Briefings, messages, in-game guide topics, translations | [BriefAndProximityFormat.md](BriefAndProximityFormat.md), [GuideTopics.md](GuideTopics.md), [WZJsonLocalizedString.md](WZJsonLocalizedString.md) |

---

## Kinds of mods and where they go

Put mods under `<config dir>/mods/…` ([ConfigDir.md](ConfigDir.md)). Release builds use a **versioned** subfolder for every kind except campaign mods (for example `mods/<version>/autoload`). The game creates these folders for you.

| Kind | Folder | How it's enabled |
|---|---|---|
| Alternate campaign / campaign balance mod | `mods/campaign/` | Shows up in the campaign selector. Needs `mod-info.json` ([CampaignMods.md](CampaignMods.md)) |
| Skirmish / multiplayer mod | `mods/<version>/multiplay/` | `--mod_mp=name.wz`. Joining players download it from the host |
| Global mod (campaign + skirmish) | `mods/<version>/global/` | `--mod=name.wz` |
| Always-on mod (handy while developing) | `mods/<version>/autoload/` | Loaded automatically |
| Music | `mods/<version>/music/` | Loaded automatically |
| Challenge | `challenges/` | Challenges menu. A challenge can bring its own `rules`/`extra` scripts ([Scripting.md](Scripting.md#challenges)) |

---

## Where your scripts plug in

| Context | Entry point | Notes |
|---|---|---|
| **Campaign** | The campaign JSON points to levels, and each level names its script | Use `libcampaign` ([js-campaign.md](js-campaign.md)) for factories, groups, reinforcements, victory conditions, area triggers |
| **Skirmish / multiplayer rules** | `multiplay/script/mods/init.js` | A placeholder file for mods, which `rules.js` includes **last**. Override this one file instead of replacing `rules.js`, and `include()` your own files from it |
| **Challenge** | `"scripts": { "rules": …, "extra": … }` in the challenge JSON | `rules` replaces the default rules entirely. `extra` runs alongside them |
| **AI player** | An AI `.json` naming a `.js` file in `multiplay/skirmish/` | See any bundled AI |
| **Scavengers** | `multiplay/script/scavengers/init.js` | Runs as the scavenger player |

Rules scripts receive events for all players (`receiveAllEvents(true)`) and can change game state for anyone. That's what makes new mechanics possible without engine changes.

**Events the rules already handle.** `rules.js` already defines events such as `eventDestroyed`, so **don't redefine them**. Register an event namespace with `namespace("mymod_")` at the top level of your script, then define `mymod_eventDestroyed(...)`. The engine calls every namespaced variant as well as the plain event. The game uses this itself: `libcampaign` (`cam_`), the skirmish end conditions (`conditions_`), BoneCrusher (`bc_`).

---

## The rules of the road: determinism and saving

Multiplayer runs in lockstep: **every client runs the rules script and must reach the same result.** Script state is also saved and restored. Follow these rules and your mod will be multiplayer-safe and save-safe:

1. **Randomness:** use `syncRandom(n)`, never `Math.random()`, in anything that affects the game.
2. **Integers:** prefer integer math for anything that affects the game (for example `(w * h) >> 12`, not `w * h / 4096`).
3. **State lives in global variables.** Globals (`var` and properties on the global object) are saved and restored automatically. Top-level `const`/`let` contents, closure variables and function properties are **not**. [Scripting.md](Scripting.md#state-that-changes-during-the-game) has the details, and the `wz/saved-script-state` ESLint rule catches mistakes.
4. **Never store game objects globally.** Store `id`s and re-fetch with `getObject(type, player, id)`, or re-enumerate.
5. **Orders and production are queued**, not applied immediately, so don't expect to read back a change in the same event.
6. **Presentation-only code** (sounds, camera) may differ per client. Game-state code must not.
7. **Player input that should change game state** goes through `syncRequest(...)` → `eventSyncRequest(...)`, so every client applies it.

---

## Where to look

**The authoritative list of what scripts can do** comes from the doc comments in `src/wzapi.cpp` and `src/quickjs_backend.cpp`, which are rendered into:

- [js-events.md](js-events.md): everything you can react to (built, destroyed, attacked, seen, picked up, researched, rank gained, area entered, sync request, chat, beacons, transporters, design screen, …)
- [js-functions.md](js-functions.md): everything you can do (spawn, order, build, damage/heal, fire weapons at a location, change power, research, limits, alliances, camera, weather, sky, sound, textures, reticule buttons, …)
- [js-objects.md](js-objects.md): what you can read about droids, structures, features, research
- [js-globals.md](js-globals.md): `Stats` (read every rule value) and **`Upgrades`** (write per-player rule values at runtime, the basis of research effects and dynamic balance)
- [js-campaign.md](js-campaign.md): `libcampaign`

**Review working code:**

- Economy and spawning: `data/mp/multiplay/script/rules/oildrum.js` (spawns features from synchronized rules events, reacts to `eventPickup`)
- Base setup, limits, tech levels: `data/mp/multiplay/script/rules/setup/`
- A whole faction's behavior: `data/mp/multiplay/script/scavengers/ultimateScavengers.js`
- A complete alternative game mode built on the `multiplay/script/mods/` hook: [Wave Defense](https://github.com/vaut/WZmodWave), [Tower Defense](https://github.com/aco4/WZmodTowerDefense)
- Missions, waves, reinforcements, cutscenes: any `data/base/script/campaign/cam*.js`, then Reclamation and Fractured Kingdom
- Weather and atmosphere: `data/mp/multiplay/script/functions/weather.js`

**Inspect a running game:** the `jsdebug` cheat opens the script debugger, which shows script globals and stat values. See [Debugging.md](Debugging.md).

---

## "I want to…" cookbook

| I want to… | Use |
|---|---|
| Add units, weapons, structures, research | Stats JSON + PIE models. Make them available with research results or `enableStructure` / `enableComponent` / `makeComponentAvailable` / `enableTemplate` |
| Make units stop to fire | `"fireOnMove": 0` on the weapon in `weapons.json` |
| Give a vehicle a second gun (such as a hull MG) | A body with `"weaponSlots": 2` and two weapons in the template |
| Add damage types (such as "mechanical" vs "energy") | Weapons have a `weaponClass` (`KINETIC` or `HEAT`), and bodies have `armourKinetic` and `armourHeat`. Change them per player at runtime with `Upgrades[player].Body[id].Armour` / `.Thermal` |
| Restrict tech per faction or player | Per-player `makeComponentAvailable`, `enableComponent`, `enableResearch` |
| Change balance mid-game, per player | Write to `Upgrades[player]…`, or use `setPowerModifier`, `setStructureLimits`, `setDroidLimit` |
| Scale AI difficulty | Rules JS: `setPowerModifier` for income (as `rules/setup/powermodifier.js` does), and `Upgrades[player]` for damage, production speed and hit points |
| Build a different economy | Power: `setPowerModifier` / `setPower` / `playerPower` / `extraPowerTime`. Spawn pickups with `addFeature` and handle `eventPickup`. Drive logistics units with `orderDroidLoc` + `eventDroidIdle`. Keep counters in globals |
| Unlock research from loot | Drop pickup features on `eventDestroyed`, then `eventPickup` → `enableResearch` / `completeResearch` |
| Make things explode, burn, chain-react | A splash/incendiary weapon in `weapons.json`, triggered with `fireWeaponAtLoc` / `fireWeaponAtObj` from `eventDestroyed` |
| Spawn or replace units and buildings | `addDroid`, `addStructure` (takes a direction), `addFeature`, `removeObject`. Factory production: `buildDroid` |
| Heal, damage, promote | `setHealth`, `setDroidExperience`, `eventDroidRankGained` |
| Give vision or radar coverage | `addSpotter` / `removeSpotter`, `setRevealStatus` |
| Trigger on location | Map labels + `eventArea<label>`, `enumArea`, `enumRange`, `addLabel` |
| Script missions, waves, timers | Timers: `setTimer`, `queue`, `removeTimer`. Campaign helpers: `libcampaign` (`camSetFactories`, `camManageGroup`, …). Missions: `setMissionTime`, `setReinforcementTime`, and the transporter functions |
| Custom win/lose conditions | The rules script, or a challenge's `rules` script, with `gameOverMessage` |
| Change teams, ownership, colors | `setAlliance`, `donateObject`, `changePlayerColour` |
| Capture buildings or units | `donateObject(object, player)` |
| Atmosphere | `setWeather`, `setSky`, `setSunPosition`, `setSunIntensity`, `setFogColour`, `replaceTexture` |
| A day/night cycle (and an economy that follows it) | `setTimer` + `setSunIntensity` / `setSunPosition` / `setSky`. Drive any day/night game rules from the same JS clock |
| Sound and voice | Register files in `audio/audio.json`, then play them with `playSound(file[, x, y, z])` or `emitSound` |
| Cinematics | `cameraSlide`, `cameraTrack`, `cameraZoom`, `hideInterface` / `showInterface`, video sequences |
| Story and help | Briefings and messages, `hackAddMessage`, `addGuideTopic`, `console`, `chat`, `addBeacon` |
| Custom player commands | `setReticuleButton(…, callback)`. Turn the click into game state with `syncRequest` → `eventSyncRequest` |
| Animated units and buildings | PIE `EVENT` / `ANIMOBJECT` (move, fire, die, activate) and texture animation ([Animation.md](Animation.md)) |
| Keep players out of or inside areas | `setNoGoArea`, `setScrollLimits` |
| A new AI or faction brain | An AI JSON + JS, or a script controlling a player slot |

---

## Your first mod: salvage drums

*Destroyed ground units sometimes leave an oil drum that anyone can collect.* It uses only stock assets and two files.

```
salvage.wz
└── multiplay/
    └── script/
        └── mods/
            ├── init.js
            └── salvage.js
```

`multiplay/script/mods/init.js`:

```js
include("multiplay/script/mods/salvage.js");
```

`multiplay/script/mods/salvage.js`:

```js
// Destroyed ground units sometimes leave an oil drum.
namespace("salvage_"); // the engine will also call salvage_eventXxx for every eventXxx

const SALVAGE_CHANCE = 25; // percent (const: never changes, so it doesn't need saving)

// Runs alongside rules.js's own eventDestroyed. Neither replaces the other.
function salvage_eventDestroyed(victim)
{
	if (victim.type !== DROID || isVTOL(victim))
	{
		return;
	}
	if (syncRandom(100) >= SALVAGE_CHANCE) // syncRandom, never Math.random: every client must agree
	{
		return;
	}
	const x = victim.x;
	const y = victim.y;
	const terrain = terrainType(x, y);
	if (terrain === TER_WATER || terrain === TER_CLIFFFACE)
	{
		return;
	}
	// Only one feature or structure per tile - getObject(x, y) returns whatever occupies it
	if (!getObject(x, y))
	{
		addFeature("OilDrum", x, y);
	}
}
```

Zip the `multiplay` folder, rename the zip to `salvage.wz`, and drop it into `mods/<version>/autoload/` to try it. Things to notice:

- There's no state to save here. If there were (a counter, say), it would go in a global `var`.
- `rules.js` also has an `eventPickup` for oil drums (it reschedules random drum spawns). **Read the rules you're extending:** `data/mp/multiplay/script/rules/` is short.
- The same approach scales up: barrels produced at derricks, trucks that ferry them, refineries, pipelines that break and get repaired, chain explosions. All of that is events, orders, `addFeature`/`addStructure`, `fireWeaponAtLoc` and `setPower`, with the bookkeeping in globals.

---

## When there's no direct API: patterns that work

Before reaching for C++, try one of these:

- **Swap instead of mutate.** Need a unit to change form, model or loadout? Remove it and `addDroid` the replacement at the same spot, then restore health (`setHealth`) and experience (`setDroidExperience`).
- **Spotters instead of per-building sensor switches.** "This outpost only sees while powered" can be done with `addSpotter`/`removeSpotter` on the outpost's tile.
- **Structures as visuals.** Cables, pipes, fences and markers can be line-buildable wall-type structures with their own models. Broken versions are separate structures you swap in with `addStructure`.
- **Bake visuals into models.** Spinning radars, smoke stacks, idle loops, death animations: PIE animation and texture animation, not code.
- **Features as markers and pickups.** Features can be pickups (`eventPickup`), obstacles, or destructible props (`eventDestroyed`).
- **Map regions as "places".** A cave level, an island, a space platform: build it as an enclosed area of the map, and move units between areas with transporters or swapping.
- **Reticule buttons as commands.** `setReticuleButton` with a callback, plus `syncRequest` for anything that changes game state.

---

## When an engine change *is* the right answer

Sometimes a mod really does need something the engine doesn't offer. Even then, **build the mod first**. A working mod shows exactly which small capability is missing, and it's how good ideas reach the base game (see *Ultimate Scavengers* above). Good engine changes for modding look like this:

- **General**, not tied to your mod: a new stat field (`"passableBy": [...]`) or script function. **Never** a check for your mod's stat IDs or filename prefixes.
- **Data- or script-driven**: off by default, so unmodded games behave exactly as before.
- **Deterministic and saved**: game state lives in game objects or savegames, not in file-static maps. Randomness uses `gameRand`.
- **Doesn't override player settings** (keymaps, camera, controls) or change global rules (building spacing, weapon slot counts) as a side effect.
- **Small and reviewable**, with a short design discussion first: open an issue describing the mod you're building and the smallest capability that would unblock it.

Red flags that you're forking when you should be modding:

- Adding `if (stats->id == "MyModThing")` anywhere in `src/`
- Keeping game state in `static std::map<uint32_t, …>` outside save/load
- Changing objects directly from UI code instead of sending an order or `syncRequest`
- Raising global limits (`MAX_WEAPONS`, map size) "just in case"
- Removing checks (design validation, "cheated" flags, spacing rules) to make your content fit
- Reimplementing a screen the game already has (design, build menu) for one mod

---

## For AI coding assistants

If you're an AI assistant helping someone make a Warzone 2100 mod, campaign, faction or new mechanic:

1. **Default to a `.wz` mod made of data + JavaScript.** Do not modify `src/` or `lib/` unless the user explicitly wants an engine change *and* you have confirmed there's no script or data route.
2. **Check what's possible first:** search `src/wzapi.cpp` and `src/quickjs_backend.cpp` (look for `//-- ##` doc comments), and `doc/js-*.md`. Prefer events over polling.
3. **Review working examples** to learn the patterns. Don't copy whole game files into the mod, and override only what you must. Examples: `data/mp/multiplay/script/rules/` (especially `oildrum.js`), `data/base/script/campaign/`, the bundled add-on campaigns, and the skirmish AIs.
4. **Follow the determinism and saving rules** above: `syncRandom`, integer math, state in global `var`s, ids not objects, `syncRequest` for player input.
5. **Extend, don't replace:** add to skirmish rules through `multiplay/script/mods/init.js`, and handle events through a `namespace("yourmod_")` prefix instead of redefining `eventXxx` functions the rules already define.
6. **If something truly can't be done,** say so, use one of the patterns above to get close, and suggest the smallest *general* engine capability as an upstream issue rather than writing a mod-specific engine patch.
