#!/usr/bin/env python3
# Compares the GameState documents of two new-format savegame folders (their gamestate.wz archives).
#
# Usage: cmpsave.py <savegame folder A> <savegame folder B> [--all]
#
# Compares the "gameState" document section by section and prints, for each differing section,
# the first differing JSON path (with both values, shortened).
# The disk-only "localState" and "pendingResume" parts are compared too,
# and "setup" is reported but not counted (it carries the save name).
# --all lists up to 10 differing paths per section instead of the first one.
# Exit status: 0 identical, 1 differs, 2 bad input.
import json
import os
import sys
import zipfile


def load(folder):
    with zipfile.ZipFile(os.path.join(folder, 'gamestate.wz')) as z:
        return json.loads(z.read('gamestate.json'))


def short(v):
    s = json.dumps(v, sort_keys=False)
    return s if len(s) <= 120 else s[:117] + '...'


def diffs(a, b, path, out, limit):
    if len(out) >= limit:
        return
    if type(a) is not type(b):
        out.append((path, a, b))
    elif isinstance(a, dict):
        for k in list(a.keys()) + [k for k in b.keys() if k not in a]:
            if k not in a or k not in b:
                out.append((path + '/' + k, a.get(k, '<missing>'), b.get(k, '<missing>')))
            else:
                diffs(a[k], b[k], path + '/' + k, out, limit)
            if len(out) >= limit:
                return
        if list(a.keys()) != list(b.keys()) and len(out) < limit and set(a) == set(b):
            out.append((path + ' (key order)', list(a.keys())[:5], list(b.keys())[:5]))
    elif isinstance(a, list):
        if len(a) != len(b):
            out.append((path + ' (length %d vs %d)' % (len(a), len(b)), None, None))
        for i, (x, y) in enumerate(zip(a, b)):
            diffs(x, y, '%s[%d]' % (path, i), out, limit)
            if len(out) >= limit:
                return
    elif a != b:
        out.append((path, a, b))


def main(argv):
    args = argv[1:]
    limit = 1
    if '--all' in args:
        args.remove('--all')
        limit = 10
    if len(args) != 2:
        print('usage: cmpsave.py <savegame folder A> <savegame folder B> [--all]', file=sys.stderr)
        return 2
    a, b = load(args[0]), load(args[1])
    differing = 0
    sections = list(a['gameState'].keys()) + [k for k in b['gameState'].keys() if k not in a['gameState']]
    parts = [('gameState', k) for k in sections] + [('localState', None), ('pendingResume', None), ('setup', None)]
    for part, key in parts:
        x = a.get(part, {}) if key is None else a[part].get(key, '<missing>')
        y = b.get(part, {}) if key is None else b[part].get(key, '<missing>')
        name = part if key is None else key
        if x == y:
            continue
        out = []
        diffs(x, y, name, out, limit)
        if part != 'setup':
            differing += 1
        print('DIFFERS %s%s' % (name, ' (not counted)' if part == 'setup' else ''))
        for p, u, v in out:
            print('    %s: %s -> %s' % (p, short(u), short(v)) if u is not None else '    %s' % p)
    print('IDENTICAL' if differing == 0 else '%d section(s) differ' % differing)
    return 1 if differing else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
