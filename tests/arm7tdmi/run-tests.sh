#!/usr/bin/env sh
set -euo pipefail

cd "$(dirname "$0")"
../../build/arm7tdmi/rundir/arm7tdmi "$@"
