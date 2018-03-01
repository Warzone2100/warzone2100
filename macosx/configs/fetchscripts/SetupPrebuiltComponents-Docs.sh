#!/bin/bash

if [ -z "$XCODE_VERSION_ACTUAL" ]; then
	# Apparently not being run by Xcode, so assume that $PATH is properly
	# configured to expose a2x (if installed).
	echo "info: Using supplied PATH without modification: ${PATH}"
else
	# Script is being run by Xcode
	# Xcode reconfigures $PATH to point inside its toolchain.
	#
	# Explicitly configure PATH to include:
	#	- MacPorts installation folders (/opt/local/bin:/opt/local/sbin)
	#   - The Homebrew symlink location (/usr/local/bin)
	#	- The original $PATH provided by Xcode
	# IMPORTANT: The MacPorts and Homebrew paths *must* come before the original PATH, or a2x breaks.

    export PATH=/opt/local/bin:/opt/local/sbin:/usr/local/bin:${PATH}
fi

OutDir="WarzoneHelp"
DirectorY="${OutDir}-d55b5bf"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
SHA256Sum="4447ce854b9a634f9c67d13ac03479b8c58981f256cb65f0a563a8b4ee629cfe"

if [[ ! "0" == "$(type -aP a2x &> /dev/null; echo ${?})" ]]; then
	echo "warning: Cannot build docs locally; downloaded materials may be outdated"
	echo "info: Follow the instructions in macosx/README.md to install the prerequisites for building the docs"
	configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${SHA256Sum}"
	result=${?}
	if [ result -ne 0 ]; then
		echo "error: Fetching (old) cached docs failed."
		exit ${result}
	fi
	if [ -n "$BUILT_PRODUCTS_DIR" ]; then
		# copy the extracted copy to the $BUILT_PRODUCTS_DIR
		cp -af external/${OutDir}/* ${BUILT_PRODUCTS_DIR}/WarzoneHelp
	fi
	exit 0
fi

mWarzoneHelp="macosx/external/WarzoneHelp"
if [ -n "$BUILT_PRODUCTS_DIR" ]; then
	# When running from Xcode, output the generated documentation into the BUILT_PRODUCTS_DIR
	mWarzoneHelp="${BUILT_PRODUCTS_DIR}/WarzoneHelp"
fi
mWarzoneHelpResources="${mWarzoneHelp}/Contents/Resources"
mWarzoneHelpLproj="${mWarzoneHelp}/Contents/Resources/en.lproj"

alias sed="/usr/bin/sed"
alias grep="/usr/bin/grep"

if [ "${ACTION}" = "clean" ]; then
	# Force cleaning when directed
	rm -fRv "external/${OutDir}"
	exit 0
fi

cd "${SRCROOT}/.."
if [[ -d "${mWarzoneHelp}" ]]; then
	rm -fr "${mWarzoneHelp}"
fi
mkdir -p "${mWarzoneHelpLproj}"
cp -af macosx/Resources/WarzoneHelp/Info.plist "${mWarzoneHelp}/Contents"


# ASCIIDoc based docs

A2XPATH="$(type -aP a2x)"
BREWPREFIX="$(brew --prefix)"
brewCheckSuccess=${?}
if [[ brewCheckSuccess -eq 0 ]] && [[ $A2XPATH == $BREWPREFIX/* ]]; then
    # HOMEBREW WORKAROUND:
    # Workaround an issue where a2x fails while trying to xmllint by explicitly
    # exporting the expected catalog file path.
    export XML_CATALOG_FILES="$BREWPREFIX/etc/xml/catalog"
    echo "info: Homebrew workaround: export XML_CATALOG_FILES = $XML_CATALOG_FILES"
fi

# quickstartguide
a2x_run_output=$(a2x -f chunked -D "${mWarzoneHelpLproj}" doc/quickstartguide.asciidoc 2>&1)
a2x_result=${?}
if [ $a2x_result -eq 0 ]; then
	cp -af ${mWarzoneHelpLproj}/quickstartguide.chunked/* ${mWarzoneHelpLproj}
	rm -fr ${mWarzoneHelpLproj}/quickstartguide.chunked

	# man page
	a2x -f xhtml -d manpage -D "${mWarzoneHelpLproj}" doc/warzone2100.6.asciidoc

	# Cleanup
	cp -af "${mWarzoneHelpLproj}/docbook-xsl.css" "${mWarzoneHelpLproj}/images" ${mWarzoneHelpResources}
	rm -fr "${mWarzoneHelpLproj}/docbook-xsl.css" "${mWarzoneHelpLproj}/images"
	sed -i '' -e 's:href="docbook-xsl.css:href="../docbook-xsl.css:g' ${mWarzoneHelpLproj}/*.html
	sed -i '' -e 's:src="images/:src="../images/:g' ${mWarzoneHelpLproj}/*.html
	sed -i '' -e 's:warzone2100:Warzone:g' ${mWarzoneHelpLproj}/warzone2100.6.html
else
	echo "error: Something went wrong with a2x, the quickstart guide and manpage could not be generated. (Try manually running `brew install docbook-xsl`.)"
	echo "warning: ${a2x_run_output}"
	exit ${a2x_result}
fi

# javascript doc
	grep src/qtscript.cpp -e '//==' | sed -E 's:.*//== ?::' > doc/js-globals.md
	grep src/qtscriptfuncs.cpp -e '//==' | sed -E 's:.*//== ?::' >> doc/js-globals.md
	grep src/qtscript.cpp -e '//__' | sed -E 's:.*//__ ?::' > doc/js-events.md
	grep src/qtscript.cpp -e '//--' | sed -E 's:.*//-- ?::' > doc/js-functions.md
	grep src/qtscriptfuncs.cpp -e '//--' | sed -E 's:.*//-- ?::' >> doc/js-functions.md
	grep src/qtscript.cpp -e '//;;' | sed -E 's:.*//;; ?::' > doc/js-objects.md
	grep src/qtscriptfuncs.cpp -e '//;;' | sed -E 's:.*//;; ?::' >> doc/js-objects.md
	grep data/base/script/campaign/libcampaign.js -e '.*//;;' | sed -E 's://;; ?::' > doc/js-campaign.md
cp -af doc/Scripting.md ${mWarzoneHelpLproj}
cp -af doc/js-{globals,events,functions,objects,campaign}*.md ${mWarzoneHelpLproj}

# Cleanup

# Build the index
rm -f "${mWarzoneHelpLproj}/WarzoneHelp.helpindex"
hiutil -Caf "${mWarzoneHelp}/WarzoneHelp.helpindex" "${mWarzoneHelpLproj}"
cp -af "${mWarzoneHelp}/WarzoneHelp.helpindex" "${mWarzoneHelpLproj}"
rm -f "${mWarzoneHelp}/WarzoneHelp.helpindex"

exit 0
