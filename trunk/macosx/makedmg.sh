#!/bin/sh

MYDIR=`dirname "$0"`
cd "$MYDIR"
MYDIR=`pwd`

sh makebundle.sh

VOL="Warzone 2100"
DMG="Warzone 2100.dmg"
TMPDMG="tmp-warzone.dmg"

if [ -e "$MYDIR/$DMG" ]; then
    echo "Removing old dmg..."
    rm "$MYDIR/$DMG"
fi

echo "Creating dmg..."

SIZE=`du -sk "Warzone 2100.app" | awk '{printf "%.3gn",($1*10.5)/(10240)}'`
hdiutil create "$TMPDMG" -megabytes ${SIZE} -ov -type UDIF >/dev/null 2>&1
DISK=`hdid -nomount "$TMPDMG" | awk '/scheme/ {print substr ($1, 6, length)}'`
newfs_hfs -v "${VOL}" /dev/r${DISK}s2 >/dev/null 2>&1
hdiutil eject "$DISK" >/dev/null 2>&1

echo "Copying application bundle and README.txt to dmg..."

hdid "$TMPDMG" >/dev/null 2>&1
ditto -rsrcFork -v "Warzone 2100.app" "/Volumes/${VOL}/Warzone 2100.app" >/dev/null 2>&1
cp README.txt "/Volumes/${VOL}/README.txt" >/dev/null 2>&1
hdiutil eject "$DISK" >/dev/null 2>&1

echo "Cleaning up application bundle..."

rm -rf "Warzone 2100.app"

echo "Compressing dmg..."

hdiutil convert "$TMPDMG" -format UDZO -o "$DMG" >/dev/null 2>&1
rm -f "$TMPDMG"

echo "Done."

