#!/bin/sh

# Move an existing config directory to $SNAP_USER_COMMON
# (which is shared across revisions of the snap).
if [ ! -d "$SNAP_USER_COMMON/warzone2100" ]; then
  if [ -d "$SNAP_USER_DATA/.local/share/warzone2100" ]; then
    mv "$SNAP_USER_DATA/.local/share/warzone2100" "$SNAP_USER_COMMON/"
  fi
fi

exec env "XDG_DATA_HOME=$SNAP_USER_COMMON" env "XDG_CONFIG_HOME=$SNAP_USER_COMMON" "$@"
