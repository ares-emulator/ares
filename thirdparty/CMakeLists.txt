add_library(sljit STATIC sljit/sljit_src/sljitLir.c)

target_include_directories(sljit PUBLIC ../thirdparty)
target_compile_definitions(sljit PUBLIC SLJIT_HAVE_CONFIG_PRE=1 SLJIT_HAVE_CONFIG_POST=1)
target_compile_options(sljit PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Wno-conditional-uninitialized>)

option(ARES_ENABLE_CHD "Enable CHD format support via libchdr" ON)
option(ARES_ENABLE_RCHEEVOS "Enable RetroAchievements support" OFF)

if(ARES_ENABLE_RCHEEVOS)
  find_package(CURL REQUIRED)

  add_library(
    rcheevos
    STATIC
    rcheevos/src/rapi/rc_api_common.c
    rcheevos/src/rapi/rc_api_info.c
    rcheevos/src/rapi/rc_api_runtime.c
    rcheevos/src/rapi/rc_api_user.c
    rcheevos/src/rc_client.c
    rcheevos/src/rc_compat.c
    rcheevos/src/rc_util.c
    rcheevos/src/rc_version.c
    rcheevos/src/rcheevos/alloc.c
    rcheevos/src/rcheevos/condition.c
    rcheevos/src/rcheevos/condset.c
    rcheevos/src/rcheevos/consoleinfo.c
    rcheevos/src/rcheevos/format.c
    rcheevos/src/rcheevos/lboard.c
    rcheevos/src/rcheevos/memref.c
    rcheevos/src/rcheevos/operand.c
    rcheevos/src/rcheevos/rc_validate.c
    rcheevos/src/rcheevos/richpresence.c
    rcheevos/src/rcheevos/runtime.c
    rcheevos/src/rcheevos/runtime_progress.c
    rcheevos/src/rcheevos/trigger.c
    rcheevos/src/rcheevos/value.c
    rcheevos/src/rhash/hash.c
    rcheevos/src/rhash/hash_rom.c
    rcheevos/src/rhash/md5.c
  )

  target_include_directories(
    rcheevos
    PUBLIC rcheevos/include
    PRIVATE rcheevos/src
  )
  target_compile_definitions(
    rcheevos
    PUBLIC
      RC_CLIENT_SUPPORTS_HASH
      RC_HASH_NO_DISC
      RC_HASH_NO_ZIP
      RC_HASH_NO_ENCRYPTED
  )
  target_link_libraries(rcheevos PUBLIC CURL::libcurl)
  target_enable_feature(rcheevos "RetroAchievements support")
else()
  target_disable_feature(sljit "RetroAchievements support")
endif()

if(ARES_ENABLE_CHD)
  # lzma
  add_subdirectory(libchdr/deps/lzma-25.01 EXCLUDE_FROM_ALL)
  list(APPEND CHDR_INCLUDES lzma)

  # miniz
  add_subdirectory(libchdr/deps/miniz-3.1.1 EXCLUDE_FROM_ALL)
  set_target_properties(
    miniz
    PROPERTIES POSITION_INDEPENDENT_CODE ON FOLDER thirdparty PREFIX ""
  )
  list(APPEND CHDR_LIBS miniz)
  
  # zstd
  option(ZSTD_BUILD_SHARED "BUILD SHARED LIBRARIES" OFF)
  option(ZSTD_BUILD_PROGRAMS "BUILD PROGRAMS" OFF)
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
    libchdr/src/libchdr_flac.c
    libchdr/src/libchdr_huffman.c
    libchdr/src/libchdr_codec_cdfl.c
    libchdr/src/libchdr_codec_cdlz.c
    libchdr/src/libchdr_codec_cdzl.c
    libchdr/src/libchdr_codec_cdzs.c
    libchdr/src/libchdr_codec_flac.c
    libchdr/src/libchdr_codec_huff.c
    libchdr/src/libchdr_codec_lzma.c
    libchdr/src/libchdr_codec_zlib.c
    libchdr/src/libchdr_codec_zstd.c
    libchdr/deps/lzma-25.01/src/LzmaDec.c
  )

  list(APPEND CHDR_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/libchdr/include)

  add_library(chdr-static STATIC ${CHDR_SOURCES})
  target_include_directories(chdr-static PUBLIC ${CHDR_INCLUDES} PUBLIC libchdr/include)
  target_link_libraries(chdr-static PRIVATE ${CHDR_LIBS} ${PLATFORM_LIBS})
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
  TZXFile/TZXBlockKansasCityStandard.cpp
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
if(ARES_ENABLE_RCHEEVOS)
  set_target_properties(rcheevos PROPERTIES FOLDER thirdparty PREFIX "")
endif()
if(ARES_ENABLE_CHD)
  set_target_properties(chdr-static PROPERTIES FOLDER thirdparty PREFIX "")
  set_target_properties(miniz PROPERTIES FOLDER thirdparty PREFIX "")
  set_target_properties(zstd PROPERTIES FOLDER thirdparty PREFIX "")
endif()
set_target_properties(qon PROPERTIES FOLDER thirdparty PREFIX "")
