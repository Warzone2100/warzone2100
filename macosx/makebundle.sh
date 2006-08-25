#!/bin/sh

MYDIR=`dirname "$0"`
cd "$MYDIR"
MYDIR=`pwd`
cd ..
WORKDIR=`pwd`

echo "Creating application bundle in $MYDIR..."

cd "$MYDIR"
mkdir Warzone\ 2100.app
cd Warzone\ 2100.app
APPDIR=`pwd`
mkdir Contents
cd Contents

echo "Converting and adding Info.plist..."

plutil -convert binary1 -o ./Info.plist "$WORKDIR"/macosx/Info.plist

echo "Adding warzone2100.sh startup script..."

mkdir MacOS
cd MacOS

cp "$WORKDIR"/macosx/warzone2100.sh .
chmod 755 warzone2100.sh

cd ..
mkdir Frameworks
mkdir Resources
cd Resources
mkdir lib

echo "Adding warzone2100.icns..."

cp "$WORKDIR"/macosx/warzone2100.icns .

echo "Adding data..."

cp -R "$WORKDIR"/data .

echo "Cleaning out unnecessary data files..."

cd data
rm Makefile
rm Makefile.am
rm Makefile.in
find . -name .svn -exec rm -rf \{\} \; 2>/dev/null

echo "Adding warzone2100 binary, libraries, and frameworks..."

cd "$MYDIR"

ruby consolidatelibs.rb "$WORKDIR"/src/warzone2100 "$APPDIR"

