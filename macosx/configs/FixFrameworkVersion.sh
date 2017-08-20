#!/bin/bash

#
# Some frameworks (ex: Qt 5) use a non-standard naming for their current Version inside
# the framework bundle.
#
# This script renames the "Current" Version to "A" (which fixes Xcode build issues).
#

FrameworkPath="$1"

cd "${FrameworkPath}"
if [ ! -d "Versions" ]; then
    echo "error: ${FrameworkPath} lacks Versions directory, exiting" >&2
    exit 1
fi
cd Versions

if [ ! -d "A" ]; then

    # Check that "Current" symlink exists
    if [ ! -L "Current" ]; then
        echo "error: ${FrameworkPath}/Versions/Current symlink does not exist" >&2
        exit 1
    fi

    # Get the ultimate target of the "Current" symlink
    CurrentTarget="$(ls -l Current | sed -e 's/.* -> //')"
    # echo "Symlink points to: ${CurrentTarget}"

    # Check that the target does not contain path separators
    if [[ "$CurrentTarget" == *"/"* ]]; then
        echo "error: ${FrameworkPath}/Versions/Current symlink target (\"$CurrentTarget\") is invalid" >&2
        exit 1
    fi

    # Verify that the path exists
    if [ ! -d "${CurrentTarget}" ]; then
        echo "error: Cannot find target of Current, ${FrameworkPath}/Versions/${CurrentTarget} does not exist" >&2
    fi

    OldVersionPath="$CurrentTarget"
    echo "info: Adjusting current version from ${FrameworkPath}/Versions/$OldVersionPath -> ${FrameworkPath}/Versions/A" >&2

    # Rename the Current Version to "A"
    if mv "${CurrentTarget}" "A"; then
        echo "Rename \"${CurrentTarget}\" version -> \"A\""
    else
        echo "error: Attempting to rename the Current Version (\"${CurrentTarget}\") to \"A\" failed" >&2
        exit 1
    fi

    # Update the Current symlink to "A"
    if ln -sfn "A" "Current"; then
        echo "Updated \"Current\" symlink -> \"A\""
    else
        echo "error: Failed to update the \"Current\" symlink -> \"A\"" >&2
        exit 1
    fi

    # Check the install name of libraries in the framework, and update paths as necessary
    cd A
    for f in *
    do
        if [ -f "$f" ]; then #only process files
            OtoolOutput="$(otool -D $f)"
            # use echo to collapse newlines + whitespace, and sed to remove the prefix
            CurrentInstallName="$(echo $OtoolOutput | sed -e "s/^$f: //")"
            if [ ! [["$CurrentInstallName" = *"is not an object file"]] ]; then
                # found object file
                # check if the CurrentInstallName contains the Version we replaced
                FixedInstallName="$(echo $CurrentInstallName | sed -e "s/\/Versions\/$OldVersionPath\//\/Versions\/A\//")"
                if [[ "$FixedInstallName" != "$CurrentInstallName" ]]; then
                    # The install name requires adjustment
                    echo "info: Adjusting install name from (\"$CurrentInstallName\") -> (\"$FixedInstallName\")."
                    # Use install_name_tool to adjust the install name
                    install_name_tool -id "$FixedInstallName" "$f"
                fi
            fi
        fi
    done

else
    # "A" version already exists
    echo "info: ${FrameworkPath}/Versions/A already exists, skipping" >&2
fi

exit 0

