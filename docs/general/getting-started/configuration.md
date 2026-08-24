---
sidebar_position: 2
description: Configuring SLJIT.
---

# Configuration

SLJIT's behavior can be controlled by a series of defines, described in `sljitConfig.h`.

| Define | Enabled By Default| Description |
| --- | --- | --- |
| `SLJIT_DEBUG` | ✅ | Enable assertions. Should be disabled in release mode. |
| `SLJIT_VERBOSE` | ✅ | When this macro is enabled, the `sljit_compiler_verbose` function can be used to dump SLJIT instructions. Otherwise this function is not available. Should be disabled in release mode. |
| `SLJIT_SINGLE_THREADED` | ❌ | Single threaded programs can define this flag which eliminates the `pthread` dependency. |
