#!/bin/bash
set -euo pipefail

pushd ../ruby/video/metal
xcrun -sdk macosx metal -o shaders.ir -c -gline-tables-only -frecord-sources Shaders.metal
xcrun -sdk macosx metallib -o shaders.metallib shaders.ir
popd
