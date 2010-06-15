#!/bin/sh
sed -e 's/r\([0-9]\+\)/[rev]\1[\/rev]/g' \
    -e 's/ticket:\([0-9]\+\)/[ticket]\1[\/ticket]/g'\
    -e 's/\ \ \*/\ */g' \
    -e 's/^\ \*\ \(.*\):/\n[\/list][b]\1[\/b][list]/g' \
    -e 's/^\ \ \*/\ [*]/'
