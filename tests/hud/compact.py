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

"""Checks a HUD geometry dump (--hud-geometry-dump) taken in the Compact layout, on the compact contract's rules
(tests/hud/README.md).

Exit status: 0 the contract holds, 1 it doesn't, 2 bad input.
"""

import argparse
import json
import sys

# The stats panel (src/hci.h), and its header's page control and menu button (src/hud/hudids.h)
IDSTAT_FORM = 14000
IDHUD_STATS_PAGES = 0x7E000112
IDHUD_STATS_MENU = 0x7E000113
# The minimum heights with Larger Touch Targets
HEADER_MIN = 18
MENU_ITEM_MIN = 36
# The radar's square is empty in its corners, and the console takes the mouse only on its lines
OVERLAP_EXEMPT_CLASSES = ("RadarWidget", "InGameConsoleWidget")
# The header menu's items, the panel filters' menu's choices and search field
MENU_ITEM_CLASSES = ("::MenuItem", "WzClickableOptionsChoiceWidget::make(iV_fonts)::make_shared_enabler", "::MenuSearchField")


def inside(rect, width, height):
    x, y, w, h = rect
    return x >= 0 and y >= 0 and x + w <= width and y + h <= height


def overlap(a, b):
    ax, ay, aw, ah = a
    bx, by, bw, bh = b
    return ax < bx + bw and bx < ax + aw and ay < by + bh and by < ay + ah


def is_panel(widget):
    key = widget["key"]
    return (key.count("/") == 1 and "#" not in key) or (key.startswith("#") and "/" not in key)


def check_state(state, width, height, larger_targets):
    problems = []
    for widget in state["widgets"] + state.get("overlayWidgets", []):
        if widget["rect"][2] > 0 and widget["rect"][3] > 0 and not inside(widget["rect"], width, height):
            problems.append("%s %s is not inside the screen" % (widget["key"], widget["rect"]))
    panels = [w for w in state["widgets"] if is_panel(w) and w["rect"][2] > 0 and w["rect"][3] > 0
              and not any(name in w.get("class", "") for name in OVERLAP_EXEMPT_CLASSES)]
    for i, a in enumerate(panels):
        for b in panels[i + 1:]:
            if overlap(a["rect"], b["rect"]):
                problems.append("panels %s %s and %s %s overlap" % (a["key"], a["rect"], b["key"], b["rect"]))
    if larger_targets:
        prefix = "/%d" % IDSTAT_FORM
        header = {prefix + "/%d" % IDHUD_STATS_MENU: "menu button", prefix + "/%d" % IDHUD_STATS_PAGES: "page control",
                  prefix + "#0": "close button"}
        for widget in state["widgets"]:
            name = header.get(widget["key"])
            if name is not None and widget["rect"][3] < HEADER_MIN:
                problems.append("the stats panel's %s %s is less than %d px high" % (name, widget["rect"], HEADER_MIN))
        for widget in state.get("overlayWidgets", []):
            if widget.get("class", "").endswith(MENU_ITEM_CLASSES) and widget["rect"][3] < MENU_ITEM_MIN:
                problems.append("menu item %s %s is less than %d px high" % (widget["key"], widget["rect"], MENU_ITEM_MIN))
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("dump")
    parser.add_argument("--larger-targets", action="store_true", help="check the sizes for Larger Touch Targets too")
    parser.add_argument("--quiet", action="store_true", help="print only the problems")
    args = parser.parse_args()
    path = args.dump
    try:
        with open(path) as f:
            dump = json.load(f)
        width = dump["screen"]["width"]
        height = dump["screen"]["height"]
        states = dump["states"]
    except (OSError, ValueError, KeyError) as error:
        print("compact: %s: %s" % (path, error))
        return 2
    failed = False
    for state in states:
        if not state.get("available", True):
            if not args.quiet:
                print("%s: %s: not available, skipped" % (path, state["name"]))
            continue
        problems = check_state(state, width, height, args.larger_targets)
        for problem in problems:
            print("%s: %s: %s" % (path, state["name"], problem))
        failed = failed or bool(problems)
    if not args.quiet or failed:
        print("compact: %s: %s" % (path, "FAILED" if failed else "holds"))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
