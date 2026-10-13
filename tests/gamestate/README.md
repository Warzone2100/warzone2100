# GameState fidelity tests

Scripts that check whether a game restored from a GameState snapshot continues exactly like the game that wrote it.
They compare the per-tick sync CRC (`--gamestate-crc-trace`),
which is the same `syncDebug()` CRC multiplayer uses to detect a desync,
so "identical" means the restored game simulated exactly what the original did.

All of them need a built `warzone2100` binary, and most run headless.
Results go to `$OUT` (default `/tmp/gamestate-forkcrc`).

| Script | What it checks |
|---|---|
| `forkcrc.sh <bin> <test> <save tick> <end tick> [--dying \| --dying-filter <filter>] [--no-loaded] [--before-messages]` | **Fork test.** Run A starts a game, saves at the save tick (with `--dying`, at the first tick from there that ends with objects pending destruction) and keeps running, and run B loads that save in a fresh process. The traces are compared after the save tick. On a difference, both runs are repeated with the full sync-debug log of the first differing tick and the first differing lines are printed. A game message read between the save and the next tick (which the save missed) also fails the cell. `--before-messages` saves before the messages for the next tick instead. |
| `forkcrc.sh <bin> <test> <tick> <end tick> --rewind <k>` | **Rewind test.** Snapshots in-process at the tick, keeps running `k` more ticks, restores the snapshot in-process and keeps running, and is compared with an uninterrupted run. If the two runs already differ before the snapshot (ex. a different `SEED`), the ticks the rewound run simulated twice are compared with each other instead. |
| `doublesave.sh <bin> <save> [--no-loaded]` | **Double-save test.** Loads a save and saves it again right after the load (before the first tick), and compares the two GameState documents section by section. |
| `betweenticks.sh <bin>` | **Between-ticks save test.** A fork cell saved between two ticks, right after a message processed there raised a player's sensor ranges (a test mod's `eventSyncRequest` handler), so tile coverage changes are still pending at the save. |
| `rewindfault.sh <bin> <test> <tick> <end tick> <k> <fault>` | **Rewind fault test.** A rewind test whose snapshot is damaged right before it is restored (`--gamestate-rewind-fault`), compared with an uninterrupted run: the restore is rejected before it changes anything and the live game goes on (the run must be IDENTICAL to the reference), or it restores anyway (shows unresolved references and compares the ticks after the snapshot), or it fails after changing live state (shows the stage, the exit status, the last tick and the assertions after the failure). |
| `replayrewind.sh <bin> <test> <tick> <end tick> <k>` | **Replay rewind test.** Records a headless skirmish game, plays its replay back (`--loadreplay`), and plays it back again with an in-process rewind; both playbacks must reproduce the recording's CRCs (the rewound one after the snapshot). The first step towards replay seek-back. |
| `matrix.sh <bin> [filter]` | Runs the standard set of fork, rewind and double-save cells. |
| `camtransition.sh <bin>` | **Campaign transition test.** Plays `CAM_2A` into the off-world mission `SUB_2_1` and back out without a player (a test mod built from the campaign scripts calls `camNextLevel`), twice, and checks that no assertion fires and that both runs start every level at the same `gameTime` with the same power and trace the same CRCs (no tick runs between a `loadLevel` and the next level). Covers what has to be rebuilt or dropped when worlds are freed and swapped between ticks. The matrix also runs fork cells and a double save through this level change, with its test mod (`EXTRA=--mod=camtransition.wz`, which `forkcrc.sh` and `doublesave.sh` pass to the game). |
| `camlaunch.sh <bin>` | **Campaign transporter launch test.** Launches the player's transporter from `CAM_2A`'s home base as the launch button does, twice: once in a pre-off-world level, whose exit loads the off-world mission `SUB_2_1` (the home world, which the level change keeps, must keep no coverage of the transporter), and once with `CAM_2A`'s own conditions, where the transporter stays. Checks the tile coverage of both worlds every tick. |
| `camreinforce.sh <bin> [<reinforce tick> <save tick> <end tick>]` | **Campaign reinforcement test.** A test mod sends the player's transporter from `CAM_2A` into the off-world mission `SUB_2_1` and gives it reinforcements every 30 s; at `<reinforce tick>` (default 33000), `--gamestate-test-reinforce` sends reinforcements as the transporter screen does. They must start flying in (`eventTransporterArrived`) 10 s before the 30 s are up, at the same tick headless, after a save and load while they wait (a `forkcrc.sh` cell saved at `<save tick>`, default 45000), and with the GUI (`xvfb-run`, if installed; the run must also be IDENTICAL to the headless one). |
| `deadgrid.sh <bin>` | **Dead grid readers test.** Runs the `dead_grid` movement bench scenario, in which objects die in a tick where a grid reader that runs later in that tick would find them (the grid keeps them until the end of the tick): a second squish of a dead person, a truck picking a dead structure to repair or help build, a dead droid in the way of a new structure, and a dead droid keeping a gate open. Checks the scenario's `deadgrid:` lines and that no dead action target asserts. The matrix also saves this scenario at the ticks of those deaths. |
| `cmptrace.py`, `difflog.py`, `cmpsave.py` | The comparison helpers used by the scripts (`cmptrace.py --replayed <trace>` compares a rewound run with itself). |

`<test>` is a `--skirmish` settings file from `data/mp/tests` (ex. `highground`, `miza`), which runs bundled AIs,
or `mb:<scenario>` for a movement bench scenario (ex. `mb:perf_battle`),
which runs without AI scripts and so isolates engine restore fidelity,
or `cam:<start level>` for a campaign (`--game`, ex. `cam:CAM_2A`).
`CAM_1A` pauses on a guide topic, and a level can end early when nobody plays it, so keep end ticks short.

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
| `--gamestate-save-at-tick=<tick>:<path>` | Write a savegame at the first tick >= tick, and keep running. It is written at the save point, like a menu save: between two ticks, once the game loop has processed every message for the next tick (`src/gamestate_capture.h`). |
| `--gamestate-save-when-dying` | Delay that save (and the rewind snapshot) until a tick that ends with objects pending destruction. |
| `--gamestate-dying-filter=<filter>` | What those objects must include: comma-separated, a minimum count, `droid`, `structure`, `feature`, `pendingvis` (an object whose tile visibility removal is still pending). |
| `--gamestate-exit-at-tick=<tick>` | Quit at the first tick >= tick. |
| `--gamestate-rewind-test=<tick>:<k>` | Snapshot in-process at the tick, restore it in-process `k` ticks later. Logs the snapshot and restore times. |
| `--gamestate-rewind-fault=<kind>[,continue]` | Damage the rewind snapshot right before the restore: `truncate`, `section-version:<section>`, `bad-stat`, `duplicate-id`, `dangling-ref`, `overlap`, `edge`, `off-map`, `over-limit`, or `throw-at:<stage>` (a throw when the restore reaches that stage: `teardown`, `terrain`, `globals`, `formations`, `worlds`, `dying`, `pass1`, `pass2`, `post`, `sections`, `scripting`, `derived`, `counters`). A restore that fails before changing live state lets the live game go on; a later failure quits, or with `,continue` ticks the half-restored game on. `rewindfault.sh`. |
| `--gamestate-test-reinforce=<tick>` | At the first capture point at or after `<tick>`, during an off-world campaign mission, send reinforcements as the player does from the transporter screen: a home base droid is put aboard the transporter waiting there, and it is launched. Not in a game loaded at or after `<tick>`. `camreinforce.sh`. |
| `--gamestate-resave-on-load=<path>` | After a GameState load has completed, save again before the first tick, and quit. |
| `--gamestate-test-seed=<seed>` | Seed the new game's random seed and every script's `Math.random()` from `<seed>` instead of the clock, and fix the scripts' `Date` to a time derived from it, so a run can be repeated in another process. `forkcrc.sh` passes `$SEED` (default 1). |
| `--gamestate-save-before-messages` | Take the `--gamestate-save-at-tick` save and the rewind snapshot right after a tick, before the game loop processes the messages for the next tick, instead of at the save point. A game saved there can resume differently. `forkcrc.sh --before-messages`. (`--gamestate-save-after-messages`, which selects the default save point, is also accepted, as is `forkcrc.sh --after-messages`.) |
| `--gamestate-save-when-coverage-pending` | Delay that save until tile coverage changes are pending (ex. a range upgrade between ticks). `forkcrc.sh --coverage-pending`. |
| `--gamestate-check-coverage=<ticks>` | Every `<ticks>` ticks, right before a harness save or rewind snapshot, and right after a GameState load, check in both worlds that every droid's and structure's tile coverage matches a recompute from its current state, that the per-tile counts add up to the watched tiles, and that watched tiles are explored for every player sharing vision, and log the checks that differ. A loaded game can only recompute it. `forkcrc.sh` passes `$CHECK_COVERAGE` (default 0, off; `matrix.sh` sets 10), and a difference fails the cell. |
| `--gamestate-test-suppress-loaded-event` | On a GameState load, don't fire `eventGameLoaded` and drop the queued `eventResearched` replay. Separates engine restore fidelity from what load-time script handlers do. |

Reading the results:

- A tick whose CRC differs is echoed a few ticks later in the `GAME_GAME_TIME` message, so one difference repeats at a
  fixed stride for the rest of the run. `cmptrace.py` reports "echo chain only" when that is all that differs, and
  otherwise the first tick that is not an echo (where the simulation itself parts). When consecutive ticks differ, the
  simulation has parted at the first one.
- Movement bench saves are loaded with the standard rules script rather than the bench's own, and fork cells that use
  bundled AIs also measure what the AI scripts restore (see the `var` convention); use `--no-loaded` and the `mb:` cells to
  isolate the engine.
