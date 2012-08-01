#!/bin/sh

if [ ! -f configs/CS-ID.xcconfig ]; then
    cat <<EOF > configs/CS-ID.xcconfig
// Global settings for Code Signing

CODE_SIGN_IDENTITY = 

EOF
fi
