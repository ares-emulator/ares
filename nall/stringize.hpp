#pragma once

// The # preprocessing operator converts the argument into a quoted string literal.
// In order to use this for macro parameters which need to be expanded, this must
// be a two-level-macro call.

#define stringize_(arg) #arg
#define stringize(arg) stringize_(arg)
