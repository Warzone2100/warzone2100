#!/bin/sh
sed -e 's/r\([0-9]\+\)/[rev]\1[\/rev]/g' -e 's/ticket:\([0-9]\+\)/[ticket]\1[\/ticket]/g' -e 's/\ \ \*/\ */g' -e 's/^\ \*\ \(.*\):/\n[b]\1[\/b]/g'
