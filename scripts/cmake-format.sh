#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"/.. || exit 1

gersemi -i CMakeLists.txt ares cmake nall ruby hiro libco mia desktop-ui thirdparty/CMakeLists.txt
