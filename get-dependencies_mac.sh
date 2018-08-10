#!/bin/bash

# Optionally, specify:
# a VCPKG_BUILD_TYPE : This will be used to modify the current triplet (once vcpkg is downloaded)
# "-autofix"         : Do not prompt to fix problems - automatically proceed

############################

# To ensure reproducible builds, pin to a specific vcpkg commit
VCPKG_COMMIT_SHA="df671c6a36edfe35b95f89f92eeebdf70812d03f"

############################

# Handle input arguments
AUTOFIX=0
VCPKG_BUILD_TYPE=
if [ -n "$1" ]; then
	if [ "$1" == "-autofix" ]; then
		AUTOFIX=1
	else
		VCPKG_BUILD_TYPE="$1"
	fi
fi

if [ -n "$2" ]; then
	if [ "$2" == "-autofix" ]; then
		AUTOFIX=1
	fi
fi

# Download & build vcpkg
if [ ! -d "vcpkg/.git" ]; then
	# Clone the vcpkg repo
	echo "Cloning https://github.com/Microsoft/vcpkg.git ..."
	git clone -q https://github.com/Microsoft/vcpkg.git
else
	# On CI (for example), the vcpkg directory may have been cached and restored
	echo "Skipping git clone for vcpkg (local copy already exists)"
	git pull
fi
cd vcpkg
git reset --hard $VCPKG_COMMIT_SHA
./bootstrap-vcpkg.sh
bootstrapresult=${?}
if [ $bootstrapresult -ne 0 ]; then
	# vcpkg currently requires modern gcc to compile itself on macOS
	# on initial bootstrapping failure, offer to Homebrew install gcc
	echo ""
	if [ $AUTOFIX -ne 1 ]; then
		read -n 1 -p "Use Homebrew to install gcc6, and retry vcpkg bootstrap? [y or n]: " homebrewinstallinput
		echo ""
	else
		echo "-autofix: Use Homebrew to install gcc6"
		homebrewinstallinput="y"
	fi
	if [ "${homebrewinstallinput}" == "y" ]; then
		if [! command -v brew >/dev/null 2>&1 ]; then
			echo "Homebrew does not seem to be installed. Please follow the instructions at: https://brew.sh"
		fi
		if ! brew install gcc6; then
			echo "brew install gcc6 failed - is Homebrew installed?"
			exit 1
		fi
		# retry vcpkg bootstrap
		./bootstrap-vcpkg.sh
		bootstrap-result=$?
		if [ $retVal -ne 0 ]; then
			echo "2nd attempt vcpkg bootstrap failed - cannot continue"
			# Check if the CXX envar is set, as this may cause issues with vcpkg bootstrap compile
			if [ -n  "$CXX" ]; then
				echo "WARNING: The CXX environment variable is set, which may interfere with vcpkg bootstrap's detection of Homebrew-installed gcc6"
			fi
			exit 1
		fi
	elif [ "${homebrewinstallinput}" == "n" ]; then
		echo "vcpkg bootstrapping failed - cannot continue"
		exit 1
	else
		echo "Invalid input: ${homebrewinstallinput}"
		exit 1
	fi
fi

if [ -n "${VCPKG_BUILD_TYPE}" ]; then
	# Add VCPKG_BUILD_TYPE to the specified triplet
	triplet="x64-osx" # vcpkg macOS default
	if [ -n "${VCPKG_DEFAULT_TRIPLET}" ]; then
		echo "Using VCPKG_DEFAULT_TRIPLET=${VCPKG_DEFAULT_TRIPLET}"
		triplet="${VCPKG_DEFAULT_TRIPLET}"
	fi
	tripletFile="triplets/${triplet}.cmake";
	tripletCommand="set(VCPKG_BUILD_TYPE \"${VCPKG_BUILD_TYPE}\")"
	if grep -Fq "${tripletCommand}" "${tripletFile}"; then
		echo "Already modified triplet (${triplet}) to use VCPKG_BUILD_TYPE: \"${VCPKG_BUILD_TYPE}\""
	else
		echo "" >> "${tripletFile}"
		echo "${tripletCommand}" >> "${tripletFile}"
		echo "Modified triplet (${triplet}) to use VCPKG_BUILD_TYPE: \"${VCPKG_BUILD_TYPE}\""
	fi
fi

# Download & build WZ macOS dependencies
./vcpkg install physfs harfbuzz libogg libtheora libvorbis libpng sdl2 glew freetype gettext zlib
vcpkgInstallResult=${?}
if [ $vcpkgInstallResult -ne 0 ]; then
	echo "It appears that `vcpkg install` has failed (return code: $vcpkgInstallResult)"
	exit 1
fi

# Ensure dependencies are always the desired version (in case of prior runs with different vcpkg commit / version)
./vcpkg upgrade --no-dry-run

sleep 1

echo "vcpkg install finished"
cd - > /dev/null

exit 0
