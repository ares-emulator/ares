---
sidebar_position: 3
description: Comparing the performance of different regex engines.
---

# Performance Comparison

> [!NOTE]
> This page was last updated in 2015.

Processing text or raw byte-sequences are among the common tasks performed by most software tools.
These tasks usually involve pattern matching algorithms, and the most popular tool for thjs purpose is regular expressions.
Regular expressions have evolved a lot since Kleene defined the regular sets in the 1950's.
Today we have several widely used regular expression engines which have different [features](https://en.wikipedia.org/wiki/Comparison_of_regular_expression_engines).
This makes any performance comparison a difficult task, since a faster engine is not necessarily better.
Depending on the use case it might be enough to know whether a POSIX compatible regular expression matches a given line, without needing the actual position of the match (grep utility).
Other use cases however require the position of capturing brackets, unicode support, conditional and atomic block support, just to name a few.
The former case needs a less sophisticated algorithm, which is likely be much faster than the latter, but again, that does not mean the former is better.
More about these engine types can be found [here](regular-expression-engine-types.md).

## Participants

The following popular engines were chosen:
- PCRE2 10.10
- tre 0.8.0
- Oniguruma 5.9.6
- re2 by Google
- PCRE2 10.10 with SLJIT support

Before anyone jumps to conclusions, I should note the following:
- The engines were not fine tuned (because of my lack of knowledge about their internal workings). I just compiled them with the default options. I know enabling or disabling some features can heavily affect the results. If you feel that you have a better configuration, just drop me an email and I will update the results (hzmester(at)freemail(dot)hu).
- The regular expression engines are compiled with `-O3` to allow the best performance.
- This comparison page was inspired by the work of John Maddock (see his own regex comparison [here](http://www.boost.org/doc/libs/1_41_0/libs/regex/doc/gcc-performance.html)). The input is also the same he used before: [mtent12.zip](http://www.gutenberg.org/files/3200/old/mtent12.zip). It is a text file (e-book) of roughly 20 MB.
- Only common patterns are selected. They are not pathological cases nor do they use any PERL specific features. The comparison was caseful.

The testing environment can be downloaded from [here](https://zherczeg.github.io/sljit/regex-test.tgz). Just extract, type `make`. Download the [mtent12.zip](http://www.gutenberg.org/files/3200/old/mtent12.zip), rename the `mtent12.txt` inside to `mark.txt` and run `runtest`.

## Results

### Linux x86-64

- **CPU:** Intel Xeon 2.67 GHz
- **Compiler:** GCC 4.8.2

| Regular Expression | PCRE | PCRE-DFA | TRE | Oniguruma | RE2 | PCRE-JIT |
| --- | --- | --- | --- | --- | --- | --- |
| `Twain` | 5 ms | 20 ms | 486 ms | 16 ms | 3 ms | 16 ms |
| `(?i)Twain` | 79 ms | 124 ms | 598 ms | 160 ms | 73 ms | 16 ms |
| `[a-z]shing` | 564 ms | 997 ms | 724 ms | 15 ms | 113 ms | 14 ms |
| `Huck[a-zA-Z]+\|Saw[a-zA-Z]+` | 30 ms | 32 ms | 711 ms | 43 ms | 59 ms | 3 ms |
| `\b\w+nn\b` | 837 ms | 1453 ms | 1186 ms | 1083 ms | 59 ms | 113 ms |
| `[a-q][^u-z]{13}x` | 746 ms | 2741 ms | 1928 ms | 61 ms | 3512 ms | 2 ms |
| `Tom\|Sawyer\|Huckleberry\|Finn` | 40 ms | 42 ms | 1242 ms | 51 ms | 61 ms | 29 ms |
| `(?i)Tom\|Sawyer\|Huckleberry\|Finn` | 424 ms | 489 ms | 1887 ms | 820 ms | 98 ms | 94 ms |
| `.{0,2}(Tom\|Sawyer\|Huckleberry\|Finn)` | 5164 ms | 5742 ms | 3425 ms | 127 ms | 66 ms | 443 ms |
| `.{2,4}(Tom\|Sawyer\|Huckleberry\|Finn)` | 5298 ms | 7097 ms | 5248 ms | 124 ms | 66 ms | 487 ms |
| `Tom.{10,25}river\|river.{10,25}Tom` | 83 ms | 108 ms | 804 ms | 90 ms | 68 ms | 18 ms |
| `[a-zA-Z]+ing` | 1373 ms | 2224 ms | 954 ms | 1420 ms | 129 ms | 92 ms |
| `\s[a-zA-Z]{0,12}ing\s` | 592 ms | 1007 ms | 1329 ms | 78 ms | 82 ms | 140 ms |
| `([A-Za-z]awyer\|[A-Za-z]inn)\s` | 1112 ms | 1489 ms | 1313 ms | 243 ms | 111 ms | 46 ms |
| `["'][^"']{0,30}[?!\.]["']` | 65 ms | 114 ms | 687 ms | 91 ms | 63 ms | 15 ms |

### Linux x86-32

- **CPU:** Intel Xeon 2.67 GHz
- **Compiler:** GCC 4.8.2

| Regular Expression | PCRE | PCRE-DFA | TRE | Oniguruma | RE2 | PCRE-JIT |
| --- | --- | --- | --- | --- | --- | --- |
| `Twain` | 5 ms | 20 ms | 548 ms | 17 ms | 4 ms | 18 ms |
| `(?i)Twain` | 97 ms | 144 ms | 683 ms | 139 ms | 74 ms | 16 ms |
| `[a-z]shing` | 594 ms | 996 ms | 766 ms | 15 ms | 107 ms | 14 ms |
| `Huck[a-zA-Z]+\|Saw[a-zA-Z]+` | 26 ms | 27 ms | 822 ms | 49 ms | 60 ms | 3 ms |
| `\b\w+nn\b` | 978 ms | 1494 ms | 1205 ms | 1151 ms | 59 ms | 114 ms |
| `[a-q][^u-z]{13}x` | 770 ms | 3112 ms | 2080 ms | 61 ms | 780 ms | 2 ms |
| `Tom\|Sawyer\|Huckleberry\|Finn` | 38 ms | 37 ms | 1346 ms | 58 ms | 61 ms | 29 ms |
| `(?i)Tom\|Sawyer\|Huckleberry\|Finn` | 464 ms | 481 ms | 1853 ms | 663 ms | 93 ms | 93 ms |
| `.{0,2}(Tom\|Sawyer\|Huckleberry\|Finn)` | 6726 ms | 5636 ms | 3641 ms | 139 ms | 70 ms | 406 ms |
| `.{2,4}(Tom\|Sawyer\|Huckleberry\|Finn)` | 7107 ms | 7252 ms | 5700 ms | 136 ms | 70 ms | 434 ms |
| `Tom.{10,25}river\|river.{10,25}Tom` | 85 ms | 105 ms | 904 ms | 106 ms | 69 ms | 18 ms |
| `[a-zA-Z]+ing` | 1740 ms | 2369 ms | 892 ms | 1387 ms | 144 ms | 90 ms |
| `\s[a-zA-Z]{0,12}ing\s` | 708 ms | 968 ms | 1271 ms | 98 ms | 97 ms | 167 ms |
| `([A-Za-z]awyer\|[A-Za-z]inn)\s` | 1300 ms | 1479 ms | 1583 ms | 255 ms | 104 ms | 45 ms |
| `["'][^"']{0,30}[?!\.]["']` | 73 ms | 118 ms | 765 ms | 100 ms | 65 ms | 14 ms |

### Windows x86-64

- **CPU:** Intel Core i5 3.2 GHz
- **Compiler:** GCC 4.4.7

| Regular expression | PCRE | PCRE-DFA | TRE | Oniguruma | RE2 | PCRE-JIT |
| --- | --- | --- | --- | --- | --- | --- |
| `Twain` | 7 ms | 12 ms | 315 ms | 20 ms | 8 ms | 16 ms |
| `(?i)Twain` | 47 ms | 90 ms | 414 ms | 110 ms | 131 ms | 16 ms |
| `[a-z]shing` | 330 ms | 704 ms | 485 ms | 19 ms | 131 ms | 15 ms |
| `Huck[a-zA-Z]+\|Saw[a-zA-Z]+` | 20 ms | 22 ms | 518 ms | 38 ms | 130 ms | 2 ms |
| `\b\w+nn\b` | 521 ms | 987 ms | 1109 ms | 782 ms | 131 ms | 85 ms |
| `[a-q][^u-z]{13}x` | 428 ms | 1852 ms | 1168 ms | 43 ms | 3316 ms | 1 ms |
| `Tom\|Sawyer\|Huckleberry\|Finn` | 25 ms | 29 ms | 863 ms | 44 ms | 132 ms | 23 ms |
| `(?i)Tom\|Sawyer\|Huckleberry\|Finn` | 239 ms | 358 ms | 1310 ms | 424 ms | 132 ms | 68 ms |
| `.{0,2}(Tom\|Sawyer\|Huckleberry\|Finn)` | 2970 ms | 3583 ms | 2170 ms | 85 ms | 133 ms | 268 ms |
| `.{2,4}(Tom\|Sawyer\|Huckleberry\|Finn)` | 2965 ms | 4134 ms | 3298 ms | 84 ms | 136 ms | 296 ms |
| `Tom.{10,25}river\|river.{10,25}Tom` | 54 ms | 81 ms | 655 ms | 80 ms | 138 ms | 14 ms |
| `[a-zA-Z]+ing` | 773 ms | 1686 ms | 567 ms | 922 ms | 181 ms | 70 ms |
| `\s[a-zA-Z]{0,12}ing\s` | 338 ms | 685 ms | 864 ms | 76 ms | 167 ms | 96 ms |
| `([A-Za-z]awyer\|[A-Za-z]inn)\s` | 664 ms | 1087 ms | 875 ms | 182 ms | 139 ms | 35 ms |
| `["'][^"']{0,30}[?!\.]["']` | 44 ms | 78 ms | 456 ms | 79 ms | 142 ms | 10 ms |
