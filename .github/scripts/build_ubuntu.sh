#!/usr/bin/env bash
set -euo pipefail

# configure
cmake --preset $TARGET_PRESET --debug-find

# change into the build directory
pushd build

ninja

popd
