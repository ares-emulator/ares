#!/usr/bin/env sh
set -euo pipefail

cd "$(dirname "$0")"
git clone --depth 1 https://github.com/SingleStepTests/m68000.git tests

cd tests
python3 decode.py
