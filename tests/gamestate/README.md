# GameState fidelity tests

Scripts that check whether a game restored from a GameState snapshot continues exactly like the game that wrote it.
They compare the per-tick sync CRC (`--gamestate-crc-trace`),
which is the same `syncDebug()` CRC multiplayer uses to detect a desync,
so "identical" means the restored game simulated exactly what the original did.

All of them need a built `warzone2100` binary, and most run headless.
Results go to `$OUT` (default `/tmp/gamestate-forkcrc`).

| Script | What it checks |
|---|---|
| `forkcrc.sh <bin> <test> <save tick> <end tick> [--dying \| --dying-filter <filter>] [--no-loaded]` | **Fork test.** Run A starts a game, saves at the save tick (with `--dying`, at the first tick from there that ends with objects pending destruction) and keeps running; run B loads that save in a fresh process. The traces are compared after the save tick. On a difference, both runs are repeated with the full sync-debug log of the first differing tick and the first differing lines are printed. |
| `forkcrc.sh <bin> <test> <tick> <end tick> --rewind <k>` | **Rewind test.** Snapshots in-process at the tick, keeps running `k` more ticks, restores the snapshot in-process and keeps running; compared with an uninterrupted run. If the two runs already differ before the snapshot (ex. a different `SEED`), the ticks the rewound run simulated twice are compared with each other instead. |
| `doublesave.sh <bin> <save> [--no-loaded]` | **Double-save test.** Loads a save and saves it again right after the load (before the first tick); compares the two GameState documents section by section. |
| `matrix.sh <bin> [filter]` | Runs the standard set of fork, rewind and double-save cells. |
| `deadgrid.sh <bin>` | **Dead grid readers test.** Runs the `dead_grid` movement bench scenario, in which objects die in a tick where a grid reader that runs later in that tick would find them (the grid keeps them until the end of the tick): a second squish of a dead person, a truck picking a dead structure to repair or help build, a dead droid in the way of a new structure, and a dead droid keeping a gate open. Checks the scenario's `deadgrid:` lines and that no dead action target asserts. The matrix also saves this scenario at the ticks of those deaths. |
| `cmptrace.py`, `difflog.py`, `cmpsave.py` | The comparison helpers used by the scripts (`cmptrace.py --replayed <trace>` compares a rewound run with itself). |

`<test>` is a `--skirmish` settings file from `data/mp/tests` (ex. `highground`, `miza`), which runs bundled AIs, or
`mb:<scenario>` for a movement bench scenario (ex. `mb:perf_battle`), which runs without AI scripts and so isolates
engine restore fidelity, or `cam:<start level>` for a campaign (`--game`, ex. `cam:CAM_2A`; `CAM_1A` pauses on a guide
topic, and a level can end early when nobody plays it, so keep end ticks short). `mb:gamestate_deaths` is a scenario for
these tests: every few seconds it kills, in one tick, several droids (some while being repaired), a commander, a repair
facility while it repairs, a wall and an oil drum, and its `eventDestroyed` handler removes one more droid.
`mb:gamestate_scripts` is another: it saves while some members of a script group are aboard a transporter (which drops
them after the save), and with object labels armed for one player only (`resetLabel(label, player)`) on objects first
seen after the save; a group member missing after a load, or an `eventObjectSeen` the original game did not have, spawns
a droid. `gamestate_cobra` and `gamestate_bonecrusher` are `miza` with every slot running Cobra or BoneCrusher (`miza`
itself runs SemperFi, NullBot and the default AI, Nexus), so that the matrix covers every bundled AI.

`mb:gamestate_deaths` is a scenario for these tests:
every few seconds it kills, in one tick, several droids (some while being repaired), a commander,
a repair facility while it repairs, a wall and an oil drum, and its `eventDestroyed` handler removes one more droid.
`mb:gamestate_rewind` changes script state after a rewind snapshot that an in-process restore must roll back
(a new global, `groupSizes[]` of a group filled later, a declared global, a group member, a timer).
`mb:gamestate_scripts` is another: it saves while some members of a script group are aboard a transporter
(which drops them after the save),
and with object labels armed for one player only (`resetLabel(label, player)`) on objects first seen after the save.
A group member missing after a load, or an `eventObjectSeen` the original game did not have, spawns a droid.
`gamestate_cobra` and `gamestate_bonecrusher` are `miza` with every slot running Cobra or BoneCrusher
(`miza` itself runs SemperFi, NullBot and the default AI, Nexus), so that the matrix covers every bundled AI.

Options passed to the game (all inert unless given).
They are developer options: `--help` doesn't list them, and their descriptions aren't translated.

| Option | Effect |
|---|---|
| `--gamestate-save-at-tick=<tick>:<path>` | Write a savegame at the first tick >= tick (between two ticks, like a menu save), and keep running. |
| `--gamestate-save-when-dying` | Delay that save (and the rewind snapshot) until a tick that ends with objects pending destruction. |
| `--gamestate-dying-filter=<filter>` | What those objects must include: comma-separated, a minimum count, `droid`, `structure`, `feature`, `pendingvis` (an object whose tile visibility removal is still pending). |
| `--gamestate-exit-at-tick=<tick>` | Quit at the first tick >= tick. |
| `--gamestate-rewind-test=<tick>:<k>` | Snapshot in-process at the tick, restore it in-process `k` ticks later. |
| `--gamestate-resave-on-load=<path>` | After a GameState load has completed, save again before the first tick, and quit. |
| `--gamestate-test-seed=<seed>` | Seed the new game's random seed and every script's `Math.random()` from `<seed>` instead of the clock, and fix the scripts' `Date` to a time derived from it, so a run can be repeated in another process. `forkcrc.sh` passes `$SEED` (default 1). |
| `--gamestate-test-suppress-loaded-event` | On a GameState load, don't fire `eventGameLoaded` and drop the queued `eventResearched` replay. Separates engine restore fidelity from what load-time script handlers do. |

Reading the results:

- A tick whose CRC differs is echoed a few ticks later in the `GAME_GAME_TIME` message, so one difference repeats at a
  fixed stride for the rest of the run. `cmptrace.py` reports "echo chain only" when that is all that differs, and
  otherwise the first tick that is not an echo (where the simulation itself parts). When consecutive ticks differ, the
  simulation has parted at the first one.
- Movement bench saves are loaded with the standard rules script rather than the bench's own, and fork cells that use
  bundled AIs also measure what the AI scripts restore (see the `var` convention); use `--no-loaded` and the `mb:` cells to
  isolate the engine.
