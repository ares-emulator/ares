add_library(sljit STATIC sljit/sljit_src/sljitLir.c)

add_library(
  sevenzip-sdk
  STATIC
  lzma/7zAlloc.c
  lzma/7zArcIn.c
  lzma/7zBuf.c
  lzma/7zBuf2.c
  lzma/7zCrc.c
  lzma/7zCrcOpt.c
  lzma/7zDec.c
  lzma/7zFile.c
  lzma/7zStream.c
  lzma/Aes.c
  lzma/AesOpt.c
  lzma/Bcj2.c
  lzma/Bra.c
  lzma/Bra86.c
  lzma/BraIA64.c
  lzma/CpuArch.c
  lzma/Delta.c
  lzma/Lzma2Dec.c
  lzma/LzmaDec.c
  lzma/MtDec.c
  lzma/Ppmd7.c
  lzma/Ppmd7Dec.c
  lzma/Sha256.c
  lzma/Sha256Opt.c
  lzma/Sort.c
  lzma/SwapBytes.c
  lzma/Threads.c
)

target_include_directories(sljit PUBLIC ../thirdparty)
target_include_directories(sevenzip-sdk PUBLIC ../thirdparty/lzma)
target_compile_definitions(sljit PUBLIC SLJIT_HAVE_CONFIG_PRE=1 SLJIT_HAVE_CONFIG_POST=1)
target_compile_definitions(sevenzip-sdk PRIVATE Z7_ST Z7_EXTRACT_ONLY Z7_PPMD_SUPPORT)
target_compile_options(sljit PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Wno-conditional-uninitialized>)

option(ARES_ENABLE_CHD "Enable CHD format support via libchdr" ON)

if(ARES_ENABLE_CHD)
  if(OS_MACOS)
    set(WITH_SYSTEM_ZLIB ON CACHE BOOL "Use system provided zlib library" FORCE)
  endif()

  add_subdirectory(libchdr EXCLUDE_FROM_ALL)
endif()

add_library(
  tzxfile
  STATIC
  TZXFile/TZXAudioGenerator.cpp
  TZXFile/TZXBlock.cpp
  TZXFile/TZXBlockArchiveInfo.cpp
  TZXFile/TZXBlockCustomInfo.cpp
  TZXFile/TZXBlockGroupEnd.cpp
  TZXFile/TZXBlockGroupStart.cpp
  TZXFile/TZXBlockHardwareType.cpp
  TZXFile/TZXBlockLoopEnd.cpp
  TZXFile/TZXBlockLoopStart.cpp
  TZXFile/TZXBlockMessage.cpp
  TZXFile/TZXBlockPause.cpp
  TZXFile/TZXBlockPulseSequence.cpp
  TZXFile/TZXBlockPureData.cpp
  TZXFile/TZXBlockPureTone.cpp
  TZXFile/TZXBlockStandardSpeedData.cpp
  TZXFile/TZXBlockStopTheTape48K.cpp
  TZXFile/TZXBlockTextDescription.cpp
  TZXFile/TZXBlockTurboSpeedData.cpp
  TZXFile/TZXFile.cpp
)

target_include_directories(tzxfile PUBLIC ../thirdparty/TZXFile)

add_library(
  ymfm
  STATIC
  ymfm/src/ymfm_adpcm.cpp
  ymfm/src/ymfm_misc.cpp
  ymfm/src/ymfm_opl.cpp
  ymfm/src/ymfm_opm.cpp
  ymfm/src/ymfm_opn.cpp
  ymfm/src/ymfm_opq.cpp
  ymfm/src/ymfm_opz.cpp
  ymfm/src/ymfm_pcm.cpp
  ymfm/src/ymfm_ssg.cpp
)

target_include_directories(ymfm PUBLIC ../thirdparty/ymfm/src)

target_compile_options(ymfm PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-unreachable-code>)

add_library(qon STATIC qon/qon.cpp)

set_target_properties(ymfm PROPERTIES FOLDER thirdparty PREFIX "")
set_target_properties(tzxfile PROPERTIES FOLDER thirdparty PREFIX "")
set_target_properties(sljit PROPERTIES FOLDER thirdparty PREFIX "")
set_target_properties(sevenzip-sdk PROPERTIES FOLDER thirdparty PREFIX "")
if(ARES_ENABLE_CHD)
  set_target_properties(chdr-static PROPERTIES FOLDER thirdparty PREFIX "")
  if(TARGET miniz)
    set_target_properties(miniz PROPERTIES FOLDER thirdparty PREFIX "")
  endif()
  if(TARGET chdr-lzma)
    set_target_properties(chdr-lzma PROPERTIES FOLDER thirdparty PREFIX "")
  endif()
  if(TARGET zstd)
    set_target_properties(zstd PROPERTIES FOLDER thirdparty PREFIX "")
  endif()
endif()
set_target_properties(qon PROPERTIES FOLDER thirdparty PREFIX "")
