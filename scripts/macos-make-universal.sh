#!/bin/bash
set -euo pipefail

if ! command -v lipo >/dev/null; then
    echo "Command lipo not found; please install XCode"
    exit 1
fi

if ! command -v gmake >/dev/null; then
    echo "Please install make via Homebrew (brew install make)"
    exit 1
fi

# Change to parent directory (top-level)
cd "$(dirname "$0")"/.. || exit 1

echo "Building for amd64..."
gmake arch=amd64 object.path=obj-amd64 output.path=out-amd64 "$@"

echo "Building for arm64..."
gmake arch=arm64 object.path=obj-arm64 output.path=out-arm64 "$@"

echo "Assembling universal binary"
rm -rf desktop-ui/out
cp -a desktop-ui/out-amd64 desktop-ui/out
lipo -create -output desktop-ui/out/ares.app/Contents/MacOS/ares \
    desktop-ui/out-amd64/ares.app/Contents/MacOS/ares \
    desktop-ui/out-arm64/ares.app/Contents/MacOS/ares

if [ "${MACOS_KEYCHAIN_PASSWORD:-}" != "" ]; then
    security unlock-keychain -p "$MACOS_KEYCHAIN_PASSWORD" "$MACOS_KEYCHAIN_NAME"
fi

if [ "${MACOS_CERTIFICATE_NAME:-}" == "" ]; then
    echo "Signing using self-signed"
    ENTITLEMENTS=desktop-ui/resource/ares.selfsigned.entitlements
else
    echo "Signing using certificate: ${MACOS_CERTIFICATE_NAME}"
    ENTITLEMENTS=desktop-ui/resource/ares.entitlements
fi
codesign --force --deep --options runtime --entitlements "${ENTITLEMENTS}" --sign "${MACOS_CERTIFICATE_NAME:--}" desktop-ui/out/ares.app
