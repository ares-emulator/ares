---
sidebar_position: 2
description: Generating machine code from bytecode with SLJIT.
---

# Bytecode Interpreters

SLJIT's approach is very effective for bytecode interpreters, since their machine-independent bytecode (middle-level representation) typically contains instructions which either can be easly translated to machine code, or which are not worth to translate to machine code in the first place.

The following table gives a possible mapping for a subset of bytecode instructions targeting a stack machine:

| Bytecode Instruction | Effect | Mapping to SLJIT |
| --- | --- | --- |
| `pop` | Pop a value from the stack. | ✅ Easy to implement, just decrement the stack pointer. |
| `add` | Pop two values from the stack. Add them and push the result onto the stack. | ✅ Easy to implement if `value` is limited to a single type (e.g. integers). More complex types may require additional checks and dispatching. In those cases, it may be beneficial to just JIT the "fast path" (e.g. the integer case). |
| `resolve` | Pop a value representing the identifier of a variable from the stack. Lookup the variable by its identifier. If found, push its address onto the stack. Otherwise push `NULL` onto the stack. | ❌ Not suitable for JIT compilation. Call a native C/C++ helper instead. |
