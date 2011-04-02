#!/bin/sh

# We need to be in the working copy's root directory
cd "`dirname "$0"`/.."

# Add the comment to the top of the file
cat > po/POTFILES.in << EOF
# List of source files which contain translatable strings.
EOF

find lib src data -type f |
	grep -v '\/.svn\/' |
	grep -e '\.c\(pp\|xx\)\?$' -e 'data.*strings.*\.txt$' -e 'data.*sequenceaudio.*\.tx.$' -e '\.slo$' -e '\.rmsg$' |
	grep -v -e '\.lex\.c\(pp\|xx\)\?$' -e '\.tab\.c\(pp\|xx\)\?$' -e 'lib/netplay/miniupnpc/*' -e 'lib/betawidget/*' |
	sort >> po/POTFILES.in
