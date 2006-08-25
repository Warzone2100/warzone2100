#!/bin/sh

MYDIR=`dirname "$0"`
cd "$MYDIR"
pwd
exec ./warzone2100 --datadir ../Resources/data

