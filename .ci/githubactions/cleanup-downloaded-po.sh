#!/bin/sh

# Expects input of $1 base file and $2 .po file

BASE_POT="$1"
TARGET_PO="$2"

# 1.) Perform msgmerge with the base .pot, with similar settings to what WZ uses (wrapping, etc), but also removing location lines
# 2.) Use sed to remove "#." extracted comment lines (these are in the .pot, we don't need to store them in every .po file in the repo)
# 3.) Cleanup .bak file
msgmerge --no-wrap --width=1 --no-location "--output-file=$TARGET_PO" "$TARGET_PO" "$BASE_POT" && sed -i.bak '/^#\./d' "$TARGET_PO" && rm "$TARGET_PO.bak"
