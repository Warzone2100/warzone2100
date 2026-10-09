# HUD geometry contract tests

The HUD can be drawn by another skin and extended by modules (`src/hud/`), but every **existing** interactive element
must keep its screen position, size, hit-test shape, z-order and place in its list, as players click it by position.
These scripts check that, and that the Classic HUD keeps looking exactly the same.

They need a built `warzone2100` binary, `xvfb-run` and `python3` (with Pillow, the screenshots are compared faster).
Under software rendering (Mesa llvmpipe) the 3D view may come out black, but the HUD is drawn as usual.

| Script | What it does |
|---|---|
| `geometry.sh <bin> <out> <logical WxH> <scale %> [<config line>...]` | Takes one dump: starts the `highground` skirmish test (`TEST`) at the given logical screen size and display scale, with any extra `[General]` config lines (ex. `groupmenu=0`, `hudSkin=1`) over the Classic HUD's settings, stops game time at 3000 ms (`AT`), puts the HUD in each state in turn (`STATES`, by default the panel states), and writes `<out>.json`, with one screenshot per state in `<out>.shots/`. |
| `contract.sh <bin> [<variant>=<config line>[;<config line>...]]...` | The contract test. For each size of the matrix (`MATRIX=quick` or `full`, or a list like `1366x768@100`), with the groups bar on and off, it takes a Classic reference dump and checks each variant's dump against it with `contract.py`. `DETERMINISM=1` takes the reference twice and compares the two. `BASELINE=<dir>` compares the Classic dumps and screenshots with those in the `OUT` of a run of another build (ex. upstream master with only the commit that adds these tests): the Classic HUD must be pixel-identical. States the baseline build doesn't have (ex. `timers`) are skipped. |
| `contract.py <reference.json> <candidate.json> [--top-band <px>] [--quiet] [--new-states]` | Checks a dump against a Classic reference dump of the same size and scale (below). `--new-states` skips the states the reference doesn't have. |
| `imgcmp.py <reference shots> <candidate shots> [--tolerance <delta>] [--max-pixels <n>]` | Compares two sets of screenshots, state by state. |

## The dump

`--hud-geometry-dump=<file>[,<game time ms>[,<state>...]]` (test only, `src/hud/hudgeometrydump.h`) stops game time,
so every state shows the same game, and clears the console's messages every frame, so no log line shows in a
screenshot. Each state is one or more of the following, joined with `+` (ex. `build+select`).

The panel states, which are the default ones:

- `none`: no panel open.
- `build`, `manufacture`, `research` and `commander`: that panel.
- `order`: the order panel, for the local player's first droid.
- `timers`: the campaign's timer row (the mission timer, and the transporter's ETA and load button), in any game.
- `launch`: the same, with the launch button.

The panels are opened with the test hook `--gamestate-ui-stress` uses (`intTestShowForm()`).

Selection and input:

- `select` selects the local player's first droid.
- `selectall` selects all of their units (ex. `selectall+order` shows every order button they have between them).
- `hover=<key>` points at the widget with that key and waits for its tooltip or detail popover (ex.
  `build+hover=/14000/14004#5`). The screenshot shows it, and the popover is listed in `overlayWidgets`.
- `click=<key>` clicks a widget first (ex. a filter chip), so the state shows its effect. A clicked filter changes
  which items are listed, so such states are for screenshots and behavior checks, not for the contract.
- `touch` makes clicks count as taps (ex. `research+touch+click=/14000/2113929490@1` opens the page picker).

Other elements:

- `flash` makes the mission timer and the launch button flash (ex. `launch+flash`).
- `guide=<topic id>` opens the in-game guide at that topic (ex. `guide=wz2100::unitorders::firing`), to see its
  pictures of the HUD's buttons.
- `netwarn` shows an example of each multiplayer connection warning (a player dropped, one left, every other player
  out of sync, and waiting 12 s for the last), and `netwarnopen` the same with the Modern skin's panel naming them.
  They aren't default states, as the contract doesn't apply to them: the Modern panel stands where the Classic
  column of images doesn't.
- `debug=<droids|structures|features>` opens the debug menu's panel for adding them.

Each state in the JSON has its `name`, `available` (false when it couldn't be set up, ex. with no droid), and:

- `widgets`: every visible widget of the HUD screen, in paint (z) order, with its screen rectangle, type and class,
  and a **key** that names it the same way in every build: the IDs of its ancestors and its own ID (ex.
  `/3000/3500`), or, for a widget without an ID, its index among the unnamed widgets under its nearest named ancestor
  (ex. `/14000#0`). A second widget with the same key gets `~1` added, and so on.
- `overlayWidgets`: the same for overlay screens (popovers and the like), with keys starting with `overlay<z>:<n>`.
- `hitMap`: which widget a click at each point of the screen reaches (every 2 logical pixels, run-length encoded by
  row), found with the widget library's own mouse-target search, overlays first (`overlay:root` when an overlay
  screen's own form takes it).

### Naming HUD additions

Widgets with IDs in the HUD addition range (`src/hud/hudids.h`) are **additions**: they aren't part of the Classic
HUD, and every widget below them is an addition too. Unnamed additions are numbered on their own (`@<n>` instead of
`#<n>`), so they never renumber the Classic widgets beside them.

Widgets with IDs in the wrapper range only reposition Classic widgets (a panel's contents, when the panel grows).
They are listed as additions (`<parent>/wrapper:<n>`), and skipped when naming the widgets inside them, so those keep
their Classic names.

## The contract (`contract.py`)

Against the Classic reference of the same size, scale and settings:

1. Every Classic widget is still there, with the same screen rectangle, with these exceptions (marked in the dump):
   - The stats panel may grow up and right, the object row up, and the order panel down and right (panel extensions).
   - A grown panel's close button (`movedClose`) moves to the extension's corner. It may be anywhere inside the
     panel, and rules 3 and 4 skip it.
   - With the panel filters' page control, the stats panel's header controls move into its row (`movedControl`).
     They may be anywhere inside the panel, and rules 3 and 4 skip them.
   - A list's page tabs that an extension shows in their place (`replacedPageTabs`: the object row's, with the row
     summary strip, and the stats panels', with the panel filters' page control) are hidden but still listed where
     they were. The points that reached them may reach anything.
   - The stats panel may give up the rows at the top of its contents that it no longer shows (`topInset`), and the
     points there may reach anything.
   - A widget a skin places a little differently in its row (`skinPlaced`: the mission timer's time, after the Modern
     skin's stopwatch) may move up to 16 px, keeping its size, and the points between it and its parent (or
     nothing) may change hands.
2. No new widget appears outside the addition range.
3. The Classic widgets keep their relative z-order.
4. Every sampled point that reached a Classic widget still reaches it. A point that reached nothing may reach a HUD
   addition or a panel that grew over it, and nothing else.
5. The HUD additions that take the mouse are on screen, and panel extensions stay below the top band (`--top-band`)
   kept for the console and timers.

By rule 4, a detail popover may never take a click.
