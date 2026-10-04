---
sidebar_position: 1
description: Introduction to SLJIT.
---

# Introduction

## Overview

SLJIT is a stack-less, low-level and platform-independent JIT compiler.
Perhaps *platform-independent assembler* describes it even better.
Its core design principle is that it does not try to be smarter than the developer.
This principle is achieved by providing control over the generated machine code like you would have in other assembly languages.
Unlike other assembly languages however, SLJIT utilizes a *low-level intermediate representation* (LIR) that is platform-independent, which greatly improves portability.

SLJIT tries to strike a good balance between performance and maintainability.
The LIR code can be compiled to many CPU architectures, and the performance of the generated code is very close to code written in native assembly languages.
Although SLJIT does not support higher level features such as automatic register allocation, it can be a code generation backend for other JIT compiler libraries.
Developing these intermediate libraries on top of SLJIT takes far less time, because they only need to support a single backend.

If you want to jump right in and see SLJIT in action, take a look at the [Tutorial](../tutorial/01-overview.md).


## Features

- Supports a variety of [target architectures](#supported-platforms)
- Supports a large number of operations
    - Self-modifying code
    - Tail calls
    - Fast calls (not ABI compatible)
    - Byte order reverse (endianness switching)
    - Unaligned memory accesses
    - SIMD[^1]
    - Atomic operations[^1]
- Allows direct access to registers (both integer and floating point)
- Supports stack space allocation for function local variables
- Supports [all-in-one](getting-started/setup.md#sljit-all-in-one) compilation
    - Allows SLJIT's API to be completely hidden from external use
- Allows serializing the compiler into a byte buffer
    - Useful for ahead-of-time (AOT) compilation
    - Code generation can be resumed after deserialization (partial AOT compilation)

## Supported Platforms

| Platform | 64-bit | 32-bit |
| --- | --- | --- |
| `x86` | ✅ | ✅ |
| `ARM` | ✅ | ✅[^2] |
| `RISC-V` | ✅ | ✅ |
| `s390x` | ✅ | ❌ |
| `PowerPC` | ✅ | ✅ |
| `LoongArch` | ✅ | ❌ |
| `MIPS` | ✅[^3] | ✅[^3] |

[^1]: only on certain platforms
[^2]: ARM-v6, ARM-v7 and Thumb2 instruction sets
[^3]: III, R1
