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

function skirmish
{
	echo
	echo " ==== $1 : $2 ===="
	run "--skirmish=$1.json --autogame" "$1 : Running"
	# TBD: Use the below instead when we have ported player part of savegames to JSON, and can save more AI state. For now,
	# this will crash, because it expects an AI name. We do not want to use AI names for challenge files.
	#run "--skirmish=$1.json --autogame --saveandquit=savegames/skirmish/$1.gam" "$1 : Launching and saving"
	#run "--autogame --loadskirmish=$1" "$1 : Loading and running"
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

skirmish highground "Basic skirmish"
skirmish miza "All AIs"
skirmish miza_challenge "Best AI vs 7 old timers"
