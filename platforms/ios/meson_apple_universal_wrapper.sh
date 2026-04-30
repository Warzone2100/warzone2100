#!/bin/sh
set -eu

if [ "$#" -lt 1 ]; then
  echo "usage: $0 <compiler> [args...]" >&2
  exit 2
fi

real_compiler="$1"
shift

needs_single_arch=0
for arg in "$@"; do
  if [ "$arg" = "-E" ]; then
    needs_single_arch=1
    break
  fi
done

if [ "$needs_single_arch" -eq 0 ]; then
  exec "$real_compiler" "$@"
fi

set -- "$@"
filtered_args=""
arch_seen=0

while [ "$#" -gt 0 ]; do
  if [ "$1" = "-arch" ] && [ "$#" -ge 2 ]; then
    if [ "$arch_seen" -eq 0 ]; then
      filtered_args="${filtered_args}
$1
$2"
      arch_seen=1
    fi
    shift 2
    continue
  fi

  filtered_args="${filtered_args}
$1"
  shift
done

set --
old_ifs=$IFS
IFS='
'
for arg in $filtered_args; do
  [ -n "$arg" ] && set -- "$@" "$arg"
done
IFS=$old_ifs

exec "$real_compiler" "$@"
