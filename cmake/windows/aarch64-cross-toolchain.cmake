# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR arm64)

set(CMAKE_C_FLAGS --target=aarch64-w64-windows-gnu) # --sysroot=/clangarm64/ -resource-dir=/clangarm64/lib/clang/18)
set(CMAKE_CXX_FLAGS --target=aarch64-w64-windows-gnu) # --sysroot=/clangarm64/ -resource-dir=/clangarm64/lib/clang/18)

set(CMAKE_FIND_ROOT_PATH /clangarm64)

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
