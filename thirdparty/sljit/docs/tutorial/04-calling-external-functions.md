# Calling External Functions

To call an external function from JIT-ed code, simply invoke `sljit_emit_icall` and specify `SLJIT_CALL`, which represents the platform's default C calling convention. There are other calling conventions available as well for more advanced use cases.

Similar to `sljit_emit_enter`, `sljit_emit_icall` requires knowledge about the target function's signature. So, to call a function that takes an `sljit_sw` as its sole argument and returns an `sljit_sw`, you would specify its signature with `SLJIT_ARGS1(W, W)`. Integer arguments are passed in registers `R0`, `R1` and `R2` and the result (if present) is returned in `R0`.

To point SLJIT to the function you want to call, you can use `SLJIT_FUNC_ADDR` to pass its address as an immediate value.

So, to call a function `sljit_sw print_num(sljit_sw a)`, passing the value in `S2`, you could do the following:

```c
/* R0 = S2 */
sljit_emit_op1(C, SLJIT_MOV, SLJIT_R0, 0, SLJIT_S2, 0);
/* print_num(R0) */
sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, W), SLJIT_IMM, SLJIT_FUNC_ADDR(print_num));
```

*The complete source code of the example can be found [here](sources/func_call.c).*
