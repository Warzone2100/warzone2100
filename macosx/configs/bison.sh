#!/bin/bash

# System Bison executable
SYS_BISON="bison"
# Local Warzone Bison executable (relative to macosx/)
LOCAL_BISON="external/bison/built/bin/bison" 

# If a Warzone Bison executable exists, use instead of the system executable
if [ -e "$LOCAL_BISON" ]; then
	${LOCAL_BISON} ${@}
    exit ${?}
else
	${SYS_BISON} ${@}
    exit ${?}
fi
