#!/bin/sh

# Output the filenames that have actual modifications, ignoring certain .po file header lines
# Note: Requires GNU Diffutils
diff --brief --recursive --unified=1 --new-file \
     --ignore-matching-lines='^"POT-Creation-Date: .*"' \
     --ignore-matching-lines='^"PO-Revision-Date: .*"' \
     --ignore-matching-lines='^"Last-Translator: .*"' \
     "$@" \
  | cut -d' ' -f 2 | sed "s#$1*##"

exit 0
