#!/bin/bash
set -euo pipefail

if [ ! -d "SDL" ]; then
    git clone https://github.com/libsdl-org/SDL.git -b SDL2
else
    git -C SDL fetch
fi
git -C SDL reset --hard "$(git -C SDL rev-parse HEAD)"
mkdir -p SDL/build
pushd SDL/build

SDLARGS=()
SDLARGS+=("-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64")
SDLARGS+=("-DCMAKE_OSX_DEPLOYMENT_TARGET=10.13")

cmake .. "${SDLARGS[@]}"
cmake --build .

popd

cp SDL/build/libSDL2-2.0.0.dylib .
