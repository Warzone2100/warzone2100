#!/bin/sh

# To use this, the following environment variables must first be manually set in the workflow step that runs it:
# env:
#   WORKFLOW_RUN_CONCLUSION: ${{ github.event.workflow_run.conclusion }}
#   WORKFLOW_RUN_HEAD_SHA: ${{ github.event.workflow_run.head_sha }}

if [ "${GITHUB_EVENT_NAME}" = "workflow_run" ]; then
  echo "GITHUB_EVENT_NAME = ${GITHUB_EVENT_NAME}"
  # Verify it succeeded
  if [ "${WORKFLOW_RUN_CONCLUSION}" != "success" ]; then
    echo "::error ::GITHUB_EVENT_NAME is ${GITHUB_EVENT_NAME}, but the WORKFLOW_RUN_CONCLUSION was: ${WORKFLOW_RUN_CONCLUSION}"
    exit 1
  fi
  if [ -z "${WORKFLOW_RUN_HEAD_SHA}" ]; then
    echo "::error ::WORKFLOW_RUN_HEAD_SHA is not set or is empty"
    exit 1
  fi
  # Use the context of the workflow_run
  echo "WORKFLOW_RUN_HEAD_SHA=${WORKFLOW_RUN_HEAD_SHA}"
  WZ_GITHUB_SHA="${WORKFLOW_RUN_HEAD_SHA}"
  # Get the tag name
  COMMIT_ASSOCIATED_TAG="$(git describe --tags --exact-match --abbrev=0 "${WZ_GITHUB_SHA}")"
  RESULT=$?
  if [ $RESULT -ne 0 ]; then
    echo "::error ::workflow_run trigger does not appear to have run on a tag? (sha: ${WZ_GITHUB_SHA} doesn't match a tag')"
    exit 1
  fi
  WZ_GITHUB_REF="refs/tags/${COMMIT_ASSOCIATED_TAG}"
  WZ_GITHUB_HEAD_REF="" # since it should be a tag, these should always be empty
  WZ_GITHUB_BASE_REF=""
  # Switch to the desired tag ref
  echo "Checking out ref: ${WZ_GITHUB_REF}"
  git checkout "${WZ_GITHUB_REF}"
else
  # Normal case: just use the GITHUB_ variables, and do not override the current checkout
  WZ_GITHUB_SHA="${GITHUB_SHA}"
  WZ_GITHUB_REF="${GITHUB_REF}"
  WZ_GITHUB_HEAD_REF="${GITHUB_HEAD_REF}"
  WZ_GITHUB_BASE_REF="${GITHUB_BASE_REF}"
fi
echo "WZ_GITHUB_SHA=${WZ_GITHUB_SHA}"
echo "WZ_GITHUB_REF=${WZ_GITHUB_REF}"
echo "WZ_GITHUB_HEAD_REF=${WZ_GITHUB_HEAD_REF}"
echo "WZ_GITHUB_BASE_REF=${WZ_GITHUB_BASE_REF}"
echo "::set-output name=WZ_GITHUB_SHA::${WZ_GITHUB_SHA}"
echo "::set-output name=WZ_GITHUB_REF::${WZ_GITHUB_REF}"
echo "::set-output name=WZ_GITHUB_HEAD_REF::${WZ_GITHUB_HEAD_REF}"
echo "::set-output name=WZ_GITHUB_BASE_REF::${WZ_GITHUB_BASE_REF}"
echo "WZ_GITHUB_SHA=${WZ_GITHUB_SHA}" >> $GITHUB_ENV
echo "WZ_GITHUB_REF=${WZ_GITHUB_REF}" >> $GITHUB_ENV
echo "WZ_GITHUB_HEAD_REF=${WZ_GITHUB_HEAD_REF}" >> $GITHUB_ENV
echo "WZ_GITHUB_BASE_REF=${WZ_GITHUB_BASE_REF}" >> $GITHUB_ENV
