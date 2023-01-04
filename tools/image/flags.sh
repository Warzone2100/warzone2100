#!/usr/bin/env bash

# This script generates icons of countries flags, that are used for the language selector
# in the game options menu. It downloads the images as SVGs from a github repository,
# converts them into PNGs, and shape them into the format expected by the interface code.

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

WZ_ROOT=$(dirname "$(dirname "$SCRIPT_DIR")")

main() {
	check_dependencies

	temp_dir=$(mktemp -d)
	trap cleanup_temp_dir EXIT

	cd "$temp_dir" || exit 1

	download_images || exit 1

	log 'Rendering images...'
	for flag in BG ES CZ DK DE GR US GB ES EE ES FI FR NL IE HR HU ID IT KR LT LV NO NO NL PL BR PT RO RU SK SI SE TR UZ UA CN TW VA; do
		file="$flag.svg"

		if ! [ -e "$file" ]; then
			log 'Unable to find flag file "%s"' "$file"
			continue
		fi

		rsvg-convert -a -w 1000 -h 1000 "$flag.svg" -o temp.png

		gravity=Center
		# these flags have details on the left side that would be clipped out with gravity=Center
		if grep -q "$flag " <<< 'CZ GR SI US CN TW '; then
			gravity=West
		fi

		convert temp.png -resize 50^x45^ -gravity "$gravity" -extent 50x45 "$WZ_ROOT/data/base/images/flags/flag-$flag.png"
	done
}

check_dependencies() {
	for bin in rsvg-convert convert curl; do
		if ! [ "$(which "$bin")" ]; then
			log 'This script requires the command "%s". Please install it before continuing.' "$bin"
			exit 1
		fi
	done
}

download_images() {
	log 'Downloading images...'
	repo_owner=madebybowtie
	repo_name=FlagKit
	repo_svgs_path=Assets/SVG
	download_url="https://codeload.github.com/$repo_owner/$repo_name/tar.gz/master"

	curl "$download_url" --silent --show-error | tar -xz --xform='s#^.+/##x' "$repo_name-master/$repo_svgs_path"
}

cleanup_temp_dir() {
	if [ -d "$temp_dir" ]; then
		log 'Cleaning up temporary directory...'
		rm -rf "$temp_dir"
	fi
}

log() {
	pattern="$1"; shift
	printf "$pattern\n" "$@" 1>&2
}

main
