#!/bin/sh

# Updates the Arcade ROM database to the latest version from MAME.
# This script requires MAME to be in your PATH, or you can pass the path to MAME as an argument.
# mame2bml is also required to be present and up-to-date.

# Specify the mame drivers to include in the database
export CORES="sega/sg1000a.cpp nintendo/aleck64.cpp"

export PATH=$PATH:$(pwd)/tools/mame2bml/out
if ! command -v mame2bml &> /dev/null
then
    echo "mame2bml could not be found. Please build it first."
    exit 1
fi

if [ -z "$1" ]
then
    if ! command -v mame &> /dev/null
    then
        echo "MAME could not be found. Please install it first."
        exit 1
    fi
else
    export PATH=$PATH:$1
fi

# Extract mame.xml
mame -listxml > mame.xml

# Convert MAME XML to BML
mame2bml mame.xml mia/Database/Arcade.bml $CORES
rm mame.xml