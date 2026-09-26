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

# One HUD geometry dump (--hud-geometry-dump), under a virtual display (xvfb-run): a skirmish game is started at the
# given logical screen size, display scale and settings, game time stops at AT, the HUD is put in each state in turn,
# and each state's widget geometry and hit map are written to <out>.json, with one screenshot per state in
# <out>.shots/.
#
# usage: geometry.sh <warzone2100 binary> <out> <logical WxH> <display scale %> [<config line>...]
#   <config line>: a line for the [General] section of the config file (over the Classic HUD's settings), ex.
#   groupmenu=0, hudSkin=1
# Environment: TEST (default highground), AT (game time in ms, default 3000), STATES (default: the panel states),
#   SEED (default 1), RENDER_RESOLUTION (the 3D view, in % of the window, default 50), HDR (HDR lighting: off, on or
#   auto, default off), RUN_TIMEOUT (default 600 seconds).
# Exit status: 0 the dump was written, 2 the run failed (or no xvfb-run).
set -u
BIN=$(cd "$(dirname "${1:?binary}")" && pwd)/$(basename "$1")
OUTBASE=${2:?out}
LOGICAL=${3:?logical WxH}
SCALE=${4:?display scale}
shift 4
TEST=${TEST:-highground}
AT=${AT:-3000}
STATES=${STATES:-}
SEED=${SEED:-1}
RUN_TIMEOUT=${RUN_TIMEOUT:-600}
command -v xvfb-run > /dev/null || { echo "geometry: no xvfb-run"; exit 2; }

LW=${LOGICAL%x*}
LH=${LOGICAL#*x}
WW=$((LW * SCALE / 100))
WH=$((LH * SCALE / 100))
CFG="$OUTBASE.cfg"
rm -rf "$CFG" "$OUTBASE.json" "$OUTBASE.shots"
mkdir -p "$CFG" "$OUTBASE.shots"
{
	echo "[General]"
	echo "configVersion=4"
	echo "displayScale=$SCALE"
	echo "autoAdjustDisplayScale=false"
	echo "width=$WW"
	echo "height=$WH"
	# Cheaper 3D rendering, for software rendering under Xvfb (the HUD is drawn as usual)
	echo "shadows=false"
	echo "terrainShadows=false"
	echo "renderResolution=${RENDER_RESOLUTION:-50}"
	echo "vsync=0"
	echo "hdrLighting=${HDR:-off}"
	# The Classic HUD's settings, except those the config lines set
	for line in hudSkin=0 hudTooltips=0 hudTextOutline=0 hud.powerDetails=0 hud.statusLine=0 hud.objectBadges=0 \
		hud.idleIndicators=0 hud.rowSummary=0 hud.selectionReadout=0 hud.readableBars=0 hud.panelFilters=0 \
		hud.spectatorStrip=0; do
		case " $* " in
		*" ${line%%=*}="*) ;;
		*) echo "$line" ;;
		esac
	done
	for line in "$@"; do
		echo "$line"
	done
} > "$CFG/config"
# (The notification that campaign videos are missing would cover the top of the screen)
echo '{"notifications": {"campaignVideoNotification": {"skip": true}}}' > "$CFG/notifications.json"
SPEC="$OUTBASE.json,$AT"
[ -n "$STATES" ] && SPEC="$SPEC,$STATES"
timeout "$RUN_TIMEOUT" xvfb-run -a -s "-screen 0 $((WW + 16))x$((WH + 16))x24" "$BIN" --configdir="$CFG" --nosound --window \
	--resolution="${WW}x${WH}" --autogame --skirmish="$TEST.json" --gamestate-test-seed="$SEED" \
	--hud-geometry-dump="$SPEC" > "$OUTBASE.log" 2>&1
RC=$?
[ $RC -eq 0 ] && [ -s "$OUTBASE.json" ] || { echo "geometry: $OUTBASE: RUN FAILED (exit $RC; see $OUTBASE.log)"; exit 2; }
cp "$CFG"/hudtest/*.png "$OUTBASE.shots/" 2> /dev/null
exit 0
