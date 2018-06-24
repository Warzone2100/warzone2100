#!/bin/bash

# ########
# USAGE:
#
# Execute travis_deploy.sh, specifying the input path and file matching pattern
# Optionally, clean up older uploads by specifying --cleanold
#
# So, for example:
#	./travis_deploy.sh "tmp/build_output" "warzone2100-*_macOS.zip" --cleanold
# Will:
#   1. Clean up older files matching `warzone2100-*_macOS.zip` in the deployment path on the server.
#   2. scp all files matching `warzone2100-*_macOS.zip` in the input path to the deployment path on the server.
#
# Example (using this with travis_build.sh):
#	./travis_build.sh regular "tmp/build_output"
#	./travis_deploy.sh "tmp/build_output" "warzone2100-*_macOS.zip" --cleanold
#
# ########
# REQUIRED ENVIRONMENT VARIABLES:
#
# Set these as secure environment variables in the Travis Repository Settings:
# https://docs.travis-ci.com/user/environment-variables/#Defining-Variables-in-Repository-Settings
#
# - SECURE_UPLOAD_BASE64_KEY: The base64-encoded private SSH key used for uploading to the buildbot.
# - DEPLOY_UURL: Used in the scp command. Example: "buildbot@buildbot.wz2100.net"
# - DEPLOY_KNOWN_HOSTS_BASE64: The base64-encoded data to be added to the SSH known_hosts file.
# - DEPLOY_UPLOAD_PATH: The path into which to upload the files. Example: "public_html/files/"
#
# To get the latest SSH public keys for the server in the DEPLOY_UURL (to be added to known_hosts), 
# execute:
# 	ssh-keyscan -H <server>
# (Note: Always verify the information that ssh-keyscan produces with a trusted source.)
#
#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#

# Handle arguments
if [ -z "$1" ]; then
	echo "travis_deploy.sh requires a first argument specifying an input path"
	exit 1
fi
INPUT_DIR="$1"
if [ ! -d "${INPUT_DIR}" ]; then
	echo "ERROR: Supplied an input path that does not exist: \"$INPUT_DIR\""
	exit 1
fi
if [ -z "$2" ]; then
	echo "travis_deploy.sh requires a second argument specifying a file matching pattern"
	exit 1
fi
FILE_MATCH_PATTERN="$2"
if [[ "${FILE_MATCH_PATTERN}" == *\/* ]] || [[ "${FILE_MATCH_PATTERN}" == *\\* ]]; then
	# Error: FILE_MATCH_PATTERN cannot contain \ or /
	echo "error: File matching pattern cannot contain '\' or '/' (pattern is: \"${FILE_MATCH_PATTERN}\")"
	exit 1
fi

CLEAN_OLD=false
if [ -n "$3" ] && [ "$3" == "--cleanold" ]; then
	CLEAN_OLD=true
fi

# Handle scenarios in which deployment should never happen
if [ -n "${TRAVIS_PULL_REQUEST}" ] && [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then
	# Skip deploying from pull requests
	echo "Skip deploy from pull requests."
	exit 0
fi

# Handle required environment variables
MISSING_REQUIRED_ENV_VAR="false"

if [ -z "${SECURE_UPLOAD_BASE64_KEY}" ]; then
	# To deploy builds to the buildbot server, a secure environment variable named
	# SECURE_UPLOAD_BASE64_KEY must be set in Travis-CI repo settings.
	#
	# Its value should be a base64-encoded SSH private key.
	#
	echo "Missing required environment variable: SECURE_UPLOAD_BASE64_KEY"
	MISSING_REQUIRED_ENV_VAR="true"
fi

if [ -z "${DEPLOY_UURL}" ]; then
	echo "Missing required environment variable: DEPLOY_UURL"
	MISSING_REQUIRED_ENV_VAR="true"
fi

if [ -z "${DEPLOY_UPLOAD_PATH}" ]; then
	echo "Missing required environment variable: DEPLOY_UPLOAD_PATH"
	MISSING_REQUIRED_ENV_VAR="true"
fi

if [ "${MISSING_REQUIRED_ENV_VAR}" != "false" ]; then
	echo "Skipping deploy; one or more required environment variables are not set."
	exit 1
fi

echo "Set up SSH"

# DO NOT CHANGE THE FOLLOWING LINE: Always turn off command traces while dealing with the private key
set +x

# Get the encrypted private key from the Travis-CI settings
echo ${SECURE_UPLOAD_BASE64_KEY} | base64 --decode > ~/.ssh/id_rsa
chmod 600 ~/.ssh/id_rsa
ssh-add ~/.ssh/id_rsa

# BE CAREFUL ABOUT CHANGING THE LINES ABOVE: The private key *MUST NOT* be output to the build log.


# Output to the known_hosts file any required entries
if [ -n "${DEPLOY_KNOWN_HOSTS_BASE64}" ]; then
	echo "Adding known_hosts data"
	echo ${DEPLOY_KNOWN_HOSTS_BASE64} | base64 --decode >> ~/.ssh/known_hosts
fi

if [ "$CLEAN_OLD" = true ] ; then
	# Clean up older uploads
	if [[ "${DEPLOY_UPLOAD_PATH}" == *\\* ]] || [[ "${DEPLOY_UPLOAD_PATH}" == *\'* ]]; then
		# Error: DEPLOY_UPLOAD_PATH cannot contain \ or '
		echo "error: DEPLOY_UPLOAD_PATH cannot contain \ or ' (is: \"${DEPLOY_UPLOAD_PATH}\")"
		exit 2
	fi
	if [[ "${FILE_MATCH_PATTERN}" == *\\* ]] || [[ "${FILE_MATCH_PATTERN}" == *\'* ]]; then
		# Error: FILE_MATCH_PATTERN cannot contain \ or '
		echo "error: FILE_MATCH_PATTERN cannot contain \ or ' (is: \"${FILE_MATCH_PATTERN}\")"
		exit 2
	fi
	echo "Clean-up older \"${FILE_MATCH_PATTERN}\" in \"${DEPLOY_UURL}:${DEPLOY_UPLOAD_PATH}\""
	if ! ssh "${DEPLOY_UURL}" -C "cd '${DEPLOY_UPLOAD_PATH}' && find . -maxdepth 1 -type f -name '${FILE_MATCH_PATTERN}' -printf '%T@ %f\n' | sort -k1,1r | cut -d' ' -f2 | grep -v '/$' | tail -n +3 | xargs -I {} rm -- {}"; then
		result="${?}"
		echo "error: Failed to clean up older uploads!"
		#exit ${result}
	fi
	echo "  -> Clean complete."
fi

echo "Upload all \"${FILE_MATCH_PATTERN}\" in \"$INPUT_DIR\" -> \"${DEPLOY_UURL}:${DEPLOY_UPLOAD_PATH}\""

# Upload all matching files in the input directory
cd "${INPUT_DIR}"
for file in `find . -type f -name "${FILE_MATCH_PATTERN}"`; do
	filename=$(basename $file)
	echo "  -> ${filename} ..."
	echo "       scp -pC \"${file}\" \"${DEPLOY_UURL}:${DEPLOY_UPLOAD_PATH}${filename}\""
	scp -pC "${file}" "${DEPLOY_UURL}:${DEPLOY_UPLOAD_PATH}${filename}"
	result=${?}
	if [ $result -ne 0 ]; then
		echo "error: Upload did not complete!"
		exit ${result}
	fi
	echo "       Upload complete."
done


echo "Finished uploading all matching files."

exit 0
