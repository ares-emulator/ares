add_library(sljit STATIC sljit/sljit_src/sljitLir.c)

add_library(
  sevenzip-sdk
  STATIC
  libchdr/deps/lzma-25.01/src/real/7zAlloc.c
  libchdr/deps/lzma-25.01/src/real/7zArcIn.c
  libchdr/deps/lzma-25.01/src/real/7zBuf.c
  libchdr/deps/lzma-25.01/src/real/7zBuf2.c
  libchdr/deps/lzma-25.01/src/real/7zCrc.c
  libchdr/deps/lzma-25.01/src/real/7zCrcOpt.c
  libchdr/deps/lzma-25.01/src/real/7zDec.c
  libchdr/deps/lzma-25.01/src/real/7zFile.c
  libchdr/deps/lzma-25.01/src/real/7zStream.c
  libchdr/deps/lzma-25.01/src/real/Aes.c
  libchdr/deps/lzma-25.01/src/real/AesOpt.c
  libchdr/deps/lzma-25.01/src/real/Bcj2.c
  libchdr/deps/lzma-25.01/src/real/Bra.c
  libchdr/deps/lzma-25.01/src/real/Bra86.c
  libchdr/deps/lzma-25.01/src/real/BraIA64.c
  libchdr/deps/lzma-25.01/src/real/CpuArch.c
  libchdr/deps/lzma-25.01/src/real/Delta.c
  libchdr/deps/lzma-25.01/src/real/Lzma2Dec.c
  libchdr/deps/lzma-25.01/src/real/LzmaDec.c
  libchdr/deps/lzma-25.01/src/real/MtDec.c
  libchdr/deps/lzma-25.01/src/real/Ppmd7.c
  libchdr/deps/lzma-25.01/src/real/Ppmd7Dec.c
  libchdr/deps/lzma-25.01/src/real/Sha256.c
  libchdr/deps/lzma-25.01/src/real/Sha256Opt.c
  libchdr/deps/lzma-25.01/src/real/Sort.c
  libchdr/deps/lzma-25.01/src/real/SwapBytes.c
  libchdr/deps/lzma-25.01/src/real/Threads.c
)

target_include_directories(sljit PUBLIC ../thirdparty)
target_include_directories(sevenzip-sdk PUBLIC ../thirdparty/libchdr/deps/lzma-25.01/include/real)
target_compile_definitions(sljit PUBLIC SLJIT_HAVE_CONFIG_PRE=1 SLJIT_HAVE_CONFIG_POST=1)
target_compile_definitions(sevenzip-sdk PRIVATE Z7_ST Z7_EXTRACT_ONLY Z7_PPMD_SUPPORT)
target_compile_options(sljit PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Wno-conditional-uninitialized>)

option(ARES_ENABLE_CHD "Enable CHD format support via libchdr" ON)

if(ARES_ENABLE_CHD)
  # lzma
  add_subdirectory(libchdr/deps/lzma-25.01 EXCLUDE_FROM_ALL)
  list(APPEND CHDR_LIBS chdr-lzma)

  if(OS_MACOS)
    option(WITH_SYSTEM_ZLIB "Use system zlib" ON)
  endif()
  # zlib
  if(WITH_SYSTEM_ZLIB)
    find_package(ZLIB REQUIRED)
    list(APPEND PLATFORM_LIBS ZLIB::ZLIB)
    list(APPEND CHDR_DEFINES CHDR_SYSTEM_ZLIB)
  else()
    add_subdirectory(libchdr/deps/miniz-3.1.1 EXCLUDE_FROM_ALL)
    list(APPEND CHDR_LIBS miniz)
  endif()

  # zstd
  add_subdirectory(libchdr/deps/zstd-1.5.7 EXCLUDE_FROM_ALL)
  list(APPEND CHDR_LIBS zstd)
  #--------------------------------------------------
  # chdr
  #--------------------------------------------------

  set(
    CHDR_SOURCES
    libchdr/src/libchdr_bitstream.c
    libchdr/src/libchdr_cdrom.c
    libchdr/src/libchdr_chd.c
    libchdr/src/libchdr_codec_cdfl.c
    libchdr/src/libchdr_codec_cdlz.c
    libchdr/src/libchdr_codec_cdzl.c
    libchdr/src/libchdr_codec_cdzs.c
    libchdr/src/libchdr_codec_flac.c
    libchdr/src/libchdr_codec_huff.c
    libchdr/src/libchdr_codec_lzma.c
    libchdr/src/libchdr_codec_zlib.c
    libchdr/src/libchdr_codec_zstd.c
    libchdr/src/libchdr_flac.c
    libchdr/src/libchdr_huffman.c
  )

  add_library(chdr-static STATIC ${CHDR_SOURCES})
  target_include_directories(chdr-static PUBLIC libchdr/include)
  target_link_libraries(chdr-static PRIVATE ${CHDR_LIBS} ${PLATFORM_LIBS})
  target_compile_definitions(chdr-static PRIVATE ${CHDR_DEFINES})
  target_compile_options(
    chdr-static
    PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-unreachable-code -Wno-unused-function>
  )
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
  if(NOT WITH_SYSTEM_ZLIB)
    set_target_properties(miniz PROPERTIES FOLDER thirdparty PREFIX "")
  endif()
  set_target_properties(chdr-lzma PROPERTIES FOLDER thirdparty PREFIX "")
  set_target_properties(zstd PROPERTIES FOLDER thirdparty PREFIX "")
endif()
set_target_properties(qon PROPERTIES FOLDER thirdparty PREFIX "")
