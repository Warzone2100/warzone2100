#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later
#
# This file is part of Warzone 2100.
# Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)
#
# Warzone 2100 is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Additional terms under Section 7 of the GNU General Public License
# version 3 apply to this file: version 1.0 of the Warzone 2100
# additional terms, or (at your option) any later version, as published
# by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
# the top-level directory of the source distribution. This notice must
# be kept intact.
#
# Warzone 2100 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.

"""Checks a HUD geometry dump (--hud-geometry-dump) against a Classic reference dump of the same screen size, display
scale and settings, on the contract's rules (tests/hud/README.md).

Exit status: 0 the contract holds, 1 it doesn't, 2 bad input.
"""

import argparse
import json
import sys

# The Classic panels (src/hci.h, src/intorder.h)
IDSTAT_FORM = 14000
IDOBJ_FORM = 3000
IDORDER_FORM = 8000
# The panels that may grow, and the sides they may grow on
GROWABLE = {
    IDSTAT_FORM: {"up", "right"},
    IDOBJ_FORM: {"up"},
    IDORDER_FORM: {"down", "right"},
}
# The panel extensions (src/hud/hudids.h)
EXTENSION_IDS = {0x7E000110, 0x7E000120}


def load(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def is_addition(widget):
    return widget.get("addition", False)


def in_extension(key):
    return any(part.split("~", 1)[0].isdigit() and int(part.split("~", 1)[0]) in EXTENSION_IDS
               for part in key.split("/"))


def own_id(key):
    last = key.rsplit("/", 1)[-1].split("~", 1)[0]
    return int(last) if last.isdigit() else None


def growth(ref, cand):
    """The sides a rectangle grew on, or None if it shrank or moved"""
    rx, ry, rw, rh = ref
    cx, cy, cw, ch = cand
    sides = set()
    if cx > rx or cy > ry or cx + cw < rx + rw or cy + ch < ry + rh:
        return None
    if cx < rx:
        sides.add("left")
    if cy < ry:
        sides.add("up")
    if cx + cw > rx + rw:
        sides.add("right")
    if cy + ch > ry + rh:
        sides.add("down")
    return sides


def expand_rows(hitmap, width):
    """The hit map's rows as lists of keys (None: nothing), one entry per sample"""
    keys = hitmap["keys"]
    step = hitmap["step"]
    samples = (width + step - 1) // step
    rows = []
    for row in hitmap["rows"]:
        out = [None] * samples
        for i in range(0, len(row), 2):
            x0 = row[i] // step
            x1 = row[i + 2] // step if i + 2 < len(row) else samples
            name = keys[row[i + 1]] if row[i + 1] >= 0 else None
            for s in range(x0, x1):
                out[s] = name
        rows.append(out)
    return rows


def check_state(ref, cand, screen, top_band, problems, notes):
    name = ref["name"]

    def fail(msg):
        problems.append(f"{name}: {msg}")

    if ref["available"] != cand["available"]:
        fail(f"available {ref['available']} in the reference, {cand['available']} here")
        return

    ref_widgets = {w["key"]: w for w in ref["widgets"] if not is_addition(w)}
    cand_all = {w["key"]: w for w in cand["widgets"]}
    cand_classic = {w["key"]: w for w in cand["widgets"] if not is_addition(w)}
    grown = set()
    for key, rw in ref_widgets.items():
        cw = cand_all.get(key)
        if cw is None:
            fail(f"Classic widget {key} ({rw['class']}) is missing")
            continue
        if is_addition(cw):
            fail(f"Classic widget {key} is now named as a HUD addition")
            continue
        if cw["rect"] != rw["rect"]:
            wid = own_id(key)
            sides = growth(rw["rect"], cw["rect"])
            if wid in GROWABLE and sides is not None and sides <= GROWABLE[wid]:
                grown.add(key)
                notes.append(f"{name}: {key} grew {'/'.join(sorted(sides))}: {rw['rect']} -> {cw['rect']}")
            else:
                fail(f"Classic widget {key} ({rw['class']}) moved: {rw['rect']} -> {cw['rect']}")
    for key, cw in cand_classic.items():
        if key not in ref_widgets:
            fail(f"new widget {key} ({cw['class']}) outside the HUD addition ID range")

    ref_order = [w["key"] for w in sorted(ref["widgets"], key=lambda w: w["z"]) if not is_addition(w)]
    cand_order = [w["key"] for w in sorted(cand["widgets"], key=lambda w: w["z"])
                  if w["key"] in ref_widgets and not is_addition(w)]
    if [k for k in ref_order if k in cand_all] != cand_order:
        fail("the Classic widgets' z-order changed")

    for w in cand["widgets"] + cand.get("overlayWidgets", []):
        if not is_addition(w) or w.get("mouseTransparent"):
            continue
        x, y, wd, ht = w["rect"]
        if x < 0 or y < 0 or x + wd > screen["width"] or y + ht > screen["height"]:
            fail(f"HUD addition {w['key']} is off screen: {w['rect']}")
        if top_band > 0 and y < top_band and wd > 0 and ht > 0 and in_extension(w["key"]):
            fail(f"panel extension {w['key']} is in the top band ({top_band} px): {w['rect']}")

    ref_rows = expand_rows(ref["hitMap"], screen["width"])
    cand_rows = expand_rows(cand["hitMap"], screen["width"])
    step = ref["hitMap"]["step"]
    addition_keys = {w["key"] for w in cand["widgets"] + cand.get("overlayWidgets", []) if is_addition(w)}
    changed = {}
    for yi, (rrow, crow) in enumerate(zip(ref_rows, cand_rows)):
        for xi, (r, c) in enumerate(zip(rrow, crow)):
            if r == c:
                continue
            if r is None and (c in addition_keys or c in grown):
                continue
            pair = (r, c)
            if pair not in changed:
                changed[pair] = [xi * step, yi * step, 0]
            changed[pair][2] += 1
    for (r, c), (x, y, count) in sorted(changed.items(), key=lambda kv: -kv[1][2]):
        fail(f"{count} sample(s) reached {c} instead of {r or 'nothing'} (first at {x},{y})")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("reference")
    parser.add_argument("candidate")
    parser.add_argument("--top-band", type=int, default=0, metavar="PX",
                        help="the height at the top kept for the console and timers")
    parser.add_argument("--quiet", action="store_true", help="don't print the notes")
    args = parser.parse_args()
    ref = load(args.reference)
    cand = load(args.candidate)
    if ref["screen"] != cand["screen"]:
        print(f"different screens: {ref['screen']} vs {cand['screen']}")
        return 2
    problems = []
    notes = []
    ref_states = {s["name"]: s for s in ref["states"]}
    for state in cand["states"]:
        if state["name"] not in ref_states:
            problems.append(f"{state['name']}: not in the reference")
            continue
        check_state(ref_states[state["name"]], state, ref["screen"], args.top_band, problems, notes)
    if not args.quiet:
        for note in notes:
            print("  note: " + note)
    for problem in problems[:200]:
        print("  FAIL: " + problem)
    if len(problems) > 200:
        print(f"  ... and {len(problems) - 200} more")
    print(f"{args.candidate}: {len(cand['states'])} state(s), "
          f"{'CONTRACT HOLDS' if not problems else f'{len(problems)} problem(s)'}")
    return 0 if not problems else 1


if __name__ == "__main__":
    sys.exit(main())
