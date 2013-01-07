#!/bin/sh

if [ ! -f configs/CS-ID.xcconfig ]; then
    cat <<EOF > configs/CS-ID.xcconfig
// Global settings for Code Signing

CODE_SIGN_IDENTITY = 
OTHER_CODE_SIGN_FLAGS = --identifier net.wz2100.$(PRODUCT_NAME) --requirements $(CODE_SIGN_REQUIREMENTS_PATH) -vvv
CODE_SIGN_REQUIREMENTS_PATH = $(DERIVED_FILE_DIR)/frameworkrequirement.rqset
CODE_SIGN_RESOURCE_RULES_PATH = 

EOF
fi
