---
sidebar_position: 1
description: Setting up SLJIT.
---

# Setup

## Prerequisites

To compile SLJIT, you need a C/C++ compiler with support for the `C99` standard.

In case you want to build [the tests](#building-the-tests) or [the examples](#building-the-examples), you additionally need [GNU Make](https://www.gnu.org/software/make/). If you are using the MSVC toolchain on Windows (where GNU Make is not available), you can use [CMake](https://cmake.org/) to build the tests instead.

## Using SLJIT in Your Project

Place the contents of the [`sljit_src` folder](https://github.com/zherczeg/sljit/tree/master/sljit_src) in a suitable location in your project's source directory.

SLJIT can be used in one of two ways:

### SLJIT as a Library

Compile `sljitLir.c` as a separate translation unit. Be sure to add `sljit_src` to the list of include directories, e.g. by specifying `-Ipath/to/sljit` when compiling with GCC / Clang.

To use SLJIT, include `sljitLir.h` and link against `sljitLir.o`.

### SLJIT All-in-one

In case you want to avoid exposing SLJIT's interface to other translation units, you can also embed SLJIT as a hidden implementation detail. To do so, define `SLJIT_CONFIG_STATIC` before including `sljitLir.c` (yes, the C file):

```c title="hidden.c"
#define SLJIT_CONFIG_STATIC 1
#include "sljitLir.c"

// SLJIT can be used in hidden.c, but not in other translation units
```

This technique can also be used to generate code for multiple target architectures:

```c title="x86-32.c"
#define SLJIT_CONFIG_STATIC 1
#define SLJIT_CONFIG_X86_32 1
#include "sljitLir.c"

// Generate code for x86 32-bit
```

```c title="x86-64.c"
#define SLJIT_CONFIG_STATIC 1
#define SLJIT_CONFIG_X86_64 1
#include "sljitLir.c"

// Generate code for x86 64-bit
```

Both `x86-32.c` and `x86-64.c` can be linked together into the same binary / library without running into issues due to clashing symbols.

## Building the Tests

### Using GNU Make

Navigate to the root directory of the SLJIT repository and build the default target with GNU Make:

```bash
make
```

The test executable `sljit_test` can be found in the `bin` directory. To run the tests, simply execute it.

### Using CMake

> [!NOTE]
> CMake support is currently only intended as a crutch for Windows systems where GNU Make is not available.

To build the tests on Windows using MSVC and NMake, do the following:

1. Open a suitable [developer command prompt](https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell?view=vs-2022) and navigate into the root directory of the SLJIT repository
2. Execute the following:
    ```bash
    cmake -B bin -G "NMake Makefiles"
    cmake --build bin
    ```

The test executable `sljit_test.exe` can be found in the `bin` directory.

## Building the Examples

> [!NOTE]
> You cannot currently build the examples on Windows using MSVC / CMake.

Build the `examples` target with GNU Make:

```bash
make examples
```

The different example executables can be found in the `bin` directory.
