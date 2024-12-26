#!/usr/bin/env sh
set -euo pipefail

cd "$(dirname "$0")"/.. || exit 1

otherArgs=()

if [ "${MACOS_CERTIFICATE_NAME:-}" != "" ]; then
  echo "Signing using certificate: ${MACOS_CERTIFICATE_NAME}"
  otherArgs+=("-DARES_CODESIGN_IDENTITY=${MACOS_CERTIFICATE_NAME}")
fi

if [ "${MACOS_NOTARIZATION_TEAMID:-}" != "" ]; then
  echo "Signing with team ID: ${MACOS_NOTARIZATION_TEAMID}"
  otherArgs+=("-DARES_CODESIGN_TEAM=${MACOS_NOTARIZATION_TEAMID}")
fi

cmake --preset macos "${@:-}" "${otherArgs:-}"

pushd build_macos

if ! command -v xcbeautify >/dev/null; then
    xcodebuild build -quiet -configuration RelWithDebInfo \
              DEBUG_INFORMATION_FORMAT="dwarf-with-dsym"
else
    xcodebuild -configuration RelWithDebInfo \
              DEBUG_INFORMATION_FORMAT="dwarf-with-dsym" \
              2>&1 | xcbeautify --renderer terminal
fi

open ./desktop-ui/RelWithDebInfo

popd
