#!/bin/bash
set -euo pipefail

if [ ! -d "MoltenVK" ]; then
	git clone https://github.com/KhronosGroup/MoltenVK.git
else
	git -C MoltenVK fetch
fi
git -C MoltenVK reset --hard "$(cat HEAD)"
pushd MoltenVK
./fetchDependencies --macos
make macos MVK_LOG_LEVEL_INFO=0 MVK_LOG_LEVEL_DEBUG=0
popd
cp MoltenVK/MoltenVK/dylib/macOS/libMoltenVK.dylib .
