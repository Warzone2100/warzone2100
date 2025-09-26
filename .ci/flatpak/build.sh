#!/bin/bash
# This requires a bunch of environment variables to be set. See the CI workflow

USE_BUILDER_FLATPAK=true

if [ "${USE_BUILDER_FLATPAK}" = true ]; then
  FLATPAK_BUILDER_CMD="flatpak run --filesystem=/tmp org.flatpak.Builder"
  FLATPAK_CMD="flatpak run --filesystem=/tmp --command=flatpak org.flatpak.Builder"
  OSTREE_CMD="flatpak run --filesystem=/tmp --command=ostree org.flatpak.Builder"
  APPSTREAMCLI_CMD="flatpak run --filesystem=/tmp --command=appstreamcli org.flatpak.Builder"
else
  FLATPAK_BUILDER_CMD="flatpak-builder"
  FLATPAK_CMD="flatpak"
  OSTREE_CMD="ostree"
  APPSTREAMCLI_CMD="appstreamcli"
fi

echo "::group::flatpak-builder --version"
${FLATPAK_BUILDER_CMD} --version
echo "::endgroup::"
echo "::group::flatpak --version"
${FLATPAK_CMD} --version
echo "::endgroup::"
echo "::group::ostree --version"
${OSTREE_CMD} --version
echo "::endgroup::"
echo "::group::appstreamcli --version"
${APPSTREAMCLI_CMD} --version
echo "::endgroup::"

BUNDLE_SOURCES_ARG=""
if [ "${WZ_FLATPAK_BUNDLE_SOURCES}" == "true" ]; then
  BUNDLE_SOURCES_ARG="--bundle-sources"
fi

echo "::group::flatpak-builder"
${FLATPAK_BUILDER_CMD} --repo=${WZ_FLATPAK_LOCAL_REPO_NAME} --disable-rofiles-fuse --force-clean --default-branch=${WZ_FLATPAK_BRANCH} --mirror-screenshots-url=${WZ_FLATPAK_MIRROR_SCREENSHOTS_URL} --compose-url-policy=full ${BUNDLE_SOURCES_ARG} "${WZ_FLATPAK_BUILD_DIR}" ${WZ_FLATPAK_MANIFEST_PATH}
echo "::endgroup::"

if [[ "$WZ_FLATPAK_TARGET_ARCH" != "$WZ_FLATPAK_BUILD_ARCH" ]]; then
  SRC_LOCAL_REPO_NAME="${WZ_FLATPAK_LOCAL_REPO_NAME}"

  echo "::warn::Cross-compile support no longer included in flatpak yaml"

  # Create a new repository containing the commits for the cross target arch
  echo "::group::Creating new local repo for target arch: ${WZ_FLATPAK_LOCAL_REPO_NAME}"
  WZ_FLATPAK_LOCAL_REPO_NAME="${WZ_FLATPAK_TARGET_ARCH}-repo"
  ${OSTREE_CMD} init --mode archive-z2 --repo=${WZ_FLATPAK_LOCAL_REPO_NAME}
  echo "::endgroup::"
  
  echo "::group::Rename commits to new target arch repo"
  for i in app/${WZ_FLATPAK_APPID} \
           runtime/${WZ_FLATPAK_APPID}.Debug \
           runtime/${WZ_FLATPAK_APPID}.Sources
  do
     # Rename the commits to target arch
     echo "Processing: --src-ref=${i}/${WZ_FLATPAK_BUILD_ARCH}/${WZ_FLATPAK_BRANCH}"
     ${FLATPAK_CMD} build-commit-from --src-ref=${i}/${WZ_FLATPAK_BUILD_ARCH}/${WZ_FLATPAK_BRANCH} --src-repo=${SRC_LOCAL_REPO_NAME} \
        ${WZ_FLATPAK_LOCAL_REPO_NAME} ${i}/${WZ_FLATPAK_TARGET_ARCH}/${WZ_FLATPAK_BRANCH}
  done
  echo "::endgroup::"
fi

echo "::group::flatpak build-bundle"
${FLATPAK_CMD} build-bundle --arch=${WZ_FLATPAK_TARGET_ARCH} ${WZ_FLATPAK_LOCAL_REPO_NAME} "${WZ_FLATPAK_BUNDLE}" --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo ${WZ_FLATPAK_APPID} ${WZ_FLATPAK_BRANCH}
echo "::endgroup::"

echo "Generated .flatpak: \"${WZ_FLATPAK_BUNDLE}\""
echo "  -> SHA512: $(sha512sum "${WZ_FLATPAK_BUNDLE}")"
echo "  -> Size (bytes): $(stat -c %s "${WZ_FLATPAK_BUNDLE}")"

WZ_FLATPAK_BUILD_PATH="${WZ_FLATPAK_BUILD_DIR}/files/share"

WZ_FLATPAK_APPSTREAM_PATH="${WZ_FLATPAK_BUILD_PATH}/metainfo/${WZ_FLATPAK_APPID}.metainfo.xml"
if [ ! -f "${WZ_FLATPAK_APPSTREAM_PATH}" ]; then
  # try the old path?
  WZ_FLATPAK_APPSTREAM_PATH="${WZ_FLATPAK_BUILD_PATH}/appdata/${WZ_FLATPAK_APPID}.appdata.xml"
fi

if [ -f "${WZ_FLATPAK_APPSTREAM_PATH}" ]; then
  echo "::group::Validating appstream"
  echo "appstreamcli validate ${WZ_FLATPAK_APPSTREAM_PATH}"
  ${APPSTREAMCLI_CMD} validate "${WZ_FLATPAK_APPSTREAM_PATH}"
  echo "::endgroup::"
else
  echo "::warning ::Could not find appstream file to validate?"
fi

echo "::group::Verify icon and metadata in app-info"
test -f "${WZ_FLATPAK_BUILD_PATH}/app-info/icons/flatpak/128x128/${WZ_FLATPAK_APPID}.png" || { echo "Missing 128x128 icon in app-info" ; exit 1; }
test -f "${WZ_FLATPAK_BUILD_PATH}/app-info/xmls/${WZ_FLATPAK_APPID}.xml.gz" || { echo "Missing ${WZ_FLATPAK_APPID}.xml.gz in app-info" ; exit 1; }
echo "::endgroup::"

echo "::group::Check screenshots"
if [ ! -d "${WZ_FLATPAK_BUILD_PATH}/app-info/media" ]; then
  echo "::notice ::Screenshots not mirrored by flatpak-builder?"
else
  echo "Found screenshots:"
  find "${WZ_FLATPAK_BUILD_PATH}/app-info/media" -type f
fi
echo "::endgroup::"

echo "::group::Commit screenshots to the OSTree repository --branch=screenshots/${WZ_FLATPAK_TARGET_ARCH}"
if [ -d "${WZ_FLATPAK_BUILD_PATH}/app-info/media" ]; then
  ${OSTREE_CMD} commit --repo=${WZ_FLATPAK_LOCAL_REPO_NAME} --canonical-permissions --branch=screenshots/${WZ_FLATPAK_TARGET_ARCH} "${WZ_FLATPAK_BUILD_PATH}/app-info/media"
else
  echo "::warning ::Screenshots not added to OSTree repository"
fi
echo "::endgroup::"

# Output final WZ_FLATPAK_LOCAL_REPO_NAME in GitHub Actions
if [ "${GITHUB_ACTIONS}" == "true" ]; then
  echo "Running on GitHub Actions - outputting final WZ_FLATPAK_LOCAL_REPO_NAME"
  echo "WZ_FLATPAK_LOCAL_REPO_NAME=${WZ_FLATPAK_LOCAL_REPO_NAME}"
  echo "WZ_FLATPAK_LOCAL_REPO_NAME=${WZ_FLATPAK_LOCAL_REPO_NAME}" >> $GITHUB_OUTPUT
fi
