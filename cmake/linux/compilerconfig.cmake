include_guard(GLOBAL)

option(ENABLE_COMPILER_TRACE "Enable clang time-trace" OFF)
mark_as_advanced(ENABLE_COMPILER_TRACE)

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
    ${_ares_gcc_common_options}
  )
endif()
