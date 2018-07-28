#!/bin/bash

# The Qt 5 Release frameworks (as of 5.9.1) contain cruft not necessary for Release builds,
# that also interferes with Xcode's built-in code signing.
#
# Specifically:
#       - The Version is "5" at /Versions/5, instead of "A" at /Versions/A (breaks automatic code-signing)
#       - The frameworks contain additional debug libraries, which can be safely removed (also breaks automatic code-signing)
#       - The frameworks contain ".prl" files in their root folder that must be removed (also breaks verifying code signature of containing app bundle)
#       - The internal linker paths point to /Versions/5 in other Qt frameworks (which must be fixed-up if we are changing the Version of everything to A)

OutDir="QT"
DirectorY="external/${OutDir}"

QtFrameworks=('QtCore' 'QtGui' 'QtNetwork' 'QtOpenGL' 'QtPrintSupport' 'QtScript' 'QtWidgets')
QtPlugins=('platforms/libqcocoa.dylib' 'printsupport/libcocoaprintersupport.dylib')


get_script_dir () {
	# See: https://stackoverflow.com/a/246128
	SOURCE="${BASH_SOURCE[0]}"
	while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
	  DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null && pwd )"
	  SOURCE="$(readlink "$SOURCE")"
	  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
	done
	DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null && pwd )"
	echo "$DIR"
}
SCRIPT_DIR="$(get_script_dir)"

updateInternalLibraryPaths() {
    f=$1

    otool -L $f | while read -r line ; do
        # Trim the line down to *just* the path, without any of the trailing information (or leading/trailing whitespace)
        OrigPath="$(echo "$line" | sed -e "s/(compatibility version .*, current version .*)$//" -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"

        # Check if the path contains that of any of the other affected QtFrameworks
        for Framework in "${QtFrameworks[@]}" ; do
            FixedPath="$(echo $OrigPath | sed -e "s/$Framework\.framework\/Versions\/.*\//$Framework\.framework\/Versions\/A\//")"
            if [[ "$FixedPath" != "$OrigPath" ]]; then
                # The path requires adjustment
                echo "info: [$f] Adjusting linked library path from (\"$OrigPath\") -> (\"$FixedPath\")."
                install_name_tool -change "$OrigPath" "$FixedPath" "$f"
            fi
        done
    done
}

fixFrameworkVersion() {
    # call FixFrameworkVersion.sh
	"${SCRIPT_DIR}/../FixFrameworkVersion.sh" "${DirectorY}/$1"
    if [ $? -ne 0 ]; then
        echo "error: [$1] FixFrameworkVersion.sh \"${DirectorY}/$1\" failed"
        exit 1
    fi

    # Remove the "_debug" libraries, which muck up automatic codesigning
    if cd "${DirectorY}/$1/Versions/A"; then
        rm *_debug 2> /dev/null
        cd - > /dev/null
    else
        echo "error: [$1] Unable to change directory into the framework's A Version"
        exit 1
    fi

	# Clean up the framework root
	if cd "${DirectorY}/$1"; then
        # Remove the "_debug" symlink
        rm *_debug 2> /dev/null
        # Remove the ".prl" files
        rm *.prl 2> /dev/null
        cd - > /dev/null
    else
        echo "error: [$1] Unable to change directory into the framework's root"
        exit 1
    fi

    # Update internal library paths (Versions) to other Qt frameworks
    if cd "${DirectorY}/$1/Versions/A"; then
        for f in * ; do
            if [ -f "$f" ]; then # only process files
                updateInternalLibraryPaths $f
            fi
        done
        cd - > /dev/null
    fi

}

fixDylibPaths() {
    f="${DirectorY}/$1"

    if [ ! -f "$f" ]; then
        echo "error: fixDylibPaths: invalid path, \"$1\""
        exit 1
    fi

    echo "info: [$f] Examining dylib"
    updateInternalLibraryPaths $f
}

for Framework in "${QtFrameworks[@]}" ; do
    fixFrameworkVersion "$Framework.framework"
done

for Plugin in "${QtPlugins[@]}" ; do
    fixDylibPaths "plugins/$Plugin"
done

echo "Fixed QT framework Versions"

exit 0
