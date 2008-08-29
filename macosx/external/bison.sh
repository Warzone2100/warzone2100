#!/bin/bash

SYS_BISON="bison"                   # System Bison executable
LOCAL_BISON="`pwd`/external/bison/src/bison" # Local Warzone Bison executable

# If a Warzone Bison executable exists, use instead of the system executable
if [ -e $LOCAL_BISON ]; then
	$LOCAL_BISON $*
else
	$SYS_BISON $*
fi
