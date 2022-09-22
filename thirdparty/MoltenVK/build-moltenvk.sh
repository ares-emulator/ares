#!/bin/bash
set -euo pipefail

if [ -d "MoltenVK" ]; then
	git -C MoltenVK pull
else
	git clone https://github.com/KhronosGroup/MoltenVK.git
fi
pushd MoltenVK
./fetchDependencies --macos
make macos MVK_LOG_LEVEL_INFO=0 MVK_LOG_LEVEL_DEBUG=0
popd
cp MoltenVK/MoltenVK/dylib/macOS/libMoltenVK.dylib .
git -C MoltenVK rev-parse HEAD >HEAD
