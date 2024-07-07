#!/bin/bash

## Game configuration values
## One file for each game type

## Warzone2100 configuration directory
## Leave commented to use the default one
## Required to randomize maps
## Snap default
#cfgdir="$HOME/snap/warzone2100/common/warzone2100"
## Flatpak default
#cfgdir="$HOME/.var/app/net.wz2100.wz2100/data/warzone2100/"
## Home default
#cfgdir="$HOME/.local/share/warzone2100"

## Map pool
## Leave commented to skip randomized maps or provide a list of map names.
## The autohost file will be updated each time with one of these values
#maps=("map1" "map2" "map3")

## Host file
## The name of the file in your 'autohost' directory inside cfgdir.
hostfile="my_game.json"

## Players
## The number of players required to start the game.
players=2


## Run script
## Do not edit below
DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi
if [ ! -f "$DIR/common.sh" ]; then
  echo "[ERROR] common.sh not found."
  exit 1
fi
if [ ! -f "$DIR/config.sh" ]; then
  echo "[ERROR] config.sh not found."
  echo "Copy 'config_sample.sh' to 'config.sh' and check if the file is readable."
  exit 1
fi
. "$DIR/common.sh"
. "$DIR/config.sh"
run_host
