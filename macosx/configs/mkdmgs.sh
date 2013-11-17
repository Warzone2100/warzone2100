#!/bin/bash

# Config
bsurl="http://downloads.sf.net/project/warzone2100"

simgflnme="wztemplate.sparseimage"
simgfl="${bsurl}/build-tools/mac/${simgflnme}"
simgflmd5="da10e06f2b9b2b565e70dd8e98deaaad"

sequencenme="sequences.wz"
sequence="${bsurl}/warzone2100/Videos/high-quality-en/${sequencenme}"
sequencemd5="9a1ee8e8e054a0ad5ef5efb63e361bcc"

sequencelonme="sequences-lo.wz"
sequencelo="${bsurl}/warzone2100/Videos/standard-quality-en/${sequencenme}"
sequencelomd5="ab2bbc28cef2a3f2ea3c186e18158acd"

relbuild="${CONFIGURATION_BUILD_DIR}"
dmgout="${OBJROOT}/dmgout"

. "${PROJECT_DIR}/../src/autorevision.cache"
bldtg="$(echo "${VCS_TAG}" | sed -e 's:/:_:g' -e 's:^v::')_[${VCS_SHORT_HASH}]"

# Fail if not release
if [ ! "${CONFIGURATION}" = "Release" ]; then
	echo "error: This should only be run as Release" >&2
	exit 1
fi

# codesign setup
function signd {
	if [ ! -z "${CODE_SIGN_IDENTITY}" ]; then
		# Local Config
		local appth="${1}"
		local idetd="${CODE_SIGN_IDENTITY}"
		local resrul="${SRCROOT}/configs/ResourceRules.plist"
		local csreq="${SRCROOT}/Resources/wz2100requirement.rqset"

		# Sign and verify the app
		cp -a "${resrul}" "${appth}/ResourceRules.plist"
		/usr/bin/codesign -f --sign "${idetd}" --resource-rules="${appth}/ResourceRules.plist" --requirements "${csreq}" -vvv "${appth}"
		rm "${appth}/ResourceRules.plist"
		if ! /usr/bin/codesign --verify -vvv "${appth}"; then
			echo "warning: Code is improperly signed!" 1>&2
		fi

		# Verify the frameworks
		local fsignd=''
		local framelst="$(\ls -1 "${appth}/Contents/Frameworks" | sed -n 's:.framework$:&:p')"
		for fsignd in ${framelst}; do
			if [[ -d "${appth}/Contents/Frameworks/${fsignd}/Versions/A" ]] && [[ ! "${fsignd}" == "SDL.framework" ]]; then
				if ! /usr/bin/codesign --verify -vvv "${appth}/Contents/Frameworks/${fsignd}/Versions/A"; then
					echo "warning: ${fsignd} is not properly signed."
				fi
			fi
		done
	else
		echo "warning: No code signing identity configured; code will not be signed."
	fi
}

# Check our sums
function ckmd5 {
	local FileName="${1}"
	local MD5Sum="${2}"
	local MD5SumLoc="$(md5 -q "${FileName}")"
	if [ -z "${MD5SumLoc}" ]; then
		echo "error: Unable to compute md5 for ${FileName}" >&2
		exit 1
	elif [ "${MD5SumLoc}" != "${MD5Sum}" ]; then
		echo "error: MD5 does not match for ${FileName}" >&2
		exit 1
	fi
}

# Make a dir and get the sparseimage
mkdir -p "${dmgout}"
cd  "${dmgout}"
if [ ! -f "${simgflnme}" ]; then
	echo "Fetching ${simgfl}"
	if ! curl -L -O --connect-timeout "30" "${simgfl}"; then
		echo "error: Unable to fetch ${simgfl}" >&2
		exit 1
	fi
	ckmd5 "${simgflnme}" "${simgflmd5}"
else
	echo "${simgflnme} already exists, skipping"
fi

# Get the sequences

# Comment out the following to skip the high qual seq
if [[ ! -f "${sequencenme}" ]] && [[ "${VCS_TICK}" = "0" ]] && [[ ! "${VCS_TAG}" == *_beta* ]] && [[ ! "${VCS_TAG}" == *_rc* ]]; then
	echo "Fetching ${sequencenme}"
	if ! curl -L --connect-timeout "30" -o "${sequencenme}" "${sequence}"; then
		echo "error: Unable to fetch ${sequence}" >&2
		exit 1
	fi
	ckmd5 "${sequencenme}" "${sequencemd5}"
else
	echo "${sequencenme} already exists or is not needed, skipping"
fi
#

# Comment out the following to skip the low qual seq
if [[ ! -f "${sequencelonme}" ]] && [[ "${VCS_TICK}" = "0" ]] && [[ ! "${VCS_TAG}" == *_beta* ]] && [[ ! "${VCS_TAG}" == *_rc* ]]; then
	echo "Fetching ${sequencelonme}"
	if [ -f "/Library/Application Support/Warzone 2100/sequences.wz" ]; then
		cp -a "/Library/Application Support/Warzone 2100/sequences.wz" "${sequencelonme}"
	elif ! curl -L --connect-timeout "30" -o "${sequencelonme}" "$sequencelo"; then
		echo "error: Unable to fetch ${sequencelo}" >&2
		exit 1
	fi
	ckmd5 "${sequencelonme}" "${sequencelomd5}"
else
	echo "${sequencelonme} already exists or is not needed, skipping"
fi
#

# Copy over the app
cd ../../
echo "Copying the app cleanly."
rm -r -f "${dmgout}/Warzone.app"
if ! tar -c --exclude '*.svg' --exclude 'Makefile*' --exclude 'makefile*' --exclude '.DS_Store' --exclude '.MD5SumLoc' -C "${relbuild}" Warzone.app | tar -xC "${dmgout}"; then
	echo "error: Unable to copy the app" >&2
	exit 1
fi

# Make the dSYM Bundle
mkdir -p "${dmgout}/warzone2100-${bldtg}-dSYM"
cp -af "${relbuild}"/*.dSYM "${dmgout}/warzone2100-${bldtg}-dSYM"
cd "${dmgout}"
rm -f "warzone2100-${bldtg}-dSYM.tgz"
tar -czf "warzone2100-${bldtg}-dSYM.tgz" --exclude '.DS_Store' "warzone2100-${bldtg}-dSYM"

# mkredist.bash

cd Warzone.app/Contents/Resources/data/

echo "== Compressing base.wz =="
if [ -d base/ ]; then
  cd base/
  zip -r -q ../base.wz *
  cd ..
  rm -rf base/
fi

echo "== Compressing mp.wz =="
if [ -d mp/ ]; then
  cd mp/
  zip -r -q ../mp.wz *
  cd ..
  rm -rf mp/
fi

cd mods/

modlst="$(\ls -1)"
for moddr in ${modlst}; do
	if [ -d "${moddr}" ]; then
		cd "${moddr}"
		if [ "${moddr}" = "campaign" ]; then
			sbtyp=".cam"
		elif [ "${moddr}" = "global" ]; then
			sbtyp=".gmod"
		elif [ "${moddr}" = "multiplay" ]; then
			sbtyp=".mod"
		elif [ "${moddr}" = "music" ]; then
			sbtyp=".music"
		else
			sbtyp=""
		fi
		modlstd="$(\ls -1)"
		for modwz in ${modlstd}; do
			if [ -d "${modwz}" ]; then
				echo "== Compressing ${modwz}${sbtyp}.wz =="
				cd "${modwz}"
				zip -r -q ../${modwz}${sbtyp}.wz *
				cd ..
				rm -rf "${modwz}/"
			fi
		done
		cd ..
	fi
done

cd "${dmgout}"

rm -rf ./out ./temp
mkdir "temp/"
mkdir "out/"
mv "warzone2100-${bldtg}-dSYM" "temp/warzone2100-${bldtg}-dSYM"
mv "warzone2100-${bldtg}-dSYM.tgz" "out/warzone2100-${bldtg}-dSYM.tgz"

echo "== Creating DMG =="
cp -a "wztemplate.sparseimage" "temp/wztemplatecopy.sparseimage"
hdiutil resize -size 220m "temp/wztemplatecopy.sparseimage"

hdiutilOut="$(hdiutil mount "temp/wztemplatecopy.sparseimage" | tr -d "\t")"

mountpth="$(echo "${hdiutilOut}" | sed -E 's:(/dev/disk[0-9])( +)::')"

cp -a Warzone.app/* "${mountpth}/Warzone.app"
signd "${mountpth}/Warzone.app"

hdiutil detach "${mountpth}"
hdiutil convert "temp/wztemplatecopy.sparseimage" -format UDZO -o "out/warzone2100-${bldtg}-novideo.dmg"


if [ -f "${sequencelonme}" ]; then
	echo "== Creating LQ DMG =="
	hdiutil resize -size 770m "temp/wztemplatecopy.sparseimage"

	hdiutilOut="$(hdiutil mount "temp/wztemplatecopy.sparseimage" | tr -d "\t")"

	mountpth="$(echo "${hdiutilOut}" | sed -E 's:(/dev/disk[0-9])( +)::')"

	rm -f "${mountpth}/Warzone.app/Contents/Resources/data/sequences.wz"
	cp -a "sequences-lo.wz" "${mountpth}/Warzone.app/Contents/Resources/data/sequences.wz"

	signd "${mountpth}/Warzone.app"
	hdiutil detach "${mountpth}"
	hdiutil convert "temp/wztemplatecopy.sparseimage" -format UDZO -o "out/warzone2100-${bldtg}-lqvideo.dmg"
else
	echo "${sequencelonme} does not exist, skipping"
fi


if [ -f "${sequencenme}" ]; then
	echo "== Creating HQ DMG =="
	hdiutil resize -size 1145m "temp/wztemplatecopy.sparseimage"

	hdiutilOut="$(hdiutil mount "temp/wztemplatecopy.sparseimage" | tr -d "\t")"

	mountpth="$(echo "${hdiutilOut}" | sed -E 's:(/dev/disk[0-9])( +)::')"

	rm -f "${mountpth}/Warzone.app/Contents/Resources/data/sequences.wz"
	cp -a "sequences.wz" "${mountpth}/Warzone.app/Contents/Resources/data/sequences.wz"

	signd "${mountpth}/Warzone.app"
	hdiutil detach "${mountpth}"
	hdiutil convert "temp/wztemplatecopy.sparseimage" -format UDZO  -o "out/warzone2100-${bldtg}-hqvideo.dmg"
else
	echo "${sequencenme} does not exist, skipping"
fi

echo "== Cleaning up =="
rm -f "temp/wztemplatecopy.sparseimage"

# Open the dir
open "out"

exit 0
