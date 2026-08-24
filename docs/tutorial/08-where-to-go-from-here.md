# Where To Go From Here

You should now have a basic understanding of how to use SLJIT. In case things are still unclear or you want to know more about a certain topic, take a look at [`sljitLir.h`](https://github.com/zherczeg/sljit/blob/master/sljit_src/sljitLir.h).

Also, you may have noticed the commented out calls to `dump_code` throughout the sources in this tutorial. If you are interested in what assembly SLJIT actually generates, you can provide your own routine to do so (assuming you are on Linux `x86-32` or `x86-64`):

```c
static void dump_code(void *code, sljit_uw len)
{
    FILE *fp = fopen("/tmp/sljit_dump", "wb");
    if (!fp)
        return;
    fwrite(code, len, 1, fp);
    fclose(fp);
#if defined(SLJIT_CONFIG_X86_64)
    system("objdump -b binary -m l1om -D /tmp/sljit_dump");
#elif defined(SLJIT_CONFIG_X86_32)
    system("objdump -b binary -m i386 -D /tmp/sljit_dump");
#endif
}
```

And lastly, to see SLJIT used in a more complex scenario, take a look at [this](sources/brainfuck.c) [Brainfuck](https://en.wikipedia.org/wiki/Brainfuck) compiler.
