#!/bin/sh

MYDIR=`dirname "$0"`
cd "$MYDIR"
MYDIR=`pwd`
cd ..
WORKDIR=`pwd`

echo "Creating application bundle in $MYDIR..."

cd "$MYDIR"
mkdir Warzone.app
cd Warzone.app
mkdir Contents
cd Contents

echo "Converting and adding Info.plist..."

plutil -convert binary1 -o ./Info.plist "$WORKDIR"/macosx/Info.plist

echo "Adding Warzone startup script..."

mkdir MacOS
cd MacOS

cp "$WORKDIR"/macosx/Warzone .
chmod 755 Warzone

cd ..
mkdir Frameworks
mkdir Resources
cd Resources
mkdir lib

echo "Adding icon..."

cp "$WORKDIR"/macosx/warzone.icns .

echo "Adding Warzone 2100 data..."

cp -R "$WORKDIR"/data .

echo "Cleaning out unnecessary data files..."

cd data
rm Makefile
rm Makefile.am
rm Makefile.in
find . -name .svn -exec rm -rf \{\} \; 2>/dev/null

echo "Adding warzone binary..."

cp "$WORKDIR"/src/warzone .

echo "Adding libraries and frameworks and updating references..."

cp -R /System/Library/Frameworks/OpenAL.framework ../../Frameworks
install_name_tool -change /System/Library/Frameworks/OpenAL.framework/Versions/A/OpenAL @executable_path/../../Frameworks/OpenAL.framework/Versions/A/OpenAL warzone

for i in `otool -L warzone | grep local | sed -e 's/^.*\/opt\/local\/lib\/\([^ ]*\).*$/\1/'` ; do
    cp /opt/local/lib/$i ../lib
    install_name_tool -change /opt/local/lib/$i @executable_path/../lib/$i warzone
done

cd ../lib
for i in *.dylib ; do
    install_name_tool -id $i $i
    for j in *.dylib ; do
        install_name_tool -change /opt/local/lib/$j @executable_path/../lib/$j $i
    done
done

cd "$MYDIR"

VOL="Warzone 2100"
DMG="Warzone 2100.dmg"
TMPDMG="tmp-warzone.dmg"

if [ -e "$MYDIR/$DMG" ]; then
    echo "Removing old dmg..."
    rm "$MYDIR/$DMG"
fi

echo "Creating dmg..."

SIZE=`du -sk Warzone.app | awk '{printf "%.3gn",($1*10.5)/(10240)}'`
hdiutil create "$TMPDMG" -megabytes ${SIZE} -ov -type UDIF >/dev/null 2>&1
DISK=`hdid -nomount "$TMPDMG" | awk '/scheme/ {print substr ($1, 6, length)}'`
newfs_hfs -v "${VOL}" /dev/r${DISK}s2 >/dev/null 2>&1
hdiutil eject "$DISK" >/dev/null 2>&1

echo "Copying application bundle and README.txt to dmg..."

hdid "$TMPDMG" >/dev/null 2>&1
ditto -rsrcFork -v "Warzone.app" "/Volumes/${VOL}/Warzone.app" >/dev/null 2>&1
cp README.txt "/Volumes/${VOL}/README.txt" >/dev/null 2>&1
hdiutil eject "$DISK" >/dev/null 2>&1

echo "Cleaning up application bundle..."

rm -rf Warzone.app

echo "Compressing dmg..."

hdiutil convert "$TMPDMG" -format UDZO -o "$DMG" >/dev/null 2>&1
rm -f "$TMPDMG"

echo "Done."

