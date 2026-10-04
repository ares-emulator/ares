# Accessing Structures

While SLJIT does not support record types (structs and classes) directly, it is still very easy to work with them. Assuming you have in `S0` the address to a `struct point` defined as follows:

```c
struct point_st {
	sljit_sw x;
	sljit_s32 y;
	sljit_s16 z;
	sljit_s8 d;
};
```

To move member `y` into `R0`, you can use the `SLJIT_MEM1` addressing mode, which allows us to specify an offset. To obtain the offset of `y` in `point_st`, you can use the handy `SLJIT_OFFSETOF` macro like so:

```c
sljit_emit_op1(C, SLJIT_MOV_S32, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_S0), SLJIT_OFFSETOF(struct point_st, y));
```

And always keep in mind to use the correctly typed variant of `SLJIT_MOV`!

*The complete source code of the example can be found [here](sources/struct_access.c).*
