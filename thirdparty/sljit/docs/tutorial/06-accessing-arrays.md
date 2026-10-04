# Accessing Arrays

Accessing arrays works similar to accessing structures. In case you are dealing with arrays of items of at most 8 bytes, you can utilize the shift of SLJIT's `SLJIT_MEM2` addressing mode. For example, assuming you have the base address of an array of `sljit_sw`s in `S0` and want to access the `S2`-th item:

```c
sljit_sw s0[];
r0 = s0[s2];
```

You can simply use `SLJIT_WORD_SHIFT` to correctly account for the size of each item:

```c
sljit_emit_op1(C, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM2(SLJIT_S0, SLJIT_S2), SLJIT_WORD_SHIFT);
```

In case the size of an item cannot be represented with `SLJIT_MEM2`'s shift, you can still fall back to calculating the offset manually and passing it into `SLJIT_MEM1`. This can be combined with structure access as well. Suppose you want to assemble the following:

```c
struct point_st {
	sljit_sw x;
	sljit_s32 y;
	sljit_s16 z;
	sljit_s8 d;
};

point_st s0[];
r0 = s0[s2].x;
```

As `point_st` is larger than 8 bytes, you need to convert the array index to a byte offset beforehand and store it in a temporary:

```c
/* calculate the array index: R2 = S2 * sizeof(point_st) */
sljit_emit_op2(C, SLJIT_MUL, SLJIT_R1, 0, SLJIT_S2, 0, SLJIT_IMM, sizeof(struct point_st));
/* access the array item: R0 = S0[R2] */
sljit_emit_op1(C, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_S0), SLJIT_R2);
```

Above example relies on the fact that `x` is the first member of `point_st` and thus shares its address with the surrounding struct. In case you want to access a different member, you need to further take its offset into account.

*The complete source code of the example can be found [here](sources/array_access.c).*
