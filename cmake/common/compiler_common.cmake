# ares CMake common compiler options module

include_guard(GLOBAL)

option(ARES_COMPILE_DEPRECATION_AS_WARNING "Downgrade deprecation warnings to actual warnings" FALSE)
mark_as_advanced(ARES_COMPILE_DEPRECATION_AS_WARNING)

include(CheckIPOSupported)
option(ENABLE_IPO "Enable interprocedural optimization (LTO)" YES)
message(STATUS "Checking if interprocedural optimization is supported")
if(ENABLE_IPO)
  check_ipo_supported(RESULT ipo_supported OUTPUT output)
  if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
    message(STATUS "Checking if interprocedural optimization is supported - done")
  else()
    message(STATUS "Checking if interprocedural optimization is supported - failure")
    message(DEBUG "IPO support failure reason: ${output}")
  endif()
else()
  message(STATUS "Checking if interprocedural optimization is supported - skipped")
endif()

# Set C and C++ language standards to C17 and C++17
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Set symbols to be hidden by default for C and C++
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

# clang options for C, C++, ObjC, and ObjC++
set(
  _ares_clang_common_options
  -Wblock-capture-autoreleasing
  # -Wswitch
  -Wdeprecated
  -Wno-deprecated-literal-operator
  -Wno-empty-body
  -Wno-switch
  -Wno-parentheses
  -Wbool-conversion
  -Wconstant-conversion
  # -Wshorten-64-to-32
  # -Wanon-enum-enum-conversion
  -Wint-conversion
  -Wnon-literal-null-conversion
  -Winfinite-recursion
  -Werror=return-type
  # -Wparentheses
  -Wpointer-sign
  -Wquoted-include-in-framework-header
  -Wnewline-eof
  # -Wsign-compare
  # -Wstrict-prototypes
  # -Wcomma
  -Wignored-pragmas
  -Wunguarded-availability
  -Wuninitialized
  -Wunreachable-code
  # -Wunused
  -Wno-unused
  -Wvla
  -Wformat-security
  -Wno-error=strict-prototypes
  -Wno-shorten-64-to-32
  -Wno-sign-compare
  -Wno-comma
  -Wno-protocol
  -Wno-comma
  -Wno-deprecated-copy-with-user-provided-copy
  -Wno-deprecated-copy
  -Wno-anon-enum-enum-conversion
  -Wno-deprecated-copy-with-user-provided-dtor
)

set(_ares_clang_c_options ${_ares_clang_common_options})

# clang options for C++
set(
  _ares_clang_cxx_options
  ${_ares_clang_common_options}
  -Wvexing-parse
  -Wdelete-non-virtual-dtor
  -Wrange-loop-analysis
  -Wmove
  -Winvalid-offsetof
  -Wno-delete-non-abstract-non-virtual-dtor
)

if(NOT DEFINED CMAKE_COMPILE_WARNING_AS_ERROR)
  set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
endif()
