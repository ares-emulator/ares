# Branching

Branching allows us to divert control flow. This can be useful to implement higher-level constructs such as conditionals and loops.

Branches, also referred to as *jumps*, come in two flavors:

- **Conditional:** only taken if a condition is met; otherwise, execution continues at the next instruction
- **Unconditional:** always taken

Jumps can be emitted via calls to `sljit_emit_cmp` (conditional) and `sljit_emit_jump` (unconditional). Both of these functions return a pointer to `struct sljit_jump`, which needs to be connected to a `struct sljit_label` (the jump's target) later on.

Labels can be emitted via calls to `sljit_emit_label` and connected to jumps via `sljit_set_label`. The whole mechanism is very similar to labels and gotos in C.

In order to map higher-level constructs such as conditionals and loops to SLJIT, it helps to think about them in terms of labels and (un)conditional gotos. As an example, take the following simple function body:

```c
if ((a & 1) == 0)
    return c;
return b;
```

Expressing the implicit control flow with lables and gotos yields:


```
    R0 = a & 1;
    if R0 == 0 then goto ret_c;
    R0 = b;
    goto out;
ret_c:
    R0 = c;
out:
    return R0;
```

This is also what higher-level language compilers do under the hood. The result can now easily be assembled with SLJIT:

```c
#include "sljitLir.h"

#include <stdio.h>
#include <stdlib.h>

typedef sljit_sw (SLJIT_FUNC *func3_t)(sljit_sw a, sljit_sw b, sljit_sw c);

static int branch(sljit_sw a, sljit_sw b, sljit_sw c)
{
	void *code;
	sljit_uw len;
	func3_t func;

	struct sljit_jump *ret_c;
	struct sljit_jump *out;

	/* Create a SLJIT compiler */
	struct sljit_compiler *C = sljit_create_compiler(NULL);

	/* 3 arg, 1 temp reg, 3 save reg */
	sljit_emit_enter(C, 0, SLJIT_ARGS3(W, W, W, W), 1, 3, 0);

	/* R0 = a & 1, S0 is argument a */
	sljit_emit_op2(C, SLJIT_AND, SLJIT_R0, 0, SLJIT_S0, 0, SLJIT_IMM, 1);

	/* if R0 == 0 then jump to ret_c, where is ret_c? we assign it later */
	ret_c = sljit_emit_cmp(C, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0);

	/* R0 = b, S1 is argument b */
	sljit_emit_op1(C, SLJIT_MOV, SLJIT_RETURN_REG, 0, SLJIT_S1, 0);

	/* jump to out */
	out = sljit_emit_jump(C, SLJIT_JUMP);

	/* here is the 'ret_c' should jump, we emit a label and set it to ret_c */
	sljit_set_label(ret_c, sljit_emit_label(C));

	/* R0 = c, S2 is argument c */
	sljit_emit_op1(C, SLJIT_MOV, SLJIT_RETURN_REG, 0, SLJIT_S2, 0);

	/* here is the 'out' should jump */
	sljit_set_label(out, sljit_emit_label(C));

	/* end of function */
	sljit_emit_return(C, SLJIT_MOV, SLJIT_RETURN_REG, 0);

	/* Generate machine code */
	code = sljit_generate_code(C, 0, NULL);
	len = sljit_get_generated_code_size(C);

	/* Execute code */
	func = (func3_t)code;
	printf("func return %ld\n", (long)func(a, b, c));

	/* dump_code(code, len); */

	/* Clean up */
	sljit_free_compiler(C);
	sljit_free_code(code, NULL);
	return 0;
}

int main()
{
	return branch(4, 5, 6);
}
```

*The complete source code of the example can be found [here](sources/branch.c).*

Building on these basic techniques, you can further use branches to generate a loop. So, given the following function body:

```c
i = 0;
ret = 0;
for (i = 0; i < a; ++i) {
    ret += b;
}
return ret;
```

You can again make the control flow explicit using labels and gotos:


```
    i = 0;
    ret = 0;
loopstart:
    if i >= a then goto out;
    ret += b
    goto loopstart;
out:
    return ret;
```

And then use that to assemble the function with SLJIT:

```c
#include "sljitLir.h"

#include <stdio.h>
#include <stdlib.h>

typedef sljit_sw (SLJIT_FUNC *func2_t)(sljit_sw a, sljit_sw b);

static int loop(sljit_sw a, sljit_sw b)
{
	void *code;
	sljit_uw len;
	func2_t func;

	struct sljit_label *loopstart;
	struct sljit_jump *out;

	/* Create a SLJIT compiler */
	struct sljit_compiler *C = sljit_create_compiler(NULL);

	/* 2 arg, 2 temp reg, 2 saved reg */
	sljit_emit_enter(C, 0, SLJIT_ARGS2(W, W, W), 2, 2, 0);

	/* R0 = 0 */
	sljit_emit_op2(C, SLJIT_XOR, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R1, 0);
	/* RET = 0 */
	sljit_emit_op1(C, SLJIT_MOV, SLJIT_RETURN_REG, 0, SLJIT_IMM, 0);
	/* loopstart: */
	loopstart = sljit_emit_label(C);
	/* R1 >= a --> jump out */
	out = sljit_emit_cmp(C, SLJIT_GREATER_EQUAL, SLJIT_R1, 0, SLJIT_S0, 0);
	/* RET += b */
	sljit_emit_op2(C, SLJIT_ADD, SLJIT_RETURN_REG, 0, SLJIT_RETURN_REG, 0, SLJIT_S1, 0);
	/* R1 += 1 */
	sljit_emit_op2(C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 1);
	/* jump loopstart */
	sljit_set_label(sljit_emit_jump(C, SLJIT_JUMP), loopstart);
	/* out: */
	sljit_set_label(out, sljit_emit_label(C));

	/* return RET */
	sljit_emit_return(C, SLJIT_MOV, SLJIT_RETURN_REG, 0);

	/* Generate machine code */
	code = sljit_generate_code(C, 0, NULL);
	len = sljit_get_generated_code_size(C);

	/* Execute code */
	func = (func2_t)code;
	printf("func return %ld\n", (long)func(a, b));

	/* dump_code(code, len); */

	/* Clean up */
	sljit_free_compiler(C);
	sljit_free_code(code, NULL);
	return 0;
}

int main()
{
	return loop(4, 5);
}
```

The other conditionals and loops can be achieved very similarly.

*The complete source code of the example can be found [here](sources/loop.c).*
