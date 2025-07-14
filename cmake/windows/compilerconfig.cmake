if(WIN32 AND NOT MINGW)
  add_compile_definitions(EXCLUDE_MANIFEST_FROM_RC) #global
endif()

include_guard(GLOBAL)

option(
  ARES_BUILD_LOCAL
  "Allows the compiler to generate code optimized for the target machine; increases performance."
  ON
)

include(ccache)
include(compiler_common)

if(MSVC AND ENABLE_CCACHE AND CCACHE_PROGRAM)
  if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    file(COPY_FILE ${CCACHE_PROGRAM} "${CMAKE_CURRENT_BINARY_DIR}/cl.exe")
    set(CMAKE_VS_GLOBALS "CLToolExe=cl.exe" "CLToolPath=${CMAKE_BINARY_DIR}" "UseMultiToolTask=true")
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
  elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    file(COPY_FILE ${CCACHE_PROGRAM} "${CMAKE_CURRENT_BINARY_DIR}/clang-cl.exe")
    set(CMAKE_VS_GLOBALS "CLToolExe=clang-cl.exe" "CLToolPath=${CMAKE_BINARY_DIR}" "UseMultiToolTask=true")
  endif()
endif()

if(CMAKE_GENERATOR_PLATFORM)
  set(arch ${CMAKE_GENERATOR_PLATFORM})
  set(platform windows-${arch})
else()
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL ARM64)
    set(arch arm64)
    set(platform windows-${arch})
  else()
    set(arch x64)
    set(platform windows-${arch})
  endif()
endif()

add_compile_definitions(_WIN32_WINNT=0x0601) #global

option(
  ARES_MINGW_USE_DWARF_SYMBOLS
  "Generate DWARF debug symbols (instead of CodeView) for use with gdb or lldb. Applies to MSYS2/MinGW environments."
)

set(
  _ares_msvc_cxx_options
  /W2
  /wd4146 # unary minus applied to unsigned type
  /wd4244 # loss of data converting float to integer
  /wd4804 # unsafe use of bool in operation
  /wd4805 # unsafe mix of types in operation
  /MP
  /Zc:__cplusplus
  /utf-8
  /permissive-
  $<$<NOT:$<CONFIG:Debug>>:/GS->
)

set(
  _ares_clang_cl_c_cxx_options
  -Wno-reorder-ctor
  -Wno-missing-braces
  -Wno-char-subscripts
  -Wno-misleading-indentation
  -Wno-bitwise-instead-of-logical
  -Wno-self-assign-overloaded
  -Wno-overloaded-virtual
)

if(MSVC)
  # work around https://gitlab.kitware.com/cmake/cmake/-/issues/20812
  string(REPLACE "/Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
  string(REPLACE "/Ob1" "/Ob2" CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
endif()

# add general compiler flags and optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # we are on either msys2/mingw clang, or clang-cl
  # add common options
  add_compile_options(
    "$<$<COMPILE_LANGUAGE:C>:${_ares_clang_c_options}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${_ares_clang_cxx_options}>"
  )
  if(NOT MSVC)
    # we are on msys2 clang
    # statically link libc++
    add_link_options(-static-libstdc++)

    # msys2/mingw-specific invocations to make clang emit debug symbols
    if(NOT DEFINED ARES_MINGW_USE_DWARF_SYMBOLS)
      set(ARES_MINGW_USE_DWARF_SYMBOLS OFF)
    endif()

    set(_ares_mingw_clang_debug_compile_options -g "$<IF:$<BOOL:${ARES_MINGW_USE_DWARF_SYMBOLS}>,-gdwarf,-gcodeview>")
    set(
      _ares_mingw_clang_debug_link_options
      -g
      "$<IF:$<BOOL:${ARES_MINGW_USE_DWARF_SYMBOLS}>,-gdwarf,-fuse-ld=lld;-Wl$<COMMA>--pdb=>"
    )
    add_compile_options("$<$<CONFIG:Debug,RelWithDebInfo>:${_ares_mingw_clang_debug_compile_options}>")
    add_link_options("$<$<CONFIG:Debug,RelWithDebInfo>:${_ares_mingw_clang_debug_link_options}>")

    add_compile_options(-fwrapv)
  else()
    # we are on clang-cl
    # generate PDBs rather than embed debug symbols
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)

    add_compile_options(
      "$<$<COMPILE_LANGUAGE:CXX>:${_ares_msvc_cxx_options}>"
      "$<$<COMPILE_LANGUAGE:C,CXX>:${_ares_clang_cl_c_cxx_options}>"
    )

    # work around https://gitlab.kitware.com/cmake/cmake/-/issues/26559
    add_compile_options($<$<AND:$<BOOL:${ENABLE_IPO}>,$<NOT:$<CONFIG:Debug>>>:-flto=thin>)
    add_link_options(
      $<$<AND:$<BOOL:${ENABLE_IPO}>,$<NOT:$<CONFIG:Debug>>>:-flto=thin>
      $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO>
      /Debug
      $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>
      $<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>
    )

    # add -fwrapv
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/clang:-fwrapv>")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:${_ares_msvc_cxx_options}>")

  if(CMAKE_COMPILE_WARNING_AS_ERROR)
    add_link_options(/WX)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(NOT DEFINED ARES_MINGW_USE_DWARF_SYMBOLS)
    set(ARES_MINGW_USE_DWARF_SYMBOLS ON)
  endif()

  set(_ares_mingw_gcc_debug_compile_options -g "$<IF:$<BOOL:${ARES_MINGW_USE_DWARF_SYMBOLS}>,-gdwarf,-gcodeview>")
  set(_ares_mingw_gcc_debug_link_options -g "$<IF:$<BOOL:${ARES_MINGW_USE_DWARF_SYMBOLS}>,-gdwarf,-Wl$<COMMA>--pdb=>")
  add_compile_options("$<$<CONFIG:Debug,RelWithDebInfo>:${_ares_mingw_gcc_debug_compile_options}>")
  add_link_options("$<$<CONFIG:Debug,RelWithDebInfo>:${_ares_mingw_gcc_debug_link_options}>")

  add_compile_options(${_ares_gcc_common_options})
  add_link_options(-static-libstdc++)
endif()

# arch/machine-specific optimizations
if(ARES_BUILD_LOCAL)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options($<$<NOT:$<CONFIG:Debug>>:-march=native>)
  else()
    # todo: arch optimizations on msvc
  endif()
else()
  if(${arch} STREQUAL x64)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:-march=x86-64-v2>")
    endif()
  else()
    # todo: arm64 arch baseline
  endif()
endif()

if(NOT MINGW)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS) #global
endif()
