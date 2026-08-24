#include "sljitLir.h"

#include <stdio.h>
#include <stdlib.h>

typedef sljit_sw (SLJIT_FUNC *func3_t)(sljit_sw a, sljit_sw b, sljit_sw c);

static int add3(sljit_sw a, sljit_sw b, sljit_sw c)
{
    void *code;
    sljit_uw len;
    func3_t func;

    /* Create a SLJIT compiler */
    struct sljit_compiler *C = sljit_create_compiler(NULL);

    /* Start a context (function prologue) */
    sljit_emit_enter(C,
        0,                       /* Options */
        SLJIT_ARGS3(W, W, W, W), /* 1 return value and 3 parameters of type sljit_sw */
        1,                       /* 1 scratch register used */
        3,                       /* 3 saved registers used */
        0);                      /* 0 bytes allocated for function local variables */

    /* The first argument of a function is stored in register SLJIT_S0, the 2nd in SLJIT_S1, etc. */
    /* R0 = first */
    sljit_emit_op1(C, SLJIT_MOV, SLJIT_R0, 0, SLJIT_S0, 0);

    /* R0 = R0 + second */
    sljit_emit_op2(C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_S1, 0);

    /* R0 = R0 + third */
    sljit_emit_op2(C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_S2, 0);

    /* This statement moves R0 to RETURN REG and returns */
    /* (in fact, R0 is the RETURN REG itself) */
    sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);

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
    return add3(4, 5, 6);
}
