#!/bin/sh
# GameState fork test: does a game loaded from a save continue exactly like the game that wrote it?
#
#   run A: start a headless game, write a save at tick T and keep running to tick E, tracing sync CRCs
#   run B: load that save in a fresh process and run to tick E, tracing sync CRCs
#   then compare both traces on the ticks after the save
#
# The trace is the per-tick syncDebug() CRC that multiplayer uses to detect a desync,
# so IDENTICAL means the loaded game simulated exactly what the original did.
# On DIFFERS, both runs are repeated with full sync-debug logs for the first differing tick,
# and the first differing log lines are printed.
#
# usage: forkcrc.sh <warzone2100 binary> <skirmish test> <save tick> <end tick> [options]
#   <skirmish test>   a --skirmish settings file name, ex. highground (data/mp/tests/highground.json),
#                     or mb:<scenario> for a movement bench scenario, ex. mb:perf_battle (run A then uses
#                     --movementbench, which needs no AI scripts and quits at its own tick budget),
#                     or cam:<start level> for a campaign, ex. cam:CAM_2A
#                     (--game, but CAM_1A pauses on a guide topic)
#   options:
#     --dying         save (or take the rewind snapshot) at the first tick >= <save tick> that ends with objects
#                     pending destruction
#     --dying-filter <filter>  like --dying, with a --gamestate-dying-filter (ex. 3,structure)
#     --no-loaded     load with --gamestate-test-suppress-loaded-event (no eventGameLoaded / research replay)
#     --before-messages  save (or take the rewind snapshot) right after a tick, before the game loop processes the
#                     messages for the next tick, rather than at the save point that menu saves use
#                     (--gamestate-save-before-messages)
#     --after-messages  the save point (the default, accepted for older command lines)
#     --coverage-pending  save (at or after <save tick>) once tile coverage changes are pending, as after a range
#                     upgrade between ticks (--gamestate-save-when-coverage-pending)
#     --rewind <k>    instead of a save + load, snapshot in-process at <save tick> and restore it <k> ticks later
#
# Environment: OUT (default /tmp/gamestate-forkcrc) holds the config dir, traces and logs.
#              RUN_TIMEOUT (default 600) is the per-run time limit in seconds (a stalled run counts as failed).
#              EXTRA (default none): more options for every run (ex. --mod=<test mod>).
#              SEED (default 1) is passed as --gamestate-test-seed, so every run of a cell plays the same game.
#              CHECK_COVERAGE (default 0, off) is passed as --gamestate-check-coverage=<ticks> to every run,
#              and a tile coverage difference it finds fails the cell.
# Exit status: 0 identical, 1 differs, 2 a run failed.
set -u

BIN=${1:?binary}
TEST=${2:?skirmish test}
SAVETICK=${3:?save tick}
ENDTICK=${4:?end tick}
shift 4
DYING=
DYINGTAG=
NOLOADED=
REWIND=
SAVEPOINT=
SAVEPOINTTAG=
while [ $# -gt 0 ]; do
	case "$1" in
		--dying) DYING=--gamestate-save-when-dying ;;
		--dying-filter) shift; DYING="--gamestate-save-when-dying --gamestate-dying-filter=${1:?dying filter}"; DYINGTAG=".dying_$(echo "$1" | tr ',' '_')" ;;
		--no-loaded) NOLOADED=--gamestate-test-suppress-loaded-event ;;
		--before-messages) SAVEPOINT="$SAVEPOINT --gamestate-save-before-messages"; SAVEPOINTTAG="$SAVEPOINTTAG.beforemsg" ;;
		--after-messages) ;;
		--coverage-pending) SAVEPOINT="$SAVEPOINT --gamestate-save-when-coverage-pending"; SAVEPOINTTAG="$SAVEPOINTTAG.covpending" ;;
		--rewind) shift; REWIND=${1:?rewind ticks} ;;
		*) echo "unknown option $1" >&2; exit 2 ;;
	esac
	shift
done

HERE=$(cd "$(dirname "$0")" && pwd)
RUN_TIMEOUT=${RUN_TIMEOUT:-600}
SEED=${SEED:-1}
CHECK_COVERAGE=${CHECK_COVERAGE:-0}
COVOPT=
[ "$CHECK_COVERAGE" -gt 0 ] && COVOPT="--gamestate-check-coverage=$CHECK_COVERAGE"
OUT=${OUT:-/tmp/gamestate-forkcrc}
SAVEDIR=skirmish
case "$TEST" in
	mb:*) START="--movementbench=${TEST#mb:}"; ACOMMON="--configdir=$OUT/config --gamestate-test-seed=$SEED" ;;
	cam:*) START="--game=${TEST#cam:}"; ACOMMON=""; SAVEDIR=campaign ;;
	*) START="--skirmish=$TEST.json"; ACOMMON="" ;;
esac
TEST=$(echo "$TEST" | tr ':' '_')
[ -n "$DYINGTAG" ] || DYINGTAG=${DYING:+.dying}
TAG="$TEST.$SAVETICK$DYINGTAG$SAVEPOINTTAG${NOLOADED:+.noloaded}${REWIND:+.rewind$REWIND}"
CFG="$OUT/config"
mkdir -p "$OUT" "$CFG"
SAVENAME="fork_$(echo "$TAG" | tr '.' '_')"
rm -rf "$CFG/savegames/$SAVEDIR/$SAVENAME" "$CFG/savegames/$SAVEDIR/$SAVENAME.gam"

COMMON="--configdir=$CFG --headless --autogame --gamestate-test-seed=$SEED $COVOPT ${EXTRA:-}"
[ -n "$ACOMMON" ] || ACOMMON="$COMMON"
case "$ACOMMON" in *check-coverage*) ;; *) ACOMMON="$ACOMMON $COVOPT ${EXTRA:-}" ;; esac

# coverage_report <status>: with CHECK_COVERAGE, report the coverage checks that found a difference.
# Returns the status to exit with (a difference fails an otherwise identical cell).
coverage_report() {
	[ -n "$COVOPT" ] || return "$1"
	n=$(cat "$OUT/$TAG.A.log" "$OUT/$TAG.B.log" | grep -c 'coverage check (')
	[ "$n" -eq 0 ] && return "$1"
	echo "$TAG: COVERAGE DIFFERS in $n check(s); first:"
	cat "$OUT/$TAG.A.log" "$OUT/$TAG.B.log" | grep 'coverage check (' | head -1 | sed 's/^.*GameState harness: /  /'
	[ "$1" -eq 0 ] && return 1
	return "$1"
}

# checks_report <status>: report a game message read between a capture and the next tick
# (the capture missed it: gamestate_capture.cpp), and the coverage differences (coverage_report).
# Returns the status to exit with.
checks_report() {
	s=$1
	n=$(cat "$OUT/$TAG.A.log" "$OUT/$TAG.B.log" | grep -c 'the capture missed it')
	if [ "$n" -gt 0 ]; then
		echo "$TAG: $n GAME MESSAGE(S) READ AFTER A CAPTURE; first:"
		cat "$OUT/$TAG.A.log" "$OUT/$TAG.B.log" | grep 'the capture missed it' | head -1 | sed 's/^.*\] /  /'
		[ "$s" -eq 0 ] && s=1
	fi
	coverage_report "$s"
}

run_a() { # $1 = extra args
	timeout "$RUN_TIMEOUT" "$BIN" $ACOMMON "$START" --gamestate-crc-trace="$OUT/$TAG.A.trace" \
		--gamestate-exit-at-tick="$ENDTICK" $1 > "$OUT/$TAG.A.log" 2>&1
}
run_b() { # $1 = extra args
	timeout "$RUN_TIMEOUT" "$BIN" $COMMON --load$SAVEDIR="$SAVENAME" $NOLOADED --gamestate-crc-trace="$OUT/$TAG.B.trace" \
		--gamestate-exit-at-tick="$ENDTICK" $1 > "$OUT/$TAG.B.log" 2>&1
}

if [ -n "$REWIND" ]; then
	# Reference: an uninterrupted run. Test: the same run, rewound in-process.
	timeout "$RUN_TIMEOUT" "$BIN" $ACOMMON "$START" --gamestate-crc-trace="$OUT/$TAG.A.trace" \
		--gamestate-exit-at-tick="$ENDTICK" > "$OUT/$TAG.A.log" 2>&1 || { echo "$TAG RUN A FAILED"; exit 2; }
	timeout "$RUN_TIMEOUT" "$BIN" $ACOMMON "$START" --gamestate-crc-trace="$OUT/$TAG.B.trace" \
		--gamestate-rewind-test="$SAVETICK:$REWIND" $DYING $SAVEPOINT --gamestate-exit-at-tick="$ENDTICK" > "$OUT/$TAG.B.log" 2>&1 \
		|| { echo "$TAG RUN B (rewind) FAILED"; exit 2; }
	AT=$(sed -n 's/.*rewind snapshot taken at gameTime \([0-9]*\).*/\1/p' "$OUT/$TAG.B.log" | head -1)
	[ -n "$AT" ] || { echo "$TAG NO REWIND SNAPSHOT"; exit 2; }
	grep -q "rewound from gameTime" "$OUT/$TAG.B.log" || { echo "$TAG REWIND RESTORE DID NOT HAPPEN"; exit 2; }
	# Compare with the uninterrupted run when both agree up to the snapshot (SEED makes them play the same game).
	# Otherwise compare the ticks the rewound run simulated twice (before the restore and after it).
	if python3 -I "$HERE/cmptrace.py" "$OUT/$TAG.A.trace" "$OUT/$TAG.B.trace" --until "$AT" > /dev/null; then
		RESULT=$(python3 -I "$HERE/cmptrace.py" "$OUT/$TAG.A.trace" "$OUT/$TAG.B.trace" --after "$AT" --last)
		STATUS=$?
		echo "$TAG (snapshot at $AT): $RESULT"
	else
		RESULT=$(python3 -I "$HERE/cmptrace.py" --replayed "$OUT/$TAG.B.trace" --after "$AT")
		STATUS=$?
		echo "$TAG (snapshot at $AT; runs not reproducible, re-simulated ticks vs first pass): $RESULT"
	fi
	checks_report $STATUS
	exit $?
fi

run_a "--gamestate-save-at-tick=$SAVETICK:savegames/$SAVEDIR/$SAVENAME.gam $DYING $SAVEPOINT" || { echo "$TAG RUN A FAILED"; exit 2; }
AT=$(sed -n 's/.*GameState harness: wrote save .* at gameTime \([0-9]*\) (\([0-9]*\) object.*/\1 \2/p' "$OUT/$TAG.A.log" | head -1)
[ -n "$AT" ] || { echo "$TAG NO SAVE WRITTEN (see $OUT/$TAG.A.log)"; exit 2; }
SAVEDAT=${AT% *}
PENDING=${AT#* }
run_b "" || { echo "$TAG RUN B FAILED"; exit 2; }

RESULT=$(python3 -I "$HERE/cmptrace.py" "$OUT/$TAG.A.trace" "$OUT/$TAG.B.trace" --after "$SAVEDAT")
STATUS=$?
echo "$TAG (saved at $SAVEDAT, $PENDING pending destruction): $RESULT"
if [ $STATUS -eq 1 ]; then
	# Prefer the first tick where the simulation itself differs over an echoed CRC (see cmptrace.py)
	TICK=$(echo "$RESULT" | sed -n 's/.*first non-echo difference at tick \([0-9]*\).*/\1/p')
	[ -n "$TICK" ] || TICK=$(echo "$RESULT" | sed -n 's/DIFFERS at tick \([0-9]*\).*/\1/p')
	run_a "--gamestate-save-at-tick=$SAVETICK:savegames/$SAVEDIR/$SAVENAME.gam $DYING $SAVEPOINT --gamestate-crc-detail-tick=$TICK" || exit 2
	run_b "--gamestate-crc-detail-tick=$TICK" || exit 2
	python3 -I "$HERE/difflog.py" "$OUT/$TAG.A.trace.detail.txt" "$OUT/$TAG.B.trace.detail.txt" | head -40
fi
checks_report $STATUS
exit $?
