add_library(lzma STATIC
  include/7zTypes.h
  include/7zWindows.h
  include/Alloc.h
  include/Bra.h
  include/Compiler.h
  include/CpuArch.h
  include/Delta.h
  include/LzFind.h
  include/LzHash.h
  include/Lzma86.h
  include/LzmaDec.h
  include/LzmaEnc.h
  include/Precomp.h
  include/RotateDefs.h
  include/Sort.h
  src/Alloc.c
  src/Bra.c
  src/Bra86.c
  src/BraIA64.c
  src/CpuArch.c
  src/Delta.c
  src/LzFind.c
  src/Lzma86Dec.c
  src/LzmaDec.c
  src/LzmaEnc.c
  src/Sort.c
)

target_compile_definitions(lzma PRIVATE Z7_ST)

target_include_directories(lzma PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(lzma INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_target_properties(lzma PROPERTIES POSITION_INDEPENDENT_CODE ON)

option(WITH_LZMA_ASM "Use lzma asm" ON)
if(WITH_LZMA_ASM)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(CheckSymbolExists)
    check_symbol_exists("__aarch64__" "" CPU_ARM64)
    if(CPU_ARM64)
      enable_language(ASM)
      set_source_files_properties(src/LzmaDec.c PROPERTIES COMPILE_DEFINITIONS Z7_LZMA_DEC_OPT)
      target_sources(lzma PRIVATE Asm/arm64/LzmaDecOpt.S)
      set_source_files_properties(Asm/arm64/LzmaDecOpt.S PROPERTIES LANGUAGE ASM)
    endif()
  elseif(WIN32)
    include(CheckSymbolExists)
    check_symbol_exists("_M_AMD64" "" CPU_X64)
    if(CPU_X64)
      enable_language(ASM_MASM)
      set_source_files_properties(src/LzmaDec.c PROPERTIES COMPILE_DEFINITIONS Z7_LZMA_DEC_OPT)
      target_sources(lzma PRIVATE Asm/x86/LzmaDecOpt.asm)
      set_source_files_properties(Asm/x86/LzmaDecOpt.asm PROPERTIES LANGUAGE ASM_MASM)
    endif()
  endif()
endif()
