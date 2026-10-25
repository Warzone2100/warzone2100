#!/bin/sh
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

# The HUD Compact layout test (tests/hud/README.md).
# For each screen size at the engine's minimum height (480), in both skins, with Larger Touch Targets off and on (and
# with them the panel filters, whose tab goes beside the stats panel), a dump in the Compact layout (geometry.sh) is
# checked with compact.py: everything on screen, no panels overlapping, and with Larger Touch Targets, the header
# controls and menu items at least their minimum sizes.
#
# usage: compact.sh <warzone2100 binary>
# Environment: OUT (default /tmp/hud-compact), SIZES (default 640x480@200 854x480@200; lower heights need the
#   engine's minimum, MIN_WZ_GAMESCREEN_HEIGHT, lowered), STATES (by default the panel states but the design screen,
#   the selections, and the header menu and panel filters opened), and those of geometry.sh.
# Exit status: 0 every check passed, 1 a check failed, 2 a run failed.
set -u
BIN=${1:?binary}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-/tmp/hud-compact}
SIZES=${SIZES:-"640x480@200 854x480@200"}
MENU="click=/14000/2113929491"
FILTERS="click=/14000/2113929488@0"
DEFAULT_STATES="none,build,manufacture,research,commander,order,timers,launch,select,selectall,build+$MENU,manufacture+$MENU,research+$MENU,build+timers+$FILTERS,research+$FILTERS"
mkdir -p "$OUT"

STATUS=0
note_fail() { [ "$1" -gt "$STATUS" ] && STATUS=$1; }
for size in $SIZES; do
	RES=${size%@*}
	SCALE=${size#*@}
	for skin in 0 1; do
		for targets in 0 1; do
			TAG="$RES@$SCALE.skin$skin.targets$targets"
			STATES=${STATES:-$DEFAULT_STATES} "$HERE/geometry.sh" "$BIN" "$OUT/$TAG" "$RES" "$SCALE" "hudSkin=$skin" "hudLayout=2" \
				"hudLargerTouchTargets=$targets" "hud.panelFilters=$targets" || { note_fail 2; continue; }
			FLAGS="--quiet"
			[ "$targets" = 1 ] && FLAGS="$FLAGS --larger-targets"
			# shellcheck disable=SC2086
			python3 -I "$HERE/compact.py" "$OUT/$TAG.json" $FLAGS || note_fail 1
		done
	done
done
echo "compact: $( [ $STATUS -eq 0 ] && echo PASSED || echo "FAILED ($STATUS)" ) (results in $OUT)"
exit $STATUS
