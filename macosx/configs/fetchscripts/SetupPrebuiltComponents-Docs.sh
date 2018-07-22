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

cd "${SRCROOT}/.."

mWarzoneHelp="macosx/external/docs"
if [ -n "$BUILT_PRODUCTS_DIR" ]; then
	# When running from Xcode, output the generated documentation into the BUILT_PRODUCTS_DIR
	mWarzoneHelp="${BUILT_PRODUCTS_DIR}/docs"
fi

if [[ -d "${mWarzoneHelp}" ]]; then
	rm -fr "${mWarzoneHelp}"
fi
mkdir -p "${mWarzoneHelp}"

# Build JS Scripting API docs
	grep src/qtscript.cpp -e '//==' | sed -E 's:.*//== ?::' > doc/js-globals.md
	grep src/qtscriptfuncs.cpp -e '//==' | sed -E 's:.*//== ?::' >> doc/js-globals.md
	grep src/qtscript.cpp -e '//__' | sed -E 's:.*//__ ?::' > doc/js-events.md
	grep src/qtscript.cpp -e '//--' | sed -E 's:.*//-- ?::' > doc/js-functions.md
	grep src/qtscriptfuncs.cpp -e '//--' | sed -E 's:.*//-- ?::' >> doc/js-functions.md
	grep src/qtscript.cpp -e '//;;' | sed -E 's:.*//;; ?::' > doc/js-objects.md
	grep src/qtscriptfuncs.cpp -e '//;;' | sed -E 's:.*//;; ?::' >> doc/js-objects.md
	grep data/base/script/campaign/libcampaign.js -e '.*//;;' | sed -E 's://;; ?::' > doc/js-campaign.md
cp -af doc/Scripting.md ${mWarzoneHelp}
cp -af doc/js-{globals,events,functions,objects,campaign}*.md ${mWarzoneHelp}

if [[ ! "0" == "$(type -aP a2x &> /dev/null; echo ${?})" ]]; then
	echo "warning: Cannot build all docs locally (a2x missing)"
	echo "info: Follow the instructions in macosx/README.md to install the prerequisites for building the docs"
	exit 0
fi

alias sed="/usr/bin/sed"
alias grep="/usr/bin/grep"

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
a2x_run_output=$(a2x -f xhtml -D "${mWarzoneHelp}" doc/quickstartguide.asciidoc 2>&1)
a2x_result=${?}
if [ $a2x_result -eq 0 ]; then

	# man page
	a2x -f xhtml -d manpage -D "${mWarzoneHelp}" doc/warzone2100.6.asciidoc

	# Cleanup
	sed -i '' -e 's:warzone2100:Warzone:g' ${mWarzoneHelp}/warzone2100.6.html
else
	echo "error: Something went wrong with a2x, the quickstart guide and manpage could not be generated. (Try manually running `brew install docbook-xsl`.)"
	echo "warning: ${a2x_run_output}"
	exit ${a2x_result}
fi

exit 0
