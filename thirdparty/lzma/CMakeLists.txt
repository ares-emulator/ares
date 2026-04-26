add_library(
  lzma_sdk_standalone
  STATIC
  C/7zAlloc.c
  C/7zArcIn.c
  C/7zBuf.c
  C/7zBuf2.c
  C/7zCrc.c
  C/7zCrcOpt.c
  C/7zDec.c
  C/7zFile.c
  C/7zStream.c
  C/Aes.c
  C/AesOpt.c
  C/Alloc.c
  C/Bcj2.c
  C/Bra.c
  C/Bra86.c
  C/CpuArch.c
  C/Delta.c
  C/Lzma2Dec.c
  C/LzmaDec.c
)

target_compile_definitions(lzma_sdk_standalone PRIVATE Z7_ST)
target_include_directories(lzma_sdk_standalone PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/C)
set_target_properties(lzma_sdk_standalone PROPERTIES POSITION_INDEPENDENT_CODE ON)
