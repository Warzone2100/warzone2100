#!/bin/bash
# This requires a bunch of environment variables to be set. See the CI workflow

echo "::group::flatpak-builder"
flatpak-builder --repo=${WZ_FLATPAK_LOCAL_REPO_NAME} --disable-rofiles-fuse --force-clean --default-branch=${WZ_FLATPAK_BRANCH} --mirror-screenshots-url=${WZ_FLATPAK_MIRROR_SCREENSHOTS_URL} "${WZ_FLATPAK_BUILD_DIR}" ${WZ_FLATPAK_MANIFEST_PATH}
echo "::endgroup::"

if [[ "$WZ_FLATPAK_TARGET_ARCH" != "$WZ_FLATPAK_BUILD_ARCH" ]]; then
  SRC_LOCAL_REPO_NAME="${WZ_FLATPAK_LOCAL_REPO_NAME}"

  # Create a new repository containing the commits for the cross target arch
  echo "::group::Creating new local repo for target arch: ${WZ_FLATPAK_LOCAL_REPO_NAME}"
  WZ_FLATPAK_LOCAL_REPO_NAME="${WZ_FLATPAK_TARGET_ARCH}-repo"
  ostree init --mode archive-z2 --repo=${WZ_FLATPAK_LOCAL_REPO_NAME}
  echo "::endgroup::"
  
  echo "::group::Rename commits to new target arch repo"
  for i in app/${WZ_FLATPAK_APPID} \
           runtime/${WZ_FLATPAK_APPID}.Debug
  do
     # Rename the commits to target arch
     echo "Processing: --src-ref=${i}/${WZ_FLATPAK_BUILD_ARCH}/${WZ_FLATPAK_BRANCH}"
     flatpak build-commit-from --src-ref=${i}/${WZ_FLATPAK_BUILD_ARCH}/${WZ_FLATPAK_BRANCH} --src-repo=${SRC_LOCAL_REPO_NAME} \
        ${WZ_FLATPAK_LOCAL_REPO_NAME} ${i}/${WZ_FLATPAK_TARGET_ARCH}/${WZ_FLATPAK_BRANCH}
  done
  echo "::endgroup::"
fi

echo "::group::flatpak build-bundle"
flatpak build-bundle --arch=${WZ_FLATPAK_TARGET_ARCH} ${WZ_FLATPAK_LOCAL_REPO_NAME} "${WZ_FLATPAK_BUNDLE}" --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo ${WZ_FLATPAK_APPID} ${WZ_FLATPAK_BRANCH}
echo "::endgroup::"

echo "Generated .flatpak: \"${WZ_FLATPAK_BUNDLE}\""
echo "  -> SHA512: $(sha512sum "${WZ_FLATPAK_BUNDLE}")"
echo "  -> Size (bytes): $(stat -c %s "${WZ_FLATPAK_BUNDLE}")"

WZ_FLATPAK_BUILD_PATH="${WZ_FLATPAK_BUILD_DIR}/files/share"

echo "::group::Validating: appdata/${WZ_FLATPAK_APPID}.appdata.xml"
appstream-util validate ${WZ_FLATPAK_BUILD_PATH}/appdata/${WZ_FLATPAK_APPID}.appdata.xml
echo "::endgroup::"

echo "::group::Verify icon and metadata in app-info"
test -f "${WZ_FLATPAK_BUILD_PATH}/app-info/icons/flatpak/128x128/${WZ_FLATPAK_APPID}.png" || { echo "Missing 128x128 icon in app-info" ; exit 1; }
test -f "${WZ_FLATPAK_BUILD_PATH}/app-info/xmls/${WZ_FLATPAK_APPID}.xml.gz" || { echo "Missing ${WZ_FLATPAK_APPID}.xml.gz in app-info" ; exit 1; }
echo "::endgroup::"

echo "::group::Commit screenshots to the OSTree repository"
ostree commit --repo=${WZ_FLATPAK_LOCAL_REPO_NAME} --canonical-permissions --branch=screenshots/${WZ_FLATPAK_TARGET_ARCH} "${WZ_FLATPAK_BUILD_DIR}/screenshots"
echo "::endgroup::"
