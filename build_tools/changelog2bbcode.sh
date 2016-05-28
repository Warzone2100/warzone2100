#!/bin/sh
sed -e 's/  \*/ [\*]/g' \
    -e 's/ \* \(.*\):/[\/list][b]\1[\/b][list]/g' \
    -e 's/commit\:\([a-f0-9]\{10\}\)[a-f0-9]*/\[githash\]\1\[\/githash\]/g' \
    -e 's/ticket\:\([a-f0-9]*\)/\[ticket\]\1\[\/ticket\]/g'
