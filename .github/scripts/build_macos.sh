#!/usr/bin/env sh
set -euo pipefail

# configure
cmake --preset $TARGET_PRESET

# change into the build directory
pushd build

# build; pipe output to render warnings neatly
xcodebuild -configuration RelWithDebInfo \
            DEBUG_INFORMATION_FORMAT="dwarf-with-dsym" \
            -parallelizeTargets \
            2>&1 | xcbeautify --renderer github-actions

popd

# package debug symbols

# move dependency dSYMs to build output directory
ditto .deps/ares-deps-macos-universal/lib/*.dSYM build/desktop-ui/RelWithDebInfo

mkdir build/dsyms

# ares.app.dSYM already exists alongside ares.app; package it with the rest
mv build/desktop-ui/RelWithDebInfo/*.dSYM build/dSYMs/
