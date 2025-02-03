#!/bin/sh
#
# Because git subtree doesn't provide an easy way to automatically merge changes
# from upstream, this shell script will do the job instead.
# If you don't have a POSIX-compatible shell on your system, feel free to use
# this as a reference for what commands to run, rather than running it directly.

# Change to the root directory, or exit with failure
# to prevent git subtree scrawling over some other repo.
cd "$(dirname "$0")"/.. || exit 1

# Merge changes from the upstream parallel-rdp repository.
git subtree pull --prefix=ares/n64/vulkan/parallel-rdp https://github.com/Themaister/parallel-rdp-standalone.git master --squash
git subtree pull --prefix=thirdparty/sljit https://github.com/zherczeg/sljit.git master --squash
git subtree pull --prefix=thirdparty/libchdr https://github.com/rtissera/libchdr master --squash
git subtree pull --prefix=thirdparty/librashader https://github.com/SnowflakePowered/librashader master --squash
git subtree pull --prefix=thirdparty/slang-shaders https://github.com/libretro/slang-shaders master --squash
git subtree pull --prefix=thirdparty/ymfm https://github.com/aaronsgiles/ymfm main --squash
