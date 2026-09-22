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

# The HUD geometry contract test (tests/hud/README.md).
# For each screen size and display scale of the matrix, with the groups bar on and off, a Classic reference dump is
# taken (geometry.sh), and each variant's dump (ex. another skin, or modules turned on) is checked against it with
# contract.py: every Classic widget keeps its screen rectangle, z-order and the points that reach it, and additions
# only take points nothing took.
# With BASELINE=<dir> (the OUT of a run of another build), the Classic dumps must also match that build's, and their
# screenshots be pixel-identical.
#
# usage: contract.sh <warzone2100 binary> [<variant>=<config line>[;<config line>...]]...
#   ex. contract.sh build/src/warzone2100 modern=hudSkin=1 "badges=hud.objectBadges=1;hud.powerDetails=1"
# Environment: OUT (default /tmp/hud-contract), MATRIX (quick (default) or full), DETERMINISM (1: take the Classic
#   reference twice and compare), TOP_BAND (passed to contract.py, default 0), and those of geometry.sh.
# Exit status: 0 every check passed, 1 a check failed, 2 a run failed.
set -u
BIN=${1:?binary}
shift
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-/tmp/hud-contract}
MATRIX=${MATRIX:-quick}
DETERMINISM=${DETERMINISM:-0}
TOP_BAND=${TOP_BAND:-0}
BASELINE=${BASELINE:-}
mkdir -p "$OUT"
case "$MATRIX" in
quick) SIZES="640x480@100 1366x768@100 1280x720@150" ;;
full) SIZES="640x480@100 1024x768@100 1366x768@100 1920x1080@100 3440x1440@100 1280x720@150 1024x768@200 1920x1080@200 640x480@300 1280x720@300" ;;
*) SIZES="$MATRIX" ;;
esac

STATUS=0
note_fail() { [ "$1" -gt "$STATUS" ] && STATUS=$1; }
for size in $SIZES; do
	RES=${size%@*}
	SCALE=${size#*@}
	for groups in 1 0; do
		TAG="$RES@$SCALE.groups$groups"
		REF="$OUT/classic.$TAG"
		"$HERE/geometry.sh" "$BIN" "$REF" "$RES" "$SCALE" "groupmenu=$groups" || { note_fail 2; continue; }
		echo "$TAG: classic reference taken"
		if [ "$DETERMINISM" = 1 ]; then
			"$HERE/geometry.sh" "$BIN" "$REF.again" "$RES" "$SCALE" "groupmenu=$groups" || { note_fail 2; continue; }
			python3 -I "$HERE/contract.py" "$REF.json" "$REF.again.json" --quiet || note_fail 1
			python3 -I "$HERE/imgcmp.py" "$REF.shots" "$REF.again.shots" || note_fail 1
		fi
		if [ -n "$BASELINE" ]; then
			echo "$TAG: classic vs the baseline build"
			python3 -I "$HERE/contract.py" "$BASELINE/classic.$TAG.json" "$REF.json" --quiet || note_fail 1
			python3 -I "$HERE/imgcmp.py" "$BASELINE/classic.$TAG.shots" "$REF.shots" || note_fail 1
		fi
		for variant in "$@"; do
			NAME=${variant%%=*}
			LINES=${variant#*=}
			CAND="$OUT/$NAME.$TAG"
			OLDIFS=$IFS
			IFS=';'
			# shellcheck disable=SC2086
			"$HERE/geometry.sh" "$BIN" "$CAND" "$RES" "$SCALE" "groupmenu=$groups" $LINES
			RC=$?
			IFS=$OLDIFS
			[ $RC -eq 0 ] || { note_fail 2; continue; }
			python3 -I "$HERE/contract.py" "$REF.json" "$CAND.json" --top-band "$TOP_BAND" || note_fail 1
		done
	done
done
echo "contract: $( [ $STATUS -eq 0 ] && echo PASSED || echo "FAILED ($STATUS)" ) (results in $OUT)"
exit $STATUS
