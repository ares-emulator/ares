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

SDLARGS=()
SDLARGS+=("-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64")
if [ -n "${GITHUB_ACTIONS+1}" ]; then
    if [ $ImageOS == "macos12" ]; then
        SDLARGS+=(-DCMAKE_OSX_DEPLOYMENT_TARGET=10.11)
        echo "Added 10.11 deployment target to SDL build arguments"
    elif [ $ImageOS == "macos14" ]; then
        SDLARGS+=(-DCMAKE_OSX_DEPLOYMENT_TARGET=10.13)
        echo "Added 10.13 deployment target to SDL build arguments"
    fi
fi

cmake .. "${SDLARGS[@]}"
cmake --build .

popd

cp SDL/build/libSDL2-2.0.0.dylib .
