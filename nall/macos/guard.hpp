#ifndef NALL_MACOS_GUARD_HPP
#define NALL_MACOS_GUARD_HPP

#define Boolean CocoaBoolean
#define decimal CocoaDecimal
#ifdef DEBUG
#undef DEBUG
#define DEBUG CocoaDebug
#endif

#else
#undef NALL_MACOS_GUARD_HPP

#undef Boolean
#undef decimal
#undef DEBUG

#endif
