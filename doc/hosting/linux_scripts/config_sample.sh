#!/bin/bash

## Common configuration values
## This file is shared between all games

## Administrators
#################
## adminhashes: the list of admin players' hash
## adminkeys: the list of admin players' public key
## The host is always administrator
## Put keys and/or hashes according to your likings and between quotes.
adminkeys=(
# 'admin1 key'
# 'admin2 key'
)
adminhashes=(
# 'admin3 hash'
# 'admin4 hash'
)

## Ports
########
## portmin: the lowest port to use if available
## portmax: the highest port to use
## Set a range of ports slightly larger than the maximum number of simultaneous
## games in case a game crashes and cannot release the port properly before the
## next game is created.
portmin=2100
portmax=2105

## Game command
###############
## Uncomment or modifiy the command to run Warzone 2100.
## The script will add all the required parameters to run the game.

## Installed from snap
#wz2100cmd="snap run warzone2100"

## Installed from flatpack
#wz2100cmd="flatpak run net.wz2100.wz2100"

## Installed in PATH
#wz2100cmd="warzone2100"

## Built from source
#wz2100cmd="/<path>/<to>/<warzone2100>/bin/warzone2100"
