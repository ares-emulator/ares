include_guard(GLOBAL)

option(ENABLE_COMPILER_TRACE "Enable clang time-trace" OFF)
mark_as_advanced(ENABLE_COMPILER_TRACE)

option(
  ARES_BUILD_LOCAL
  "Allows the compiler to generate code optimized for the target machine; increases performance."
  ON
)
option(
  ARES_ENABLE_MINIMUM_CPU
  "Defines a project minimum instruction set version in order to expose advanced SSE functionality to the compiler. Currently x86-64-v2 on x86_64; not defined on arm64."
  ON
)

include(ccache)
include(compiler_common)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # add common options
  add_compile_options(
    "$<$<COMPILE_LANGUAGE:C>:${_ares_clang_c_options}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${_ares_clang_cxx_options}>"
    -fwrapv
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(
    "${_ares_gcc_common_options}"
    "$<$<COMPILE_LANGUAGE:CXX>:${_ares_gcc_cxx_options}>"
  )
endif()

if(ARES_BUILD_LOCAL)
  add_compile_options($<$<NOT:$<CONFIG:Debug>>:-march=native>)
else()
  if(ARES_ENABLE_MINIMUM_CPU)
    string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} LOWERCASE_CMAKE_SYSTEM_PROCESSOR)
    if(LOWERCASE_CMAKE_SYSTEM_PROCESSOR MATCHES "(i[3-6]86|x86|x64|x86_64|amd64|e2k)")
      add_compile_options($<$<NOT:$<CONFIG:Debug>>:-march=x86-64-v2>)
    endif()
  endif()
endif()
