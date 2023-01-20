#!/bin/sh

# Change to the root directory, or exit on failure
cd "$(dirname "$0")"/.. || exit 1

# This assumes that remotes with these names are already configured
git subtree push --prefix=nall nall master
git subtree push --prefix=ruby ruby master
git subtree push --prefix=hiro hiro master
git subtree push --prefix=libco libco master