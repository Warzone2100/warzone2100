#!/bin/sh

# We need to be in the working copy's root directory
cd "`dirname "$0"`/../.."

export LC_ALL=C
export LC_COLLATE=C

# extract core game message json strings
find data -name '*.json' -type f \
	-not \( -path 'data/mp/multiplay/maps/*' -prune \) \
	-not \( -path 'data/base/guidetopics/*' -prune \) \
	-exec \
	python3 po/scripts/parseJson.py '{}' ';' |
	python3 po/scripts/aggregateParsedJson.py > po/custom/fromJson.txt

# extract guide json strings
find data/base/guidetopics -name '*.json' -type f -exec \
	python3 po/scripts/extractGuideJsonStrings.py '{}' 'po/guide/extracted/' ';'

#########################################
# Generate main warzone2100 POTFILES.in

# Add the comment to the top of the file
cat > po/POTFILES.in << EOF
# List of source files which contain translatable strings.
EOF

find lib src data po -type f |
	grep -e '\.c\(pp\|xx\)\?$' -e 'data.*strings.*\.txt$' -e 'data.*sequenceaudio.*\.tx.$' -e '\.slo$' -e '\.rmsg$' -e 'po/custom/.*\.txt' -e '\.js$' |
	grep -v -e '\.lex\.c\(pp\|xx\)\?$' -e '\.tab\.c\(pp\|xx\)\?$' -e 'lib/netplay/miniupnpc/*' -e 'lib/betawidget/*' -e '_moc\.' -e 'po/custom/files.js' |
	grep -v -e '_lexer\.cpp' -e '_parser\.cpp' -e 'lib/[^/]*/3rdparty/.*' |
	sort >> po/POTFILES.in

#########################################
# Generate guide POTFILES.in

# Add the comment to the top of the file
cat > po/guide/POTFILES.in << EOF
# List of files which contain extracted translatable guide strings.
EOF

find po/guide/extracted -type f |
	grep -e '\.txt$' |
	sort >> po/guide/POTFILES.in
