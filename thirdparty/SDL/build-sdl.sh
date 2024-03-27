#!/bin/bash
set -euo pipefail

if [ ! -d "SDL" ]; then
    git clone https://github.com/libsdl-org/SDL.git -b SDL2
else
    git -C SDL fetch
fi
git -C SDL reset --hard "$(cat HEAD)"
mkdir -p SDL/build
pushd SDL/build

if [ -n "${GITHUB_ACTIONS+1}" ]; then
    sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
    echo "Set Xcode version in order to target macOS 10.11 when building SDL."
fi
cmake .. "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64" \
          -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11
cmake --build .

if [ -n "${GITHUB_ACTIONS+1}" ]; then
    sudo xcode-select -s /Applications/Xcode_14.2.app/Contents/Developer
    echo "Set Xcode to 14.2 to continue build."
fi
popd
cp SDL/build/libSDL2-2.0.0.dylib .
