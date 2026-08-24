---
sidebar_position: 3
description: The inner workings of SLJIT.
---

# Architecture

## Low-level Intermediate Representation

Defining a LIR which provides wide range of optimization opportunities and still can be efficiently translated to machine code on all CPUs is the biggest challenge of this project.
Those instruction forms and features which are supported on many (but not necessarily on all) architectures are carefully selected and a LIR is created from them.
These features are also emulated by the remaining architectures with low overhead.
For example, SLJIT supports various memory addressing modes and setting status register bits.

## Generic CPU Model

The CPU has:
  - integer registers, which can store either an `int32_t` (4 byte) or `intptr_t` (4 or 8 byte) value
  - floating point registers, which can store either a single (4 byte) or double (8 byte) precision value
  - boolean status flags

Some platforms additionally have support for vector registers, which may alias the floating point registers.

### Integer Registers

The most important rule is: when a source operand of an instruction is a register, the data type of the register must match the data type expected by an instruction.

For example, the following code snippet is a valid instruction sequence:

```c
sljit_emit_op1(compiler, SLJIT_MOV32,
    SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R1), 0);
// An int32_t value is loaded into SLJIT_R0
sljit_emit_op1(compiler, SLJIT_REV32,
    SLJIT_R0, 0, SLJIT_R0, 0);
// the int32_t value in SLJIT_R0 is byte swapped
// and the type of the result is still int32_t
```

The next code snippet is not allowed:

```c
sljit_emit_op1(compiler, SLJIT_MOV,
    SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R1), 0);
// An intptr_t value is loaded into SLJIT_R0
sljit_emit_op1(compiler, SLJIT_REV32,
    SLJIT_R0, 0, SLJIT_R0, 0);
// The result of the instruction is undefined.
// Even crash is possible for some instructions
// (e.g. on MIPS-64).
```

However, it is always allowed to overwrite a register regardless of its previous value:

```c
sljit_emit_op1(compiler, SLJIT_MOV,
    SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R1), 0);
// An intptr_t value is loaded into SLJIT_R0
sljit_emit_op1(compiler, SLJIT_MOV32,
    SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R2), 0);
// From now on SLJIT_R0 contains an int32_t
// value. The previous value is discarded.
```

Type conversion instructions are provided to convert an `int32_t` value to an `intptr_t` value and vice versa.
In certain architectures these conversions are *nops* (no instructions are emitted).

#### Memory Accessing

Register arguments of `SLJIT_MEM1` / `SLJIT_MEM2` addressing modes must contain `intptr_t` data.

#### Signed / Unsigned Values

Most operations are executed in the same way regardless of if the value is signed or unsigned.
These operations have only one instruction form (e.g. `SLJIT_ADD` / `SLJIT_MUL`).
Instructions where the result depends on the sign have two forms (e.g. integer division, long multiply).

### Floating Point Registers

Floating point registers can either contain a single or double precision value.
Similar to integer registers, the data type of the value stored in a source register must match the data type expected by the instruction.
Otherwise the result is undefined (even a crash is possible).

#### Rounding

Similar to standard C, floating point computation results are rounded toward zero.

### Boolean Status Flags

Conditional branches usually depend on the value of CPU status flags.
These status flags are boolean values and can be set by certain instructions.

For better compatibility with CPUs without flags, SLJIT exposes only two such flags:
  - The **zero** (or equal) flag is set if `SLJIT_SET_Z` was specified and the result is zero.
  - The **variable** flag's meaning depends on the arithmetic operation that sets it, as well as the type of flag requested. For example, `SLJIT_ADD` supports setting the `Z`, `CARRY` and `OVERFLOW` flags. The latter two will however both map to the variable flag and can thus not be requested at the same time.

Be aware that different operations give different guarantees on what state the flags will be in afterwards.
Also, not all flags are supported by all operations.
For more details, refer to [`sljitLir.h`](https://github.com/zherczeg/sljit/blob/master/sljit_src/sljitLir.h).

## Complex Instructions

We noticed that introducing complex instructions for common tasks can improve performance.
For example, compare and branch instruction sequences can be optimized if certain conditions apply, but these conditions depend on the target CPU.
SLJIT can do these optimizations, but it needs to understand the "purpose" of the generated code.
Static instruction analysis has a large performance overhead however, so we chose another approach: we introduced complex instruction forms for certain non-atomic tasks.
SLJIT can optimize these instructions more efficiently since their purpose is known to the compiler.
These complex instruction forms can often be assembled from other SLJIT instructions, but we recommended to use them since the compiler can optimize them on certain CPUs.

## Generating Functions

SLJIT is often used for generating function bodies which are
called from C.
SLJIT provides two complex instructions for generating function entry and return: `sljit_emit_enter` and `sljit_emit_return`.
The `sljit_emit_enter` function also initializes the compilation context, which specifies the current register mapping, local space size and other configurations.
The `sljit_set_context` function can also set this context without emitting any machine instructions.

This context is important since it affects the compiler, so the first instruction after a compiler is created must be either `sljit_emit_enter` or `sljit_set_context`.
The context can be changed by calling `sljit_emit_enter` or `sljit_set_context` again.

## Types

SLJIT defines several types for representing data on the target platform, often times in both a signed and unsigned variant:
- Integers of varying size: `sljit_s8` / `sljit_u8`, `sljit_s16` / `sljit_u16`, `sljit_s32` / `sljit_u32`
- Machine word, capable of holding a pointer: `sljit_sw` / `sljit_uw`, `sljit_sp` / `sljit_up`
- Floating point types: `f32`, `f64`

It is recommended to use these types instead of the default C types such as `long`, as it improves both the readability and the portability of the code.
