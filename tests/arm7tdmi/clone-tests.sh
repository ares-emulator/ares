#!/usr/bin/env sh
set -euo pipefail

cd "$(dirname "$0")"
git clone --depth 1 https://github.com/SingleStepTests/ARM7TDMI.git tests

cd tests/v1
python3 transcode_json.py
