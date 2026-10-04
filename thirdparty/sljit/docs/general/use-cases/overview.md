---
sidebar_position: 1
description: Determining when to use JIT compilation.
---

# Overview

## JIT Compilation in General

Just like any other technique in a programmer's toolbox, just-in-time (JIT) compilation should not be blindly used. It is not a magic wand that performs miracles without drawbacks. In general, JIT compilation can decrease the number of executed instructions, thus speeding up the execution. You can for example embed constants and constant pointers, eliminating certain loads. However, this typically comes with added complexity and memory consumption. On embedded systems, large amounts of JIT-ed code might decrease the efficiency of the instruction cache.

In the practical experience of SLJIT's author, JIT compilation is similar to code inlining (a static compiler optimization). It basically has the same disadvantages as well.

As a rule of thumb, you should focus on the most frequently executed parts of your program. When in doubt, profile your application to see where the hotspots are located. Never JIT compile generic (especially complex) algorithms. Their C/C++ counterparts usually perform better.

## SLJIT in Particular

**Advantages:**
- The execution can be continued from any LIR instruction. In other words, jumping into and out of the code is safe.
- The target of (conditional) jump and call instructions can be dynamically modified during the execution of the code.
- Constants can be modified during the execution of the code.
- Support for fast, non-ABI compliant function calls between JIT-ed functions. Requires only a few machine instructions and all registers are keeping their values.
- Move with update instructions, meaning the base register is updated before the actual load or store.

**Disadvantages:**
- Limited number of arguments (3 machine words) for ABI compatible function calls on all platforms.
