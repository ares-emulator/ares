---
sidebar_position: 2
description: A comparison of regular experssion engine types.
---

# Regular Expression Engine Types

Matching regular expressions is not considered a complex task and many people think that this problem was solved a long time ago.
The reason for this belief is a great deal of misinformation which has been spread across the internet and the education.
On the contrary there is no best solution for matching a pattern at the moment, and choosing the right regular expression engine is as difficult as choosing the right programming language.
The aim of this page is showing the advantages and disadvantages of different engine types.

In general we distinguish two engine types:
- **Performance-oriented engines:** very fast in general, and very slow in special (called pathological) cases.
- **Balanced engines:** no weaknesses, no strengths. Usually predictable worst case runtime.

Balanced engines are slower than performance-oriented engines, but adding some pathological patterns to the benchmark set can quickly turn the tide.

The following sections go into more detail regarding the different engine types from a technical point of view.
A performance comparison of some of the engines can be found [here](performance-comparison.md).

## Multiple Choice Engine with Backtracking

**Type:** Usually performance-oriented engines

**Advantages:**
- Large feature set (assertions, conditional blocks, executing code blocks, backtracking control)
- Submatch capture
- Lots of optimization opportunities (mostly backtracking elimination)

**Disadvantages:**
- Large and complex code base
- May use a large amount of stack
- Examples for pathological cases:
    - `(a*)*b`
    - `(?:a?){N}a{N}` (where `N` is a positive integer number)

**Execution modes:** Depth-first search algorithm
- Interpreted execution of a Non-deterministic Finite Automaton (NFA); example: [PCRE interpreter](https://www.pcre.org/)
- Generating machine code from an NFA; example: [Irregexp engine](https://blog.chromium.org/2009/02/irregexp-google-chromes-new-regexp.html)
- Generating machine code from an Abstract Syntax Tree (AST); example: [PCRE JIT compiler](https://www.pcre.org/)

## Single Choice Engine with Pre-generated State Machine

**Type:** Usually performance-oriented engines

**Advantages:**
- Simple and very fast matching algorithm
- Partial matching support is easy
- Multiple patterns can be matched at the same time (on a single core)

**Disadvantages:**
- Large memory consumption (can be limited at the expense of performance)
- Limited feature set (and this feature set is not fully supported, sometimes necessitating falling back to other processing modes)
- Examples for pathological cases:
    - `a[^b]{N}a` (where `N` is a positive integer number)
    - `a[^b]{1,N}abcde` (where `N` is a positive integer number)

**Execution modes:** Linear search time, exponential state machine build time (especially for combining multiple patterns into a single state machine)
- Following the state transitions of a Deterministic Finite Automaton (DFA); example: [RE2 engine](https://github.com/google/re2)
- DFA based multi pattern matching; example: [MPM library](https://github.com/zherczeg/mpm)

## Single Choice Engine with State Tracking

**Type:** Usually balanced engines

**Advantages:**
- No pathological cases
- Partial matching support is not difficult

**Disadvantages:**
- Matching speed is generally low due to the complex state update mechanism
- Limited feature set (due to synchronization issues, they have a similar feature set as the engines with pre-generated state machine)

**Execution modes:** Linear search time
- Interpreted execution of Deterministic Finite Automaton (DFA); example: [PCRE-DFA interpreter](https://www.pcre.org/)
- Parallel matching of NFA; example: [TRE engine](http://laurikari.net/tre/)
