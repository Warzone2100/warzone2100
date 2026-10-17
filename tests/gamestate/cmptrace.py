#!/usr/bin/env python3
# Compares two per-tick sync-CRC traces (--gamestate-crc-trace) on the ticks both contain.
#
# Usage: cmptrace.py <reference trace> <test trace> [--after <tick>] [--until <tick>] [--last]
#        cmptrace.py --replayed <trace>
#
# --after <tick>  only compare ticks greater than <tick> (ex. the save / resume tick)
# --until <tick>  only compare ticks up to and including <tick>
# --last          if a trace contains a tick more than once (a rewind), use its last occurrence
# --list          also print every differing tick (up to 50)
# --replayed      compare the ticks a rewound run simulated twice: the first time against the last time
#
# A tick whose CRC differs is echoed in the GAME_GAME_TIME message a few ticks later,
# so a single difference repeats at a fixed stride for the rest of the run.
# When the differing ticks after the first one all fall on that echo chain,
# the result says so ("echo chain only").
# Otherwise it names the first tick that is not an echo,
# which is where the simulation itself (rather than the echoed CRC) parts.
#
# Prints one line: IDENTICAL (<n> ticks), DIFFERS at tick <t> (<n> ticks compared), or NO OVERLAP.
# Exit status: 0 identical, 1 differs, 2 no overlap / bad input.
import sys


def load(path, last):
    crcs = {}
    with open(path) as f:
        for line in f:
            parts = line.split()
            if len(parts) != 2:
                continue
            tick, crc = int(parts[0]), int(parts[1])
            if last or tick not in crcs:
                crcs[tick] = crc
    return crcs


def replayed_ticks(path):
    seen = set()
    twice = set()
    with open(path) as f:
        for line in f:
            parts = line.split()
            if len(parts) != 2:
                continue
            tick = int(parts[0])
            (twice if tick in seen else seen).add(tick)
    return twice


def main(argv):
    args = [a for a in argv[1:]]
    after = -1
    until = None
    replayed = False
    if '--replayed' in args:
        args.remove('--replayed')
        replayed = True
    if '--until' in args:
        i = args.index('--until')
        until = int(args[i + 1])
        del args[i:i + 2]
    last = False
    show_all = False
    if '--list' in args:
        args.remove('--list')
        show_all = True
    if '--last' in args:
        args.remove('--last')
        last = True
    if '--after' in args:
        i = args.index('--after')
        after = int(args[i + 1])
        del args[i:i + 2]
    if replayed and len(args) == 1:
        ref = load(args[0], False)
        test = load(args[0], True)
        twice = replayed_ticks(args[0])
        common = sorted(t for t in ref if t in twice and t > after and (until is None or t <= until))
    elif len(args) == 2 and not replayed:
        ref = load(args[0], False)
        test = load(args[1], last)
        common = sorted(t for t in ref if t in test and t > after and (until is None or t <= until))
    else:
        print('usage: cmptrace.py <reference> <test> [--after <tick>] [--until <tick>] [--last] | --replayed <trace>', file=sys.stderr)
        return 2
    if not common:
        print('NO OVERLAP')
        return 2
    differing = [t for t in common if ref[t] != test[t]]
    if differing:
        note = ''
        spacing = min((b - a for a, b in zip(common, common[1:])), default=0)
        if len(differing) >= 2 and differing[1] - differing[0] == spacing:
            # Consecutive ticks differ: that is no echo chain (the echo lags by the lockstep latency)
            note = '; consecutive ticks differ from the first one'
        elif len(differing) >= 2:
            stride = differing[1] - differing[0]
            echoes = set()
            first_real = None
            for i, t in enumerate(differing):
                if i > 0 and t not in echoes and first_real is None:
                    first_real = t
                echoes.add(t + stride)
            if first_real is None:
                note = '; echo chain only (stride %d)' % stride
            else:
                note = '; first non-echo difference at tick %d' % first_real
        print('DIFFERS at tick %d (%d of %d ticks differ, first compared tick %d%s)' % (differing[0], len(differing), len(common), common[0], note))
        if show_all:
            print('differing ticks: ' + ' '.join(str(t) for t in differing[:50]))
        return 1
    print('IDENTICAL (%d ticks, %d..%d)' % (len(common), common[0], common[-1]))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
