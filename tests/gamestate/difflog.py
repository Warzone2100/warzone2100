#!/usr/bin/env python3
# Diffs two sync-debug detail logs (<trace>.detail.txt from --gamestate-crc-detail-tick / -on-save),
# block by block for the gameTimes both contain,
# and prints the first differing lines of each block whose CRC differs.
#
# Usage: difflog.py <reference detail log> <test detail log> [context lines]
import difflib
import os
import re
import sys


def blocks(path):
    result = {}
    current = None
    with open(path, errors='replace') as f:
        for line in f:
            m = re.match(r'===== BEGIN gameTime=(\d+), \d+ entries, CRC (\S+)', line)
            if m:
                current = int(m.group(1))
                result[current] = [m.group(2)]
                continue
            if line.startswith('===== END'):
                current = None
                continue
            if current is not None:
                result[current].append(line.rstrip('\n'))
    return result


def main(argv):
    if len(argv) < 3:
        print('usage: difflog.py <reference> <test> [context]', file=sys.stderr)
        return 2
    context = int(argv[3]) if len(argv) > 3 else 2
    for path in argv[1:3]:
        if not os.path.exists(path):
            print('no detail log %s (the run did not reach the requested tick?)' % path)
            return 2
    ref = blocks(argv[1])
    test = blocks(argv[2])
    differing = 0
    for t in sorted(set(ref) & set(test)):
        # Skip blocks whose CRCs match, which are identical as far as the sync check is concerned
        # (ex. a resumed run only has the tail of its first block, but its CRC was re-seeded from the save).
        if ref[t][0] == test[t][0]:
            continue
        differing += 1
        print('==== gameTime %d: %d vs %d entries' % (t, len(ref[t]) - 1, len(test[t]) - 1))
        for line in list(difflib.unified_diff(ref[t][1:], test[t][1:], lineterm='', n=context))[2:40]:
            print(line[:300])
    if differing == 0:
        print('detail logs identical on %d common gameTime(s)' % len(set(ref) & set(test)))
    return 1 if differing else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
