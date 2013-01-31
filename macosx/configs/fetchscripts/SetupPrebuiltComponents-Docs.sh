#!/bin/bash
export PATH=${PATH}:/sw/bin:/opt/local/bin:/usr/local/bin

OutDir="WarzoneHelp"
DirectorY="${OutDir}-d55b5bf"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="247d204b649909bb3c4da5c5aed10425"

mWarzoneHelp="macosx/external/WarzoneHelp/Contents/Resources"
mWarzoneHelpLproj="${mWarzoneHelp}/en.lproj"


if [[ ! "0" == "$(type -aP a2x &> /dev/null; echo ${?})" ]] || [[ ! "0" == "$(type -aP pdflatex &> /dev/null; echo ${?})" ]]; then
	echo "note: cannot build docs locally; downloaded materials may be outdated"
	configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
	exit ${?}
fi
test -r /sw/bin/init.sh && . /sw/bin/init.sh
alias sed="/usr/bin/sed"
alias grep="/usr/bin/grep"


cd "${SRCROOT}/.."
if [[ -d "macosx/external/WarzoneHelp" ]]; then
	rm -fr "macosx/external/WarzoneHelp"
fi
mkdir -p "${mWarzoneHelpLproj}"
cp -af macosx/Resources/WarzoneHelp/info.plist macosx/external/WarzoneHelp/Contents


# ASCIIDoc based docs

# quickstartguide
a2x -f chunked -D "${mWarzoneHelpLproj}" doc/quickstartguide.asciidoc
cp -af ${mWarzoneHelpLproj}/quickstartguide.chunked/* ${mWarzoneHelpLproj}
rm -fr ${mWarzoneHelpLproj}/quickstartguide.chunked

# man page
a2x -f xhtml -d manpage -D "${mWarzoneHelpLproj}" doc/warzone2100.6.asciidoc

# Cleanup
cp -af "${mWarzoneHelpLproj}/docbook-xsl.css" "${mWarzoneHelpLproj}/images" ${mWarzoneHelp}
rm -fr "${mWarzoneHelpLproj}/docbook-xsl.css" "${mWarzoneHelpLproj}/images"
sed -i '' -e 's:href="docbook-xsl.css:href="../docbook-xsl.css:g' ${mWarzoneHelpLproj}/*.html
sed -i '' -e 's:src="images/:src="../images/:g' ${mWarzoneHelpLproj}/*.html
sed -i '' -e 's:warzone2100:Warzone:g' ${mWarzoneHelpLproj}/warzone2100.6.html


# LaTeX based docs
export TEXINPUTS=${TEXINPUTS}:/sw/share/doc/python25/Doc/texinputs/

# javascript doc
grep src/qtscript.cpp -e '//==' | sed 's://==::' > doc/globals.tex
grep src/qtscriptfuncs.cpp -e '//==' | sed 's://==::' >> doc/globals.tex
grep src/qtscript.cpp -e '//__' | sed 's://__::' > doc/events.tex
grep src/qtscript.cpp -e '//--' | sed 's://--::' > doc/functions.tex
grep src/qtscriptfuncs.cpp -e '//--' | sed 's://--::' >> doc/functions.tex
grep src/qtscriptfuncs.cpp -e '//;;' | sed 's://;;::' > doc/objects.tex
cd doc/
if ! pdflatex javascript.tex; then
	exit 1
fi
pdflatex javascript.tex
cd ..
cp -af doc/javascript.pdf ${mWarzoneHelpLproj}

# Build the index
rm -f "${mWarzoneHelpLproj}/WarzoneHelp.helpindex"
hiutil -Caf "${mWarzoneHelp}/WarzoneHelp.helpindex" "${mWarzoneHelpLproj}"
cp -af "${mWarzoneHelp}/WarzoneHelp.helpindex" "${mWarzoneHelpLproj}"
rm -f "${mWarzoneHelp}/WarzoneHelp.helpindex"

exit 0
