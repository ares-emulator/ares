#!/bin/sh

# Updates the Arcade ROM database to the latest version from MAME.
# This script requires MAME to be in your PATH, or you can pass the path to MAME as an argument.
# mame2bml is also required to be present and up-to-date.

CORES="sega/sg1000a.cpp nintendo/aleck64.cpp" # Mame arcade systems to include
SOFTWARE="neogeo:neogeo:Neo Geo" # Mame software list to include (format: "mame_name:ares_name:database_name") comma separated

PATH=$PATH:$(pwd)/tools/mame2bml/out
SCRIPT_CWD=$(pwd)

if ! command -v mame2bml &> /dev/null
then
    echo "mame2bml could not be found. Please build it first."
    exit 1
fi

if [ -z "$1" ]
then
    if ! command -v mame &> /dev/null
    then
        echo "MAME could not be found. Please install it first, or pass the path to MAME as an argument."
        exit 1
    fi
else
    PATH=$PATH:$1
fi

MAME_CMD=$(command -v mame)
MAME_DIR=$(dirname $MAME_CMD)

# Determine MAME hash directory
if [ -z "$HASH_PATH" ]; then
    if [ -d "$MAME_DIR/hash" ]; then
        HASH_PATH="$MAME_DIR/hash"
    elif [ -d "/usr/share/games/mame/hash" ]; then
        HASH_PATH="/usr/share/games/mame/hash"
    else
        echo "Could not determine the MAME hash directory. Please set HASH_PATH manually and re-run the script."
        exit 1
    fi
fi

# Extract mame.xml
echo "Extracting MAME XML..."
mame -listxml > mame.xml

echo "Converting mame.xml to BML..."
mame2bml mame.xml mia/Database/Arcade.bml $CORES
rm mame.xml

IFS=',' read -r -a software_array <<< "$SOFTWARE"
for i in "${software_array[@]}"
do
    IFS=':' read -r -a pair <<< "$i"
    echo "Converting ${pair[0]}.xml to BML..."
    mame2bml "$HASH_PATH/${pair[0]}.xml" "mia/Database/${pair[2]}.bml" ${pair[1]}
done

echo "Done."