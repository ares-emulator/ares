#!/usr/bin/env bash
set -euo pipefail

# configure
cmake --preset $TARGET_PRESET

# clone the SDL repository
git clone https://github.com/libsdl-org/SDL.git
# change into the SDL directory
pushd SDL
mkdir build
# build SDL
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
# install SDL libs and include files to /usr/local
sudo cmake --install build --prefix=/usr/local
popd