#!/bin/bash

function check {
  if [ ! $(command -v netstat) ] && [ ! $(command -v ss) ]; then
    echo "[ERROR] netstat or ss is required to check for available ports"
    exit 1
  fi
  if [ ! -v wz2100cmd ]; then
    echo "[ERROR] wz2100cmd variable is not set."
    exit 1
  fi
  if [ "$players" == "" ] || [ ! $players -gt 0 ]; then
    echo "[ERROR] The number of players is not set."
    exit 1
  fi
  if [ "$hostfile" == "" ]; then
    echo "[ERROR] No hostfile given."
    exit 1
  fi
  if [ "$portmin" == "" ] || [ ! $portmin -gt 0 ]; then
    portmin=2100
  fi
  if [ "$portmax" == "" ] || [ $portmax -lt $portmin ]; then
    portmax=$portmin
  fi
}

function run_host {
  check
  hostpath="$cfgdir/autohost"
  # Find next available port
  port=$((portmin - 1))
  while true; do
    port=$((port + 1))
    if [ $(command -v netstat) ]; then
      ports="$(netstat -nt | grep [0-9]*\.[0-9]*\.[0-9]*\.[0-9]*:${port})"
    else
      ports="$(ss -ntO | grep [0-9]*\.[0-9]*\.[0-9]*\.[0-9]*:${port})"
    fi
    if [ "$ports" == "" ] || [ "$port" -gt "$portmax" ]; then
      break
    fi
  done

  if [ "$port" -gt "$portmax" ]; then
    echo "No more port available"
    return 1
  fi
  # Set random map
  if [ ${#maps[@]} -gt 0 ]; then
    if [ "$cfgdir" == "" ]; then
      echo "[ERROR] cfgdir must be set to edit autohost file"
      return 1
    fi
    if [ ! -f "$hostpath/$hostfile" ]; then
      echo "[ERROR] Host file \"$hostpath/$hostfile\" not found."
      return 1
    fi
    rnd=$(($RANDOM % ${#maps[@]}))
    map=${maps[$rnd]}
    sed -i "s/\"map\": \"\(.*\)\",$/\"map\": \"$map\",/g" $hostpath/$hostfile
  fi
  # Set admin list
  admcmd=""
  for adm in ${adminkeys[@]}
  do
     admcmd+=" --addlobbyadminpublickey=${adm}"
  done
  for adm in ${adminhashes[@]}
  do
     admcmd+=" --addlobbyadminhash=${adm}"
  done
  if [ "$cfgdir" == "" ]; then
    configdir=""
  else
    configdir="--configdir=$cfgdir"
  fi
  # Run game
  exec $wz2100cmd $configdir --autohost=$hostfile --gameport=$port --startplayers=$players --enablelobbyslashcmd $admcmd --headless --nosound
}
