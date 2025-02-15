#pragma once

#include <nall/platform.hpp>
#include <nall/stdint.hpp>
#include <nall/string.hpp>

//implementation heavily based on example from https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex

namespace nall {

struct instruction_set {
  instruction_set() = delete;

  static auto vendor() -> string { return info.vendor; }
  static auto brand() -> string { return info.brand; }

  static auto sse3() -> bool { return info.f_1_ecx >> 0 & 1; }
  static auto pclmulqdq() -> bool { return info.f_1_ecx >> 1 & 1; }
  static auto monitor() -> bool { return info.f_1_ecx >> 3 & 1; }
  static auto ssse3() -> bool { return info.f_1_ecx >> 9 & 1; }
  static auto fma() -> bool { return info.f_1_ecx >> 12 & 1; }
  static auto cmpxchg16b() -> bool { return info.f_1_ecx >> 13 & 1; }
  static auto sse41() -> bool { return info.f_1_ecx >> 19 & 1; }
  static auto sse42() -> bool { return info.f_1_ecx >> 20 & 1; }
  static auto movbe() -> bool { return info.f_1_ecx >> 22 & 1; }
  static auto popcnt() -> bool { return info.f_1_ecx >> 23 & 1; }
  static auto aes() -> bool { return info.f_1_ecx >> 25 & 1; }
  static auto xsave() -> bool { return info.f_1_ecx >> 26 & 1; }
  static auto osxsave() -> bool { return info.f_1_ecx >> 27 & 1; }
  static auto avx() -> bool { return info.f_1_ecx >> 28 & 1; }
  static auto f16c() -> bool { return info.f_1_ecx >> 29 & 1; }
  static auto rdrand() -> bool { return info.f_1_ecx >> 30 & 1; }

  static auto msr() -> bool { return info.f_1_edx >> 5 & 1; }
  static auto cx8() -> bool { return info.f_1_edx >> 8 & 1; }
  static auto sep() -> bool { return info.f_1_edx >> 11 & 1; }
  static auto cmov() -> bool { return info.f_1_edx >> 15 & 1; }
  static auto clfsh() -> bool { return info.f_1_edx >> 19 & 1; }
  static auto mmx() -> bool { return info.f_1_edx >> 23 & 1; }
  static auto fxsr() -> bool { return info.f_1_edx >> 24 & 1; }
  static auto sse() -> bool { return info.f_1_edx >> 25 & 1; }
  static auto sse2() -> bool { return info.f_1_edx >> 26 & 1; }

  static auto fsgsbase() -> bool { return info.f_7_ebx >> 0 & 1; }
  static auto bmi1() -> bool { return info.f_7_ebx >> 3 & 1; }
  static auto hle() -> bool { return info.isIntel && info.f_7_ebx >> 4 & 1; }
  static auto avx2() -> bool { return info.f_7_ebx >> 5 & 1; }
  static auto bmi2() -> bool { return info.f_7_ebx >> 8 & 1; }
  static auto erms() -> bool { return info.f_7_ebx >> 9 & 1; }
  static auto invpcid() -> bool { return info.f_7_ebx >> 10 & 1; }
  static auto rtm() -> bool { return info.isIntel && info.f_7_ebx >> 11 & 1; }
  static auto avx512f() -> bool { return info.f_7_ebx >> 16 & 1; }
  static auto rdseed() -> bool { return info.f_7_ebx >> 18 & 1; }
  static auto adx() -> bool { return info.f_7_ebx >> 19 & 1; }
  static auto avx512pf() -> bool { return info.f_7_ebx >> 26 & 1; }
  static auto avx512er() -> bool { return info.f_7_ebx >> 27 & 1; }
  static auto avx512cd() -> bool { return info.f_7_ebx >> 28 & 1; }
  static auto sha() -> bool { return info.f_7_ebx >> 29 & 1; }

  static auto prefetchwt1() -> bool { return info.f_7_ecx >> 0 & 1; }

  static auto lahf() -> bool { return info.f_81_ecx >> 0 & 1; }
  static auto lzcnt() -> bool { return info.isIntel && info.f_81_ecx >> 5 & 1; }
  static auto abm() -> bool { return info.isAMD && info.f_81_ecx >> 5 & 1; }
  static auto sse4a() -> bool { return info.isAMD && info.f_81_ecx >> 6 & 1; }
  static auto xop() -> bool { return info.isAMD && info.f_81_ecx >> 11 & 1; }
  static auto tbm() -> bool { return info.isAMD && info.f_81_ecx >> 21 & 1; }

  static auto syscall() -> bool { return info.isIntel && info.f_81_edx >> 11 & 1; }
  static auto mmxext() -> bool { return info.isAMD && info.f_81_edx >> 22 & 1; }
  static auto rdtscp() -> bool { return info.isIntel && info.f_81_edx >> 27 & 1; }
  static auto _3dnowext() -> bool { return info.isAMD && info.f_81_edx >> 30 & 1; }
  static auto _3dnow() -> bool { return info.isAMD && info.f_81_edx >> 31 & 1; }

private:
  struct information {
    information();

    string vendor;
    string brand;
    bool isIntel = false;
    bool isAMD = false;
    u32 f_1_ecx = 0;
    u32 f_1_edx = 0;
    u32 f_7_ebx = 0;
    u32 f_7_ecx = 0;
    u32 f_81_ecx = 0;
    u32 f_81_edx = 0;
  };

  static inline const information info;
};

}

#if defined(NALL_HEADER_ONLY)
  #include <nall/instruction-set.cpp>
#endif
