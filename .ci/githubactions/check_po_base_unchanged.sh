#!/bin/sh

# Expects input of $1 base file and $2 .po file

diff --ignore-trailing-space --ignore-blank-lines \
     --ignore-matching-lines='^#' \
     --ignore-matching-lines='^".*\:.*"' \
     "$1" "$2" >/dev/null 2>&1

result=$?
if [ $result -eq 0 ]; then
  printf '%s\0' "$2"
fi
  
exit 0
