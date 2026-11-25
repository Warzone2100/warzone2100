#!/bin/sh
# Built-in objects in script globals test: Map, Set and Date globals (and subclass instances of them) must be saved
# and restored with their contents, iteration order, shared identity and prototypes.
#
# A global test mod adds a skirmish AI whose globals hold Maps, Sets and Dates
# (nested, shared between globals, cyclic, subclass instances, held by class instances,
# with attached own properties, with -0 / NaN / object keys, a large Map with most of its entries deleted,
# a Map with a deleted entry a live iterator still points at ...), and changes them every second.
# Every second it also logs a digest of all of them
# (their full contents in iteration order, which objects are the same object, their prototypes)
# and the result of checks on them (ex. `reg instanceof Registry`, `cyc.get("self") === cyc`).
# Two of them also have Map keys / Set values that can't be saved (Symbols, BigInts, functions),
# which must be left out (with a warning when saving) rather than restored as colliding keys.
#
# Run A plays the game, saves at the save tick and keeps going, and run B loads that save.
# Run C loads the same save from its legacy files only
# (scriptstate.json, whose globals are sorted by name, so some references come before their objects).
# Run R is run A with an in-process rewind instead
# (snapshot at the save tick, restored 3 s later onto the live script instances).
# After the save (or the snapshot), all runs must log the same digests, and every check must pass.
# The save is then loaded and saved again straight away (doublesave.sh): the two documents must be identical.
#
# usage: builtins.sh <warzone2100 binary> [<save tick> <end tick>]
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 300 seconds).
# Exit status: 0 passed, 1 failed, 2 a run failed.
set -u
BIN=${1:?binary}
SAVETICK=${2:-10000}
ENDTICK=${3:-20000}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-300}
CFG="$OUT/config"
MOD="$OUT/builtins-mod"
SAVENAME=builtins
LEGACYNAME=${SAVENAME}_legacy
rm -rf "$MOD" "$CFG/savegames/skirmish/$SAVENAME" "$CFG/savegames/skirmish/$SAVENAME.gam" "$CFG/savegames/skirmish/$LEGACYNAME"
mkdir -p "$MOD/multiplay/skirmish" "$MOD/tests" "$CFG/mods/global"

cat > "$MOD/tests/builtins.json" <<'EOF'
{
	"challenge": {
		"bases": 1,
		"difficulty": "Easy",
		"map": "Sk-HighGround",
		"maxPlayers": 2,
		"powerLevel": 1,
		"scavengers": "false",
		"version": 2
	},
	"player_0": { "team": 0, "ai": "multiplay/skirmish/builtins.js" },
	"player_1": { "difficulty": "Medium", "team": 1, "ai": "multiplay/skirmish/builtins.js" }
}
EOF
cat > "$MOD/multiplay/skirmish/builtins.json" <<'EOF'
{ "AI": { "js": "builtins.js", "name": "BuiltinsTest", "tip": "builtins.sh test AI (does nothing but keep Map / Set / Date globals)" } }
EOF
cat > "$MOD/multiplay/skirmish/builtins.js" <<'EOF'
// builtins.sh: Map / Set / Date script globals

class Registry extends Map { lookup(k) { return this.has(k) ? this.get(k) : "none"; } }
class Stamp extends Date { label() { return "T" + this.getTime(); } }
class Bag extends Set { total() { let t = 0; for (const x of this) t += x; return t; } }
class Inventory
{
	constructor() { this.items = new Map(); this.tags = new Set(); }
	count(kind) { return this.items.has(kind) ? this.items.get(kind) : 0; }
}

var n;          // tick counter
var shared;     // a plain object referenced from several globals
var objKey;     // a plain object used as a Map key and a Set value
var m;          // a Map with mixed keys and values
var s;          // a Set
var d;          // a Date
var dInvalid;   // an invalid Date
var dates;      // an array holding the same Date twice
var reg;        // a Map subclass instance
var stamped;    // a Date subclass instance
var nested;     // a plain object whose Map is also a value in m
var cyc;        // a Map that contains itself (as a key and as a value)
var bigm;       // a large Map with most of its entries deleted
var meta;       // a Set with an attached own property
var emptyMap;   // an empty Map
var bag;        // a Set subclass instance
var inv;        // a class instance holding a Map and a Set
var depots;     // a Map whose value is a class instance (only referenced from there) holding a Map and a Set
var zm;         // a Map with a deleted entry that a live iterator still points at (a "zombie" record) when saved
var holdIter = function () {}; // a function (so never saved) whose closure keeps zm's iterator alive
var lossy;      // a Map with some keys that can't be saved (Symbols, a BigInt, a function and a class)
var lossySet;   // a Set with some values that can't be saved
// The same objects as d, s, the Map nested in m, reg, stamped and bag, under names that sort before theirs
// (so they are forward references in the legacy scriptstate.json, which sorts the globals)
var aliasOfD;
var aliasOfS;
var aliasOfInner;
var aliasOfReg;
var aliasOfStamped;
var aliasOfBag;

function setUp()
{
	n = 0;
	shared = { tag: "shared" };
	objKey = { id: 7 };
	m = new Map();
	m.set("a", 1).set(objKey, shared).set(-0, "zero").set(NaN, "nan").set(3, [1, 2, shared]);
	m.set("inner", new Map([["x", new Set([1, 2])], ["when", new Date(86400000)]]));
	m.delete("a");
	m.set("a", 2); // now after "inner"
	m.set(objKey, "overwritten keeps its position");
	m.set("shared", shared);
	m.set("undef", undefined).set("null", null).set("inf", -Infinity);
	s = new Set(["one", 2, objKey, -0, NaN, shared]);
	s.delete(2);
	s.add(2);
	d = new Date(1700000000123);
	dInvalid = new Date(NaN);
	dates = [d, d, new Date(-1)];
	reg = new Registry([["k1", "v1"], [objKey, d]]);
	stamped = new Stamp(1234);
	nested = { inner: m.get("inner") };
	cyc = new Map();
	cyc.set("self", cyc);
	cyc.set(cyc, "selfkey");
	bigm = new Map();
	for (let i = 0; i < 3000; i++) bigm.set("k" + i, i);
	for (let i = 0; i < 3000; i++) if (i % 100) bigm.delete("k" + i);
	meta = new Set([1]);
	meta.note = { text: "attached", to: meta };
	emptyMap = new Map();
	bag = new Bag([1, 2, 3]);
	inv = new Inventory();
	inv.items.set("tank", 2);
	inv.tags.add("armored");
	depots = new Map([["north", new Inventory()]]);
	depots.get("north").items.set("rock", 0);
	zm = new Map([["a", 1], ["b", 2], ["c", 3]]);
	// `it` points at "a"'s record, which stays allocated (empty) after the delete while `it` is alive
	const it = zm.keys();
	it.next();
	zm.delete("a");
	holdIter = () => it;
	lossy = new Map([["keep1", 1], [Symbol("a"), 2], [Symbol("b"), 3], [setUp, 4], [Registry, 5], [10n, 6], ["keep2", 7]]);
	lossySet = new Set(["v", Symbol("s"), 5n, () => 1, "w"]);
	aliasOfD = d;
	aliasOfS = s;
	aliasOfInner = m.get("inner");
	aliasOfReg = reg;
	aliasOfStamped = stamped;
	aliasOfBag = bag;
}

// Changes every global a bit, deterministically
function mutate()
{
	n++;
	const firstKey = m.keys().next().value; // moved to the end, with its value
	const firstValue = m.get(firstKey);
	m.delete(firstKey);
	m.set(firstKey, firstValue);
	m.set("n" + n, n);
	m.set(objKey, n); // keeps its position
	m.get("inner").get("x").add(n);
	s.delete((n + 1) % 4);
	s.add(n % 4);
	d.setTime(d.getTime() + 1000);
	reg.set("r" + (n % 3), n);
	stamped.setTime(stamped.getTime() + 1);
	bigm.delete("k" + (n * 100));
	bigm.set("new" + n, n);
	meta.add(n);
	bag.add(10 + n);
	inv.items.set("tank", inv.count("tank") + 1);
	depots.get("north").items.set("rock", n);
}

// Map keys / Set values the save format can't recreate (they are left out when saving)
function saveable(k)
{
	return typeof k !== "symbol" && typeof k !== "bigint" && typeof k !== "function";
}

// A canonical description of v: contents in iteration order, prototypes, and identity (#<id> on revisits)
function describe(v, ids)
{
	if (v === null) return "null";
	switch (typeof v)
	{
		case "undefined": return "undef";
		case "number": return Object.is(v, -0) ? "-0" : String(v);
		case "string": return JSON.stringify(v);
		case "boolean": return String(v);
		case "object": break;
		default: return typeof v;
	}
	if (ids.has(v)) return "#" + ids.get(v);
	const id = ids.size;
	ids.set(v, id);
	const proto = Object.getPrototypeOf(v);
	const pname = (proto === null) ? "null" : (proto.constructor ? proto.constructor.name : "?");
	let body = "";
	if (v instanceof Map) body = [...v].filter((e) => saveable(e[0])).map((e) => describe(e[0], ids) + "=>" + describe(e[1], ids)).join(",");
	else if (v instanceof Set) body = [...v].filter(saveable).map((x) => describe(x, ids)).join(",");
	else if (v instanceof Date) body = String(v.getTime());
	else if (Array.isArray(v)) body = v.map((x) => describe(x, ids)).join(",");
	const own = Object.keys(v).filter((k) => !Array.isArray(v) || !/^\d+$/.test(k)).map((k) => k + ":" + describe(v[k], ids)).join(",");
	return pname + "#" + id + "(" + body + "|" + own + ")";
}

function hash(str)
{
	let h = 0x811c9dc5;
	for (let i = 0; i < str.length; i++) h = Math.imul(h ^ str.charCodeAt(i), 16777619) >>> 0;
	return ("0000000" + h.toString(16)).slice(-8);
}

function checks()
{
	const failed = [];
	const c = (name, ok) => { if (!ok) failed.push(name); };
	c("types", m instanceof Map && s instanceof Set && d instanceof Date && bigm instanceof Map && emptyMap instanceof Map);
	c("subclass", reg instanceof Registry && reg instanceof Map && reg.lookup("k1") === "v1" && reg.lookup("zz") === "none");
	c("datesubclass", stamped instanceof Stamp && stamped instanceof Date && stamped.label() === "T" + (1234 + n));
	c("objkey", m.get(objKey) === (n ? n : "overwritten keeps its position") && reg.get(objKey) === d && s.has(objKey));
	c("zero", m.has(0) && m.get(-0) === "zero" && [...m.keys()].every((k) => !Object.is(k, -0)) && [...s].every((x) => !Object.is(x, -0)));
	c("nan", m.get(NaN) === "nan" && s.has(NaN));
	c("shared", m.get("shared") === shared && m.get(3)[2] === shared && s.has(shared));
	c("nested", nested.inner === m.get("inner") && m.get("inner").get("when").getTime() === 86400000);
	c("dates", dates[0] === dates[1] && dates[0] === d && dates[2].getTime() === -1 && d.getTime() === 1700000000123 + 1000 * n);
	c("invalid", isNaN(dInvalid.getTime()));
	c("cycle", cyc.get("self") === cyc && cyc.get(cyc) === "selfkey" && cyc.size === 2);
	c("undefnull", m.has("undef") && m.get("undef") === undefined && m.get("null") === null && m.get("inf") === -Infinity);
	c("big", bigm.size === 30 + n - Math.min(n, 29) && bigm.get("k0") === 0);
	c("meta", meta.note.text === "attached" && meta.note.to === meta);
	c("empty", emptyMap.size === 0);
	c("aliases", aliasOfD === d && aliasOfS === s && aliasOfInner === m.get("inner") && aliasOfInner === nested.inner);
	c("subaliases", aliasOfReg === reg && aliasOfStamped === stamped && aliasOfBag === bag);
	c("setsubclass", bag instanceof Bag && bag instanceof Set && bag.total() === 6 + 10 * n + n * (n + 1) / 2);
	c("classholds", inv instanceof Inventory && inv.items instanceof Map && inv.tags instanceof Set && inv.tags.has("armored")
		&& inv.count("tank") === 2 + n && depots.get("north") instanceof Inventory && depots.get("north").count("rock") === n);
	c("zombie", zm.size === 2 && [...zm.keys()].join() === "b,c" && zm.get("b") === 2);
	// All keys / values before a restore, only the saveable ones after (never merged ones like null, "10" or "5")
	c("lossy", (lossy.size === 7 || lossy.size === 2) && lossy.get("keep1") === 1 && lossy.get("keep2") === 7
		&& !lossy.has(null) && !lossy.has("10") && !lossy.has(undefined)
		&& (lossySet.size === 5 || lossySet.size === 2) && lossySet.has("v") && lossySet.has("w")
		&& !lossySet.has(null) && !lossySet.has("5") && !lossySet.has(undefined));
	return failed.length ? "FAILED:" + failed.join(",") : "ok";
}

function digest()
{
	const ids = new Map();
	const all = [n, shared, objKey, m, s, d, dInvalid, dates, reg, stamped, nested, cyc, bigm, meta, emptyMap, bag, inv, depots, zm,
		lossy, lossySet, aliasOfD, aliasOfS, aliasOfInner, aliasOfReg, aliasOfStamped, aliasOfBag];
	const str = all.map((v) => describe(v, ids)).join(";");
	return hash(str) + " len=" + str.length + " checks=" + checks();
}

function tick()
{
	mutate();
	debug("builtins: me=" + me + " t=" + gameTime + " " + digest());
	debug("builtins: me=" + me + " lossy=" + lossy.size + "/" + lossySet.size + " at t=" + gameTime);
}

function eventStartLevel()
{
	setUp();
	debug("builtins: me=" + me + " start " + digest());
	setTimer("tick", 1000);
}

function eventGameLoaded()
{
	debug("builtins: me=" + me + " loaded t=" + gameTime + " " + digest());
}
EOF
python3 -I - "$MOD" "$CFG/mods/global/builtins.wz" <<'EOF'
import os, sys, zipfile
root, dest = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(dest, "w") as z:
    for d, _, files in os.walk(root):
        for f in files:
            z.write(os.path.join(d, f), os.path.relpath(os.path.join(d, f), root))
EOF

COMMON="--configdir=$CFG --headless --autogame --mod=builtins.wz --gamestate-test-seed=1 --gamestate-exit-at-tick=$ENDTICK"
timeout "$RUN_TIMEOUT" "$BIN" $COMMON --skirmish=builtins.json --debug=script \
	--gamestate-save-at-tick="$SAVETICK:savegames/skirmish/$SAVENAME.gam" > "$OUT/builtins.A.log" 2>&1
[ -f "$CFG/savegames/skirmish/$SAVENAME/gamestate.wz" ] || { echo "builtins: RUN A FAILED (no save; see $OUT/builtins.A.log)"; exit 2; }
timeout "$RUN_TIMEOUT" "$BIN" $COMMON --loadskirmish="$SAVENAME" > "$OUT/builtins.B.log" 2>&1
cp -r "$CFG/savegames/skirmish/$SAVENAME" "$CFG/savegames/skirmish/$LEGACYNAME"
rm "$CFG/savegames/skirmish/$LEGACYNAME/gamestate.wz"
timeout "$RUN_TIMEOUT" "$BIN" $COMMON --loadskirmish="$LEGACYNAME" > "$OUT/builtins.C.log" 2>&1
timeout "$RUN_TIMEOUT" "$BIN" $COMMON --skirmish=builtins.json --gamestate-rewind-test="$SAVETICK:3000" > "$OUT/builtins.R.log" 2>&1
grep -q "rewound from gameTime" "$OUT/builtins.R.log" || { echo "builtins: RUN R: THE REWIND RESTORE DID NOT HAPPEN (see $OUT/builtins.R.log)"; exit 2; }

# The save is written at the first tick at or after the save tick (between two ticks)
SAVEDAT=$(sed -n 's/^.*GameState harness: wrote save .* at gameTime \([0-9]*\).*$/\1/p' "$OUT/builtins.A.log" | head -1)
[ -n "$SAVEDAT" ] || { echo "builtins: RUN A DID NOT LOG ITS SAVE (see $OUT/builtins.A.log)"; exit 2; }

# digests <log>: the digest lines logged after the save, in gameTime order
# (a rewound run logs the ticks between the snapshot and the restore twice, and the last one is kept)
digests() {
	sed -n 's/^.*builtins: \(me=[0-9]* t=\([0-9]*\) .*\)$/\2 \1/p' "$1" \
		| awk -v s="$SAVEDAT" '$1 > s { t = $1; $1 = ""; if (!(t in d)) o[n++] = t; d[t] = $0 } END { for (i = 0; i < n; i++) print d[o[i]] }'
}
STATUS=0
# A script error (ex. a method missing from a global that was not restored as a Map) fails the test
for RUN in A B C R; do
	NEXC=$(grep -c 'Uncaught exception' "$OUT/builtins.$RUN.log")
	if [ "$NEXC" -gt 0 ]; then
		echo "builtins: run $RUN: $NEXC UNCAUGHT SCRIPT EXCEPTION(S); first:"
		grep -A1 'Uncaught exception' "$OUT/builtins.$RUN.log" | head -2 | sed 's/^.*\] /  /'
		STATUS=1
	fi
done
digests "$OUT/builtins.A.log" > "$OUT/builtins.A.digests"
NA=$(wc -l < "$OUT/builtins.A.digests")
for RUN in B C R; do
	digests "$OUT/builtins.$RUN.log" > "$OUT/builtins.$RUN.digests"
	N=$(wc -l < "$OUT/builtins.$RUN.digests")
	if [ "$N" -eq 0 ]; then
		echo "builtins: RUN $RUN LOGGED NO DIGESTS (see $OUT/builtins.$RUN.log)"
		[ "$STATUS" -eq 0 ] && STATUS=2
	elif cmp -s "$OUT/builtins.A.digests" "$OUT/builtins.$RUN.digests"; then
		echo "builtins: run A and run $RUN logged the same $N digests after the save (at gameTime $SAVEDAT): IDENTICAL"
	else
		echo "builtins: DIGESTS DIFFER between run A and run $RUN after the save (at gameTime $SAVEDAT; A: $NA, $RUN: $N lines); first difference:"
		diff "$OUT/builtins.A.digests" "$OUT/builtins.$RUN.digests" | head -4 | sed 's/^/  /'
		STATUS=1
	fi
done
[ "$STATUS" -eq 2 ] && exit 2
# Keys / values that can't be saved are left out with a warning when saving,
# so the runs that restored the save keep only the others (2 / 2), and run A keeps them all (7 / 5)
LOSSY_STATUS=0
for W in "left out 5 entries whose keys cannot be saved (1 BigInt, 2 Symbol, 2 function)" \
	"left out 3 values that cannot be saved (1 BigInt, 1 Symbol, 1 function)"; do
	if ! grep -qF "$W" "$OUT/builtins.A.log"; then
		echo "builtins: run A DID NOT WARN: $W"
		LOSSY_STATUS=1
	fi
done
for RUN in A B C R; do
	EXPECT="2/2"
	[ "$RUN" = A ] && EXPECT="7/5"
	GOT=$(sed -n 's/^.*builtins: me=[0-9]* lossy=\([0-9]*\/[0-9]*\) .*$/\1/p' "$OUT/builtins.$RUN.log" | tail -1)
	if [ "$GOT" != "$EXPECT" ]; then
		echo "builtins: run $RUN: Map / Set sizes with unsaveable keys are \"$GOT\", expected $EXPECT"
		LOSSY_STATUS=1
	fi
done
if [ "$LOSSY_STATUS" -eq 0 ]; then
	echo "builtins: unsaveable Map keys / Set values were left out with a warning (sizes 7/5 -> 2/2 after a restore)"
else
	STATUS=1
fi

LOGS="$OUT/builtins.A.log $OUT/builtins.B.log $OUT/builtins.C.log $OUT/builtins.R.log"
FAILED=$(cat $LOGS | grep 'builtins: ' | grep -c 'checks=FAILED')
TOTAL=$(cat $LOGS | grep -c 'builtins: .*checks=')
if [ "$FAILED" -gt 0 ]; then
	echo "builtins: $FAILED of $TOTAL digests have failed checks; first:"
	cat $LOGS | grep 'checks=FAILED' | head -1 | sed 's/^.*builtins: /  /'
	STATUS=1
else
	echo "builtins: all checks passed in $TOTAL digests"
fi

OUT="$OUT" EXTRA="--mod=builtins.wz" "$HERE/doublesave.sh" "$BIN" "$SAVENAME" || STATUS=1
exit $STATUS
