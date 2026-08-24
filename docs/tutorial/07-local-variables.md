# Local Variables

In some cases you are dealing with data that cannot be mapped to registers, either because it needs to be linear in memory (think of structs and arrays) or because it is simply too large to fit into the available ones.

While you could resort to dynamic memory allocation, e.g. through a call to external function `malloc` and the likes, there is a better way for temporary variables of fixed size: *function local storage*.

Local storage can be requested via the last parameter of `sljit_emit_enter` in bytes, which in the previous examples was always zero. Under the hood, SLJIT will allocate the requested amount as a contiguous block of memory in the function's stack frame.

Due to its otherwise stackless API, SLJIT hides most of these implementation details from you. There are two ways to access this local data:

- Special register `SLJIT_SP` acts as a pointer to the base of the local data. It can *only* be used in the form of `SLJIT_MEM1(SLJIT_SP)` to access local data at an offset.
- If instead you need the actual address, you can use `sljit_get_local_base`. For example, `sljit_get_local_base(C, SLJIT_R0, 0, 0x40)` will load the address of the local data area offset by `0x40` into `R0`.

*To see local variables in action, take a look at [this](sources/array_access.c) example.*
