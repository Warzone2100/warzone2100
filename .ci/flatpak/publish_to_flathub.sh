#!/bin/bash
# This requires a bunch of environment variables to be set:
# - WZ_FLATPAK_LOCAL_REPO_NAME
# - FH_TOKEN
# - FLAT_MANAGER_URL
# - FH_REPOSITORY

if [ -z "$WZ_FLATPAK_LOCAL_REPO_NAME" ]; then
  echo "Missing WZ_FLATPAK_LOCAL_REPO_NAME environment variable"
  exit 1
fi
if [ -z "$FH_TOKEN" ]; then
  echo "Missing FH_TOKEN environment variable"
  exit 1
fi
if [ -z "$FLAT_MANAGER_URL" ]; then
  echo "Missing FLAT_MANAGER_URL environment variable"
  exit 1
fi
if [ -z "$FH_REPOSITORY" ]; then
  echo "Missing FH_REPOSITORY environment variable"
  exit 1
fi

echo "::group::flatpak build-update-repo"
flatpak build-update-repo --generate-static-deltas "${WZ_FLATPAK_LOCAL_REPO_NAME}"
exit_status=$?
if [ $exit_status -ne 0 ]; then
  echo "build-update-repo failed: ${exit_status}"
  exit ${exit_status}
fi
echo "::endgroup::"

BUILD_ID="$(flat-manager-client --token "${FH_TOKEN}" create "${FLAT_MANAGER_URL}" ${FH_REPOSITORY})"
if [ $? -ne 0 ]; then
  echo "ERROR: flat-manager-client failed to create a build id"
  exit 1
fi
BUILD_ID="$(echo -e "${BUILD_ID}" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
echo "Created build id: \"${BUILD_ID}\""

flat-manager-client --token "${FH_TOKEN}" push --commit --publish --wait "${BUILD_ID}" "${WZ_FLATPAK_LOCAL_REPO_NAME}"
exit_status=$?
if [ $exit_status -ne 0 ]; then
  echo "ERROR: Pushing / publishing failed?"
fi

flat-manager-client --token "${FH_TOKEN}" purge "${BUILD_ID}"
if [ $? -ne 0 ]; then
  echo "ERROR: Purging failed"
  exit_status=1
fi
exit ${exit_status}
