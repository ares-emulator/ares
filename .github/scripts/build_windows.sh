#!/usr/bin/env sh
set -euo pipefail

if [ "$CROSS_COMPILE" = true ]; then
  cmake --preset $NATIVE_PRESET -B build_native
  pushd build_native
  # build sourcery natively so it may be invoked during cross-compilation
  cmake --build . --target sourcery --config RelWithDebInfo
  popd
fi

cmake --preset $TARGET_PRESET
pushd build
cmake --build . --config RelWithDebInfo

if [ "$CROSS_COMPILE" = true ]; then
  cp ../.deps/ares-deps-windows-arm64/lib/*.pdb desktop-ui/rundir/
else
  cp ../.deps/ares-deps-windows-x64/lib/*.pdb desktop-ui/rundir/
fi

mkdir PDBs
mv desktop-ui/rundir/*.pdb PDBs/
popd
