#!/bin/sh

# Updates the Arcade ROM database to the latest version from MAME.
# This script requires MAME to be in your PATH, or you can pass the path to MAME as an argument.
# Requires Python 3.9 or higher

set -eu

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)

if [ -n "${1:-}" ]; then
    if [ -d "$1" ]; then
        mame="$1/mame"
        if [ ! -x "$mame" ]; then
            mame="$1/mame.exe"
        fi
    else
        mame="$1"
    fi
else
    mame=$(command -v mame || command -v mame.exe || true)
fi

if [ -z "$mame" ] || [ ! -f "$mame" ] || [ ! -x "$mame" ]; then
    echo "MAME could not be found. Please install it, or pass its executable or directory." >&2
    exit 1
fi
MAME_DIR=$(cd "$(dirname "$mame")" && pwd)
mame="$MAME_DIR/$(basename "$mame")"

if ! command -v python > /dev/null 2>&1; then
    echo "Python could not be found. Please install Python 3.9 or higher." >&2
    exit 1
fi
if ! python -c 'import sys; sys.exit(sys.version_info < (3, 9))'; then
    echo "Python 3.9 or higher is required." >&2
    exit 1
fi

# Determine MAME hash directory.
if [ -z "${HASH_PATH:-}" ]; then
    if [ -d "$MAME_DIR/hash" ]; then
        HASH_PATH="$MAME_DIR/hash"
    elif [ -d "/usr/share/games/mame/hash" ]; then
        HASH_PATH="/usr/share/games/mame/hash"
    elif [ -d "/usr/share/mame/hash" ]; then
        HASH_PATH="/usr/share/mame/hash"
    else
        echo "Could not determine the MAME hash directory. Please set HASH_PATH." >&2
        exit 1
    fi
fi
if [ ! -d "$HASH_PATH" ]; then
    echo "MAME hash directory does not exist: $HASH_PATH" >&2
    exit 1
fi
HASH_PATH=$(cd "$HASH_PATH" && pwd)

MAME_XML=$(mktemp "${TMPDIR:-/tmp}/ares-mame.xml.XXXXXXXX")
trap 'rm -f -- "$MAME_XML"' 0
trap 'exit 1' HUP INT TERM

echo "Extracting MAME XML..."
"$mame" -listxml > "$MAME_XML"

echo "Generating Arcade.bml..."
(cd "$REPO_ROOT" && python -m tools.mamedb arcade "$MAME_XML" "$HASH_PATH")

echo "Generating VsSystem.bml..."
(cd "$REPO_ROOT" && python -m tools.mamedb vsnes "$mame")

echo "Done."
