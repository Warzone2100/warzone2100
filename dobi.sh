#!/usr/bin/env bash
DEBUG=true
set -eo pipefail
test ":$DEBUG" != :true || set -x


sed \
	-e 's!%%WORKDIR%%!'"$(pwd)"'!g' \
	dobi.template \
	> "dobi.yaml"

# set default commands
set -- binary

# set image
set -- dnephin/dobi:0.11.1 "$@"

# use current user and its groups at host
for v in /etc/group /etc/passwd; do
  [ ! -r "$v" ] || set -- -v $v:$v:ro "$@"
done
set -- --user "`id -u`:`id -g`" "$@"
for g in `id -G`; do
  set -- --group-add "$g" "$@"
done
set -- -w "$(pwd)" -v "$(pwd)":"$(pwd)" -e DOCKER_HOST -v /var/run/docker.sock:/var/run/docker.sock "$@"

exec docker run --rm -it "$@"