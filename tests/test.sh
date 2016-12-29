#!/bin/bash

rm -rf tmp
mkdir -p tmp

trap ctrl_c INT

function ctrl_c() {
	echo " * Caught ctrl+c - aborting!"
	exit 1
}

function run
{
	echo
	echo " -- $2 --"
	echo
	gdb -q --ex run --ex quit --args src/warzone2100 --window --configdir=tmp --resolution=1024x768 --shadows --sound --texturecompression $1
}

function cam
{
	echo
	echo " ==== $2 ===="
	run "--game=$1 --saveandquit=savegames/campaign/$1.gam" "Initial run"
	run "--loadcampaign=$1 --saveandquit=savegames/campaign/$1-loadsave.gam" "Loadsave run"
}

echo
echo "Running Warzone2100 automated tests"
echo -n "Time is: "
date -R

cam CAM_1A "Alpha campaign"
cam CAM_2A "Beta campaign"
cam CAM_3A "Gamma campaign"
cam TUTORIAL3 "Tutorial"
cam FASTPLAY "Fastplay"
