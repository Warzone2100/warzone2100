#!/bin/sh

# We need to be in the working copy's root directory
cd ..

# Add the comment to the top of the file
cat > po/POTFILES.in << EOF
# List of source files which contain translatable strings.
EOF

find lib src data -type f | grep -v '\/.svn\/' | grep -e '\.c$' -e 'data.*strings.*\.txt$' -e '\.slo$' -e '\.rmsg$' | grep -v -e '\.lex\.c$' -e '\.tab\.c$' -e 'sqlite3\.c' -e 'GLee\.c' | sort >> po/POTFILES.in
